
#pragma once

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