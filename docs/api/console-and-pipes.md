# Console and pipe APIs

LDK provides lightweight console shims and anonymous-pipe helpers for code that
expects basic standard-handle behavior in kernel mode.

## Console state

During PEB initialization, LDK creates standard input, output, and error handles
and stores them in `RTL_USER_PROCESS_PARAMETERS`. `GetStdHandle` and console
APIs resolve `STD_INPUT_HANDLE`, `STD_OUTPUT_HANDLE`, and `STD_ERROR_HANDLE`
through that process-parameter state.

The current console surface includes:

- `GetConsoleCP`
- `GetConsoleOutputCP`
- `SetConsoleCP`
- `SetConsoleOutputCP`
- `GetConsoleMode`
- `SetConsoleMode`
- `ReadConsoleA/W`
- `WriteConsoleA/W`
- `SetConsoleCtrlHandler`

Console input reads from the standard-input console device with `ZwReadFile`.
Console output writes through the configured debug-print path. This is enough
for diagnostics and compatibility with code that expects standard handles, but
it is not an interactive console subsystem.

## Console limitations

LDK does not implement console windows, screen buffers, input records, process
control events, or full console mode semantics. `SetConsoleCtrlHandler` is a
compatibility shim rather than a full control-handler dispatch system.

## Pipes

`CreatePipe` creates a byte-stream named pipe under `\Device\NamedPipe` using a
unique `Win32Pipes.<process>.<serial>` name. `PeekNamedPipe` uses
`FSCTL_PIPE_PEEK` and reports the available data and message length fields
returned by the kernel.

The pipe helpers are intended for local anonymous-pipe scenarios. They are not a
complete named-pipe API surface.

## Tests

`ConsoleTest` covers the standard console mode and code-page paths. File and
loader tests exercise standard handles indirectly. There is currently no broad
pipe stress test.
