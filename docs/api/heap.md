# Heap APIs

LDK heap APIs provide a Win32-like heap surface backed by LDK's internal heap
objects and kernel pool allocations. They are intended for DLLs and runtime code
that expect Kernel32 heap entry points, not for emulating every Windows heap
segment behavior.

## Heap lifetime

`LdkInitialize` creates the process heap stored in the LDK PEB. `GetProcessHeap`
returns that heap. Additional heaps can be created with `HeapCreate` and
released with `HeapDestroy`.

Heap objects are tracked in the LDK heap list. Individual allocations are linked
to their owning heap so validation, sizing, walking, and teardown can reason
about live blocks.

## Supported operations

The current Kernel32-style heap surface includes:

- `HeapCreate`
- `HeapDestroy`
- `HeapAlloc`
- `HeapFree`
- `HeapReAlloc`
- `HeapSize`
- `HeapValidate`
- `HeapCompact`
- `HeapWalk`
- `HeapQueryInformation`
- `GetProcessHeap`

`HeapAlloc`, `HeapFree`, `HeapReAlloc`, and `HeapSize` operate on tracked heap
entries. `HeapValidate` checks whether a heap and optional allocation belong to
LDK's heap bookkeeping.

## Walking and query behavior

`HeapWalk` uses the internal `LdkWalkHeap` helper to enumerate busy heap
entries under the heap-list lock. The returned `PROCESS_HEAP_ENTRY` entries
represent active LDK allocations, not Windows heap segments or free-list
internals.

`HeapQueryInformation` currently supports `HeapCompatibilityInformation` and
returns compatibility value `0`. Other heap information classes return
`ERROR_INVALID_PARAMETER` until they have a meaningful LDK-backed
implementation.

`HeapCompact` validates the heap handle and returns `0`. LDK heap allocations
are kernel-pool backed and do not expose a compactable committed segment in the
Win32 heap sense.

## Tests

`HeapApiTest` creates a private heap, allocates multiple blocks, verifies
`HeapQueryInformation`, walks the heap until both blocks are observed, exercises
`HeapCompact`, then frees and destroys the heap.
