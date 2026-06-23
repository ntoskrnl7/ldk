# Runtime state

Kernel threads do not have user-mode PEB and TEB records. LDK creates a small
PEB/TEB-like runtime so code that expects selected Win32 and NTDLL state has a
controlled place to read and write it.

## Initialization

`LdkInitialize` creates runtime state in this order:

1. Heap list.
2. PEB-like process state.
3. TEB map.
4. NTDLL components.
5. Kernel32 components.

`LdkTerminate` marks shutdown in progress, unregisters modules, terminates
Kernel32 and NTDLL components, tears down the TEB map, then destroys the PEB and
heap list.

## PEB-like state

`LdkCurrentPeb()` returns the global LDK PEB record. It owns:

- Driver object and image information.
- Process heap.
- Module list and module-list resource.
- Process parameters.
- TLS and FLS allocation bitmaps.
- FLS callback table.

The initial module list contains LDK's Kernel32 and NTDLL pseudo modules. DLLs
loaded through the loader are added to the same list.

## Process parameters

LDK initializes `RTL_USER_PROCESS_PARAMETERS` with:

- Standard input, output, and error handles backed by lightweight console
  device objects.
- Image path and command line derived from the loaded driver image.
- Current directory rooted at `NtSystemRoot`.
- DLL path and `PATH` seeded from the system directory.
- Environment entries such as `windir`, `SystemDrive`, and `SystemRoot`.

Environment, current-directory, command-line, and startup-info APIs read from
this state.

## TEB-like state

LDK maps `PETHREAD` objects to LDK TEB records. `LdkCurrentTeb()` first checks a
cached pointer stored near the current stack limit. If that cache cannot be used
for example after a kernel stack expansion, it looks up the current `PETHREAD`
in the TEB map and refreshes the cache.

Each LDK TEB stores:

- A pointer back to the LDK PEB.
- The owning `PETHREAD`.
- Client id values.
- Last-error and status-related state.
- Static string conversion buffers.
- TLS/FLS slots.
- Keyed-event and alert-by-thread-id wait state.

During shutdown, new TEB creation falls back to a temporary list so teardown
paths that still touch TEB state can complete without re-entering the normal
TEB map.

## TLS, FLS, and last error

TLS and FLS indexes are allocated from bitmaps in the LDK PEB. Per-thread slot
values live in the current LDK TEB.

FLS callbacks are invoked when an LDK-created thread exits and again during TEB
map teardown for remaining TEBs. LDK invokes a callback only when both the
registered callback and slot data are non-null.

`GetLastError` and `SetLastError` are thread-local through the LDK TEB. They do
not interact with any user-mode TEB.

## Practical notes

LDK runtime state is process-like, but it is not the real user-mode process
environment. Code that assumes user-mode loader state, a real user TEB, GUI
thread state, APC behavior, or normal CRT startup should be treated as
unsupported until specifically audited and tested.
