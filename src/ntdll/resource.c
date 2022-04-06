#include "ntdll.h"

NTSTATUS
NTAPI
RtlInitializeCriticalSectionEx(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount,
    _In_ ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);

    return RtlInitializeCriticalSectionAndSpinCount(CriticalSection, SpinCount);
}

#define RtlAllocateHeap(a, b, size)        ExAllocatePoolWithTag(PagedPool, size, TAG_HEAP_POOL);
#define RtlFreeHeap(a, b, c)               ExFreePoolWithTag(a, TAG_HEAP_POOL);
#define DPRINT DbgPrint
#define DPRINT1 DbgPrint
#define ERROR_DBGBREAK KdBreakPoint();
#define UNIMPLEMENTED KdBreakPoint();

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
RtlpDeleteDeferedCriticalSection(VOID)
{
    RtlpCritSectInitialized = FALSE;
    RtlDeleteCriticalSection(&RtlCriticalSectionLock);
}
