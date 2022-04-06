#pragma once

#include <ntifs.h>
#include <wdm.h>
#include <minwindef.h>

#include "winnt.h"

#include "ntdll/ntldr.h"
#include "ntdll/nturtl.h"
#include "ntdll/ntexapi.h"

EXTERN_C_START

NTSYSAPI
VOID
NTAPI
RtlRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord
    );

#define RtlRaiseStatus          ExRaiseStatus



#define NtCreateEvent           ZwCreateEvent
#define NtSetEvent              ZwSetEvent



_When_(Timeout == NULL, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart != 0, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects (
    _In_ ULONG Count,
    _In_reads_(Count) HANDLE Handles[],
    _In_ _Strict_type_match_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

#define NtWaitForSingleObject       ZwWaitForSingleObject
#define NtWaitForMultipleObjects    ZwWaitForMultipleObjects

EXTERN_C_END