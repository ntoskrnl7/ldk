# Testing

The `test` directory builds `LdkTest.sys`, a kernel driver that links against
LDK and exercises the runtime from `DriverEntry`.

GitHub Actions verifies that the test driver builds, but kernel driver loading
must be done manually in an isolated Windows driver test environment.

## Test driver flow

`test/driver.c` runs test routines one by one and logs each result with a stable
name:

- `[LdkTest] RUN  <name>`
- `[LdkTest] PASS <name>`
- `[LdkTest] FAIL <name>`

If a test fails, `DriverEntry` calls `LdkTerminate` and returns failure so the
service start fails instead of leaving a partially tested driver loaded.

The current test suite covers:

- NTDLL current-directory and environment helpers.
- Keyed events.
- Loader helpers.
- Fibers and FLS callbacks.
- File helpers.
- Kernel32 current-directory and environment helpers.
- Legacy and newer threadpool helpers.
- Condition variables and critical sections.
- Wait-on-address synchronization.
- Library loading helpers.
- Console helpers.

## Debugger behavior

The test driver intentionally contains debugger breakpoints in some paths. When
running under WinDbg/KD, continue execution after expected breakpoints before
judging the test result.

The service control result is the final pass/fail signal:

- `sc start` success and service state `RUNNING` means all tests reached the
  end of `DriverEntry`.
- `sc start` failure with `[LdkTest] FAIL <name>` in debugger output identifies
  the failing test.
- A bugcheck or stuck `START_PENDING` state should be treated as a runtime
  failure even if no explicit `[LdkTest] FAIL` line was printed.

## Stress guidance

The in-tree tests are designed to catch common regressions quickly. For changes
to synchronization, loader, or thread lifetime behavior, also run additional
manual validation when possible:

- Repeated load/unload loops.
- Driver Verifier with pool, IRQL, and deadlock-related checks.
- Longer mixed-operation stress runs.
- Tests on the oldest and newest Windows versions the project intends to
  support.

Any new runtime primitive should include a narrow success test, timeout or
failure-path coverage when applicable, and at least one multi-threaded test if
the implementation owns shared state.
