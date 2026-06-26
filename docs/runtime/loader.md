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

`LoadLibraryExW` handles the current image path case and computes a DLL search
path from the current image directory followed by `PATH`, so private DLLs next
to the driver win over same-named DLLs found in system directories.

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
If a DLL has no entry point, the attach step succeeds. If `DLL_PROCESS_ATTACH`
returns failure, the module is unloaded and the load fails with
`STATUS_DLL_INIT_FAILED`.

Repeated `LoadLibrary` / `LdrLoadDll` calls for an already loaded dynamic
module increment the module load count and return the existing module handle.
`DONT_RESOLVE_DLL_REFERENCES` loads the image without walking imports or calling
the entry point, matching the limited behavior needed by LDK loader tests.

## Import and export resolution

Imports are resolved by module name and either imported function name or ordinal.
When an import descriptor omits `OriginalFirstThunk`, LDK falls back to
`FirstThunk` while walking the import table, matching the PE loader convention.

`GetProcAddress` calls `LdrGetProcedureAddress`. For LDK pseudo modules, the
resolver searches the registration table. For real PE images, it walks the PE
export directory. Forwarded exports are followed by loading the forwarder
module and resolving the forwarded symbol.

Ordinal export lookup is supported for real PE images. Pseudo modules such as
the built-in Kernel32 and NTDLL registrations expose name tables only.

## Unload path

`FreeLibrary` calls `LdrUnloadDll`, which calls `LdkUnloadDll`. Dynamic modules
are reference-counted. A balanced unload decrements the count; when it reaches
zero, the module is removed from the module list, called with
`DLL_PROCESS_DETACH`, and freed.

`GetModuleHandleEx` supports the tested name/address lookup paths. Passing
`GET_MODULE_HANDLE_EX_FLAG_PIN` marks a dynamic module as pinned so later
`FreeLibrary` calls do not unload it. Pseudo modules registered by LDK itself,
such as the built-in Kernel32 and NTDLL registrations, are not unloadable.

`LdkTerminate` unregisters remaining modules before tearing down Kernel32,
NTDLL, TEB, PEB, and heap state. This matters because loaded DLLs can run detach
code that touches LDK runtime services.

## Limits

LDK loading is not user-mode Windows loading. Loaded DLLs run as kernel code.
Avoid DLLs that require GUI state, COM, arbitrary CRT startup, user-mode TLS
callbacks, user-mode APC assumptions, or unsupported imports. Treat import
failures and debugger breaks in loader paths as compatibility bugs to audit, not
as recoverable normal behavior.

The current reference-counting model tracks direct public load/unload calls,
but it is not yet a full Windows loader dependency graph. Imported modules can
be loaded during import resolution, but dependency ownership and unload ordering
are still intentionally narrow. Drivers that rely on complex DLL dependency
trees should add workload-specific load/unload tests before using that pattern.
