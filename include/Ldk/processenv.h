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

EXTERN_C_END