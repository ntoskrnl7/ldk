param(
  [string] $PackageDirectory,

  [Parameter(Mandatory = $true)]
  [string] $Version,

  [ValidateSet('x86', 'x64', 'ARM', 'ARM64')]
  [string] $Architecture = 'x64',

  [ValidateSet('v142', 'v143', 'v145')]
  [string] $Toolset = 'v143',

  [ValidateSet('Debug', 'Release')]
  [string] $Configuration = 'Debug',

  [string] $WindowsSdkVersion = '10.0.22621.0',

  [string] $WorkDirectory,

  [string] $NuGetExe
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function ConvertTo-XmlEscapedText {
  param([Parameter(Mandatory = $true)][string] $Value)

  return [System.Security.SecurityElement]::Escape($Value)
}

function Resolve-MsBuildPath {
  param([Parameter(Mandatory = $true)][string] $Toolset)

  $programFilesX86 = ${env:ProgramFiles(x86)}
  $candidateRoots = @()
  if ($Toolset -eq 'v145') {
    $candidateRoots += Join-Path $programFilesX86 'Microsoft Visual Studio\18'
  } else {
    $candidateRoots += Join-Path $programFilesX86 'Microsoft Visual Studio\2022'
  }

  $candidatePaths = foreach ($root in $candidateRoots) {
    foreach ($edition in @('Enterprise', 'Professional', 'Community', 'BuildTools')) {
      Join-Path $root "$edition\MSBuild\Current\Bin\amd64\MSBuild.exe"
    }
  }

  foreach ($candidatePath in $candidatePaths) {
    if (Test-Path $candidatePath) {
      return (Resolve-Path $candidatePath).Path
    }
  }

  $vswhere = Join-Path $programFilesX86 'Microsoft Visual Studio\Installer\vswhere.exe'
  if (Test-Path $vswhere) {
    $versionRange = if ($Toolset -eq 'v145') { '[18.0,19.0)' } else { '[17.0,18.0)' }
    $matches = @(
      & $vswhere -products * -version $versionRange -requires Microsoft.Component.MSBuild -find 'MSBuild\Current\Bin\amd64\MSBuild.exe'
    )

    foreach ($match in $matches) {
      if (-not [string]::IsNullOrWhiteSpace($match) -and (Test-Path $match)) {
        return (Resolve-Path $match).Path
      }
    }
  }

  $pathCommand = Get-Command MSBuild.exe -ErrorAction SilentlyContinue
  if ($pathCommand) {
    return $pathCommand.Source
  }

  throw "MSBuild.exe for $Toolset was not found."
}

function Test-LdkWdkIncludeLayout {
  param(
    [Parameter(Mandatory = $true)]
    [string] $WindowsKitsRoot,

    [Parameter(Mandatory = $true)]
    [string] $Version
  )

  $includeRoot = Join-Path $WindowsKitsRoot "Include\$Version"
  $requiredPaths = @(
    (Join-Path $includeRoot 'shared\ntdef.h'),
    (Join-Path $includeRoot 'km\ntddk.h'),
    (Join-Path $includeRoot 'km\crt\stddef.h')
  )

  foreach ($requiredPath in $requiredPaths) {
    if (-not (Test-Path $requiredPath)) {
      return $false
    }
  }

  return $true
}

function Test-LdkWdkLibLayout {
  param(
    [Parameter(Mandatory = $true)]
    [string] $WindowsKitsRoot,

    [Parameter(Mandatory = $true)]
    [string] $Version,

    [Parameter(Mandatory = $true)]
    [string] $WdkLibArchitecture
  )

  $ntoskrnl = Join-Path $WindowsKitsRoot "Lib\$Version\km\$WdkLibArchitecture\ntoskrnl.lib"
  return (Test-Path $ntoskrnl)
}

function Resolve-LdkWdkIncludeVersion {
  param(
    [Parameter(Mandatory = $true)]
    [string] $WindowsKitsRoot,

    [Parameter(Mandatory = $true)]
    [string] $PreferredVersion
  )

  $candidateVersions = @()
  if (-not [string]::IsNullOrWhiteSpace($PreferredVersion)) {
    $candidateVersions += $PreferredVersion
  }

  $includeRoot = Join-Path $WindowsKitsRoot 'Include'
  if (Test-Path $includeRoot) {
    $candidateVersions += @(
      Get-ChildItem -Path $includeRoot -Directory |
        Where-Object {
          $parsedVersion = $null
          [version]::TryParse($_.Name, [ref]$parsedVersion)
        } |
        ForEach-Object { $_.Name } |
        Sort-Object { [version]$_ } -Descending
    )
  }

  $candidateVersions = @($candidateVersions | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
  foreach ($candidateVersion in $candidateVersions) {
    if (Test-LdkWdkIncludeLayout -WindowsKitsRoot $WindowsKitsRoot -Version $candidateVersion) {
      return $candidateVersion
    }
  }

  return $null
}

function Resolve-LdkWdkLayout {
  param(
    [Parameter(Mandatory = $true)]
    [string[]] $WindowsKitsRoots,

    [Parameter(Mandatory = $true)]
    [string] $PreferredVersion,

    [Parameter(Mandatory = $true)]
    [string] $WdkLibArchitecture
  )

  foreach ($windowsKitsRoot in $WindowsKitsRoots) {
    if (-not (Test-Path $windowsKitsRoot)) {
      continue
    }

    $candidateVersions = @()
    if (-not [string]::IsNullOrWhiteSpace($PreferredVersion)) {
      $candidateVersions += $PreferredVersion
    }

    $libRoot = Join-Path $windowsKitsRoot 'Lib'
    if (Test-Path $libRoot) {
      $candidateVersions += @(
        Get-ChildItem -Path $libRoot -Directory |
          Where-Object { Test-Path (Join-Path $_.FullName "km\$WdkLibArchitecture\ntoskrnl.lib") } |
          ForEach-Object { $_.Name } |
          Sort-Object { [version]$_ } -Descending
      )
    }

    $candidateVersions = @($candidateVersions | Where-Object { -not [string]::IsNullOrWhiteSpace($_) } | Select-Object -Unique)
    foreach ($candidateVersion in $candidateVersions) {
      if (Test-LdkWdkLibLayout -WindowsKitsRoot $windowsKitsRoot -Version $candidateVersion -WdkLibArchitecture $WdkLibArchitecture) {
        $includeVersion = Resolve-LdkWdkIncludeVersion -WindowsKitsRoot $windowsKitsRoot -PreferredVersion $candidateVersion
        if (-not $includeVersion) {
          continue
        }

        $includeRoot = Join-Path $windowsKitsRoot "Include\$includeVersion"
        $libDirectory = Join-Path $windowsKitsRoot "Lib\$candidateVersion\km\$WdkLibArchitecture"
        $securityRuntimePath = $null
        foreach ($securityRuntime in @('BufferOverflowK.lib', 'bufferoverflowfastfailk.lib')) {
          $candidateSecurityRuntimePath = Join-Path $libDirectory $securityRuntime
          if (Test-Path $candidateSecurityRuntimePath) {
            $securityRuntimePath = $candidateSecurityRuntimePath
            break
          }
        }

        $runtimeLibraryPaths = @()
        if ($securityRuntimePath) {
          $runtimeLibraryPaths += $securityRuntimePath
        }

        if ($WdkLibArchitecture -eq 'arm' -or $WdkLibArchitecture -eq 'arm64') {
          $compilerRuntimePath = Join-Path $libDirectory 'libcntpr.lib'
          if (Test-Path $compilerRuntimePath) {
            $runtimeLibraryPaths += $compilerRuntimePath
          }
        }

        return [pscustomobject]@{
          Version = $candidateVersion
          IncludeVersion = $includeVersion
          IncludeRoot = $includeRoot
          LibDirectory = $libDirectory
          RuntimeLibraryPaths = $runtimeLibraryPaths
        }
      }
    }
  }

  throw "No usable WDK layout was found for $WdkLibArchitecture. Install a WDK that contains km headers and libraries."
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path

if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
  $PackageDirectory = Join-Path $repoRoot 'artifacts\nuget'
}

if ([string]::IsNullOrWhiteSpace($WorkDirectory)) {
  $WorkDirectory = Join-Path $repoRoot "artifacts\nuget-msbuild-consumer-test\$Toolset\$Architecture\$Configuration"
}

if ($Toolset -eq 'v145' -and $Architecture -eq 'ARM') {
  throw "Visual Studio 18 2026 / v145 does not provide the 32-bit ARM target in the tested Build Tools layout. Test ARM with v142 or v143, or omit ARM for v145."
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

$WorkDirectory = [System.IO.Path]::GetFullPath($WorkDirectory)
$repoRootPrefix = $repoRoot.TrimEnd('\') + '\'
if (-not $WorkDirectory.StartsWith($repoRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
  throw "WorkDirectory must be inside the repository: $repoRoot"
}

$programFilesX86 = ${env:ProgramFiles(x86)}
$windowsKitsRoots = @(
  (Join-Path $programFilesX86 'Windows Kits\10'),
  (Join-Path ${env:ProgramFiles} 'Windows Kits\10')
) | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }

$platformByArchitecture = @{
  x86 = 'Win32'
  x64 = 'x64'
  ARM = 'ARM'
  ARM64 = 'ARM64'
}

$wdkLibArchitectureByArchitecture = @{
  x86 = 'x86'
  x64 = 'x64'
  ARM = 'arm'
  ARM64 = 'arm64'
}

$machineByArchitecture = @{
  x86 = 'X86'
  x64 = 'X64'
  ARM = 'ARM'
  ARM64 = 'ARM64'
}

$definesByArchitecture = @{
  x86 = 'WIN32;_X86_;i386=1;STD_CALL'
  x64 = 'WIN32;_WIN64;_AMD64_;AMD64'
  ARM = 'WIN32;_ARM_;ARM'
  ARM64 = 'WIN32;_WIN64;_ARM64_;ARM64'
}

$platform = $platformByArchitecture[$Architecture]
$wdkLibArchitecture = $wdkLibArchitectureByArchitecture[$Architecture]
$machine = $machineByArchitecture[$Architecture]
$architectureDefines = $definesByArchitecture[$Architecture]
$wdkLayout = Resolve-LdkWdkLayout `
  -WindowsKitsRoots $windowsKitsRoots `
  -PreferredVersion $WindowsSdkVersion `
  -WdkLibArchitecture $wdkLibArchitecture

if ($wdkLayout.Version -ne $WindowsSdkVersion) {
  Write-Host "Requested WDK $WindowsSdkVersion was not usable for $Architecture. Using WDK $($wdkLayout.Version)."
}
if ($wdkLayout.IncludeVersion -ne $wdkLayout.Version) {
  Write-Host "Using WDK $($wdkLayout.IncludeVersion) headers with WDK $($wdkLayout.Version) $Architecture libraries."
}

$WindowsSdkVersion = $wdkLayout.IncludeVersion
$wdkIncludeRoot = $wdkLayout.IncludeRoot
$wdkLibDirectory = $wdkLayout.LibDirectory
$msbuild = Resolve-MsBuildPath -Toolset $Toolset

Remove-Item -Recurse -Force -Path $WorkDirectory -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force -Path $WorkDirectory | Out-Null

$projectDirectory = Join-Path $WorkDirectory 'consumer'
$packagesDirectory = Join-Path $WorkDirectory 'packages'
New-Item -ItemType Directory -Force -Path $projectDirectory | Out-Null

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
      Where-Object { $_.Name -like 'ldk*' } |
      Sort-Object Name
  )

  if ($installedPackageCandidates.Count -ne 1) {
    throw "Expected exactly one installed ldk package under '$packagesDirectory', found $($installedPackageCandidates.Count)."
  }

  $packageRoot = $installedPackageCandidates[0].FullName
}

$packageRoot = (Resolve-Path $packageRoot).Path

$requiredPackagePaths = @(
  'build\native\ldk.props',
  'build\native\ldk.targets',
  'include\Ldk\Windows.h',
  "lib\native\$Toolset\$Architecture\$Configuration\Ldk.lib"
)

foreach ($requiredPackagePath in $requiredPackagePaths) {
  $fullPath = Join-Path $packageRoot $requiredPackagePath
  if (-not (Test-Path $fullPath)) {
    throw "Installed package is missing expected file: $fullPath"
  }
}

$targetName = 'LdkMsBuildPackageSmoke'
$projectPath = Join-Path $projectDirectory "$targetName.vcxproj"
$mainPath = Join-Path $projectDirectory 'main.c'

$sharedInclude = ConvertTo-XmlEscapedText (Join-Path $wdkIncludeRoot 'shared')
$kmInclude = ConvertTo-XmlEscapedText (Join-Path $wdkIncludeRoot 'km')
$kmCrtInclude = ConvertTo-XmlEscapedText (Join-Path $wdkIncludeRoot 'km\crt')
$kernelLibraries = @('ntoskrnl.lib', 'hal.lib', 'wmilib.lib') |
  ForEach-Object { Join-Path $wdkLibDirectory $_ } |
  Where-Object { Test-Path $_ } |
  ForEach-Object { ConvertTo-XmlEscapedText $_ }
$kernelLibraryDependencies = $kernelLibraries -join ';'
$runtimeLibraries = @($wdkLayout.RuntimeLibraryPaths | ForEach-Object { ConvertTo-XmlEscapedText $_ })
$runtimeLibraryDependencies = $runtimeLibraries -join ';'
$escapedWindowsSdkVersion = ConvertTo-XmlEscapedText $WindowsSdkVersion
$escapedToolset = ConvertTo-XmlEscapedText $Toolset
$escapedConfiguration = ConvertTo-XmlEscapedText $Configuration
$escapedPlatform = ConvertTo-XmlEscapedText $platform
$escapedArchitectureDefines = ConvertTo-XmlEscapedText $architectureDefines
$ldkProps = ConvertTo-XmlEscapedText (Join-Path $packageRoot 'build\native\ldk.props')
$ldkTargets = ConvertTo-XmlEscapedText (Join-Path $packageRoot 'build\native\ldk.targets')
$runtimeLibrary = if ($Configuration -eq 'Debug') { 'MultiThreadedDebug' } else { 'MultiThreaded' }

$vcxproj = @"
<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="17.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="$escapedConfiguration|$escapedPlatform">
      <Configuration>$escapedConfiguration</Configuration>
      <Platform>$escapedPlatform</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <ProjectGuid>{EA955542-1D9F-4B73-9A7C-3530A7A332E3}</ProjectGuid>
    <Keyword>Win32Proj</Keyword>
    <ProjectName>$targetName</ProjectName>
    <WindowsTargetPlatformVersion>$escapedWindowsSdkVersion</WindowsTargetPlatformVersion>
    <DriverType>WDM</DriverType>
    <LdkUseDriverSupport>true</LdkUseDriverSupport>
    <LdkExpectedLibToolset>$escapedToolset</LdkExpectedLibToolset>
  </PropertyGroup>
  <Import Project="$ldkProps" Condition="Exists('$ldkProps')" />
  <Import Project="`$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <PropertyGroup Label="Configuration" Condition="'`$(Configuration)|`$(Platform)'=='$escapedConfiguration|$escapedPlatform'">
    <ConfigurationType>Application</ConfigurationType>
    <CharacterSet>Unicode</CharacterSet>
    <PlatformToolset>$escapedToolset</PlatformToolset>
  </PropertyGroup>
  <Import Project="`$(VCTargetsPath)\Microsoft.Cpp.props" />
  <PropertyGroup>
    <OutDir>`$(MSBuildThisFileDirectory)bin\`$(Configuration)\</OutDir>
    <IntDir>`$(MSBuildThisFileDirectory)obj\`$(Configuration)\</IntDir>
    <TargetName>$targetName</TargetName>
    <TargetExt>.sys</TargetExt>
    <GenerateManifest>false</GenerateManifest>
  </PropertyGroup>
  <ItemDefinitionGroup>
    <ClCompile>
      <AdditionalIncludeDirectories>$sharedInclude;$kmInclude;$kmCrtInclude;%(AdditionalIncludeDirectories)</AdditionalIncludeDirectories>
      <PreprocessorDefinitions>$escapedArchitectureDefines;WINNT=1;_WIN32_WINNT=0x0601;%(PreprocessorDefinitions)</PreprocessorDefinitions>
      <AdditionalOptions>%(AdditionalOptions) /kernel</AdditionalOptions>
      <RuntimeLibrary>$runtimeLibrary</RuntimeLibrary>
      <BufferSecurityCheck>false</BufferSecurityCheck>
      <WarningLevel>Level3</WarningLevel>
      <TreatWarningAsError>true</TreatWarningAsError>
    </ClCompile>
    <Link>
      <AdditionalDependencies>$kernelLibraryDependencies;$runtimeLibraryDependencies;%(AdditionalDependencies)</AdditionalDependencies>
      <IgnoreAllDefaultLibraries>true</IgnoreAllDefaultLibraries>
      <SubSystem>Native</SubSystem>
      <AdditionalOptions>%(AdditionalOptions) /DRIVER /MACHINE:$machine</AdditionalOptions>
      <GenerateDebugInformation>true</GenerateDebugInformation>
    </Link>
  </ItemDefinitionGroup>
  <ItemGroup>
    <ClCompile Include="main.c" />
  </ItemGroup>
  <Import Project="`$(VCTargetsPath)\Microsoft.Cpp.targets" />
  <Import Project="$ldkTargets" Condition="Exists('$ldkTargets')" />
</Project>
"@

Set-Content -LiteralPath $projectPath -Value $vcxproj -Encoding UTF8

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
    CRITICAL_SECTION CriticalSection;

    UNREFERENCED_PARAMETER(RegistryPath);

    InitializeCriticalSection(&CriticalSection);
    DeleteCriticalSection(&CriticalSection);

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}
'@

Set-Content -LiteralPath $mainPath -Value $mainC -Encoding UTF8

$msbuildArgs = @(
  $projectPath,
  '/m',
  '/v:m',
  "/p:Configuration=$Configuration",
  "/p:Platform=$platform"
)

Write-Host "Building NuGet native MSBuild consumer for $Toolset $Architecture $Configuration"
& $msbuild @msbuildArgs
if ($LASTEXITCODE -ne 0) {
  throw "MSBuild NuGet native consumer failed with exit code $LASTEXITCODE."
}

$driverPath = Join-Path $projectDirectory "bin\$Configuration\$targetName.sys"
if (-not (Test-Path $driverPath)) {
  throw "MSBuild NuGet native consumer driver was not produced: $driverPath"
}

Write-Host "NuGet native MSBuild consumer test passed: $driverPath"
