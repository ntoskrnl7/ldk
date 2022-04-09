#pragma once

EXTERN_C_START

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

WINBASEAPI
BOOL
WINAPI
CloseHandle(
    _In_ HANDLE hObject
    );

WINBASEAPI
BOOL
WINAPI
DuplicateHandle(
    _In_ HANDLE hSourceProcessHandle,
    _In_ HANDLE hSourceHandle,
    _In_ HANDLE hTargetProcessHandle,
    _Outptr_ LPHANDLE lpTargetHandle,
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwOptions
    );

EXTERN_C_END