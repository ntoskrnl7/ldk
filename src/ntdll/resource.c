#include "ntdll.h"
#include "../ldk.h"



VOID
NTAPI
RtlpDeleteDeferedCriticalSection (
    VOID
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlInitializeCriticalSectionEx)
#pragma alloc_text(PAGE, RtlpDeleteDeferedCriticalSection)
#pragma alloc_text(PAGE, RtlInitializeCriticalSection)
#pragma alloc_text(PAGE, RtlInitializeCriticalSection)
#pragma alloc_text(PAGE, RtlInitializeCriticalSectionAndSpinCount)
#pragma alloc_text(PAGE, RtlEnterCriticalSection)
#pragma alloc_text(PAGE, RtlDeleteCriticalSection)
#pragma alloc_text(PAGE, RtlTryEnterCriticalSection)
#pragma alloc_text(PAGE, RtlLeaveCriticalSection)
#pragma alloc_text(PAGE, RtlGetCriticalSectionRecursionCount)
#pragma alloc_text(PAGE, RtlInitializeConditionVariable)
#pragma alloc_text(PAGE, RtlWakeConditionVariable)
#pragma alloc_text(PAGE, RtlWakeAllConditionVariable)
#pragma alloc_text(PAGE, RtlSleepConditionVariableCS)
#pragma alloc_text(PAGE, RtlSleepConditionVariableSRW)
#endif



NTSTATUS
NTAPI
RtlInitializeCriticalSectionEx (
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount,
    _In_ ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);

    PAGED_CODE();

    return RtlInitializeCriticalSectionAndSpinCount( CriticalSection,
                                                     SpinCount );
}

#define DPRINT              DbgPrint
#define DPRINT1             DbgPrint
#define ERROR_DBGBREAK      KdBreakPoint();
#define UNIMPLEMENTED       KdBreakPoint();
#define RtlGetProcessHeap   RtlProcessHeap
#define NtClose             ZwClose

#define RTL_CRITSECT_TYPE                                   0

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

#pragma warning(disable:4152 4100)

/*
* ReactOS :-)
* 
* https://github.com/reactos/reactos/blob/3fa57b8ff7fcee47b8e2ed869aecaf4515603f3f/sdk/lib/rtl/critical.c
*/
#include "reactos/critical.c"



VOID
NTAPI
RtlpDeleteDeferedCriticalSection (
    VOID
    )
{
    PAGED_CODE();

    RtlpCritSectInitialized = FALSE;
    RtlDeleteCriticalSection(&RtlCriticalSectionLock);
}
