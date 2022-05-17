#include "ntdll.h"
#include "../ldk.h"



_Function_class_(RTL_ALLOCATE_STRING_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
__drv_allocatesMem(Mem)
PVOID
NTAPI
LdkpNtdllAllocateStringRoutine (
    _In_ SIZE_T NumberOfBytes
    )
{
    return RtlAllocateHeap( RtlProcessHeap(),
                            0,
                            NumberOfBytes );
}

_Function_class_(RTL_FREE_STRING_ROUTINE)
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
NTAPI
LdkpNtdllFreeStringRoutine (
    _In_ __drv_freesMem(Mem) _Post_invalid_ PVOID Buffer
    )
{
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 Buffer );
}

const PRTL_ALLOCATE_STRING_ROUTINE RtlAllocateStringRoutine = LdkpNtdllAllocateStringRoutine;
const PRTL_FREE_STRING_ROUTINE RtlFreeStringRoutine = LdkpNtdllFreeStringRoutine;

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



LDK_TERMINATE_COMPONENT LdkpTerminateCurDir;

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
    NTSTATUS Status;

    PAGED_CODE();

    Status = LdkpInitializeKeyedEventList();
    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    Status = LdkpInitializeRtlWorkItem();
    if (! NT_SUCCESS(Status)) {
        LdkpTerminateKeyedEventList();
        return Status;
    }

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
	Status = LdkpInitializeDispatchExceptionStackVariables();
	if (! NT_SUCCESS(Status)) {
		LdkpTerminateRtlWorkItem();
		LdkpTerminateKeyedEventList();
		return Status;
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
    LdkpTerminateCurDir();

    RtlpCloseKeyedEvent();

    RtlpDeleteDeferedCriticalSection();

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
	LdkpTerminateDispatchExceptionStackVariables();
#endif

    LdkpTerminateRtlWorkItem();

	LdkpTerminateKeyedEventList();
}