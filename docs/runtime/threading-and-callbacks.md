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

## TLS callbacks, ExitThread, and FLS callbacks

LDK-created threads notify loaded dynamic modules that have PE TLS callbacks:

- `DLL_THREAD_ATTACH` callbacks run before the caller's thread start routine.
- `DLL_THREAD_DETACH` callbacks run after the start routine returns.
- `ExitThread` also runs `DLL_THREAD_DETACH` callbacks before terminating the
  current system thread.

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
- `FreeLibraryWhenCallbackReturns`

`QueueUserWorkItem` uses `RtlQueueWorkItem`.

Custom threadpool callback environments are not supported. The current
implementation is suitable for controlled work callbacks in tests, not as a
drop-in replacement for the full Windows threadpool.

## Testing

The test driver covers basic thread creation, TLS callback process/thread
notifications, FLS callback behavior, legacy work items, modern threadpool work
items, and wait-on-address stress paths that create and join many worker
threads.
