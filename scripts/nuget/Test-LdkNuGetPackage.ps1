param(
  [string] $PackageDirectory,

  [Parameter(Mandatory = $true)]
  [string] $Version,

  [ValidateSet('x86', 'x64')]
  [string] $Architecture = 'x64',

  [ValidateSet('Debug', 'Release')]
  [string] $Configuration = 'Release',

  [string] $WindowsSdkVersion = '10.0.22621.0',

  [string] $WorkDirectory,

  [string] $NuGetExe,

  [switch] $SkipDriverBuild
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
  $PackageDirectory = Join-Path $repoRoot 'artifacts\nuget'
}

if ([string]::IsNullOrWhiteSpace($WorkDirectory)) {
  $WorkDirectory = Join-Path $repoRoot "artifacts\nuget-consumer-test\$Architecture\$Configuration"
}

$WorkDirectory = [System.IO.Path]::GetFullPath($WorkDirectory)
$repoRootPrefix = $repoRoot.TrimEnd('\') + '\'
if (-not $WorkDirectory.StartsWith($repoRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "WorkDirectory must be inside the repository: $repoRoot"
}

$PackageDirectory = (Resolve-Path $PackageDirectory).Path
$packagePath = Join-Path $PackageDirectory "ldk.$Version.nupkg"
if (-not (Test-Path $packagePath)) {
  throw "NuGet package was not found: $packagePath"
}

if ([string]::IsNullOrWhiteSpace($NuGetExe)) {
  $nuget = Get-Command nuget -ErrorAction SilentlyContinue
  if (-not $nuget) {
    throw "nuget command was not found. Pass -NuGetExe or add nuget.exe to PATH."
  }

  $NuGetExe = $nuget.Source
} else {
  $NuGetExe = (Resolve-Path $NuGetExe).Path
}

$platformByArchitecture = @{
  x86 = 'Win32'
  x64 = 'x64'
}
$platform = $platformByArchitecture[$Architecture]

Remove-Item -Recurse -Force -Path $WorkDirectory -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $WorkDirectory | Out-Null

$packagesDirectory = Join-Path $WorkDirectory 'packages'
& $NuGetExe install ldk `
  -Version $Version `
  -Source $PackageDirectory `
  -OutputDirectory $packagesDirectory `
  -NonInteractive `
  -Verbosity detailed

if ($LASTEXITCODE -ne 0) {
  throw "nuget install failed with exit code $LASTEXITCODE."
}

$packageRoot = Join-Path $packagesDirectory "ldk.$Version"
if (-not (Test-Path $packageRoot)) {
  $installedPackageCandidates = @(
    Get-ChildItem -Path $packagesDirectory -Directory |
      Where-Object { $_.Name -like 'ldk.*' } |
      Sort-Object Name
  )

  if ($installedPackageCandidates.Count -ne 1) {
    throw "Expected exactly one installed ldk package under '$packagesDirectory', found $($installedPackageCandidates.Count)."
  }

  $packageRoot = $installedPackageCandidates[0].FullName
}

$packageRoot = (Resolve-Path $packageRoot).Path

$requiredPackagePaths = @(
  'README.md',
  'build\native\ldk.props',
  'build\native\ldk.targets',
  'cmake\Ldk.cmake',
  'include\Ldk\Windows.h',
  "lib\native\$Architecture\$Configuration\Ldk.lib"
)

foreach ($requiredPath in $requiredPackagePaths) {
  $fullPath = Join-Path $packageRoot $requiredPath
  if (-not (Test-Path $fullPath)) {
    throw "Installed package is missing expected file: $fullPath"
  }
}

$packageReadme = Get-Content -LiteralPath (Join-Path $packageRoot 'README.md') -Raw -Encoding UTF8
if ($packageReadme -notmatch 'LDK NuGet Package') {
  throw "Installed package README.md does not look like the NuGet package README."
}

if ($SkipDriverBuild) {
  Write-Host "NuGet package layout test passed for $Architecture ${Configuration}: $packageRoot"
  return
}

$consumerDirectory = Join-Path $WorkDirectory 'consumer'
New-Item -ItemType Directory -Force -Path $consumerDirectory | Out-Null

$cmakePackageRoot = $packageRoot.Replace('\', '/')
$cmakeLists = @"
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(ldk_nuget_package_smoke LANGUAGES C)

set(LDK_ROOT "$cmakePackageRoot")
include("$cmakePackageRoot/cmake/Ldk.cmake")

ldk_add_driver(ldk_nuget_package_smoke main.c)
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
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    CRITICAL_SECTION CriticalSection;

    UNREFERENCED_PARAMETER(RegistryPath);

    InitializeCriticalSection(&CriticalSection);
    DeleteCriticalSection(&CriticalSection);

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}
'@
Set-Content -LiteralPath (Join-Path $consumerDirectory 'main.c') -Value $mainC -Encoding UTF8

$buildDirectory = Join-Path $WorkDirectory "build_$Architecture"
$configureArgs = @(
  '-S', $consumerDirectory,
  '-B', $buildDirectory,
  '-G', 'Visual Studio 17 2022',
  '-A', $platform,
  '-T', 'host=x64',
  "-DLDK_WDK_VERSION=$WindowsSdkVersion",
  "-DCMAKE_SYSTEM_VERSION=$WindowsSdkVersion",
  "-DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=$WindowsSdkVersion"
)

Write-Host "Configuring NuGet package consumer for $Architecture $Configuration"
& cmake @configureArgs
if ($LASTEXITCODE -ne 0) {
  throw "CMake configure failed with exit code $LASTEXITCODE."
}

Write-Host "Building NuGet package consumer for $Architecture $Configuration"
& cmake --build $buildDirectory --config $Configuration --target ldk_nuget_package_smoke --parallel
if ($LASTEXITCODE -ne 0) {
  throw "CMake build failed with exit code $LASTEXITCODE."
}

$driverPath = Join-Path $buildDirectory "$Configuration\ldk_nuget_package_smoke.sys"
if (-not (Test-Path $driverPath)) {
  throw "NuGet package consumer driver was not produced: $driverPath"
}

Write-Host "NuGet package consumer test passed: $driverPath"
