#pragma once

#ifndef _ERRHANDLING_H_
#define _ERRHANDLING_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"

EXTERN_C_START

//
// Typedefs
//

typedef LONG (WINAPI *PTOP_LEVEL_EXCEPTION_FILTER)(
    _In_ struct _EXCEPTION_POINTERS *ExceptionInfo
    );

typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;

//
// Prototypes
//

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

__callback
WINBASEAPI
LONG
WINAPI
UnhandledExceptionFilter(
    _In_ struct _EXCEPTION_POINTERS* ExceptionInfo
    );

WINBASEAPI
LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter(
    _In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    );

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
UINT
WINAPI
GetErrorMode(
    VOID
    );

WINBASEAPI
UINT
WINAPI
SetErrorMode(
    _In_ UINT uMode
    );

EXTERN_C_END

#endif // _ERRHANDLING_H_
