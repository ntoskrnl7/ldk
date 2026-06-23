# Architecture

LDK provides a small Win32/NTDLL-like runtime surface for kernel-mode code. It
is built as a static WDK library and is initialized by the driver that links it.

The project is not a full Windows compatibility layer. Each API is implemented
only when the kernel-mode behavior can be bounded and tested.

Related detail pages:

- [Runtime state](runtime-state.md)
- [Loader](../runtime/loader.md)
- [File and handle APIs](../api/file-and-handles.md)
- [Heap APIs](../api/heap.md)
- [ICU virtual module](../api/icu.md)
- [NLS, strings, and time formatting](../api/nls-and-time.md)
- [Registry APIs](../api/registry.md)
- [Console and pipe APIs](../api/console-and-pipes.md)
- [Messages and debugging](../api/messages-and-debugging.md)
- [Synchronization](../runtime/synchronization.md)
- [Threading and callbacks](../runtime/threading-and-callbacks.md)
- [Environment and paths](../runtime/environment-and-paths.md)
- [API support](api-support.md)

## Initialization

Drivers normally use `LdkDriverEntry` as the custom entry point. That lets LDK
run its startup and shutdown hooks around the driver's entry point and unload
routine. Drivers with their own entry point can call `LdkInitialize` during
startup and `LdkTerminate` before unload.

The current initialization flow is:

1. Heap list setup.
2. PEB-like process state setup.
3. TEB map setup for kernel threads that enter LDK code.
4. NTDLL component initialization.
5. Kernel32 component initialization.

Termination runs the reverse side of that flow. It also marks shutdown in
progress before unregistering modules and draining runtime components, so new
waiters can reject work instead of blocking while the driver is unloading.

## Runtime state

LDK maintains lightweight PEB/TEB-style state for kernel-mode callers:

- The PEB-like state owns module lists, loader state, process heap state, and
  process-style environment/current-directory data.
- The TEB-like state stores per-thread last-error values, TLS/FLS slots,
  static conversion buffers, keyed-event wait data, and thread identity.

Kernel threads do not have normal user-mode TEBs, so LDK maps `PETHREAD`
objects to LDK TEB records and also caches the current TEB on the thread stack
when possible.

## Module registration

LDK registers small export tables for its Kernel32 and NTDLL surfaces. Loader
helpers use those registrations to resolve supported imports for DLLs or code
that expects familiar API names.

The major runtime areas are:

- `src/kernel32/`: selected Kernel32-style APIs.
- `src/ntdll/`: selected NTDLL-style APIs and RTL helpers.
- `src/peb.c` and `src/teb.c`: process/thread runtime state.
- `src/ldk.c`: initialization, termination, module registration, and unload
  coordination.

## API areas

The implemented surface is intentionally partial, but it covers several common
runtime categories:

- File and handle helpers.
- Console shims.
- Heap helpers.
- Registry read helpers.
- Environment and current-directory helpers.
- Critical sections, condition variables, keyed events, and selected wait
  primitives.
- Thread creation and basic process/thread information helpers.
- FLS/TLS and fiber-related helpers.
- Loader helpers such as `LoadLibrary`, `GetProcAddress`, and module lookup.
- Time, NLS, string conversion, and utility helpers.
- A minimal `ICU.DLL` pseudo-module for fixed-offset time-zone lookups.

Unsupported user-mode subsystems such as GUI APIs, COM, broad CRT startup
behavior, and arbitrary user-mode assumptions should not be expected to work.
