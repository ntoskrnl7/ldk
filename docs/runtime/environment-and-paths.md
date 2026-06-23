# Environment and paths

LDK stores environment, current-directory, command-line, and DLL search path
state in the process parameters owned by the LDK PEB.

## Initial state

During PEB initialization, LDK creates `RTL_USER_PROCESS_PARAMETERS` and seeds:

- Standard input, output, and error handles.
- `ImagePathName` from the loaded driver image path.
- `CommandLine` from the image path.
- `CurrentDirectory` from `NtSystemRoot`.
- `DllPath` from the system directory.
- Environment variables including `windir`, `SystemDrive`, `SystemRoot`, and
  `PATH`.

The standard handles are backed by lightweight console device objects. They are
useful for compatibility with code that expects the fields to exist, not for
full console semantics.

## Environment APIs

Kernel32 environment APIs call into the RTL environment helpers and the LDK
process parameters:

- `GetEnvironmentStrings` / `GetEnvironmentStringsW`
- `FreeEnvironmentStringsA/W`
- `GetEnvironmentVariableA/W`
- `SetEnvironmentVariableA/W`

ANSI variants convert through LDK string helpers and the current TEB's static
conversion buffers where appropriate.

When an environment variable is missing, the APIs follow Win32-style behavior:
the call fails, the returned length is zero, and the last error is set from the
underlying status mapping.

## Current directory APIs

`GetCurrentDirectoryA/W` and `SetCurrentDirectoryA/W` use
`RtlGetCurrentDirectory_U` and `RtlSetCurrentDirectory_U`.

`SetCurrentDirectoryA` accepts a quoted path retry path for compatibility with
callers that accidentally include a leading quote. The current directory is
stored in the LDK process parameters and guarded with the PEB lock where needed.

## DLL search path

The loader uses process-parameter path state when loading DLLs. `LoadLibraryExW`
computes a DLL path from `PATH` and the current image directory before calling
`LdrLoadDll`.

`LdkLoadDll` ultimately uses `RtlDosSearchPath_U` and DOS-to-NT path conversion
before opening the target file.

## Caveats

This state is private to LDK. It is not the host process environment, and it is
not synchronized with user-mode process parameters. Code that expects shell
environment variables, per-user profile expansion, drive-current-directory
slots, or process-wide user-mode loader state needs additional compatibility
work.
