param(
  [string] $Version,
  [string] $OutputDirectory,

  [string[]] $Toolset = @('v143'),

  [ValidateSet('x86', 'x64', 'ARM', 'ARM64')]
  [string[]] $Architecture = @('x86', 'x64', 'ARM', 'ARM64')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

function Compress-DirectoryWithRetry {
  param(
    [Parameter(Mandatory = $true)]
    [string] $SourceDirectory,

    [Parameter(Mandatory = $true)]
    [string] $DestinationPath
  )

  for ($attempt = 1; $attempt -le 5; $attempt++) {
    try {
      Compress-Archive -Path $SourceDirectory -DestinationPath $DestinationPath -CompressionLevel Optimal -Force
      return
    } catch {
      if ($attempt -eq 5) {
        throw
      }

      Write-Warning "Compress-Archive failed on attempt $attempt. Retrying in 2 seconds. $($_.Exception.Message)"
      Start-Sleep -Seconds 2
      Remove-Item -Force -Path $DestinationPath -ErrorAction SilentlyContinue
    }
  }
}

if ([string]::IsNullOrWhiteSpace($Version)) {
  $Version = & (Join-Path $PSScriptRoot 'Get-LdkVersion.ps1')
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory = Join-Path $repoRoot 'artifacts\release'
}

$nugetDirectory = Join-Path $repoRoot 'artifacts\nuget'
$stagingDirectory = Join-Path $repoRoot 'artifacts\nuget-staging'
$workDirectory = Join-Path $repoRoot 'artifacts\release-staging'
$workDirectory = [System.IO.Path]::GetFullPath($workDirectory)
$repoRootPrefix = $repoRoot.TrimEnd('\') + '\'
if (-not $workDirectory.StartsWith($repoRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "Work directory must be inside the repository: $repoRoot"
}

$bundleRoot = Join-Path $workDirectory "ldk-$Version"
$bundleCMakePackageDirectory = Join-Path $bundleRoot 'share\ldk\cmake'
$prebuiltZipPath = Join-Path $OutputDirectory "ldk-$Version-prebuilt.zip"
$checksumPath = Join-Path $OutputDirectory "ldk-$Version-SHA256SUMS.txt"
$packagePath = Join-Path $nugetDirectory "ldk.$Version.nupkg"
$releasePackagePath = Join-Path $OutputDirectory "ldk.$Version.nupkg"

$Toolset = @(
  $Toolset |
    ForEach-Object { $_ -split ',' } |
    ForEach-Object { $_.Trim() } |
    Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
)
if ($Toolset.Count -eq 0) {
  throw "At least one LDK prebuilt MSVC toolset must be specified."
}
foreach ($selectedToolset in $Toolset) {
  if ($selectedToolset -ne 'v143' -and $selectedToolset -ne 'v145') {
    throw "Unsupported LDK prebuilt MSVC toolset: $selectedToolset. Supported toolsets are v143 and v145."
  }
}

if (-not (Test-Path $packagePath)) {
  throw "NuGet package was not found: $packagePath. Run scripts\nuget\Pack-LdkNuGet.ps1 first."
}

function Test-LdkToolsetArchitecture {
  param(
    [Parameter(Mandatory = $true)]
    [string] $MsvcToolset,

    [Parameter(Mandatory = $true)]
    [string] $Architecture
  )

  return -not ($MsvcToolset -eq 'v145' -and $Architecture -eq 'ARM')
}

function Remove-UnselectedStagedLibraries {
  param(
    [Parameter(Mandatory = $true)]
    [string] $NativeDirectory,

    [Parameter(Mandatory = $true)]
    [string[]] $SelectedToolsets,

    [Parameter(Mandatory = $true)]
    [string[]] $SelectedArchitectures
  )

  if (-not (Test-Path $NativeDirectory)) {
    return
  }

  foreach ($toolsetDirectory in Get-ChildItem -Path $NativeDirectory -Directory) {
    if ($SelectedToolsets -notcontains $toolsetDirectory.Name) {
      Remove-Item -LiteralPath $toolsetDirectory.FullName -Recurse -Force
      continue
    }

    foreach ($architectureDirectory in Get-ChildItem -Path $toolsetDirectory.FullName -Directory) {
      if ($SelectedArchitectures -notcontains $architectureDirectory.Name -or
          -not (Test-LdkToolsetArchitecture -MsvcToolset $toolsetDirectory.Name -Architecture $architectureDirectory.Name)) {
        Remove-Item -LiteralPath $architectureDirectory.FullName -Recurse -Force
        continue
      }

      foreach ($configurationDirectory in Get-ChildItem -Path $architectureDirectory.FullName -Directory) {
        if ($configurationDirectory.Name -ne 'Debug' -and $configurationDirectory.Name -ne 'Release') {
          Remove-Item -LiteralPath $configurationDirectory.FullName -Recurse -Force
        }
      }
    }
  }
}

foreach ($selectedToolset in $Toolset) {
  foreach ($arch in $Architecture) {
    if (-not (Test-LdkToolsetArchitecture -MsvcToolset $selectedToolset -Architecture $arch)) {
      $message = "Visual Studio 18 2026 / v145 does not support the 32-bit ARM target platform."
      if ($Toolset.Count -eq 1 -and $Architecture.Count -eq 1) {
        throw $message
      }

      Write-Warning "$message Skipping release asset validation for $selectedToolset $arch."
      continue
    }

    foreach ($config in @('Debug', 'Release')) {
      $requiredPath = Join-Path $stagingDirectory "lib\native\$selectedToolset\$arch\$config\Ldk.lib"
      if (-not (Test-Path $requiredPath)) {
        throw "Required prebuilt release asset file is missing: $requiredPath."
      }
    }
  }
}

Remove-UnselectedStagedLibraries `
  -NativeDirectory (Join-Path $stagingDirectory 'lib\native') `
  -SelectedToolsets $Toolset `
  -SelectedArchitectures $Architecture

Remove-Item -Recurse -Force -Path $workDirectory -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
foreach ($stalePattern in @('ldk.*.nupkg', 'ldk-*-prebuilt.zip', 'ldk-*-SHA256SUMS.txt')) {
  Get-ChildItem -Path $OutputDirectory -Filter $stalePattern -File -ErrorAction SilentlyContinue |
    Remove-Item -Force
}

New-Item -ItemType Directory -Force -Path $bundleRoot | Out-Null
New-Item -ItemType Directory -Force -Path $bundleCMakePackageDirectory | Out-Null

Copy-Item -Path (Join-Path $repoRoot 'README.md') -Destination $bundleRoot -Force
Copy-Item -Path (Join-Path $repoRoot 'LICENSE') -Destination $bundleRoot -Force
Copy-Item -Path (Join-Path $repoRoot 'docs') -Destination (Join-Path $bundleRoot 'docs') -Recurse -Force
Remove-Item -Path (Join-Path $bundleRoot 'docs\analysis') -Recurse -Force -ErrorAction SilentlyContinue
Copy-Item -Path (Join-Path $repoRoot 'include') -Destination (Join-Path $bundleRoot 'include') -Recurse -Force
Copy-Item -Path (Join-Path $repoRoot 'cmake') -Destination (Join-Path $bundleRoot 'cmake') -Recurse -Force
Copy-Item -Path (Join-Path $repoRoot 'cmake\*') -Destination $bundleCMakePackageDirectory -Recurse -Force
Copy-Item -Path (Join-Path $repoRoot 'nuget\build') -Destination (Join-Path $bundleRoot 'build') -Recurse -Force
Copy-Item -Path (Join-Path $stagingDirectory 'lib') -Destination (Join-Path $bundleRoot 'lib') -Recurse -Force

$versionMajor = ($Version -split '\.')[0]
$configCMake = @"
get_filename_component(PACKAGE_PREFIX_DIR "`${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)

set(ldk_ROOT "`${PACKAGE_PREFIX_DIR}")
set(LDK_ROOT "`${PACKAGE_PREFIX_DIR}")

include("`${CMAKE_CURRENT_LIST_DIR}/Ldk.cmake")
"@
Set-Content -LiteralPath (Join-Path $bundleCMakePackageDirectory 'ldk-config.cmake') -Value $configCMake -Encoding ASCII

$configVersionCMake = @"
set(PACKAGE_VERSION "$Version")

if(PACKAGE_FIND_VERSION)
  if(PACKAGE_FIND_VERSION VERSION_GREATER PACKAGE_VERSION)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
  elseif(NOT PACKAGE_FIND_VERSION_MAJOR EQUAL $versionMajor)
    set(PACKAGE_VERSION_COMPATIBLE FALSE)
  else()
    set(PACKAGE_VERSION_COMPATIBLE TRUE)
    if(PACKAGE_FIND_VERSION VERSION_EQUAL PACKAGE_VERSION)
      set(PACKAGE_VERSION_EXACT TRUE)
    endif()
  endif()
endif()
"@
Set-Content -LiteralPath (Join-Path $bundleCMakePackageDirectory 'ldk-config-version.cmake') -Value $configVersionCMake -Encoding ASCII

$bundleReadme = @"
# LDK $Version Prebuilt Release Bundle

This archive is an offline-friendly prebuilt bundle built by the GitHub Actions
Package workflow.

Contents:

- include/: public LDK headers
- cmake/: CMake helpers
- share/ldk/cmake/: CMake package config for find_package(ldk CONFIG)
- build/native/: native MSBuild props and targets from the NuGet package
- lib/native/<toolset>/: prebuilt Ldk.lib for x86, x64, ARM, and ARM64, Debug and Release
- docs/: repository documentation

The prebuilt driver libraries are grouped by MSVC platform toolset. v143
contains x86, x64, ARM, and ARM64 libraries. v145 contains x86, x64, and
ARM64 libraries because Visual Studio 2026 removed the 32-bit ARM target.
Validate the final driver with the Windows, WDK, SDK, Visual Studio,
architecture, Driver Verifier, and code integrity settings that you ship.
"@
Set-Content -LiteralPath (Join-Path $bundleRoot 'README.release.md') -Value $bundleReadme -Encoding UTF8

Remove-Item -Force -Path $prebuiltZipPath -ErrorAction SilentlyContinue
Remove-Item -Force -Path $releasePackagePath -ErrorAction SilentlyContinue
Remove-Item -Force -Path $checksumPath -ErrorAction SilentlyContinue

Compress-DirectoryWithRetry -SourceDirectory $bundleRoot -DestinationPath $prebuiltZipPath
Copy-Item -Path $packagePath -Destination $releasePackagePath -Force

Get-FileHash -Algorithm SHA256 -Path $prebuiltZipPath, $releasePackagePath |
  ForEach-Object { "$($_.Hash.ToLowerInvariant())  $([System.IO.Path]::GetFileName($_.Path))" } |
  Set-Content -LiteralPath $checksumPath -Encoding ASCII

Write-Host "Created release assets:"
foreach ($releaseAssetPath in @($releasePackagePath, $prebuiltZipPath, $checksumPath)) {
  if (Test-Path $releaseAssetPath) {
    Write-Host "  $releaseAssetPath"
  }
}
