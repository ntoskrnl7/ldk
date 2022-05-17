#pragma once

#include <ntifs.h>
#include <wdm.h>
#include <minwindef.h>

#include "winnt.h"

#include "ldk.h"

#define PEB	                    LDK_PEB
#define PPEB	                PLDK_PEB
#define TEB	                    LDK_TEB
#define PTEB	                PLDK_TEB
#define USER_SHARED_DATA        SharedUserData

#define NtQueryAttributesFile   ZwQueryAttributesFile

#include "ntdll/ntldr.h"
#include "ntdll/nturtl.h"
#include "ntdll/ntexapi.h"
#include "ntdll/ntrtl.h"
#include "ntdll/ntpsapi.h"
#include "ntdll/ntioapi.h"
#include "ntdll/zwapi.h"

EXTERN_C_START

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
VOID
NTAPI
RtlRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord
    );
#else
NTSYSAPI
VOID
NTAPI
RtlRaiseException(
    _In_ PEXCEPTION_RECORD ExceptionRecord
    );
#endif

#define RtlRaiseStatus          ExRaiseStatus

#define NtCreateEvent           ZwCreateEvent
#define NtSetEvent              ZwSetEvent

#define NtWaitForSingleObject       ZwWaitForSingleObject
#define NtWaitForMultipleObjects    ZwWaitForMultipleObjects

EXTERN_C_END