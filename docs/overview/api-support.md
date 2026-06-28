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
| Loader | `LoadLibraryA/W`, `LoadLibraryExA/W`, `FreeLibrary`, `GetProcAddress`, `GetModuleHandleA/W`, `GetModuleFileNameA/W`, `DisableThreadLibraryCalls`, `AddDllDirectory`, `RemoveDllDirectory`, `SetDefaultDllDirectories` | Experimental | Supports tested name/ordinal export lookup, import-by-ordinal, load counts, pinning, basic acyclic import dependency ownership, TLS callbacks, DllMain thread notifications, process-wide user DLL directories, selected `LoadLibraryExW` search flags, `DONT_RESOLVE_DLL_REFERENCES`, and resource/datafile handles. Complex dependency graphs still need workload validation. See [Loader](../runtime/loader.md). |
| Runtime state | `GetLastError`, `SetLastError`, `GetStartupInfoA/W`, `GetCurrentProcess`, `GetCurrentProcessId`, `OpenProcess`, `GetCurrentThread` | Partial | Process state is backed by the LDK runtime instance, while thread identity follows real kernel threads with LDK TEB sidecars. |
| Environment and paths | `GetEnvironmentVariableA/W`, `SetEnvironmentVariableA/W`, `GetEnvironmentStringsA/W`, `GetCurrentDirectoryA/W`, `SetCurrentDirectoryA/W`, `SetDllDirectoryA/W`, `GetDllDirectoryA/W`, `GetCommandLineA/W` | Partial | Backed by `RTL_USER_PROCESS_PARAMETERS` and LDK PEB path state. See [Environment and paths](../runtime/environment-and-paths.md). |
| Synchronization | Critical sections, condition variables, waits, `Sleep`, `WaitOnAddress`, `WakeByAddressSingle`, `WakeByAddressAll` | Partial | Common paths are tested, including multi-threaded wait-on-address stress. See [Synchronization](../runtime/synchronization.md). |
| Threads | `CreateThread`, `OpenThread`, `ExitThread`, `GetExitCodeThread`, `GetThreadTimes`, priority helpers | Partial | Uses kernel system threads and sends tested TLS callback and DllMain thread attach/detach notifications. `CREATE_SUSPENDED` is not supported. See [Threading and callbacks](../runtime/threading-and-callbacks.md). |
| FLS | `FlsAlloc`, `FlsFree`, `FlsGetValue`, `FlsSetValue` | Partial | Stored in LDK TEB records. Callback lifetime is tied to thread exit and LDK teardown. |
| Threadpool | `QueueUserWorkItem`, `CreateThreadpoolWork`, `SubmitThreadpoolWork`, `WaitForThreadpoolWorkCallbacks`, `CloseThreadpoolWork`, `FreeLibraryWhenCallbackReturns` | Partial | Implemented over kernel work items. Custom callback environments are not supported. |
| Heap | `HeapCreate`, `HeapDestroy`, `HeapAlloc`, `HeapFree`, `HeapReAlloc`, `GetProcessHeap`, `HeapSize`, `HeapValidate`, `HeapCompact`, `HeapWalk`, `HeapQueryInformation` | Partial | `HeapWalk` enumerates tracked busy LDK allocations. `HeapQueryInformation` currently supports `HeapCompatibilityInformation`. See [Heap APIs](../api/heap.md). |
| Memory management | `VirtualQuery`, `VirtualProtect` | Partial | Thin wrappers over `ZwQueryVirtualMemory` and `ZwProtectVirtualMemory` for the current address space. They are currently exercised by the MSVC delay-load helper path. |
| File and handles | `CreateFileA/W`, `ReadFile`, `WriteFile`, `FlushFileBuffers`, `GetFileInformationByHandleEx`, `SetFileInformationByHandle`, `GetTempPathA/W`, `GetTempFileNameA/W`, `CloseHandle`, `DuplicateHandle`, `CreatePipe`, `PeekNamedPipe` | Partial | Supports selected file information classes. API-valid but unimplemented classes report `STATUS_NOT_IMPLEMENTED`; invalid classes report `ERROR_INVALID_PARAMETER`. See [File and handle APIs](../api/file-and-handles.md). |
| Console and pipes | `GetConsoleCP`, `GetConsoleOutputCP`, `GetConsoleMode`, `SetConsoleMode`, `ReadConsoleA/W`, `WriteConsoleA/W`, `SetConsoleCtrlHandler`, `SetConsoleCP`, `SetConsoleOutputCP`, `CreatePipe`, `PeekNamedPipe` | Partial | Console devices are lightweight shims created during PEB initialization. Pipes cover local anonymous-pipe scenarios. See [Console and pipe APIs](../api/console-and-pipes.md). |
| Time and system info | `GetSystemTime`, `GetLocalTime`, `GetTickCount`, `GetTickCount64`, `GetSystemInfo`, `GetVersion`, `GlobalMemoryStatusEx`, `FileTimeToSystemTime`, `SystemTimeToFileTime`, `GetTimeZoneInformation`, `SystemTimeToTzSpecificLocalTime` | Partial | Mostly thin kernel-backed helpers. Date/time formatting is documented with NLS. |
| NLS and strings | `MultiByteToWideChar`, `WideCharToMultiByte`, `GetCPInfoExA/W`, `GetLocaleInfoW`, `GetLocaleInfoEx`, `GetDateFormatW`, `GetTimeFormatW`, `GetStringTypeW`, locale enumeration and LCID/name helpers | Partial | Detailed locale records cover `en-US`, `de-DE`, `en-GB`, `ko-KR`, `zh-CN`, `ja-JP`, and `ru-RU`; this is still not full globalization coverage. See [NLS, strings, and time formatting](../api/nls-and-time.md). |
| ICU/time zones | Loader-visible ICU entry points used by MSVC STL time-zone code | Experimental | Provides a small fixed-offset time-zone subset: `Etc/UTC`, `UTC`, `GMT`, `Asia/Seoul`, `Asia/Tokyo`, `Asia/Shanghai`, `America/New_York`, `Europe/Berlin`, and `Europe/Moscow`. Full IANA tzdb data and DST transitions are not implemented. See [ICU virtual module](../api/icu.md). |
| Registry | `RegOpenKeyExW`, `RegQueryValueExW`, `RegCloseKey` | Experimental | Implemented as Unicode helpers, but not currently registered in the Kernel32 pseudo-module export table. See [Registry APIs](../api/registry.md). |
| Messages, debug, and exceptions | `FormatMessageA/W`, `GetLastError`, `SetLastError`, `GetErrorMode`, `SetErrorMode`, `IsDebuggerPresent`, `DebugBreak`, `OutputDebugStringA/W`, `RaiseException`, exception-filter helpers | Partial | `FormatMessage` supports string, system-message, and mapped-module message-table paths for tested scenarios. Insert expansion is not a full implementation. Some debug paths are intentionally debugger-facing. See [Messages and debugging](../api/messages-and-debugging.md). |

## NTDLL/RTL-style surface

| Area | Representative APIs | Status | Notes |
| --- | --- | --- | --- |
| Loader | `LdrLoadDll`, `LdrUnloadDll`, `LdrGetProcedureAddress`, `LdrGetDllHandle` | Experimental | Wraps the LDK loader and module list. `LdrGetDllHandle` covers tested name and path-aware lookup paths. |
| Synchronization | `RtlInitializeCriticalSection`, `RtlEnterCriticalSection`, `RtlLeaveCriticalSection`, condition-variable RTL APIs, SRW helpers | Partial | Implemented in LDK over kernel wait primitives and wait-on-address helpers. |
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
