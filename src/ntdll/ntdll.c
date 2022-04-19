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
RtlpInitializeKeyedEvent(
    VOID
    );

VOID
RtlpCloseKeyedEvent(
    VOID
    );

VOID
NTAPI
RtlpInitDeferedCriticalSection(
    VOID
    );

extern LARGE_INTEGER RtlpTimeout;

VOID
NTAPI
RtlpDeleteDeferedCriticalSection(
    VOID
    );



LDK_INITIALIZE_COMPONENT LdkpInitializeKeyedEventList;
LDK_TERMINATE_COMPONENT LdkpTerminateKeyedEventList;

LDK_INITIALIZE_COMPONENT LdkpInitializeRtlWorkItem;
LDK_TERMINATE_COMPONENT LdkpTerminateRtlWorkItem;

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
LDK_INITIALIZE_COMPONENT LdkpInitializeDispatchExceptionStackVariables;
LDK_TERMINATE_COMPONENT LdkpTerminateDispatchExceptionStackVariables;
#endif



LDK_INITIALIZE_COMPONENT LdkpNtdllInitialize;
LDK_TERMINATE_COMPONENT LdkpNtdllTerminate;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpNtdllInitialize)
#pragma alloc_text(PAGE, LdkpNtdllTerminate)
#endif



NTSTATUS
LdkpNtdllInitialize (
    VOID
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = LdkpInitializeKeyedEventList();
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = LdkpInitializeRtlWorkItem();
    if (!NT_SUCCESS(status)) {
        LdkpTerminateKeyedEventList();
        return status;
    }

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
	status = LdkpInitializeDispatchExceptionStackVariables();
	if (!NT_SUCCESS(status)) {
		LdkpTerminateRtlWorkItem();
		LdkpTerminateKeyedEventList();
		return status;
	}
#endif

    RtlpInitDeferedCriticalSection();
    RtlpTimeout = NtCurrentPeb()->CriticalSectionTimeout;

    RtlpInitializeKeyedEvent();
    return STATUS_SUCCESS;
}

VOID
LdkpNtdllTerminate(
    VOID
    )
{
    RtlpCloseKeyedEvent();

    RtlpDeleteDeferedCriticalSection();

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
	LdkpTerminateDispatchExceptionStackVariables();
#endif

    LdkpTerminateRtlWorkItem();

	LdkpTerminateKeyedEventList();
}