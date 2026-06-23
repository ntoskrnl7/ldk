# Threading and callbacks

LDK implements a narrow thread and callback surface for kernel-mode code that
expects selected Kernel32 behavior.

## CreateThread

`CreateThread` creates a kernel system thread with `PsCreateSystemThread`. The
thread start routine receives the caller's parameter and its return value is
used as the system-thread exit status. That lets `GetExitCodeThread` observe the
start routine result through `ThreadBasicInformation`.

If the requested stack size is larger than the remaining kernel stack, LDK runs
the start routine through `KeExpandKernelStackAndCallout`. The heap-allocated
thread context is released before entering the callout path so an `ExitThread`
inside the start routine does not leak that context.

`CREATE_SUSPENDED` is not supported.

## ExitThread and FLS callbacks

`ExitThread` invokes FLS callbacks for the current LDK TEB, then terminates the
system thread with `PsTerminateSystemThread`.

FLS callback rules in LDK:

- Callback pointers are stored in the LDK PEB.
- Per-thread FLS data lives in the LDK TEB.
- A callback is invoked only when both the callback pointer and slot data are
  non-null.
- Slot data is exchanged to `NULL` before invoking the callback.

During `LdkTerminate`, the TEB map is moved to a temporary list before FLS
callbacks are invoked. That avoids holding the TEB list lock while callback code
runs.

## Thread identity and handles

`GetCurrentThread` returns the current thread pseudo handle. `GetCurrentThreadId`
uses `PsGetCurrentThreadId`. `OpenThread`, priority helpers, `GetThreadTimes`,
and `GetExitCodeThread` are thin wrappers over native thread information calls.

The LDK TEB stores client-id values for APIs that need process/thread identity,
but this is LDK-maintained state, not a user-mode TEB.

## Threadpool

The modern threadpool work APIs are implemented over kernel work items:

- `CreateThreadpoolWork`
- `SubmitThreadpoolWork`
- `WaitForThreadpoolWorkCallbacks`
- `CloseThreadpoolWork`
- `FreeLibraryWhenCallbackReturns`

`QueueUserWorkItem` uses `RtlQueueWorkItem`.

Custom threadpool callback environments are not supported. The current
implementation is suitable for controlled work callbacks in tests, not as a
drop-in replacement for the full Windows threadpool.

## Testing

The test driver covers basic thread creation, FLS callback behavior, legacy
work items, modern threadpool work items, and wait-on-address stress paths that
create and join many worker threads.
