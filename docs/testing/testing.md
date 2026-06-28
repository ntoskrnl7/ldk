# Testing

The `test` directory builds `LdkTest.sys`, a kernel driver that links against
LDK and exercises the runtime from `DriverEntry`.

GitHub Actions verifies that the test driver builds, but kernel driver loading
must be done manually in an isolated Windows driver test environment.

## Native Win32 baseline

`test/win32` builds a user-mode console executable from the same test sources
used by the kernel test driver. This verifies that tests intended to model
Win32 behavior also pass against the real Windows user-mode APIs.

Example local runs:

```powershell
cmake -S test/win32 -B artifacts/build/win32-api-x64 -G "Visual Studio 17 2022" -A x64 -T host=x64
cmake --build artifacts/build/win32-api-x64 --config Debug --target LdkWin32ApiTest
artifacts/build/win32-api-x64/bin/Debug/LdkWin32ApiTest.exe
```

The runner accepts an optional substring filter, for example:

```powershell
artifacts/build/win32-api-x64/bin/Debug/LdkWin32ApiTest.exe NlsTest
```

CI runs this baseline for x86 and x64 Debug builds. A few cases remain
environment- or LDK-specific: symbolic-link creation is skipped when the host
lacks the required privilege, mapped-module `FormatMessage` fallback is only
checked by the kernel driver, and selected wait-on-address timeout stress paths
stay in the LDK driver suite because they are scheduling-sensitive on native
Windows.

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
- Loader helpers, including name and ordinal lookup, import-by-ordinal,
  balanced load/unload reference-count behavior, and basic imported-dependency
  unload behavior. The loader tests also cover selected `LoadLibraryExW`
  search flags, `DONT_RESOLVE_DLL_REFERENCES`, and datafile/resource-style
  handles, PE TLS callbacks, DllMain thread notifications, and
  `DisableThreadLibraryCalls`.
- Fibers and FLS callbacks.
- File helpers, including selected `GetFileInformationByHandleEx`,
  `SetFileInformationByHandle`, temporary-file, link, and final-path paths.
- Heap helpers, including `HeapQueryInformation`, `HeapWalk`, and
  `HeapCompact`.
- `FormatMessage` system-message and module/system fallback paths.
- NLS locale-name/LCID helpers, locale enumeration, date/time formatting, and
  script classification for representative Korean, Chinese, Japanese, and
  Russian data.
- ICU virtual module export, enumeration, and fixed-offset time-zone paths.
- Kernel32 current-directory and environment helpers.
- Legacy and newer threadpool helpers.
- Condition variables and critical sections.
- Wait-on-address synchronization.
- Library loading helpers.
- Console helpers.

Coverage gaps that should be closed before expanding those areas:

- Registry helpers currently have no dedicated runtime test.
- Pipe helpers do not have a broad stress test.
- NLS code-page conversion matrices and uncommon formatting tokens need broader
  dedicated coverage as those paths expand.
- Native Win32 baseline coverage intentionally excludes the LDK `ICU.DLL`
  pseudo-module because the real Windows ICU DLL has a different export surface.

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
