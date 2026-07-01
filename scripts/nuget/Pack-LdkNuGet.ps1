param(
  [string] $Version,
  [string] $OutputDirectory,
  [string] $NuGetExe,

  [string[]] $Toolset = @('v143'),

  [ValidateSet('x86', 'x64', 'ARM', 'ARM64')]
  [string[]] $Architecture = @('x86', 'x64', 'ARM', 'ARM64')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

if ([string]::IsNullOrWhiteSpace($Version)) {
  $Version = & (Join-Path $PSScriptRoot 'Get-LdkVersion.ps1')
}

if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory = Join-Path $repoRoot 'artifacts\nuget'
}

$manifest = Join-Path $repoRoot 'nuget\ldk.nuspec'
$stagingDirectory = Join-Path $repoRoot 'artifacts\nuget-staging'
New-Item -ItemType Directory -Force -Path $OutputDirectory | Out-Null

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

      Write-Warning "$message Skipping package validation for $selectedToolset $arch."
      continue
    }

    foreach ($config in @('Debug', 'Release')) {
      $requiredPath = "lib\native\$selectedToolset\$arch\$config\Ldk.lib"
      $fullPath = Join-Path $stagingDirectory $requiredPath
      if (-not (Test-Path $fullPath)) {
        throw "Required prebuilt library is missing: $fullPath. Run scripts\nuget\Build-LdkNuGetLibs.ps1 first."
      }
    }
  }
}

Remove-UnselectedStagedLibraries `
  -NativeDirectory (Join-Path $stagingDirectory 'lib\native') `
  -SelectedToolsets $Toolset `
  -SelectedArchitectures $Architecture

Write-Host "Packing LDK $Version to $OutputDirectory"
if ([string]::IsNullOrWhiteSpace($NuGetExe)) {
  $nuget = Get-Command nuget -ErrorAction SilentlyContinue
  if (-not $nuget) {
    throw "nuget command was not found. Pass -NuGetExe or add nuget.exe to PATH."
  }

  $NuGetExe = $nuget.Source
} else {
  $NuGetExe = (Resolve-Path $NuGetExe).Path
}

& $NuGetExe pack $manifest `
  -OutputDirectory $OutputDirectory `
  -Version $Version `
  -NonInteractive

if ($LASTEXITCODE -ne 0) {
  throw "nuget pack failed with exit code $LASTEXITCODE."
}
