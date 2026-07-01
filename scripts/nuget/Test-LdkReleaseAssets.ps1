param(
  [string] $ReleaseDirectory,

  [Parameter(Mandatory = $true)]
  [string] $Version,

  [ValidateSet('x86', 'x64', 'ARM', 'ARM64')]
  [string] $Architecture = 'x64',

  [ValidateSet('v142', 'v143', 'v145')]
  [string] $Toolset = 'v143',

  [ValidateSet('Debug', 'Release')]
  [string] $Configuration = 'Release',

  [string] $WindowsSdkVersion = '10.0.22621.0',

  [string] $WorkDirectory,

  [switch] $SkipDriverBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

if ([string]::IsNullOrWhiteSpace($ReleaseDirectory)) {
  $ReleaseDirectory = Join-Path $repoRoot 'artifacts\release'
}

if ([string]::IsNullOrWhiteSpace($WorkDirectory)) {
  $WorkDirectory = Join-Path $repoRoot "artifacts\release-consumer-test\$Toolset\$Architecture\$Configuration"
}

$WorkDirectory = [System.IO.Path]::GetFullPath($WorkDirectory)
$repoRootPrefix = $repoRoot.TrimEnd('\') + '\'
if (-not $WorkDirectory.StartsWith($repoRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "WorkDirectory must be inside the repository: $repoRoot"
}

$ReleaseDirectory = (Resolve-Path $ReleaseDirectory).Path
$prebuiltZipPath = Join-Path $ReleaseDirectory "ldk-$Version-prebuilt.zip"
if (-not (Test-Path $prebuiltZipPath)) {
  throw "Prebuilt release zip was not found: $prebuiltZipPath"
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

if ($Toolset -eq 'v145' -and $Architecture -eq 'ARM') {
  throw "Visual Studio 18 2026 / v145 does not provide the 32-bit ARM generator platform in the tested Build Tools layout. Test ARM with v142 or v143, or omit ARM when testing v145 libraries."
}

$platform = $platformByArchitecture[$Architecture]
if ($Architecture -eq 'ARM') {
  # Windows SDK 10.0.26100.0 no longer supports 32-bit ARM. Pin the Visual
  # Studio generator to the older SDK for ARM consumer builds.
  $platform = "$platform,version=$WindowsSdkVersion"
}

Remove-Item -Recurse -Force -Path $WorkDirectory -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $WorkDirectory | Out-Null

$unpackDirectory = Join-Path $WorkDirectory 'prebuilt'
Expand-Archive -Path $prebuiltZipPath -DestinationPath $unpackDirectory -Force

$bundleRoot = Join-Path $unpackDirectory "ldk-$Version"
if (-not (Test-Path (Join-Path $bundleRoot 'cmake\Ldk.cmake'))) {
  if (Test-Path (Join-Path $unpackDirectory 'cmake\Ldk.cmake')) {
    $bundleRoot = $unpackDirectory
  } else {
    $bundleRoot = Get-ChildItem -Path $unpackDirectory -Directory |
      Where-Object { Test-Path (Join-Path $_.FullName 'cmake\Ldk.cmake') } |
      Select-Object -First 1 -ExpandProperty FullName
  }
}

if ([string]::IsNullOrWhiteSpace($bundleRoot) -or -not (Test-Path (Join-Path $bundleRoot 'cmake\Ldk.cmake'))) {
  throw "Unpacked prebuilt release bundle does not contain cmake\Ldk.cmake."
}

foreach ($requiredPath in @(
  "lib\native\$Toolset\$Architecture\$Configuration\Ldk.lib",
  'share\ldk\cmake\ldk-config.cmake',
  'share\ldk\cmake\ldk-config-version.cmake',
  'share\ldk\cmake\Ldk.cmake',
  'include\Ldk\Windows.h'
)) {
  $fullPath = Join-Path $bundleRoot $requiredPath
  if (-not (Test-Path $fullPath)) {
    throw "Prebuilt release bundle is missing expected file: $fullPath"
  }
}

if ($SkipDriverBuild) {
  Write-Host "Prebuilt release layout test passed for $Architecture ${Configuration}: $bundleRoot"
  return
}

$consumerDirectory = Join-Path $WorkDirectory 'consumer'
New-Item -ItemType Directory -Force -Path $consumerDirectory | Out-Null

$cmakeBundleRoot = $bundleRoot.Replace('\', '/')
$cmakeLists = @"
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(POLICY CMP0149)
  cmake_policy(SET CMP0149 OLD)
endif()

project(ldk_release_asset_smoke LANGUAGES C)

find_package(ldk CONFIG REQUIRED PATHS "$cmakeBundleRoot" NO_DEFAULT_PATH)
set(LDK_PREBUILT_TOOLSET "$Toolset")

ldk_add_driver(ldk_release_asset_smoke main.c)
"@
Set-Content -LiteralPath (Join-Path $consumerDirectory 'CMakeLists.txt') -Value $cmakeLists -Encoding UTF8

$mainC = @'
#include <Ldk/Windows.h>

DRIVER_UNLOAD DriverUnload;

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
}

NTSTATUS
NTAPI
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}
'@
Set-Content -LiteralPath (Join-Path $consumerDirectory 'main.c') -Value $mainC -Encoding UTF8

$buildDirectory = Join-Path $WorkDirectory "build_${Toolset}_$Architecture"
$configureArgs = @(
  '-S', $consumerDirectory,
  '-B', $buildDirectory,
  '-G', $generatorByToolset[$Toolset],
  '-A', $platform,
  '-T', $generatorToolsetByToolset[$Toolset],
  "-DLDK_WDK_VERSION=$WindowsSdkVersion",
  "-DCMAKE_SYSTEM_VERSION=$WindowsSdkVersion",
  "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=$WindowsSdkVersion",
  "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM=$WindowsSdkVersion"
)

Write-Host "Configuring prebuilt release asset consumer for $Toolset $Architecture $Configuration"
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
  throw "CMake configure failed with exit code $LASTEXITCODE."
}

Write-Host "Building prebuilt release asset consumer for $Toolset $Architecture $Configuration"
& cmake --build $buildDirectory --config $Configuration --target ldk_release_asset_smoke --parallel
if ($LASTEXITCODE -ne 0) {
  throw "CMake build failed with exit code $LASTEXITCODE."
}

$driverPath = Join-Path $buildDirectory "$Configuration\ldk_release_asset_smoke.sys"
if (-not (Test-Path $driverPath)) {
  throw "Prebuilt release asset consumer driver was not produced: $driverPath"
}

Write-Host "Prebuilt release asset consumer test passed: $driverPath"
