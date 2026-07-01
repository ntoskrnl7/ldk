param(
  [string] $WindowsSdkVersion = '10.0.26100.0',
  [string[]] $RequiredPlatforms = @('x64', 'arm64')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function ConvertTo-LdkWdkPlatform {
  param(
    [Parameter(Mandatory = $true)]
    [string] $Platform
  )

  switch ($Platform.ToLowerInvariant()) {
    'win32' { return 'x86' }
    'x86' { return 'x86' }
    'x64' { return 'x64' }
    'arm' { return 'arm' }
    'arm64' { return 'arm64' }
    default { throw "Unsupported WDK platform '$Platform'." }
  }
}

function Test-LdkWdkPlatformLibrary {
  param(
    [Parameter(Mandatory = $true)]
    [string] $KitRoot,

    [Parameter(Mandatory = $true)]
    [string] $Version,

    [Parameter(Mandatory = $true)]
    [string] $Platform
  )

  $platformCandidates = @(
    $Platform,
    $Platform.ToLowerInvariant(),
    $Platform.ToUpperInvariant()
  ) | Select-Object -Unique

  foreach ($platformCandidate in $platformCandidates) {
    $ntoskrnl = Join-Path $KitRoot "Lib\$Version\km\$platformCandidate\ntoskrnl.lib"
    if (Test-Path $ntoskrnl) {
      return $true
    }
  }

  return $false
}

function Test-LdkWdkVersion {
  param(
    [Parameter(Mandatory = $true)]
    [string] $KitRoot,

    [Parameter(Mandatory = $true)]
    [string] $Version,

    [Parameter(Mandatory = $true)]
    [string[]] $Platforms
  )

  $ntddk = Join-Path $KitRoot "Include\$Version\km\ntddk.h"
  if (-not (Test-Path $ntddk)) {
    return $false
  }

  foreach ($platform in $Platforms) {
    if (-not (Test-LdkWdkPlatformLibrary -KitRoot $KitRoot -Version $Version -Platform $platform)) {
      return $false
    }
  }

  return $true
}

function Get-LdkWdkInstaller {
  param(
    [Parameter(Mandatory = $true)]
    [string] $Version
  )

  switch ($Version) {
    '10.0.22621.0' {
      return @{
        PackageId = 'Microsoft.WindowsWDK.10.0.22621'
        Url = 'https://download.microsoft.com/download/7/b/f/7bfc8dbe-00cb-47de-b856-70e696ef4f46/wdk/wdksetup.exe'
        Sha256 = 'A891543C0EAF610757EE7852F515C5CE89D0202F22A15DFA31C4D49F2BAFDF27'
      }
    }
    '10.0.26100.0' {
      return @{
        PackageId = 'Microsoft.WindowsWDK.10.0.26100'
        Url = 'https://download.microsoft.com/download/41fb59c2-1723-45f9-a270-96b73ad58233/KIT_BUNDLE_WDK_MEDIACREATION/wdksetup.exe'
        Sha256 = 'ED82C46BD98E0F1D07FD6E5075900E42AEDC3FA5E68C06A8764B3DC5303CFF1B'
      }
    }
    default {
      throw "No WDK installer is known for Windows SDK version '$Version'."
    }
  }
}

function Install-LdkWdkVersion {
  param(
    [Parameter(Mandatory = $true)]
    [string] $Version
  )

  $installer = Get-LdkWdkInstaller -Version $Version
  $winget = Get-Command winget -ErrorAction SilentlyContinue

  if ($winget) {
    Write-Host "Installing $($installer.PackageId) with WinGet."
    & $winget.Source install `
      --exact `
      --id $installer.PackageId `
      --source winget `
      --silent `
      --accept-package-agreements `
      --accept-source-agreements `
      --disable-interactivity

    if ($LASTEXITCODE -ne 0) {
      throw "WDK installation failed with exit code $LASTEXITCODE."
    }

    return
  }

  $installerPath = Join-Path $env:TEMP "ldk-$($installer.PackageId)-wdksetup.exe"
  Write-Host "WinGet was not found. Downloading $($installer.PackageId) from Microsoft."
  Invoke-WebRequest -Uri $installer.Url -OutFile $installerPath

  $actualHash = (Get-FileHash -Path $installerPath -Algorithm SHA256).Hash
  if ($actualHash -ne $installer.Sha256) {
    Remove-Item -LiteralPath $installerPath -Force -ErrorAction SilentlyContinue
    throw "WDK installer hash mismatch. Expected $($installer.Sha256), got $actualHash."
  }

  Write-Host "Installing $($installer.PackageId) from downloaded installer."
  $process = Start-Process -FilePath $installerPath -ArgumentList '/q' -Wait -PassThru -WindowStyle Hidden
  Remove-Item -LiteralPath $installerPath -Force -ErrorAction SilentlyContinue

  if ($process.ExitCode -ne 0) {
    throw "WDK installer failed with exit code $($process.ExitCode)."
  }
}

$kitRoots = @(
  "${env:ProgramFiles(x86)}\Windows Kits\10",
  "${env:ProgramFiles}\Windows Kits\10"
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

$requiredWdkPlatforms = @(
  foreach ($platform in $RequiredPlatforms) {
    ConvertTo-LdkWdkPlatform -Platform $platform
  }
) | Select-Object -Unique

foreach ($kitRoot in $kitRoots) {
  if ((Test-Path $kitRoot) -and (Test-LdkWdkVersion -KitRoot $kitRoot -Version $WindowsSdkVersion -Platforms $requiredWdkPlatforms)) {
    Write-Host "Found WDK $WindowsSdkVersion under $kitRoot."
    return
  }
}

Write-Host "WDK $WindowsSdkVersion was not found with required platforms: $($requiredWdkPlatforms -join ', ')."
Install-LdkWdkVersion -Version $WindowsSdkVersion

foreach ($kitRoot in $kitRoots) {
  if ((Test-Path $kitRoot) -and (Test-LdkWdkVersion -KitRoot $kitRoot -Version $WindowsSdkVersion -Platforms $requiredWdkPlatforms)) {
    Write-Host "Found WDK $WindowsSdkVersion under $kitRoot."
    return
  }
}

throw "WDK $WindowsSdkVersion with required platforms '$($requiredWdkPlatforms -join ', ')' was not found after installation."
