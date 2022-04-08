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

WINBASEAPI
__analysis_noreturn
VOID
WINAPI
RaiseException(
    _In_ DWORD dwExceptionCode,
    _In_ DWORD dwExceptionFlags,
    _In_ DWORD nNumberOfArguments,
    _In_reads_opt_(nNumberOfArguments) CONST ULONG_PTR* lpArguments
    );

EXTERN_C_END
