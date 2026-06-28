# Loader

DLL loading is the center of LDK's design. The loader provides enough
Win32/NTDLL-shaped behavior for controlled DLLs to be loaded and resolved in
kernel space.

## Public layers

The call stack is intentionally familiar:

- Kernel32 wrappers: `LoadLibraryA/W`, `LoadLibraryExA/W`, `FreeLibrary`,
  `GetProcAddress`, `GetModuleHandleA/W`, `GetModuleFileNameA/W`,
  `DisableThreadLibraryCalls`.
- NTDLL wrappers: `LdrLoadDll`, `LdrUnloadDll`, `LdrGetProcedureAddress`,
  `LdrGetDllHandle`.
- LDK core: module list lookup, PE loading, import resolution, relocation,
  entry-point invocation, and unload.

The Kernel32 and NTDLL pseudo modules are registered during PEB initialization.
Those registrations let loaded code resolve supported imports by name.

## Load path

`LoadLibraryExW` handles the current image path case and computes a DLL search
path before calling `LdrLoadDll`. The default path is the current image
directory followed by `PATH`, so private DLLs next to the driver win over
same-named DLLs found in system directories.

The tested `LoadLibraryExW` search subset includes
`LOAD_LIBRARY_SEARCH_SYSTEM32`, `LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR`,
`LOAD_LIBRARY_SEARCH_APPLICATION_DIR`, `LOAD_LIBRARY_SEARCH_USER_DIRS`,
`LOAD_LIBRARY_SEARCH_DEFAULT_DIRS`, and `LOAD_WITH_ALTERED_SEARCH_PATH`. When a
DLL is loaded from a specific directory, imports discovered while loading that
DLL inherit the same search context where applicable, which lets private
dependency DLLs live beside their owner DLL.

User DLL directories are process-wide LDK PEB state. `SetDllDirectoryA/W`
stores the single Win32-compatible directory reported by `GetDllDirectoryA/W`.
`AddDllDirectory` appends absolute directories to a cookie list, and
`RemoveDllDirectory` removes those cookies. `SetDefaultDllDirectories` controls
the search flags used by later `LoadLibraryA/W` calls that do not pass explicit
search flags. Invalid combinations such as
`LOAD_WITH_ALTERED_SEARCH_PATH` mixed with `LOAD_LIBRARY_SEARCH_*` flags are
rejected before the load reaches the native loader.

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
10. Calls image TLS callbacks with `DLL_PROCESS_ATTACH`.
11. Calls the DLL entry point with `DLL_PROCESS_ATTACH`.

TLS callbacks and entry points are called through a kernel stack expansion
helper when needed. If a DLL has no TLS callback directory or entry point, the
attach step succeeds. If `DLL_PROCESS_ATTACH` returns failure, the module is
unloaded and the load fails with
`STATUS_DLL_INIT_FAILED`. The failure path also releases import dependencies
that were loaded while preparing the failed module, so a failed attach does not
leave the failing module or its private dependencies registered.

Repeated `LoadLibrary` / `LdrLoadDll` calls for an already loaded dynamic
module increment the module load count and return the existing module handle.
`DONT_RESOLVE_DLL_REFERENCES` loads the image without walking imports or calling
the entry point, matching the limited behavior needed by LDK loader tests.

`DisableThreadLibraryCalls` is supported for loaded dynamic modules. It suppresses
later `DllMain(DLL_THREAD_ATTACH)` and `DllMain(DLL_THREAD_DETACH)` notifications
for that module, while leaving TLS callbacks and process attach/detach entry
point calls intact.

`LOAD_LIBRARY_AS_DATAFILE`, `LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE`, and
`LOAD_LIBRARY_AS_IMAGE_RESOURCE` create resource-only handles. LDK tags those
handles internally so `FreeLibrary` and resource-oriented module filename
queries can map them back to the loaded image, while `GetProcAddress` rejects
them instead of treating them as executable module handles.

When import resolution loads or references another dynamic module, LDK records
that relationship as a module dependency. The dependent module keeps one
ownership reference to each imported dynamic module, so the imported module
stays loaded while the dependent module is loaded.

MSVC delay-loaded imports are covered for the normal first-call path produced by
`delayimp.lib`: the owner DLL can load without its delay dependency, and the
first delayed call resolves the dependency through LDK's `LoadLibraryExA`,
`GetProcAddress`, and related Kernel32 pseudo-module exports.

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
zero, the module is removed from the module list, its TLS callbacks are called
with `DLL_PROCESS_DETACH`, its entry point is called with
`DLL_PROCESS_DETACH`, and the image is freed. Invalid module handles fail
instead of being treated as successful no-op unloads.

After the dependent module has run detach and its image has been freed, its
recorded dependency references are released. This keeps imported modules alive
during the dependent module's detach callback, then lets automatically loaded
dependencies unload when no other direct or dependency reference remains.

`LdrGetDllHandle` supports name lookup and path-aware lookup when a caller
passes a `DllPath` search string. Dynamic modules are registered with their NT
full path so the path-aware query does not accidentally match a same-named DLL
loaded from a different directory.

`GetModuleHandleEx` supports the tested name/address lookup paths. By default it
adds a loader reference, while `GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT`
leaves the load count unchanged. Passing `GET_MODULE_HANDLE_EX_FLAG_PIN` marks a
dynamic module as pinned so later `FreeLibrary` calls do not unload it. Pseudo
modules registered by LDK itself, such as the built-in Kernel32 and NTDLL
registrations, are not unloadable.

`LdkTerminate` unregisters remaining modules before tearing down Kernel32,
NTDLL, TEB, PEB, and heap state. This matters because loaded DLLs can run detach
code that touches LDK runtime services.

## Limits

LDK loading is not user-mode Windows loading. Loaded DLLs run as kernel code.
TLS callback notification is implemented for the tested PE TLS callback
directory path, including process attach/detach and LDK-created thread
attach/detach notifications. Loaded DLL entry points also receive tested
`DLL_THREAD_ATTACH` and `DLL_THREAD_DETACH` notifications unless disabled with
`DisableThreadLibraryCalls`. This does not make arbitrary user-mode TLS usage a
kernel-safe contract: static TLS data layout, broad CRT startup assumptions,
GUI state, COM, user-mode APC assumptions, and unsupported imports still need to
be avoided or validated for the specific driver workload. Treat import failures
and debugger breaks in loader paths as compatibility bugs to audit, not as
recoverable normal behavior.

The current dependency model covers tested acyclic import ownership and unload
ordering, including repeated owner/dependency load and unload cycles. It is
still not a full Windows loader implementation: circular import graphs,
advanced side-by-side activation behavior, delay-load notification/failure
hooks, explicit delay-unload helper behavior, and arbitrary loader callback
interactions are outside the current contract. Drivers that rely on complex DLL
dependency trees should add workload-specific load/unload tests before using
that pattern.

The `LoadLibraryExW` flag model is also intentionally narrower than Windows:
LDK supports process-wide `SetDllDirectoryA/W`, `GetDllDirectoryA/W`,
`AddDllDirectory`, `RemoveDllDirectory`, and `SetDefaultDllDirectories`, but it
does not implement per-thread DLL directory state, activation contexts,
signed-target enforcement, or the full safe search mode policy. Unsupported flag
combinations fail early instead of being silently treated as normal loads.
