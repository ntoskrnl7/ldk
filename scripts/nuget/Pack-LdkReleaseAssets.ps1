param(
  [string] $Version,
  [string] $OutputDirectory
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

if (-not (Test-Path $packagePath)) {
  throw "NuGet package was not found: $packagePath. Run scripts\nuget\Pack-LdkNuGet.ps1 first."
}

foreach ($arch in @('x86', 'x64', 'ARM', 'ARM64')) {
  foreach ($config in @('Debug', 'Release')) {
    $requiredPath = Join-Path $stagingDirectory "lib\native\$arch\$config\Ldk.lib"
    if (-not (Test-Path $requiredPath)) {
      throw "Required prebuilt release asset file is missing: $requiredPath."
    }
  }
}

Remove-Item -Recurse -Force -Path $workDirectory -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null
New-Item -ItemType Directory -Force -Path $bundleRoot | Out-Null
New-Item -ItemType Directory -Force -Path $bundleCMakePackageDirectory | Out-Null

Copy-Item -Path (Join-Path $repoRoot 'README.md') -Destination $bundleRoot -Force
Copy-Item -Path (Join-Path $repoRoot 'LICENSE') -Destination $bundleRoot -Force
Copy-Item -Path (Join-Path $repoRoot 'docs') -Destination (Join-Path $bundleRoot 'docs') -Recurse -Force
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
- lib/native/: prebuilt Ldk.lib for x86, x64, ARM, and ARM64, Debug and Release
- docs/: repository documentation

The prebuilt driver libraries target Visual Studio 2022 and Windows SDK/WDK
10.0.22621.0. Validate the final driver with the Windows, WDK, SDK, Visual
Studio, architecture, Driver Verifier, and code integrity settings that you
ship.
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
Get-ChildItem -Path $OutputDirectory -File | Sort-Object FullName | ForEach-Object {
  Write-Host "  $($_.FullName)"
}
