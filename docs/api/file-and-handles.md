# File and handle APIs

LDK implements a bounded subset of Kernel32 file and handle helpers on top of
native kernel file APIs. The goal is compatibility for audited kernel-mode
callers, not full user-mode file I/O behavior.

## Basic I/O

`CreateFileA/W` converts DOS-style paths to native paths and calls
`ZwCreateFile`. The implementation supports common creation dispositions,
sharing flags, file attributes, delete-on-close, directory opens, and reparse
point opens.

`ReadFile` and `WriteFile` use `ZwReadFile` and `ZwWriteFile`. Synchronous I/O
is the primary tested path. The functions also have partial overlapped support
for explicit byte offsets, but broad user-mode completion-port behavior is not
the compatibility target.

`FlushFileBuffers`, `GetFileSize`, `GetFileSizeEx`, `SetFilePointer`,
`SetFilePointerEx`, `SetEndOfFile`, `GetFileType`, `SetFileTime`, and basic
attribute helpers are backed by native file information queries and setters.

## File information classes

`GetFileInformationByHandleEx` currently supports:

- `FileBasicInfo`
- `FileStandardInfo`
- `FileNameInfo`
- `FileNormalizedNameInfo`
- `FileStreamInfo`
- `FileCompressionInfo`
- `FileAttributeTagInfo`
- `FileStorageInfo`
- `FileAlignmentInfo`
- `FileIdInfo`

`FileStorageInfo` is backed by the native
`FileFsSectorSizeInformation` volume query and reports the logical sector,
physical sector, effective physical sector, alignment offsets, and storage
alignment flags exposed by the underlying file system stack.

`SetFileInformationByHandle` currently supports:

- `FileBasicInfo`
- `FileRenameInfo`
- `FileRenameInfoEx`
- `FileAllocationInfo`
- `FileEndOfFileInfo`
- `FileDispositionInfo`
- `FileDispositionInfoEx`

The fallback behavior intentionally distinguishes invalid input from missing
LDK coverage:

- A class that is valid for the Win32 API but not implemented by LDK returns
  `STATUS_NOT_IMPLEMENTED` through `LdkSetLastNTError` and triggers the
  diagnostic breakpoint hook.
- A class that is not valid for that API returns `ERROR_INVALID_PARAMETER`.

This keeps unsupported-but-real Windows API surface visible during debugging
without treating every unknown value as a caller bug.

## Names, search, and links

`GetFinalPathNameByHandleW` can return NT-style names and DOS-style `\\?\`
names where the kernel object can be resolved.

`FindFirstFileW`, `FindFirstFileExW`, `FindNextFileW`, and `FindClose` cover the
normal wildcard enumeration path. `FindFirstFileExW` accepts the documented
`FIND_FIRST_EX_CASE_SENSITIVE`, `FIND_FIRST_EX_LARGE_FETCH`, and
`FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY` flags and rejects unknown flag bits with
`ERROR_INVALID_PARAMETER`. The flags are routed through the native directory
query path; LDK does not add a stricter case policy than the underlying file
system namespace provides.

`CreateHardLinkW` and `CreateSymbolicLinkW` are implemented for the tested file
scenarios. Symbolic-link tests verify the reparse tag path and target reads.

## Temporary paths and files

`GetTempPathA/W` selects a directory from `TMP`, `TEMP`, `USERPROFILE`,
`SystemRoot`, `windir`, or the current directory, and verifies that the selected
directory exists.

`GetTempFileNameA/W` follows the Win32 split between generated and explicit
unique values:

- When `uUnique` is zero, LDK generates a unique suffix, creates an empty file
  with `CREATE_NEW`, and returns the generated value.
- When `uUnique` is nonzero, LDK returns the formatted file name without
  creating the file.

Only the first three prefix characters are used, and names use a four-digit
hexadecimal unique component followed by `.TMP`.

## Device I/O

`DeviceIoControl` supports synchronous `ZwFsControlFile` and
`ZwDeviceIoControlFile` calls. Overlapped `DeviceIoControl` is intentionally
not implemented yet because correct behavior requires asynchronous IRP
completion semantics rather than a synchronous approximation.

## Tests

`FileTest` covers the common file path, copy, move, hard-link, symbolic-link,
final-path, temp-path, temp-file, `FileNameInfo`, `FindFirstFileExW` flags,
selected `GetFileInformationByHandleEx` query classes, `FileAllocationInfo`,
rename, and disposition paths. Runtime validation still requires loading
`LdkTest.sys` in a driver test VM.
