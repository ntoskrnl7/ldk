param(
  [string] $WindowsSdkVersion = '10.0.26100.0'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Test-LdkWdkVersion {
  param(
    [Parameter(Mandatory = $true)]
    [string] $KitRoot,

    [Parameter(Mandatory = $true)]
    [string] $Version
  )

  $ntddk = Join-Path $KitRoot "Include\$Version\km\ntddk.h"
  if (-not (Test-Path $ntddk)) {
    return $false
  }

  foreach ($platform in @('x64', 'arm64')) {
    $ntoskrnl = Join-Path $KitRoot "Lib\$Version\km\$platform\ntoskrnl.lib"
    if (-not (Test-Path $ntoskrnl)) {
      return $false
    }
  }

  return $true
}

$kitRoots = @(
  "${env:ProgramFiles(x86)}\Windows Kits\10",
  "${env:ProgramFiles}\Windows Kits\10"
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

foreach ($kitRoot in $kitRoots) {
  if ((Test-Path $kitRoot) -and (Test-LdkWdkVersion -KitRoot $kitRoot -Version $WindowsSdkVersion)) {
    Write-Host "Found WDK $WindowsSdkVersion under $kitRoot."
    return
  }
}

if ($WindowsSdkVersion -ne '10.0.26100.0') {
  throw "WDK $WindowsSdkVersion was not found. This helper can install WDK 10.0.26100.0 only."
}

Write-Host "WDK $WindowsSdkVersion was not found. Installing Microsoft.WindowsWDK.10.0.26100 with WinGet."
winget install `
  --exact `
  --id Microsoft.WindowsWDK.10.0.26100 `
  --source winget `
  --silent `
  --accept-package-agreements `
  --accept-source-agreements `
  --disable-interactivity

if ($LASTEXITCODE -ne 0) {
  throw "WDK installation failed with exit code $LASTEXITCODE."
}

foreach ($kitRoot in $kitRoots) {
  if ((Test-Path $kitRoot) -and (Test-LdkWdkVersion -KitRoot $kitRoot -Version $WindowsSdkVersion)) {
    Write-Host "Found WDK $WindowsSdkVersion under $kitRoot."
    return
  }
}

throw "WDK $WindowsSdkVersion was not found after installation."
