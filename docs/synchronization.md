# Synchronization

LDK implements selected Kernel32, RTL, and native synchronization primitives for
kernel-mode callers.

## Implemented areas

The current synchronization surface includes:

- Critical sections: `InitializeCriticalSection`, `EnterCriticalSection`,
  `LeaveCriticalSection`, `DeleteCriticalSection`, and RTL equivalents.
- SRW locks: public declarations and RTL-backed helpers.
- Condition variables: Kernel32 wrappers and RTL condition-variable routines.
- Waits and delay: `WaitForSingleObject`, `WaitForMultipleObjects`,
  alertable variants, `Sleep`, and `SleepEx`.
- One-time initialization: `InitOnceExecuteOnce`.
- Keyed events: `NtCreateKeyedEvent`, `NtOpenKeyedEvent`,
  `NtWaitForKeyedEvent`, and `NtReleaseKeyedEvent`.
- Wait-on-address: `WaitOnAddress`, `WakeByAddressSingle`,
  `WakeByAddressAll`, and RTL equivalents.
- Native alert wait: `NtAlertThreadByThreadId` and
  `NtWaitForAlertByThreadId`.

The detail pages are:

- [Wait-on-address synchronization](wait-on-address.md)
- [Keyed events](keyed-event.md)

## Layering

Kernel32 APIs generally translate Win32-style parameters and errors to the RTL
or native layer. NTDLL/RTL functions own the lower-level state and return
`NTSTATUS`.

The wait-on-address implementation is layered as:

`WaitOnAddress` -> `RtlWaitOnAddress` -> `NtWaitForAlertByThreadId`

Wake calls follow the reverse direction through address wait-list lookup and
`NtAlertThreadByThreadId`.

Condition variables and critical sections use the NTDLL synchronization code
and keyed-event support initialized by the NTDLL component.

## Shutdown behavior

Synchronization code must be unload-aware. During `LdkTerminate`, shutdown is
marked before runtime components are drained. The wait-on-address layer tracks
active waiters so teardown can avoid freeing wait-list state while threads are
still blocked in it. Keyed-event teardown releases waiters that remain on the
internal list.

## Testing

The test driver currently covers:

- Condition variables and critical sections.
- Keyed events.
- Wait-on-address single wake, wake-all, timeout, multiple address sizes, mixed
  address sets, direct RTL waits, native alert waits, and durability loops.

For new synchronization work, add both a narrow behavior test and at least one
multi-threaded stress case. Driver Verifier and repeated load/unload runs are
still recommended for changes that touch wait lists, thread lifetime, or
teardown paths.
