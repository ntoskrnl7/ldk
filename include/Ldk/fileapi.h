#pragma once

#ifndef _APISETFILE_
#define _APISETFILE_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "minwinbase.h"

EXTERN_C_START

//
// Constants
//

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)



WINBASEAPI
BOOL
WINAPI
CreateDirectoryA(
    _In_ LPCSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBASEAPI
BOOL
WINAPI
CreateDirectoryW(
    _In_ LPCWSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

#ifdef UNICODE
#define CreateDirectory  CreateDirectoryW
#else
#define CreateDirectory  CreateDirectoryA
#endif // !UNICODE


WINBASEAPI
HANDLE
WINAPI
CreateFileA(
    _In_ LPCSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
    );

WINBASEAPI
HANDLE
WINAPI
CreateFileW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
    );

#ifdef UNICODE
#define CreateFile  CreateFileW
#else
#define CreateFile  CreateFileA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FlushFileBuffers(
    _In_ HANDLE hFile
    );



WINBASEAPI
BOOL
WINAPI
LocalFileTimeToFileTime(
    _In_ CONST FILETIME * lpLocalFileTime,
    _Out_ LPFILETIME lpFileTime
    );



WINBASEAPI
BOOL
WINAPI
LockFile(
    _In_ HANDLE hFile,
    _In_ DWORD dwFileOffsetLow,
    _In_ DWORD dwFileOffsetHigh,
    _In_ DWORD nNumberOfBytesToLockLow,
    _In_ DWORD nNumberOfBytesToLockHigh
    );

WINBASEAPI
BOOL
WINAPI
LockFileEx(
    _In_ HANDLE hFile,
    _In_ DWORD dwFlags,
    _Reserved_ DWORD dwReserved,
    _In_ DWORD nNumberOfBytesToLockLow,
    _In_ DWORD nNumberOfBytesToLockHigh,
    _Inout_ LPOVERLAPPED lpOverlapped
    );
WINBASEAPI
BOOL
WINAPI
UnlockFile(
    _In_ HANDLE hFile,
    _In_ DWORD dwFileOffsetLow,
    _In_ DWORD dwFileOffsetHigh,
    _In_ DWORD nNumberOfBytesToUnlockLow,
    _In_ DWORD nNumberOfBytesToUnlockHigh
    );

WINBASEAPI
BOOL
WINAPI
UnlockFileEx(
    _In_ HANDLE hFile,
    _Reserved_ DWORD dwReserved,
    _In_ DWORD nNumberOfBytesToUnlockLow,
    _In_ DWORD nNumberOfBytesToUnlockHigh,
    _Inout_ LPOVERLAPPED lpOverlapped
    );



WINBASEAPI
BOOL
WINAPI
SetEndOfFile(
    _In_ HANDLE hFile
    );

WINBASEAPI
DWORD
WINAPI
GetFileAttributesA(
    _In_ LPCSTR lpFileName
    );

WINBASEAPI
DWORD
WINAPI
GetFileAttributesW(
    _In_ LPCWSTR lpFileName
    );

#ifdef UNICODE
#define GetFileAttributes  GetFileAttributesW
#else
#define GetFileAttributes  GetFileAttributesA
#endif // !UNICODE


WINBASEAPI
DWORD
WINAPI
GetFileSize(
    _In_ HANDLE hFile,
    _Out_opt_ LPDWORD lpFileSizeHigh
    );

WINBASEAPI
BOOL
WINAPI
GetFileSizeEx(
    _In_ HANDLE hFile,
    _Out_ PLARGE_INTEGER lpFileSize
    );

WINBASEAPI
DWORD
WINAPI
GetFileType(
    _In_ HANDLE hFile
    );



WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
ReadFile(
    _In_ HANDLE hFile,
    _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead,
    _Out_opt_ LPDWORD lpNumberOfBytesRead,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
    );

WINBASEAPI
BOOL
WINAPI
WriteFile(
    _In_ HANDLE hFile,
    _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
    );





WINBASEAPI
DWORD
WINAPI
SetFilePointer(
    _In_ HANDLE hFile,
    _In_ LONG lDistanceToMove,
    _Inout_opt_ PLONG lpDistanceToMoveHigh,
    _In_ DWORD dwMoveMethod
    );

WINBASEAPI
BOOL
WINAPI
SetFilePointerEx(
    _In_ HANDLE hFile,
    _In_ LARGE_INTEGER liDistanceToMove,
    _Out_opt_ PLARGE_INTEGER lpNewFilePointer,
    _In_ DWORD dwMoveMethod
    );

WINBASEAPI
BOOL
WINAPI
SetFileTime(
    _In_ HANDLE hFile,
    _In_opt_ CONST FILETIME * lpCreationTime,
    _In_opt_ CONST FILETIME * lpLastAccessTime,
    _In_opt_ CONST FILETIME * lpLastWriteTime
    );



WINBASEAPI
BOOL
WINAPI
DeleteFileA(
    _In_ LPCSTR lpFileName
    );

WINBASEAPI
BOOL
WINAPI
DeleteFileW(
    _In_ LPCWSTR lpFileName
    );


typedef struct _BY_HANDLE_FILE_INFORMATION {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD dwVolumeSerialNumber;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD nNumberOfLinks;
    DWORD nFileIndexHigh;
    DWORD nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *PBY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

WINBASEAPI
BOOL
WINAPI
GetFileInformationByHandle(
    _In_ HANDLE hFile,
    _Out_ LPBY_HANDLE_FILE_INFORMATION lpFileInformation
    );

WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetFullPathNameA(
    _In_ LPCSTR lpFileName,
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPSTR lpBuffer,
    _Outptr_opt_ LPSTR* lpFilePart
    );

WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetFullPathNameW(
    _In_ LPCWSTR lpFileName,
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPWSTR lpBuffer,
    _Outptr_opt_ LPWSTR* lpFilePart
    );



WINBASEAPI
UINT
WINAPI
GetDriveTypeA(
    _In_opt_ LPCSTR lpRootPathName
    );

WINBASEAPI
UINT
WINAPI
GetDriveTypeW(
    _In_opt_ LPCWSTR lpRootPathName
    );



WINBASEAPI
DWORD
WINAPI
GetTempPathA(
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPSTR lpBuffer
    );

WINBASEAPI
DWORD
WINAPI
GetTempPathW(
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPWSTR lpBuffer
    );

WINBASEAPI
UINT
WINAPI
GetTempFileNameA(
    _In_ LPCSTR lpPathName,
    _In_ LPCSTR lpPrefixString,
    _In_ UINT uUnique,
    _Out_writes_(MAX_PATH) LPSTR lpTempFileName
    );

WINBASEAPI
UINT
WINAPI
GetTempFileNameW(
    _In_ LPCWSTR lpPathName,
    _In_ LPCWSTR lpPrefixString,
    _In_ UINT uUnique,
    _Out_writes_(MAX_PATH) LPWSTR lpTempFileName
    );

WINBASEAPI
BOOL
WINAPI
AreFileApisANSI(
    VOID
    );

WINBASEAPI
VOID
WINAPI
SetFileApisToOEM(
    VOID
    );

WINBASEAPI
VOID
WINAPI
SetFileApisToANSI(
    VOID
    );

EXTERN_C_END

#endif // _APISETFILE_