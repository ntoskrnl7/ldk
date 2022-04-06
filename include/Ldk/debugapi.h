#pragma once

EXTERN_C_START

WINBASEAPI
BOOL
WINAPI
IsDebuggerPresent(
    VOID
    );

WINBASEAPI
VOID
WINAPI
OutputDebugStringA(
    _In_opt_ LPCSTR lpOutputString
    );

WINBASEAPI
VOID
WINAPI
OutputDebugStringW(
    _In_opt_ LPCWSTR lpOutputString
    );

EXTERN_C_END