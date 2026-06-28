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

## DLL thread notifications, ExitThread, and FLS callbacks

LDK-created threads notify loaded dynamic modules before and after the caller's
thread start routine:

- Loader-managed static TLS storage is initialized for the new LDK TEB before
  module callbacks run.
- PE TLS callbacks run with `DLL_THREAD_ATTACH`, then DLL entry points receive
  `DllMain(DLL_THREAD_ATTACH)`.
- On normal return, PE TLS callbacks run with `DLL_THREAD_DETACH`, then DLL
  entry points receive `DllMain(DLL_THREAD_DETACH)`, and that thread's static
  TLS blocks are freed.
- `ExitThread` also runs the detach notification pass before terminating the
  current system thread.

`DisableThreadLibraryCalls` suppresses the `DllMain(DLL_THREAD_ATTACH)` and
`DllMain(DLL_THREAD_DETACH)` calls for the target module. It does not suppress
PE TLS callbacks or process attach/detach entry-point calls.

The module list is snapshotted before callback execution, so loader locks are
not held while DLL code runs. The snapshot temporarily references each attached
module to keep it loaded until that thread notification pass finishes.

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

The thread itself is a real kernel thread. The LDK TEB is a sidecar attached to
the participating `PETHREAD`, so TLS/FLS, last-error, keyed-event, and
alert-by-thread-id state can follow that kernel execution context. The process
half of the client id is different: it identifies the synthetic LDK process
runtime, while `RealClientId` keeps the host kernel process id.

## Threadpool

The modern threadpool work APIs are implemented over kernel work items:

- `CreateThreadpoolWork`
- `SubmitThreadpoolWork`
- `WaitForThreadpoolWorkCallbacks`
- `CloseThreadpoolWork`
- `CreateThreadpoolCleanupGroup`
- `CloseThreadpoolCleanupGroup`
- `CloseThreadpoolCleanupGroupMembers`
- `FreeLibraryWhenCallbackReturns`

`QueueUserWorkItem` uses `RtlQueueWorkItem`.

Callback environments are supported for the default pool work-object path.
LDK honors long-function, persistent, priority, race-with-DLL, cleanup group,
and cleanup group cancel callback fields for tested work callbacks. Cleanup
groups keep a list of member work objects so
`CloseThreadpoolCleanupGroupMembers` can wait, optionally request pending
callback cancellation, invoke the cleanup callback for each member, and close
the member objects without holding the cleanup group lock while callbacks run.
`CloseThreadpoolCleanupGroup` releases only the cleanup group object; callers
should close group members first.

The current implementation remains narrower than the full Windows threadpool:
custom pools, activation contexts, finalization callbacks, timers, waits, and
I/O completion threadpool objects are not implemented.

## Testing

The test driver covers basic thread creation, TLS callback process/thread
notifications, DllMain thread notifications, `DisableThreadLibraryCalls`, FLS
callback behavior, legacy work items, modern threadpool work items, threadpool
callback environments, cleanup group member close/cancel paths, and
wait-on-address stress paths that create and join many worker threads.
