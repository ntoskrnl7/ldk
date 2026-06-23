# Registry APIs

LDK contains a small Unicode registry helper surface in `src/kernel32/winreg.c`.
It is useful for read-oriented kernel-mode compatibility paths, but it is not a
full Advapi32 registry layer.

## Implemented functions

The current implementation includes:

- `RegOpenKeyExW`
- `RegQueryValueExW`
- `RegCloseKey`

Only the wide-character variants are present.

## Root handling

`RegOpenKeyExW` maps selected predefined keys to native registry roots:

- `HKEY_CLASSES_ROOT` to `\Registry\Machine\Software\Classes`
- `HKEY_LOCAL_MACHINE` to `\Registry\Machine`
- `HKEY_USERS` to `\Registry\User`
- `HKEY_CURRENT_CONFIG` to
  `\Registry\Machine\System\CurrentControlSet\Hardware Profiles\Current`

`HKEY_CURRENT_USER` and `HKEY_PERFORMANCE_DATA` are recognized as predefined
handles but do not currently have a root mapping, so open attempts fail with
`ERROR_INVALID_HANDLE`.

For non-predefined handles, `RegOpenKeyExW` opens `lpSubKey` relative to the
incoming key handle with `OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE`.

## Query behavior

`RegQueryValueExW` queries `KeyValuePartialInformation` with `ZwQueryValueKey`.
It returns the value type, required byte count, `ERROR_MORE_DATA` for too-small
buffers, and copies the raw value bytes when a destination buffer is supplied.

The function rejects predefined root handles directly; callers must first open a
real key handle.

## Export visibility

The registry functions are declared in `include/Ldk/winreg.h` and compiled in
the Kernel32 source tree. They are not currently registered in the Kernel32
pseudo-module export table, so DLL import resolution through `kernel32.dll`
does not expose them yet.

Callers that link directly against LDK headers can use the symbols when linked
with the library, but loader-visible compatibility should be treated as pending
until the export table is updated and tested.

## Tests

There is no dedicated registry test yet. Add one before broadening registry
coverage or making these functions loader-visible.
