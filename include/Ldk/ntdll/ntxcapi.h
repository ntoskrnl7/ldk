#pragma once

#ifndef _LDK_NTXCAPI_
#define _LDK_NTXCAPI_

EXTERN_C_START

NTSYSAPI
VOID
NTAPI
RtlUnwind (
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue
    );

EXTERN_C_END

#endif // _LDK_NTXCAPI_