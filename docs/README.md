# LDK documentation

This directory contains implementation notes that are too detailed for the
top-level README.

Start here:

- [Architecture](overview/architecture.md): initialization, module
  registration, and runtime component layout.
- [API support](overview/api-support.md): supported API areas and current
  caveats.
- [Testing](testing/testing.md): test driver structure, manual load
  expectations, and stress-test guidance.
- [Documentation coverage audit](overview/documentation-audit.md):
  implementation areas, current doc coverage, and known documentation/test
  gaps.

Runtime:

- [Runtime state](overview/runtime-state.md): LDK PEB/TEB state, last-error
  storage, TLS/FLS, and current thread identity.
- [Loader](runtime/loader.md): `LoadLibrary`, `GetProcAddress`, `LdrLoadDll`,
  import resolution, and unload behavior.
- [Synchronization](runtime/synchronization.md): critical sections, SRW locks,
  condition variables, waits, keyed events, and wait-on-address overview.
- [Wait-on-address synchronization](runtime/wait-on-address.md): detailed
  `WaitOnAddress` / `RtlWaitOnAddress` / alert-by-thread-id notes.
- [Keyed events](runtime/keyed-event.md): current keyed-event implementation
  notes and limitations.
- [Threading and callbacks](runtime/threading-and-callbacks.md):
  `CreateThread`, `ExitThread`, FLS callbacks, and threadpool behavior.
- [Environment and paths](runtime/environment-and-paths.md): process
  parameters, environment variables, current directory, and DLL search path
  behavior.

APIs:

- [File and handle APIs](api/file-and-handles.md): file I/O, file information
  classes, temporary files, enumeration, and `DeviceIoControl` limits.
- [Heap APIs](api/heap.md): heap lifetime, walking, query behavior, and tests.
- [COMBASE virtual module](api/combase.md): the minimal `COMBASE.DLL`
  pseudo-module, task-memory helpers, and runtime-initialization helpers.
- [ICU virtual module](api/icu.md): the minimal `ICU.DLL` pseudo-module and
  fixed time-zone subset.
- [NLS, strings, and time formatting](api/nls-and-time.md): code-page
  conversion, locale data, string classification, and date/time formatting.
- [Registry APIs](api/registry.md): Unicode registry helper subset and export
  visibility.
- [Console and pipe APIs](api/console-and-pipes.md): standard-handle console
  shims and anonymous-pipe helpers.
- [Messages and debugging](api/messages-and-debugging.md): `FormatMessage`,
  last error, exception filters, and debugger-facing helpers.

Synchronization:

LDK is intentionally incomplete. These notes describe the current runtime
behavior and test coverage; they are not a compatibility promise for every
Win32 or NTDLL API.
