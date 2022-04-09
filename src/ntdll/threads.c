#include "ntdll.h"

#define TAG_WORK_ITEM       'kWdL'

typedef struct _LDK_WORK_ITEM {
    WORK_QUEUE_ITEM WorkItem;
    WORKERCALLBACKFUNC Function;
    PVOID Context;
} LDK_WORK_ITEM, *PLDK_WORK_ITEM;

NPAGED_LOOKASIDE_LIST LdkpRtlWorkItemLookaside;

NTSTATUS
LdkpInitializeRtlWorkItem (
    VOID
    )
{
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