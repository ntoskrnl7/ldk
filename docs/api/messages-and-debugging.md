# Messages, errors, and debugging

LDK implements a compact set of message-formatting, last-error, exception, and
debugging helpers for Kernel32 compatibility.

## Last error and error mode

`GetLastError` and `SetLastError` read and write the current LDK TEB. They do
not interact with any user-mode TEB.

`GetErrorMode` and `SetErrorMode` map to the process hard-error mode through
`ZwQueryInformationProcess` and `ZwSetInformationProcess`.

## FormatMessage

`FormatMessageA/W` supports these source paths:

- `FORMAT_MESSAGE_FROM_STRING`
- `FORMAT_MESSAGE_FROM_SYSTEM`
- `FORMAT_MESSAGE_FROM_HMODULE`

System messages are looked up from cached raw `kernel32.dll` and
`KernelBase.dll` message resources when available, then from a small built-in
fallback table for common error codes.

Module messages use the loaded image resource directory and message-table
resources. Handles with low resource-handle bits are normalized before lookup.
If `FORMAT_MESSAGE_FROM_HMODULE` and `FORMAT_MESSAGE_FROM_SYSTEM` are both set,
LDK tries the module first and then falls back to system messages.

Insert expansion and the full Windows formatting language are not complete.
Current behavior is aimed at simple system and module messages used by tests and
runtime diagnostics.

## Exceptions

`RaiseException` builds an `EXCEPTION_RECORD` and calls `RtlRaiseException` when
the build configuration uses the LDK definition.

`UnhandledExceptionFilter` has debugger-facing behavior by design. It triggers
the diagnostic breakpoint hook, gives a registered top-level filter a chance to
handle the exception, honors selected error-mode behavior, and can raise a hard
error for unhandled exceptions.

The filter also has a special read-only resource recovery path for image
resource writes, matching a compatibility need in loaded DLL code.

## Debug helpers

`IsDebuggerPresent`, `DebugBreak`, and `OutputDebugStringA/W` are implemented as
kernel/debugger-facing helpers. `OutputDebugString*` writes to the debugger
rather than any user-mode debug port.

## Tests

`FormatMessageTest` covers stack and allocated buffers for
`ERROR_ACCESS_DENIED`, wide and ANSI output, and the
`FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM` fallback path.
