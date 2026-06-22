# Keyed events

LDK contains a small native keyed-event implementation used by synchronization
code and covered by the NTDLL keyed-event tests.

Implemented APIs:

- `NtCreateKeyedEvent`
- `NtOpenKeyedEvent`
- `NtWaitForKeyedEvent`
- `NtReleaseKeyedEvent`

## Implementation

`NtCreateKeyedEvent` and `NtOpenKeyedEvent` currently wrap kernel event object
creation/opening while forcing kernel handles through object attributes.

Waiters are represented by entries in an internal wait list. Each entry stores:

- The wait key.
- The waiting LDK TEB.

`NtWaitForKeyedEvent` inserts the current thread's TEB into the list, pulses the
backing event handle, stores the keyed wait value in the TEB, then waits on the
TEB's keyed-wait semaphore.

`NtReleaseKeyedEvent` searches the wait list for a matching key. If it finds a
waiter, it removes that entry, releases the waiter's semaphore, and pulses the
backing event.

## Teardown

The NTDLL component initializes the keyed-event wait list before condition
variables and related synchronization code are initialized. During teardown, it
walks remaining wait entries, releases any still-associated TEB semaphores, and
frees the list entries before deleting the lookaside list.

## Current caveats

The implementation is intentionally small and should not be treated as a full
Windows keyed-event clone. In particular:

- The APIs are declared and implemented, but they are not currently listed in
  the NTDLL pseudo-module registration table.
- Timeout and alertable edge cases should be audited before widening use.
- The implementation is tied to the LDK TEB model.

Use the existing tests as a regression baseline, then add targeted stress tests
before relying on keyed events for new runtime behavior.
