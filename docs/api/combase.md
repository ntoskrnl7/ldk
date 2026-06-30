# COMBASE Virtual Module

LDK registers a small `COMBASE.DLL` pseudo-module for CRT and WinRT helper code
that imports basic COM task-memory and runtime-initialization helpers. This is
not a COM runtime.

## Supported exports

- `CoTaskMemAlloc`
- `CoTaskMemFree`
- `CoCreateFreeThreadedMarshaler`
- `CoCreateInstance`
- `CoGetApartmentType`
- `CoGetContextToken`
- `CoGetInterfaceAndReleaseStream`
- `CoGetObjectContext`
- `CoIncrementMTAUsage`
- `CoDecrementMTAUsage`
- `CoMarshalInterThreadInterfaceInStream`
- `RoInitialize`
- `RoUninitialize`
- `RoGetApartmentIdentifier`
- `RoRegisterForApartmentShutdown`
- `RoUnregisterForApartmentShutdown`
- `SetRestrictedErrorInfo`
- `GetRestrictedErrorInfo`
- `RoCaptureErrorContext`
- `RoFailFastWithErrorContext`
- `RoOriginateError`
- `RoOriginateErrorW`
- `RoOriginateLanguageException`
- `RoReportUnhandledError`
- `RoTransformError`
- `RoTransformErrorW`
- `WindowsCreateString`
- `WindowsCreateStringReference`
- `WindowsDeleteString`
- `WindowsStringHasEmbeddedNull`

The task-memory functions use the LDK process heap. `CoTaskMemFree(NULL)` is a
no-op, as on Windows.

The initialization helpers keep a small per-LDK-thread apartment state in the
LDK TEB. `RoInitialize` supports `RO_INIT_SINGLETHREADED` and
`RO_INIT_MULTITHREADED`, returns `S_FALSE` for balanced repeated initialization
with the same model, and returns `RPC_E_CHANGED_MODE` for conflicting models.
`CoIncrementMTAUsage` returns an opaque compatibility cookie and exposes the
current thread as an implicit MTA until the matching `CoDecrementMTAUsage`.
`RoGetApartmentIdentifier` returns a stable nonzero identifier for the current
LDK thread. Apartment shutdown registration returns an opaque cookie that can be
unregistered later, but does not invoke `IApartmentShutdown` callbacks.
`CoGetContextToken` returns the same stable per-thread apartment identity as a
context token.

The activation and marshaling helpers are intentionally limited. `CoCreateInstance`
does not activate COM classes, `CoCreateFreeThreadedMarshaler` does not create a
marshaler object, `CoGetObjectContext` does not expose a real COM context, and
the stream marshaling helpers do not create or consume COM streams. These exports
exist so CRT/STL and vccorlib import tables resolve cleanly and callers receive
explicit failure HRESULTs with cleared output pointers.

The HSTRING helpers support owned strings, header-backed string references,
delete/no-op cleanup, and embedded-null detection. `NULL` HSTRING handles are
treated as empty strings.

The restricted-error helpers are intentionally minimal. They do not create or
retain an `IRestrictedErrorInfo` object. `GetRestrictedErrorInfo` reports
`S_FALSE` with a NULL output when no error object exists, `SetRestrictedErrorInfo`
accepts NULL, originate/transform helpers return the same basic TRUE/FALSE
shape as Windows for success/failure HRESULTs, and `RoFailFastWithErrorContext`
raises a noncontinuable fail-fast exception if it is actually called.

## Loader visibility

The module is registered as `COMBASE.DLL` with the path
`\SystemRoot\system32\COMBASE.DLL`. `LoadLibraryW(L"combase.dll")` and
`GetProcAddress` can resolve the supported exports through the normal LDK loader
path without loading a real user-mode COM runtime.

## Limits

COM activation, object contexts, marshaling, real restricted-error objects,
apartment shutdown callback invocation, and advanced WinRT string APIs are not
implemented by this module. The apartment, HSTRING, error-context, and
failure-only COM helpers are a minimal CRT/WinRT-wrapper compatibility surface,
not a full COM apartment runtime.

## Tests

`CombaseCompatibilityTest` validates loader visibility, task-memory allocation,
write/read access, freeing, NULL free behavior, MTA usage cookies, apartment
querying, `RoInitialize`/`RoUninitialize` balancing, and HSTRING owned/reference
string behavior, and the safe restricted-error context paths. The Win32 baseline
runs the same loader-based export test against the real Windows `combase.dll`.
