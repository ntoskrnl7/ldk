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

function Test-LdkWdkAvailable {
  param(
    [Parameter(Mandatory = $true)]
    [string[]] $KitRoots,

    [Parameter(Mandatory = $true)]
    [string] $Version,

    [Parameter(Mandatory = $true)]
    [string[]] $Platforms
  )

  foreach ($kitRoot in $KitRoots) {
    if ((Test-Path $kitRoot) -and (Test-LdkWdkVersion -KitRoot $kitRoot -Version $Version -Platforms $Platforms)) {
      return $true
    }
  }

  return $false
}

function Complete-LdkWdkSuccess {
  $global:LASTEXITCODE = 0
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

function Invoke-LdkWinGetWdkInstall {
  param(
    [Parameter(Mandatory = $true)]
    [string] $WinGetPath,

    [Parameter(Mandatory = $true)]
    [string] $PackageId
  )

  & $WinGetPath install `
    --exact `
    --id $PackageId `
    --source winget `
    --silent `
    --accept-package-agreements `
    --accept-source-agreements `
    --disable-interactivity 2>&1 | ForEach-Object { Write-Host $_ }

  return [int]$LASTEXITCODE
}

function Repair-LdkWinGetSource {
  param(
    [Parameter(Mandatory = $true)]
    [string] $WinGetPath
  )

  Write-Host "Resetting WinGet sources before retrying WDK installation."
  & $WinGetPath source reset --force
  if ($LASTEXITCODE -ne 0) {
    Write-Warning "WinGet source reset failed with exit code $LASTEXITCODE."
  }

  Write-Host "Updating WinGet sources before retrying WDK installation."
  & $WinGetPath source update
  if ($LASTEXITCODE -ne 0) {
    Write-Warning "WinGet source update failed with exit code $LASTEXITCODE."
  }
}

function Save-LdkFileWithRetry {
  param(
    [Parameter(Mandatory = $true)]
    [string] $Uri,

    [Parameter(Mandatory = $true)]
    [string] $OutFile,

    [Parameter(Mandatory = $true)]
    [string] $Description,

    [int] $Attempts = 3
  )

  $lastErrorMessage = $null

  for ($attempt = 1; $attempt -le $Attempts; $attempt++) {
    Remove-Item -LiteralPath $OutFile -Force -ErrorAction SilentlyContinue

    try {
      Write-Host "Downloading $Description (attempt $attempt of $Attempts)."
      Invoke-WebRequest -Uri $Uri -OutFile $OutFile -UseBasicParsing -TimeoutSec 180
      return
    } catch {
      $lastErrorMessage = $_.Exception.Message
      Write-Warning "Download attempt $attempt for $Description failed: $lastErrorMessage"
      if ($attempt -lt $Attempts) {
        Start-Sleep -Seconds ([Math]::Min(60, 10 * $attempt))
      }
    }
  }

  $curl = Get-Command curl.exe -ErrorAction SilentlyContinue
  if ($curl) {
    Remove-Item -LiteralPath $OutFile -Force -ErrorAction SilentlyContinue

    Write-Host "Retrying $Description download with curl.exe."
    & $curl.Source `
      --fail `
      --location `
      --retry 3 `
      --retry-delay 10 `
      --connect-timeout 30 `
      --max-time 300 `
      --output $OutFile `
      $Uri

    if ($LASTEXITCODE -eq 0) {
      return
    }

    $lastErrorMessage = "curl.exe failed with exit code $LASTEXITCODE"
  }

  throw "Failed to download $Description. Last error: $lastErrorMessage"
}

function Install-LdkWdkFromDownloadedInstaller {
  param(
    [Parameter(Mandatory = $true)]
    [hashtable] $Installer
  )

  $installerPath = Join-Path $env:TEMP "ldk-$($installer.PackageId)-wdksetup.exe"
  Save-LdkFileWithRetry `
    -Uri $installer.Url `
    -OutFile $installerPath `
    -Description "$($installer.PackageId) from Microsoft"

  $actualHash = (Get-FileHash -Path $installerPath -Algorithm SHA256).Hash
  if ($actualHash -ne $installer.Sha256) {
    Remove-Item -LiteralPath $installerPath -Force -ErrorAction SilentlyContinue
    throw "WDK installer hash mismatch. Expected $($installer.Sha256), got $actualHash."
  }

  Write-Host "Installing $($installer.PackageId) from downloaded installer."
  $process = Start-Process -FilePath $installerPath -ArgumentList '/q' -Wait -PassThru -WindowStyle Hidden
  Remove-Item -LiteralPath $installerPath -Force -ErrorAction SilentlyContinue

  return [int]$process.ExitCode
}

function Install-LdkWdkVersion {
  param(
    [Parameter(Mandatory = $true)]
    [string] $Version,

    [Parameter(Mandatory = $true)]
    [string[]] $KitRoots,

    [Parameter(Mandatory = $true)]
    [string[]] $Platforms
  )

  $installer = Get-LdkWdkInstaller -Version $Version
  $winget = Get-Command winget -ErrorAction SilentlyContinue

  if ($winget) {
    for ($attempt = 1; $attempt -le 2; $attempt++) {
      Write-Host "Installing $($installer.PackageId) with WinGet (attempt $attempt of 2)."
      $exitCode = Invoke-LdkWinGetWdkInstall -WinGetPath $winget.Source -PackageId $installer.PackageId
      if ($exitCode -eq 0) {
        return
      }

      Write-Warning "WinGet WDK installation failed with exit code $exitCode."
      if (Test-LdkWdkAvailable -KitRoots $KitRoots -Version $Version -Platforms $Platforms) {
        Write-Host "Found WDK $Version after WinGet returned exit code $exitCode."
        return
      }

      if ($attempt -eq 1) {
        Repair-LdkWinGetSource -WinGetPath $winget.Source
      }
    }

    if (Test-LdkWdkAvailable -KitRoots $KitRoots -Version $Version -Platforms $Platforms) {
      Write-Host "Found WDK $Version after WinGet installation attempts."
      return
    }

    Write-Warning "WinGet could not install $($installer.PackageId). Falling back to the Microsoft installer URL."
  } else {
    Write-Host "WinGet was not found. Falling back to the Microsoft installer URL."
  }

  $installerExitCode = Install-LdkWdkFromDownloadedInstaller -Installer $installer
  if ($installerExitCode -eq 0) {
    return
  }

  if (Test-LdkWdkAvailable -KitRoots $KitRoots -Version $Version -Platforms $Platforms) {
    Write-Warning "WDK installer returned exit code $installerExitCode, but WDK $Version is available."
    return
  }

  throw "WDK installer failed with exit code $installerExitCode."
}

$kitRoots = @(
  "${env:ProgramFiles(x86)}\Windows Kits\10",
  "${env:ProgramFiles}\Windows Kits\10"
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

$requiredWdkPlatforms = @(
  foreach ($platformValue in $RequiredPlatforms) {
    foreach ($platform in ($platformValue -split ',')) {
      $normalizedPlatform = $platform.Trim()
      if (-not [string]::IsNullOrWhiteSpace($normalizedPlatform)) {
        ConvertTo-LdkWdkPlatform -Platform $normalizedPlatform
      }
    }
  }
) | Select-Object -Unique

if (Test-LdkWdkAvailable -KitRoots $kitRoots -Version $WindowsSdkVersion -Platforms $requiredWdkPlatforms) {
  foreach ($kitRoot in $kitRoots) {
    if ((Test-Path $kitRoot) -and (Test-LdkWdkVersion -KitRoot $kitRoot -Version $WindowsSdkVersion -Platforms $requiredWdkPlatforms)) {
      Write-Host "Found WDK $WindowsSdkVersion under $kitRoot."
      Complete-LdkWdkSuccess
      return
    }
  }
}

Write-Host "WDK $WindowsSdkVersion was not found with required platforms: $($requiredWdkPlatforms -join ', ')."
Install-LdkWdkVersion -Version $WindowsSdkVersion -KitRoots $kitRoots -Platforms $requiredWdkPlatforms

if (Test-LdkWdkAvailable -KitRoots $kitRoots -Version $WindowsSdkVersion -Platforms $requiredWdkPlatforms) {
  foreach ($kitRoot in $kitRoots) {
    if ((Test-Path $kitRoot) -and (Test-LdkWdkVersion -KitRoot $kitRoot -Version $WindowsSdkVersion -Platforms $requiredWdkPlatforms)) {
      Write-Host "Found WDK $WindowsSdkVersion under $kitRoot."
      Complete-LdkWdkSuccess
      return
    }
  }
}

throw "WDK $WindowsSdkVersion with required platforms '$($requiredWdkPlatforms -join ', ')' was not found after installation."
