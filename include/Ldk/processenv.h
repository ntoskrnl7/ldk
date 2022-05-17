#pragma once

EXTERN_C_START

WINBASEAPI
HANDLE
WINAPI
GetStdHandle(
    _In_ DWORD nStdHandle
    );


WINBASEAPI
BOOL
WINAPI
SetStdHandle(
    _In_ DWORD nStdHandle,
    _In_ HANDLE hHandle
    );

WINBASEAPI
BOOL
WINAPI
SetStdHandleEx(
    _In_ DWORD nStdHandle,
    _In_ HANDLE hHandle,
    _Out_opt_ PHANDLE phPrevValue
    );



WINBASEAPI
LPSTR
WINAPI
GetCommandLineA(
    VOID
    );

WINBASEAPI
LPWSTR
WINAPI
GetCommandLineW(
    VOID
    );



WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetCurrentDirectoryA(
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPSTR lpBuffer
    );

WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetCurrentDirectoryW(
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPWSTR lpBuffer
    );

WINBASEAPI
BOOL
WINAPI
SetCurrentDirectoryA(
    _In_ LPCSTR lpPathName
    );

WINBASEAPI
BOOL
WINAPI
SetCurrentDirectoryW(
    _In_ LPCWSTR lpPathName
    );


WINBASEAPI
_NullNull_terminated_
LPCH
WINAPI
GetEnvironmentStrings(
    VOID
    );

WINBASEAPI
_NullNull_terminated_
LPWCH
WINAPI
GetEnvironmentStringsW(
    VOID
    );

WINBASEAPI
BOOL
WINAPI
FreeEnvironmentStringsA(
    _In_ _Pre_ _NullNull_terminated_ LPCH penv
    );

WINBASEAPI
BOOL
WINAPI
FreeEnvironmentStringsW(
    _In_ _Pre_ _NullNull_terminated_ LPWCH penv
    );

WINBASEAPI
_Success_(return != 0 && return < nSize)
DWORD
WINAPI
GetEnvironmentVariableA(
    _In_opt_ LPCSTR lpName,
    _Out_writes_to_opt_(nSize, return + 1) LPSTR lpBuffer,
    _In_ DWORD nSize
    );

WINBASEAPI
_Success_(return != 0 && return < nSize)
DWORD
WINAPI
GetEnvironmentVariableW(
    _In_opt_ LPCWSTR lpName,
    _Out_writes_to_opt_(nSize, return + 1) LPWSTR lpBuffer,
    _In_ DWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
SetEnvironmentVariableA(
    _In_ LPCSTR lpName,
    _In_opt_ LPCSTR lpValue
    );

WINBASEAPI
BOOL
WINAPI
SetEnvironmentVariableW(
    _In_ LPCWSTR lpName,
    _In_opt_ LPCWSTR lpValue
    );

EXTERN_C_END