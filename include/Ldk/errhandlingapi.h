#pragma once

EXTERN_C_START

WINBASEAPI
_Check_return_ _Post_equals_last_error_
DWORD
WINAPI
GetLastError(
    VOID
    );

WINBASEAPI
VOID
WINAPI
SetLastError(
    _In_ DWORD dwErrCode
    );

EXTERN_C_END