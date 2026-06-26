param(
  [ValidateSet('x86', 'x64', 'ARM', 'ARM64')]
  [string[]] $Architecture = @('x86', 'x64', 'ARM', 'ARM64'),

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

$platformByArchitecture = @{
  x86 = 'Win32'
  x64 = 'x64'
  ARM = 'ARM'
  ARM64 = 'ARM64'
}

foreach ($arch in $Architecture) {
  foreach ($config in $Configuration) {
    $platform = $platformByArchitecture[$arch]
    if ($arch -eq 'ARM') {
      # Windows SDK 10.0.26100.0 no longer supports 32-bit ARM. Pin the
      # Visual Studio generator to the older SDK while keeping ARM64 on the
      # normal platform form.
      $platform = "$platform,version=$WindowsSdkVersion"
    }

    $buildDir = Join-Path $repoRoot "artifacts\build\ldk_${arch}_$config"
    $libOutputDir = Join-Path $OutputDirectory "lib\native\$arch\$config"

    New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
    New-Item -ItemType Directory -Force -Path $libOutputDir | Out-Null

    $configureArgs = @(
      '-S', $repoRoot,
      '-B', $buildDir,
      '-G', 'Visual Studio 17 2022',
      '-A', $platform,
      '-T', 'host=x64',
      "-DCMAKE_SYSTEM_VERSION=$WindowsSdkVersion",
      "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=$WindowsSdkVersion",
      "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM=$WindowsSdkVersion",
      '-DCMAKE_C_FLAGS=/MP',
      '-DCMAKE_CXX_FLAGS=/MP'
    )

    Write-Host "Configuring LDK $arch $config with Windows SDK $WindowsSdkVersion"
    & cmake @configureArgs
    if ($LASTEXITCODE -ne 0) {
      throw "CMake configure failed with exit code $LASTEXITCODE."
    }

    Write-Host "Building LDK $arch $config"
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
