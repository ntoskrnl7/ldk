param(
  [ValidateSet('x86', 'x64', 'ARM', 'ARM64')]
  [string[]] $Architecture = @('x86', 'x64', 'ARM', 'ARM64'),

  [string[]] $Toolset = @('v143'),

  [ValidateSet('Debug', 'Release')]
  [string[]] $Configuration = @('Debug', 'Release'),

  [string] $WindowsSdkVersion = '10.0.22621.0',

  [string] $OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
if ([string]::IsNullOrWhiteSpace($OutputDirectory)) {
  $OutputDirectory = Join-Path $repoRoot 'artifacts\nuget-staging'
}

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
  if ($selectedToolset -ne 'v142' -and $selectedToolset -ne 'v143' -and $selectedToolset -ne 'v145') {
    throw "Unsupported LDK prebuilt MSVC toolset: $selectedToolset. Supported toolsets are v142, v143, and v145."
  }
}

$platformByArchitecture = @{
  x86 = 'Win32'
  x64 = 'x64'
  ARM = 'ARM'
  ARM64 = 'ARM64'
}

$generatorByToolset = @{
  v142 = 'Visual Studio 17 2022'
  v143 = 'Visual Studio 17 2022'
  v145 = 'Visual Studio 18 2026'
}

$generatorToolsetByToolset = @{
  v142 = 'v142,host=x64'
  v143 = 'host=x64'
  v145 = 'host=x64'
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

foreach ($selectedToolset in $Toolset) {
  foreach ($arch in $Architecture) {
    if (-not (Test-LdkToolsetArchitecture -MsvcToolset $selectedToolset -Architecture $arch)) {
      $message = "Visual Studio 18 2026 / v145 does not support the 32-bit ARM target platform. Build ARM with v142 or v143, or omit ARM when staging v145 libraries."
      if ($Toolset.Count -eq 1 -and $Architecture.Count -eq 1) {
        throw $message
      }

      Write-Warning "$message Skipping $selectedToolset $arch."
      continue
    }

    foreach ($config in $Configuration) {
    $platform = $platformByArchitecture[$arch]
    if ($arch -eq 'ARM') {
      # Windows SDK 10.0.26100.0 no longer supports 32-bit ARM. Pin the
      # Visual Studio generator to the older SDK while keeping ARM64 on the
      # normal platform form.
      $platform = "$platform,version=$WindowsSdkVersion"
    }

    $buildDir = Join-Path $repoRoot "artifacts\build\ldk_${selectedToolset}_${arch}_$config"
    $libOutputDir = Join-Path $OutputDirectory "lib\native\$selectedToolset\$arch\$config"

    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    New-Item -ItemType Directory -Force -Path $libOutputDir | Out-Null

    $configureArgs = @(
      '-S', $repoRoot,
      '-B', $buildDir,
      '-G', $generatorByToolset[$selectedToolset],
      '-A', $platform,
      '-T', $generatorToolsetByToolset[$selectedToolset],
      "-DLDK_WDK_VERSION=$WindowsSdkVersion",
      "-DCMAKE_SYSTEM_VERSION=$WindowsSdkVersion",
      "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=$WindowsSdkVersion",
      "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM=$WindowsSdkVersion",
      '-DCMAKE_C_FLAGS=/MP',
      '-DCMAKE_CXX_FLAGS=/MP'
    )

    Write-Host "Configuring LDK $selectedToolset $arch $config with Windows SDK $WindowsSdkVersion"
    & cmake @configureArgs
    if ($LASTEXITCODE -ne 0) {
      throw "CMake configure failed with exit code $LASTEXITCODE."
    }

    Write-Host "Building LDK $selectedToolset $arch $config"
    & cmake --build $buildDir --config $config --target Ldk --parallel
    if ($LASTEXITCODE -ne 0) {
      throw "CMake build failed with exit code $LASTEXITCODE."
    }

    $preferredLibPaths = if ($config -eq 'Debug') {
      @(
        Join-Path $repoRoot "lib\$arch\Debug\Ldk.lib"
        Join-Path $buildDir "$config\Ldk.lib"
      )
    } else {
      @(
        Join-Path $repoRoot "lib\$arch\Ldk.lib"
        Join-Path $buildDir "$config\Ldk.lib"
      )
    }

    $ldkLib = $preferredLibPaths | Where-Object { Test-Path $_ } | Select-Object -First 1
    if ([string]::IsNullOrWhiteSpace($ldkLib)) {
      $available = Get-ChildItem -Path $repoRoot, $buildDir -Filter 'Ldk.lib' -Recurse -File -ErrorAction SilentlyContinue |
        ForEach-Object { $_.FullName }
      throw "Ldk.lib was not found for $arch $config. Available libs: $($available -join ', ')"
    }

    Copy-Item -Path $ldkLib -Destination (Join-Path $libOutputDir 'Ldk.lib') -Force

    $pdbCandidates = @(
      Get-ChildItem -Path (Split-Path -Parent $ldkLib) -Filter 'Ldk.pdb' -File -ErrorAction SilentlyContinue
    )
    foreach ($pdb in $pdbCandidates) {
      Copy-Item -Path $pdb.FullName -Destination (Join-Path $libOutputDir $pdb.Name) -Force
    }

    Write-Host "Staged LDK library in $libOutputDir"
    }
  }
}
