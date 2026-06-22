# Wait-on-address synchronization

This is the detailed note for the wait-on-address family. For the wider
synchronization map, see [Synchronization](synchronization.md).

LDK implements the Windows wait-on-address family for kernel-mode callers while
keeping the public declarations close to the Windows SDK and NTDLL API shapes.

Implemented APIs:

- `WaitOnAddress`
- `WakeByAddressSingle`
- `WakeByAddressAll`
- `RtlWaitOnAddress`
- `RtlWakeAddressSingle`
- `RtlWakeAddressAll`
- `NtAlertThreadByThreadId`
- `NtWaitForAlertByThreadId`

## Implementation

`WaitOnAddress` is a Kernel32 wrapper over `RtlWaitOnAddress`. The RTL
implementation keeps a kernel-mode wait list keyed by the watched address and
uses the native alert-by-thread-id primitive to wake blocked waiters.

The native alert layer supports both direct waiters and alert-before-wait
delivery. Pending alerts are keyed by the referenced target thread object, not
only by the numeric thread id. This avoids delivering a stale pending alert to a
later thread that happens to reuse the same thread id.

Wait list and pending alert state are protected by spin locks. Active waiters
are counted and drained during LDK termination so the driver is not unloaded
while waiters are still blocked in LDK code.

## Semantics

As on Windows, a successful wake-up from `WaitOnAddress` does not mean the
watched value has a particular new value. Callers must re-check their predicate
after every wake and use the wait in a loop.

`RtlWaitOnAddress` accepts address sizes of 1, 2, 4, and 8 bytes. Unsupported
sizes fail with `STATUS_INVALID_PARAMETER`.

`WaitOnAddress` returns `FALSE` for timeout and records the mapped last-error
value. Other successful native wait completions return `TRUE`.

## Test coverage

The test driver includes multi-threaded coverage for:

- Kernel32 `WaitOnAddress` / `WakeByAddressSingle` / `WakeByAddressAll`
- direct `RtlWaitOnAddress` / `RtlWakeAddressSingle` / `RtlWakeAddressAll`
- `NtAlertThreadByThreadId` / `NtWaitForAlertByThreadId`
- timeout handling
- wake-all and repeated wake-single behavior
- multiple independent wait addresses
- 1, 2, 4, and 8 byte address sizes
- repeated create/wait/wake/exit durability rounds

These tests are intended to catch missed wake-ups, stale pending alerts, address
isolation bugs, timeout misreporting, and unload-time pending waiter issues.
They are not a substitute for Driver Verifier or long-running soak tests in a
target deployment environment.
