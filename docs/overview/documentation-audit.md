# Documentation coverage audit

This page tracks the main implementation areas and where their behavior is
documented. It is meant to keep docs changes honest when new APIs are added.

## Covered areas

| Implementation area | Primary files | Documentation |
| --- | --- | --- |
| Initialization and module layout | `src/ldk.c`, `src/main.c`, `src/peb.c`, `src/teb.c` | [Architecture](architecture.md), [Runtime state](runtime-state.md) |
| Loader and pseudo-modules | `src/ldk.c`, `src/kernel32/libloaderapi.c`, `src/ntdll/ldr.c` | [Loader](../runtime/loader.md) |
| Kernel32/NTDLL API inventory | `src/kernel32/kernel32.c`, `src/ntdll/ntdll.c` | [API support](api-support.md) |
| File, handle, pipe, and temp-file APIs | `src/kernel32/fileapi.c`, `src/kernel32/handleapi.c`, `src/kernel32/namepipe.c` | [File and handle APIs](../api/file-and-handles.md), [Console and pipe APIs](../api/console-and-pipes.md) |
| Heap APIs | `src/kernel32/heapapi.c`, `src/ntdll/heap.c` | [Heap APIs](../api/heap.md), [Runtime state](runtime-state.md) |
| Time, system, processor topology, and affinity helpers | `src/kernel32/sysinfoapi.c`, `src/kernel32/processthreadsapi.c` | [API support](api-support.md), [Threading and callbacks](../runtime/threading-and-callbacks.md) |
| Environment and current directory | `src/kernel32/processenv.c`, `src/ntdll/environ.c`, `src/ntdll/curdir.c` | [Environment and paths](../runtime/environment-and-paths.md) |
| Synchronization | `src/kernel32/synchapi.c`, `src/ntdll/cond.c`, `src/ntdll/srwlock.c`, `src/ntdll/keyedevent.c`, `src/ntdll/waitonaddress.c`, `src/ntdll/alert.c` | [Synchronization](../runtime/synchronization.md), [Wait-on-address synchronization](../runtime/wait-on-address.md), [Keyed events](../runtime/keyed-event.md) |
| Threads, FLS, and threadpool | `src/kernel32/processthreadsapi.c`, `src/kernel32/fibersapi.c`, `src/kernel32/threadpoolapiset.c`, `src/kernel32/threadpoollegacyapiset.c`, `src/ntdll/threads.c` | [Threading and callbacks](../runtime/threading-and-callbacks.md) |
| Console shims | `src/kernel32/consoleapi.c`, `src/kernel32/consoleapi2.c` | [Console and pipe APIs](../api/console-and-pipes.md) |
| NLS, strings, locale, and time formatting | `src/kernel32/stringapiset.c`, `src/kernel32/winnls.c`, `src/kernel32/timezoneapi.c`, `src/ntdll/nls.c`, `src/ntdll/time.c` | [NLS, strings, and time formatting](../api/nls-and-time.md) |
| ICU pseudo-module | `src/icu/icu.c` | [ICU virtual module](../api/icu.md) |
| COMBASE pseudo-module | `src/combase/combase.c` | [COMBASE virtual module](../api/combase.md) |
| Registry helpers | `src/kernel32/winreg.c`, `include/Ldk/winreg.h` | [Registry APIs](../api/registry.md) |
| Messages, debug, exceptions, and last error | `src/kernel32/winbase.c`, `src/kernel32/debugapi.c`, `src/kernel32/errhandlingapi.c`, `src/ntdll/except.c`, `src/ntdll/error.c` | [Messages and debugging](../api/messages-and-debugging.md), [Runtime state](runtime-state.md) |
| Tests and manual validation | `test/` | [Testing](../testing/testing.md) |

## Known gaps

The following areas exist in code but need stronger tests or docs before they
should be treated as mature compatibility surfaces:

- Pipe helpers need a broader pipe behavior test beyond compile coverage and
  indirect use.
- NLS/date/time formatting has direct representative locale coverage, but
  broader code-page conversion matrices and uncommon formatting tokens still
  need tests when those paths are expanded.
- Console helpers need more coverage if input behavior, control handlers, or
  screen-buffer-like APIs are expanded.
- Any new pseudo-module must get both an API support table entry and a detailed
  page like [ICU virtual module](../api/icu.md).
