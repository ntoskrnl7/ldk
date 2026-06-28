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

`SetDllDirectoryA/W` stores one process-wide user DLL directory in the LDK PEB.
Passing `NULL` clears that directory and restores the default LDK loader search
state.

## DLL search path

The loader uses process-parameter path state when loading DLLs. By default,
`LoadLibraryExW` computes a DLL path from the current image directory followed
by `PATH` before calling `LdrLoadDll`. This keeps private DLLs beside the driver
ahead of same-named DLLs in system directories.

Selected `LoadLibraryExW` search flags override or extend that default. The
supported subset covers the system directory, the current image directory, the
directory of a fully qualified DLL name, the default-directory combination, and
`LOAD_WITH_ALTERED_SEARCH_PATH`. `LOAD_LIBRARY_SEARCH_USER_DIRS` uses the
directory set by `SetDllDirectoryA/W`; LDK does not currently maintain the full
Windows `AddDllDirectory` list.

`LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR` requires a fully qualified DLL path, matching
the Win32 contract. Search flags are intentionally treated as an explicit mode:
unsupported or contradictory flag combinations fail instead of falling back to
the default search path.

`LdkLoadDll` ultimately uses `RtlDosSearchPath_U` and DOS-to-NT path conversion
before opening the target file.

## Caveats

This state is private to LDK. It is not the host process environment, and it is
not synchronized with user-mode process parameters. Code that expects shell
environment variables, per-user profile expansion, drive-current-directory
slots, or process-wide user-mode loader state needs additional compatibility
work.
