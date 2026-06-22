# LDK documentation

This directory contains implementation notes that are too detailed for the
top-level README.

Start here:

- [Architecture](architecture.md): initialization, module registration, and
  runtime component layout.
- [API support](api-support.md): supported API areas and current caveats.
- [Testing](testing.md): test driver structure, manual load expectations, and
  stress-test guidance.

Runtime areas:

- [Runtime state](runtime-state.md): LDK PEB/TEB state, last-error storage,
  TLS/FLS, and current thread identity.
- [Loader](loader.md): `LoadLibrary`, `GetProcAddress`, `LdrLoadDll`, import
  resolution, and unload behavior.
- [Synchronization](synchronization.md): critical sections, SRW locks,
  condition variables, waits, keyed events, and wait-on-address overview.
- [Wait-on-address synchronization](wait-on-address.md): detailed
  `WaitOnAddress` / `RtlWaitOnAddress` / alert-by-thread-id notes.
- [Keyed events](keyed-event.md): current keyed-event implementation notes and
  limitations.
- [Threading and callbacks](threading-and-callbacks.md): `CreateThread`,
  `ExitThread`, FLS callbacks, and threadpool behavior.
- [Environment and paths](environment-and-paths.md): process parameters,
  environment variables, current directory, and DLL search path behavior.

LDK is intentionally incomplete. These notes describe the current runtime
behavior and test coverage; they are not a compatibility promise for every
Win32 or NTDLL API.
