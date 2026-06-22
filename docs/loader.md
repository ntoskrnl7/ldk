# Loader

DLL loading is the center of LDK's design. The loader provides enough
Win32/NTDLL-shaped behavior for controlled DLLs to be loaded and resolved in
kernel space.

## Public layers

The call stack is intentionally familiar:

- Kernel32 wrappers: `LoadLibraryA/W`, `LoadLibraryExA/W`, `FreeLibrary`,
  `GetProcAddress`, `GetModuleHandleA/W`, `GetModuleFileNameA/W`.
- NTDLL wrappers: `LdrLoadDll`, `LdrUnloadDll`, `LdrGetProcedureAddress`,
  `LdrGetDllHandle`.
- LDK core: module list lookup, PE loading, import resolution, relocation,
  entry-point invocation, and unload.

The Kernel32 and NTDLL pseudo modules are registered during PEB initialization.
Those registrations let loaded code resolve supported imports by name.

## Load path

`LoadLibraryExW` first checks whether the module is already known. It also
handles the current image path case and computes a DLL search path from the
current `PATH` plus the image directory.

`LdrLoadDll` forwards to `LdkLoadDll`, unless loader initialization is already
in progress. The core load path:

1. Searches the DLL path using RTL DOS path helpers.
2. Converts the DOS path to an NT path.
3. Opens and reads the image file.
4. Validates PE headers, architecture, and DLL characteristics.
5. Allocates nonpaged memory for the image.
6. Copies headers and sections.
7. Applies base relocations.
8. Resolves imports through the LDK module list.
9. Registers the loaded module.
10. Calls the DLL entry point with `DLL_PROCESS_ATTACH`.

Entry points are called through a kernel stack expansion helper when needed.

## Import and export resolution

Imports are resolved by module name and imported function name. Import-by-ordinal
in import descriptors is not currently supported.

`GetProcAddress` calls `LdrGetProcedureAddress`. For LDK pseudo modules, the
resolver searches the registration table. For real PE images, it walks the PE
export directory. Forwarded exports are followed by loading the forwarder
module and resolving the forwarded symbol.

Ordinal export lookup exists in the resolver path, but name-based resolution is
the better-tested path.

## Unload path

`FreeLibrary` calls `LdrUnloadDll`, which calls `LdkUnloadDll`. Loaded modules
are removed from the module list, called with `DLL_PROCESS_DETACH`, and freed.

`LdkTerminate` unregisters remaining modules before tearing down Kernel32,
NTDLL, TEB, PEB, and heap state. This matters because loaded DLLs can run detach
code that touches LDK runtime services.

## Limits

LDK loading is not user-mode Windows loading. Loaded DLLs run as kernel code.
Avoid DLLs that require GUI state, COM, arbitrary CRT startup, user-mode TLS
callbacks, user-mode APC assumptions, or unsupported imports. Treat import
failures and debugger breaks in loader paths as compatibility bugs to audit, not
as recoverable normal behavior.
