#include "ntdll.h"

#include "../ldk.h"

#pragma warning(disable:4152)
LDK_FUNCTION_REGISTRATION LdkpNtdllFunctionRegistration[] = {
	{
		"RtlInitializeConditionVariable",
		RtlInitializeConditionVariable
	},
    {
        "RtlSleepConditionVariableCS",
        RtlSleepConditionVariableCS
    },
    {
        "RtlSleepConditionVariableSRW",
        RtlSleepConditionVariableSRW
    },
    {
        "RtlWakeConditionVariable",
        RtlWakeConditionVariable
    },
    {
        "RtlWakeAllConditionVariable",
        RtlWakeAllConditionVariable
    },
    {
        "RtlInitializeCriticalSection",
        RtlInitializeCriticalSection
    },
    {
        "RtlInitializeCriticalSectionAndSpinCount",
        RtlInitializeCriticalSectionAndSpinCount
    },
    {
        "RtlDeleteCriticalSection",
        RtlDeleteCriticalSection
    },
    {
        "RtlEnterCriticalSection",
        RtlEnterCriticalSection
    },
    {
        "RtlLeaveCriticalSection",
        RtlLeaveCriticalSection
    },
	{
		NULL
	}
};
#pragma warning(default:4152)

LDK_MODULE LdkpNtdllModule = {
	RTL_CONSTANT_STRING("NTDLL.DLL"),
	RTL_CONSTANT_STRING("\\SystemRoot\\system32\\NTDLL.DLL"),
    LdkpNtdllFunctionRegistration,
	NULL
};



VOID
RtlpInitializeKeyedEvent(VOID);

VOID
RtlpCloseKeyedEvent(VOID);

VOID
NTAPI
RtlpInitDeferedCriticalSection(VOID);

extern LARGE_INTEGER RtlpTimeout;

VOID
NTAPI
RtlpDeleteDeferedCriticalSection(VOID);



NTSTATUS
LdkpInitializeKeyedEventList(
    VOID
    );

VOID
LdkpTerminateKeyedEventList(
    VOID
    );



NTSTATUS
LdkpInitializeRtlWorkItem (
    VOID
    );

VOID
LdkpTerminateRtlWorkItem (
    VOID
    );



NTSTATUS
NtdllInitialize (
    VOID
    )
{
    NTSTATUS status;

    KdBreakPoint();

    status = LdkpInitializeKeyedEventList();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = LdkpInitializeRtlWorkItem();
    if (!NT_SUCCESS(status)) {
        LdkpTerminateKeyedEventList();
        return status;
    }

    RtlpInitDeferedCriticalSection();
    RtlpTimeout = NtCurrentPeb()->CriticalSectionTimeout;
    RtlpInitializeKeyedEvent();
    return STATUS_SUCCESS;
}

VOID
NtdllTerminate(
    VOID
    )
{
    RtlpCloseKeyedEvent();

    RtlpDeleteDeferedCriticalSection();

    LdkpTerminateKeyedEventList();

    LdkpTerminateRtlWorkItem();
}