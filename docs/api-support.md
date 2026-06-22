# API support

LDK exposes a selected Win32/NTDLL-like surface for kernel-mode code. The table
below is a practical map of the current implementation, not a promise of full
Windows compatibility.

Status guide:

- Supported: implemented and covered by in-tree tests or simple wrappers over
  kernel primitives.
- Partial: useful, but intentionally narrower than Windows.
- Experimental: works for the tested LDK scenarios, but needs extra validation
  before relying on it in a driver.

## Kernel32-style surface

| Area | Representative APIs | Status | Notes |
| --- | --- | --- | --- |
| Loader | `LoadLibraryA/W`, `LoadLibraryExA/W`, `FreeLibrary`, `GetProcAddress`, `GetModuleHandleA/W`, `GetModuleFileNameA/W` | Experimental | Kernel-space DLL loading is the project core, but loaded modules must be audited. See [Loader](loader.md). |
| Runtime state | `GetLastError`, `SetLastError`, `GetStartupInfoA/W`, `GetCurrentProcess`, `GetCurrentThread` | Partial | State is backed by LDK PEB/TEB records, not user-mode process state. |
| Environment and paths | `GetEnvironmentVariableA/W`, `SetEnvironmentVariableA/W`, `GetEnvironmentStringsA/W`, `GetCurrentDirectoryA/W`, `SetCurrentDirectoryA/W`, `GetCommandLineA/W` | Partial | Backed by `RTL_USER_PROCESS_PARAMETERS` inside the LDK PEB. See [Environment and paths](environment-and-paths.md). |
| Synchronization | Critical sections, condition variables, waits, `Sleep`, `WaitOnAddress`, `WakeByAddressSingle`, `WakeByAddressAll` | Partial | Common paths are tested, including multi-threaded wait-on-address stress. See [Synchronization](synchronization.md). |
| Threads | `CreateThread`, `OpenThread`, `ExitThread`, `GetExitCodeThread`, `GetThreadTimes`, priority helpers | Partial | Uses kernel system threads. `CREATE_SUSPENDED` is not supported. See [Threading and callbacks](threading-and-callbacks.md). |
| FLS | `FlsAlloc`, `FlsFree`, `FlsGetValue`, `FlsSetValue` | Partial | Stored in LDK TEB records. Callback lifetime is tied to thread exit and LDK teardown. |
| Threadpool | `QueueUserWorkItem`, `CreateThreadpoolWork`, `SubmitThreadpoolWork`, `WaitForThreadpoolWorkCallbacks`, `CloseThreadpoolWork`, `FreeLibraryWhenCallbackReturns` | Partial | Implemented over kernel work items. Custom callback environments are not supported. |
| Heap | `HeapCreate`, `HeapDestroy`, `HeapAlloc`, `HeapFree`, `HeapReAlloc`, `GetProcessHeap`, `HeapSize`, `HeapValidate`, `HeapCompact`, `HeapWalk`, `HeapQueryInformation` | Partial | `GetProcessHeap` returns the heap created for the LDK PEB. |
| File and handles | `CreateFileA/W`, `ReadFile`, `WriteFile`, `FlushFileBuffers`, `CloseHandle`, `DuplicateHandle`, `CreatePipe`, `PeekNamedPipe` | Partial | Intended for controlled kernel-mode paths, not broad user-mode I/O compatibility. |
| Console | `GetConsoleCP`, `GetConsoleOutputCP`, `GetConsoleMode`, `SetConsoleMode`, `ReadConsoleA/W`, `WriteConsoleA/W`, `SetConsoleCtrlHandler`, `SetConsoleCP`, `SetConsoleOutputCP` | Partial | Console devices are lightweight shims created during PEB initialization. |
| Time and system info | `GetSystemTime`, `GetLocalTime`, `GetTickCount`, `GetTickCount64`, `GetSystemInfo`, `GetVersion`, `GlobalMemoryStatusEx`, time-zone helpers | Partial | Mostly thin kernel-backed helpers. |
| NLS and strings | Code page, locale, and conversion helpers | Partial | Enough for current tests and loader/runtime conversions. |
| Debug and exceptions | `IsDebuggerPresent`, `DebugBreak`, `OutputDebugStringA/W`, `RaiseException`, exception-filter helpers | Partial | Some paths are intentionally debugger-facing. |

## NTDLL/RTL-style surface

| Area | Representative APIs | Status | Notes |
| --- | --- | --- | --- |
| Loader | `LdrLoadDll`, `LdrUnloadDll`, `LdrGetProcedureAddress`, `LdrGetDllHandle` | Experimental | Wraps the LDK loader and module list. |
| Synchronization | `RtlInitializeCriticalSection`, `RtlEnterCriticalSection`, `RtlLeaveCriticalSection`, condition-variable RTL APIs, SRW helpers | Partial | ReactOS-derived portions should be audited case by case. |
| Wait-on-address | `RtlWaitOnAddress`, `RtlWakeAddressSingle`, `RtlWakeAddressAll` | Partial | Uses LDK alert-by-thread-id primitives internally. |
| Native alert wait | `NtAlertThreadByThreadId`, `NtWaitForAlertByThreadId`, `Zw*` aliases | Partial | Pending alerts are keyed by the target `PETHREAD` object to avoid stale thread-id reuse. |
| Keyed events | `NtCreateKeyedEvent`, `NtOpenKeyedEvent`, `NtWaitForKeyedEvent`, `NtReleaseKeyedEvent` | Partial | Implemented and tested, but currently not listed in the NTDLL pseudo-module registration table. |
| Paths and environment | `RtlGetCurrentDirectory_U`, `RtlSetCurrentDirectory_U`, `RtlQueryEnvironmentVariable_U`, `RtlSetEnvironmentVariable`, DOS path helpers | Partial | Backed by LDK process parameters. |
| Work items | `RtlQueueWorkItem` | Partial | Used by `QueueUserWorkItem`. |

## General caveats

LDK does not implement GUI APIs, COM, normal user-mode process startup, broad
CRT assumptions, or arbitrary DLL behavior. Treat every loaded DLL as kernel
code and validate it with the same standard as a driver.

For exact declarations, use the headers under `include/Ldk`. For loader-visible
exports, check the registration tables in `src/kernel32/kernel32.c` and
`src/ntdll/ntdll.c`.
