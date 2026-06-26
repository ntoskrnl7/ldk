# LDK

**LDK** stands for **Load DLL into Kernel space**.

[![Build](https://github.com/ntoskrnl7/ldk/actions/workflows/cmake.yml/badge.svg)](https://github.com/ntoskrnl7/ldk/actions/workflows/cmake.yml)
[![Release](https://github.com/ntoskrnl7/ldk/actions/workflows/release.yml/badge.svg)](https://github.com/ntoskrnl7/ldk/actions/workflows/release.yml)
![GitHub License](https://img.shields.io/github/license/ntoskrnl7/ldk)
![GitHub Release](https://img.shields.io/github/v/release/ntoskrnl7/ldk)
![Windows](https://img.shields.io/badge/Windows-7%2B-blue?logo=windows&logoColor=white)
![Visual Studio](https://img.shields.io/badge/Visual%20Studio-2017%2B-68217a?logo=visualstudio&logoColor=white)
![CMake](https://img.shields.io/badge/CMake-3.14%2B-064f8c?logo=cmake&logoColor=white)

LDK is an experimental Windows kernel-mode support library for drivers that
need a familiar Win32/NTDLL-like surface inside kernel space. It provides a
static WDK library, public headers under `include/Ldk`, and partial
implementations of APIs normally provided by `kernel32.dll` and `ntdll.dll`.

The project is useful when a driver needs to reuse carefully controlled DLL
code or when kernel-mode code benefits from familiar primitives such as
critical sections, condition variables, fibers, heap helpers, loader helpers,
environment handling, and path/string utilities.

## Status

LDK is not a general-purpose Windows compatibility layer. It implements only a
small subset of APIs, and the DLL-loading path is intentionally experimental.
Every DLL or driver integration should be audited and tested in an isolated
driver test environment before use.

Avoid loading modules that rely on user-mode TEB/PEB assumptions, unsupported
Win32 subsystems, GUI APIs, COM, CRT startup behavior, or other user-mode-only
runtime features.

## Requirements

- Windows 7 or later
- Visual Studio 2017 or later
- Windows Driver Kit compatible with your Visual Studio toolset
- CMake 3.14 or later

The CMake build uses [`FindWDK`](https://github.com/ntoskrnl7/FindWDK) through
CPM, so the WDK must be installed and discoverable on the build machine.

## Quick Start

The recommended integration path is CMake.

```cmake
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(MyDriver C)

include(cmake/CPM.cmake)

CPMAddPackage("gh:ntoskrnl7/ldk@0.7.5")
CPMAddPackage("gh:ntoskrnl7/FindWDK#master")

list(APPEND CMAKE_MODULE_PATH "${FindWDK_SOURCE_DIR}/cmake")
find_package(WDK REQUIRED)

wdk_add_driver(MyDriver CUSTOM_ENTRY_POINT "LdkDriverEntry" main.c)
target_link_libraries(MyDriver Ldk)
```

Using `LdkDriverEntry` as the custom entry point lets LDK run its initialization
and termination hooks automatically. If you use your own driver entry point,
call `LdkInitialize` during driver startup and `LdkTerminate` before unload.

## Driver Example

```c
#include <Ldk/Windows.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

typedef struct _WORKER_STATE {
    BOOL ready;
    LONG processed;
    CONDITION_VARIABLE cv;
    CRITICAL_SECTION cs;
} WORKER_STATE, *PWORKER_STATE;

DWORD
WINAPI
WorkerThread(
    LPVOID Context
    )
{
    PWORKER_STATE state = (PWORKER_STATE)Context;

    EnterCriticalSection(&state->cs);
    while (!state->ready) {
        SleepConditionVariableCS(&state->cv, &state->cs, INFINITE);
    }

    InterlockedIncrement(&state->processed);
    WakeConditionVariable(&state->cv);
    LeaveCriticalSection(&state->cs);

    return 0;
}

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(RegistryPath);

    WORKER_STATE state = {0};
    InitializeCriticalSection(&state.cs);
    InitializeConditionVariable(&state.cv);

    HANDLE thread = CreateThread(NULL, 0, WorkerThread, &state, 0, NULL);

    EnterCriticalSection(&state.cs);
    state.ready = TRUE;
    WakeAllConditionVariable(&state.cv);
    LeaveCriticalSection(&state.cs);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    DeleteCriticalSection(&state.cs);

    DriverObject->DriverUnload = DriverUnload;
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

## Build

Clone the repository and build with CMake:

```bat
git clone https://github.com/ntoskrnl7/ldk
cd ldk
cmake -S . -B build_x64 -A x64 -DWDK_WINVER=0x0602
cmake --build build_x64 --config Release
```

The helper script wraps the same flow:

```bat
build.bat . x64 Release
build.bat . x86 Release
```

Release libraries are written under:

```text
lib/<architecture>/Ldk.lib
```

## Test Driver

The `test` directory builds a sample kernel driver that links against LDK.

```bat
cd test
..\build.bat . x64 Release
```

The build output includes `LdkTest.sys`. Loading and unloading the driver must
be done manually in an appropriate Windows driver test environment. GitHub
Actions verifies the x64 test driver build, but it does not load kernel
drivers.

Additional implementation and test notes live under [`docs/`](docs/).

## NuGet and Releases

The `Package` GitHub Actions workflow builds prebuilt x86/x64 Debug/Release
libraries, packs the `ldk` NuGet package, validates the package with a minimal
WDK consumer driver, and prepares GitHub Release assets.

The `Release` workflow is the publishing entry point. It updates
`include/Ldk/internal/version.h`, creates a `v<version>` tag, then dispatches
the `Package` workflow. NuGet publishing uses NuGet Trusted Publishing and
requires the repository variable `NUGET_TRUSTED_PUBLISHING_USER`.

## Releases

Tagged releases are built by GitHub Actions. Pushing a tag such as `v0.7.6`
builds x86 and x64 package artifacts and can attach the generated NuGet package
and prebuilt ZIP bundle to the GitHub Release.

The normal publishing path is to run the `Release` workflow manually with the
target version. It creates the version commit and tag, then dispatches the
`Package` workflow against that tag.

Release assets contain:

- `ldk.<version>.nupkg`
- `ldk-<version>-prebuilt.zip`
- `ldk-<version>-SHA256SUMS.txt`

## Repository Layout

```text
include/Ldk/       Public headers
src/               LDK runtime implementation
src/kernel32/      Kernel-mode implementations of selected Kernel32 APIs
src/ntdll/         Kernel-mode implementations of selected NTDLL APIs
test/              Sample/test driver project
cmake/             CMake helper modules
msvc/              Visual Studio solution and project files
```

## Notes

- The API surface is intentionally incomplete.
- Treat DLL loading in kernel space as a last-resort technique, not as a normal
  driver architecture.
