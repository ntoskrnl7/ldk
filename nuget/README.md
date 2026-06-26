# LDK NuGet Package

LDK is an experimental Win32/NTDLL-like runtime support library for Windows
kernel-mode drivers. It provides headers, native MSBuild imports, CMake helpers,
documentation, and prebuilt `Ldk.lib` libraries for selected WDK driver
configurations.

This NuGet package is for **Visual Studio/MSBuild WDK driver projects**
(`ldk.<version>.nupkg`).

## Quick start

```powershell
Install-Package ldk
```

Driver projects get:

- public LDK headers under `include/`
- automatic include path setup
- automatic `Ldk.lib` linkage
- `_KERNEL32_` preprocessor definition
- `LdkDriverEntry` as the default driver entry point

Minimal driver:

```c
#include <Ldk/Windows.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(RegistryPath);

    DriverObject->DriverUnload = DriverUnload;

    Sleep(1);
    return STATUS_SUCCESS;
}

VOID
DriverUnload(
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
}
```

When `LdkDriverEntry` is used as the PE entry point, LDK initializes its runtime,
calls your normal `DriverEntry`, and runs termination hooks before unload.

The package is intentionally narrow. LDK is not a general Windows compatibility
layer. Audit every DLL or driver integration and validate it in an isolated
driver test environment.

## Contents

- `include/`: public LDK headers
- `docs/`: implementation notes
- `lib/native/<arch>/<config>/Ldk.lib`: prebuilt static libraries
- `build/native/ldk.props` and `ldk.targets`: MSBuild integration

Prebuilt libraries currently target:

- `x86`, `x64`, `ARM`, and `ARM64`
- `Debug` and `Release`
- Visual Studio 2022 with Windows SDK/WDK `10.0.22621.0`

## MSBuild usage

Visual Studio NuGet package restore imports `build/native/ldk.props` and
`build/native/ldk.targets` automatically for native projects. If your package
manager does not import native build files automatically, add the imports
manually:

```xml
<Import Project="$(LdkPackageRoot)build\native\ldk.props" />
...
<Import Project="$(LdkPackageRoot)build\native\ldk.targets" />
```

The targets add the package include directory, link `Ldk.lib`, define
`_KERNEL32_`, and by default set the driver entry point to `LdkDriverEntry`.
Your driver should still implement the normal `DriverEntry` function; LDK's
entry point calls it after initializing the runtime.

Set `LdkUseDriverEntry=false` before importing `ldk.targets` if your driver uses
its own entry point and calls `LdkInitialize` / `LdkTerminate` manually.

## CMake users

The NuGet package is primarily for MSBuild. For CMake-based integrations, a
driver project can consume LDK directly from GitHub through CPM:

```cmake
CPMAddPackage("gh:ntoskrnl7/ldk@<version>")
wdk_add_driver(MyDriver CUSTOM_ENTRY_POINT "LdkDriverEntry" main.c)
target_link_libraries(MyDriver Ldk)
```

GitHub Releases also include a prebuilt bundle with a CMake package config for
offline or prebuilt-library consumers.

## Release artifacts

- `ldk-<version>-prebuilt.zip`
  A prebuilt bundle containing headers, libraries, documentation, CMake helpers,
  native MSBuild imports, and package config files for offline/manual
  integration.
- `ldk-<version>-SHA256SUMS.txt`
  Checksum file for offline/manual verification.

For full package usage, API status, and implementation notes, see:
- <https://github.com/ntoskrnl7/ldk/blob/main/README.md>
- <https://github.com/ntoskrnl7/ldk/tree/main/docs>
