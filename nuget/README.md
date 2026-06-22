# LDK NuGet Package

This package provides headers and prebuilt `Ldk.lib` libraries for Windows
kernel-mode driver projects.

The package is intentionally narrow. LDK is an experimental runtime support
library, not a general Windows compatibility layer. Audit every DLL or driver
integration and validate it in an isolated driver test environment.

## Contents

- `include/`: public LDK headers
- `docs/`: implementation notes
- `lib/native/<arch>/<config>/Ldk.lib`: prebuilt static libraries
- `build/native/ldk.props` and `ldk.targets`: MSBuild integration

Prebuilt libraries currently target:

- `x86` and `x64`
- `Debug` and `Release`
- Visual Studio 2022 with Windows SDK/WDK `10.0.22621.0`

## MSBuild usage

Install the package into a WDK driver project and import the native props and
targets if your package manager does not do it automatically:

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

For CMake-based integrations, using the repository through CPM is still the
most direct source-based path:

```cmake
CPMAddPackage("gh:ntoskrnl7/ldk@<version>")
wdk_add_driver(MyDriver CUSTOM_ENTRY_POINT "LdkDriverEntry" main.c)
target_link_libraries(MyDriver Ldk)
```

GitHub Releases also include a prebuilt bundle with a CMake package config for
offline or prebuilt-library consumers.
