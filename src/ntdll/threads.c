#include "ntdll.h"



NTSTATUS
LdkpInitializeRtlWorkItem (
    VOID
    );

VOID
LdkpTerminateRtlWorkItem (
    VOID
    );

#define TAG_WORK_ITEM       'kWdL'

typedef struct _LDK_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    WORKERCALLBACKFUNC Function;
    PVOID Context;
} LDK_WORK_ITEM, *PLDK_WORK_ITEM;

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(WORKER_THREAD_ROUTINE)
VOID
LdkpWokerThreadRoutine (
    _In_ PLDK_WORK_ITEM WorkItem
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeRtlWorkItem)
#pragma alloc_text(PAGE, LdkpTerminateRtlWorkItem)
#pragma alloc_text(PAGE, LdkpWokerThreadRoutine)
#endif



NPAGED_LOOKASIDE_LIST LdkpRtlWorkItemLookaside;

NTSTATUS
LdkpInitializeRtlWorkItem (
    VOID
    )
{
    PAGED_CODE();

	ExInitializeNPagedLookasideList( &LdkpRtlWorkItemLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_WORK_ITEM),
									 TAG_WORK_ITEM,
									 0 );
	return STATUS_SUCCESS;
}

VOID
LdkpTerminateRtlWorkItem (
    VOID
    )
{
    PAGED_CODE();

    ExDeleteNPagedLookasideList( &LdkpRtlWorkItemLookaside );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(WORKER_THREAD_ROUTINE)
VOID
LdkpWokerThreadRoutine (
    _In_ PLDK_WORK_ITEM WorkItem
    )
{
    PAGED_CODE();

    WorkItem->Function( WorkItem->Context );

    ExFreeToNPagedLookasideList( &LdkpRtlWorkItemLookaside,
                                 WorkItem );
}

NTSTATUS
NTAPI
RtlQueueWorkItem (
    _In_ WORKERCALLBACKFUNC Function,
    _In_ PVOID Context,
    _In_ ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(Flags);

    PLDK_WORK_ITEM item = ExAllocateFromNPagedLookasideList( &LdkpRtlWorkItemLookaside );
    if (!item) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    item->Function = Function;
    item->Context = Context;

#pragma warning(disable: 4996)
    ExInitializeWorkItem( &item->WorkItem,
                          LdkpWokerThreadRoutine,
                          item );

    ExQueueWorkItem( &item->WorkItem,
                     DelayedWorkQueue );
#pragma warning(default: 4996)
    return STATUS_SUCCESS;
}