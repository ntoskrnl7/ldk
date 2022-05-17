#pragma once

EXTERN_C_START

WINBASEAPI
BOOL
WINAPI
SetConsoleCP(
    _In_ UINT wCodePageID
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleOutputCP(
    _In_ UINT wCodePageID
    );

EXTERN_C_END