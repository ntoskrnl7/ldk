#include "ntdll.h"



#define LDK_ALERT_BY_THREAD_ID_WAKE_BATCH 64
#define TAG_ALERT_BY_THREAD_ID_PENDING 'tAdL'

typedef struct _LDK_ALERT_BY_THREAD_ID_ENTRY {
    LIST_ENTRY Links;
    PETHREAD Thread;
    PKEVENT Event;
    BOOLEAN InList;
    BOOLEAN Woken;
} LDK_ALERT_BY_THREAD_ID_ENTRY, *PLDK_ALERT_BY_THREAD_ID_ENTRY;

typedef struct _LDK_ALERT_BY_THREAD_ID_PENDING_ENTRY {
    LIST_ENTRY Links;
    PETHREAD Thread;
} LDK_ALERT_BY_THREAD_ID_PENDING_ENTRY, *PLDK_ALERT_BY_THREAD_ID_PENDING_ENTRY;

EX_SPIN_LOCK LdkpAlertByThreadIdListLock;
LIST_ENTRY LdkpAlertByThreadIdListHead;
LIST_ENTRY LdkpAlertByThreadIdPendingListHead;
KEVENT LdkpAlertByThreadIdDrainedEvent;
NPAGED_LOOKASIDE_LIST LdkpAlertByThreadIdPendingLookaside;
LONG LdkpAlertByThreadIdWaitCount;
BOOLEAN LdkpAlertByThreadIdShuttingDown;



NTSTATUS
LdkpInitializeAlertByThreadIdList (
    VOID
    );

VOID
LdkpTerminateAlertByThreadIdList(
    VOID
    );

VOID
LdkpReferenceAlertByThreadIdWait (
    VOID
    );

VOID
LdkpDereferenceAlertByThreadIdWait (
    VOID
    );

VOID
LdkpWakeAllAlertByThreadIdWaiters (
    VOID
    );

PKEVENT
LdkpRemoveAlertByThreadIdWaiterLocked (
    _In_ PETHREAD Thread
    );

BOOLEAN
LdkpHasPendingAlertByThreadIdLocked (
    _In_ PETHREAD Thread
    );

BOOLEAN
LdkpRemovePendingAlertByThreadIdLocked (
    _In_ PETHREAD Thread
    );

VOID
LdkpClearPendingAlertByThreadIdList (
    VOID
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeAlertByThreadIdList)
#pragma alloc_text(PAGE, LdkpWakeAllAlertByThreadIdWaiters)
#endif



ULONG
LdkpCollectAlertByThreadIdEvents (
    _Out_writes_(EventCount) PKEVENT* Events,
    _In_ ULONG EventCount
    )
{
    ULONG Count = 0;
    PLIST_ENTRY Current;
    KIRQL OldIrql;

    OldIrql = ExAcquireSpinLockExclusive( &LdkpAlertByThreadIdListLock );
    Current = LdkpAlertByThreadIdListHead.Flink;
    while (Current != &LdkpAlertByThreadIdListHead && Count < EventCount) {
        PLDK_ALERT_BY_THREAD_ID_ENTRY Entry;
        PLIST_ENTRY Next;

        Next = Current->Flink;
        Entry = CONTAINING_RECORD( Current,
                                   LDK_ALERT_BY_THREAD_ID_ENTRY,
                                   Links );
        Entry->InList = FALSE;
        Entry->Woken = TRUE;
        RemoveEntryList( &Entry->Links );
        Events[Count++] = Entry->Event;

        Current = Next;
    }
    ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                OldIrql );

    return Count;
}

VOID
LdkpSetAlertByThreadIdEvents (
    _In_reads_(EventCount) PKEVENT* Events,
    _In_ ULONG EventCount
    )
{
    ULONG Index;

    for (Index = 0; Index < EventCount; Index++) {
        KeSetEvent( Events[Index],
                    IO_NO_INCREMENT,
                    FALSE );
    }
}

VOID
LdkpWakeAllAlertByThreadIdWaiters (
    VOID
    )
{
    PKEVENT Events[LDK_ALERT_BY_THREAD_ID_WAKE_BATCH];
    ULONG Count;

    PAGED_CODE();

    do {
        Count = LdkpCollectAlertByThreadIdEvents( Events,
                                                  RTL_NUMBER_OF(Events) );
        LdkpSetAlertByThreadIdEvents( Events,
                                      Count );
    } while (Count == RTL_NUMBER_OF(Events));
}

PKEVENT
LdkpRemoveAlertByThreadIdWaiterLocked (
    _In_ PETHREAD Thread
    )
{
    PLIST_ENTRY Current;

    for (Current = LdkpAlertByThreadIdListHead.Flink;
         Current != &LdkpAlertByThreadIdListHead;
         Current = Current->Flink) {
        PLDK_ALERT_BY_THREAD_ID_ENTRY Entry;

        Entry = CONTAINING_RECORD( Current,
                                   LDK_ALERT_BY_THREAD_ID_ENTRY,
                                   Links );
        if (Entry->Thread == Thread) {
            Entry->InList = FALSE;
            Entry->Woken = TRUE;
            RemoveEntryList( &Entry->Links );
            return Entry->Event;
        }
    }

    return NULL;
}

BOOLEAN
LdkpHasPendingAlertByThreadIdLocked (
    _In_ PETHREAD Thread
    )
{
    PLIST_ENTRY Current;

    for (Current = LdkpAlertByThreadIdPendingListHead.Flink;
         Current != &LdkpAlertByThreadIdPendingListHead;
         Current = Current->Flink) {
        PLDK_ALERT_BY_THREAD_ID_PENDING_ENTRY Entry;

        Entry = CONTAINING_RECORD( Current,
                                   LDK_ALERT_BY_THREAD_ID_PENDING_ENTRY,
                                   Links );
        if (Entry->Thread == Thread) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
LdkpRemovePendingAlertByThreadIdLocked (
    _In_ PETHREAD Thread
    )
{
    PLIST_ENTRY Current;

    for (Current = LdkpAlertByThreadIdPendingListHead.Flink;
         Current != &LdkpAlertByThreadIdPendingListHead;
         Current = Current->Flink) {
        PLDK_ALERT_BY_THREAD_ID_PENDING_ENTRY Entry;

        Entry = CONTAINING_RECORD( Current,
                                   LDK_ALERT_BY_THREAD_ID_PENDING_ENTRY,
                                   Links );
        if (Entry->Thread == Thread) {
            RemoveEntryList( &Entry->Links );
            ObDereferenceObject( Entry->Thread );
            ExFreeToNPagedLookasideList( &LdkpAlertByThreadIdPendingLookaside,
                                         Entry );
            return TRUE;
        }
    }

    return FALSE;
}

VOID
LdkpClearPendingAlertByThreadIdList (
    VOID
    )
{
    PLIST_ENTRY ListEntry;
    KIRQL OldIrql;

    PAGED_CODE();

    for (;;) {
        OldIrql = ExAcquireSpinLockExclusive( &LdkpAlertByThreadIdListLock );
        if (IsListEmpty( &LdkpAlertByThreadIdPendingListHead )) {
            ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                        OldIrql );
            break;
        }
        ListEntry = RemoveHeadList( &LdkpAlertByThreadIdPendingListHead );
        ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                    OldIrql );

        ObDereferenceObject( CONTAINING_RECORD( ListEntry,
                                                LDK_ALERT_BY_THREAD_ID_PENDING_ENTRY,
                                                Links )->Thread );
        ExFreeToNPagedLookasideList( &LdkpAlertByThreadIdPendingLookaside,
                                     CONTAINING_RECORD( ListEntry,
                                                        LDK_ALERT_BY_THREAD_ID_PENDING_ENTRY,
                                                        Links ) );
    }
}

NTSTATUS
LdkpInitializeAlertByThreadIdList (
    VOID
    )
{
    PAGED_CODE();

    LdkpAlertByThreadIdListLock = 0;
    InitializeListHead( &LdkpAlertByThreadIdListHead );
    InitializeListHead( &LdkpAlertByThreadIdPendingListHead );
    KeInitializeEvent( &LdkpAlertByThreadIdDrainedEvent,
                       NotificationEvent,
                       TRUE );
    ExInitializeNPagedLookasideList( &LdkpAlertByThreadIdPendingLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(LDK_ALERT_BY_THREAD_ID_PENDING_ENTRY),
                                     TAG_ALERT_BY_THREAD_ID_PENDING,
                                     0 );
    LdkpAlertByThreadIdWaitCount = 0;
    LdkpAlertByThreadIdShuttingDown = FALSE;
    return STATUS_SUCCESS;
}

VOID
LdkpTerminateAlertByThreadIdList(
    VOID
    )
{
    KIRQL OldIrql;

    PAGED_CODE();

    OldIrql = ExAcquireSpinLockExclusive( &LdkpAlertByThreadIdListLock );
    LdkpAlertByThreadIdShuttingDown = TRUE;
    ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                OldIrql );

    LdkpWakeAllAlertByThreadIdWaiters();

    KeWaitForSingleObject( &LdkpAlertByThreadIdDrainedEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );

    LdkpClearPendingAlertByThreadIdList();
    ExDeleteNPagedLookasideList( &LdkpAlertByThreadIdPendingLookaside );
}

VOID
LdkpReferenceAlertByThreadIdWait (
    VOID
    )
{
    if (InterlockedIncrement( &LdkpAlertByThreadIdWaitCount ) == 1) {
        KeClearEvent( &LdkpAlertByThreadIdDrainedEvent );
    }
}

VOID
LdkpDereferenceAlertByThreadIdWait (
    VOID
    )
{
    if (InterlockedDecrement( &LdkpAlertByThreadIdWaitCount ) == 0) {
        KeSetEvent( &LdkpAlertByThreadIdDrainedEvent,
                    IO_NO_INCREMENT,
                    FALSE );
    }
}

NTSTATUS
LdkpLookupTebByThreadId (
    _In_ HANDLE ThreadId,
    _Out_ PLDK_TEB* Teb
    )
{
    PETHREAD Thread;
    NTSTATUS Status;

    *Teb = NULL;

    Status = PsLookupThreadByThreadId( ThreadId,
                                       &Thread );
    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    *Teb = LdkReferenceTebByThread( Thread );
    ObDereferenceObject( Thread );

    if (! *Teb) {
        return STATUS_INVALID_CID;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtAlertThreadByThreadId (
    _In_ HANDLE ThreadId
    )
{
    PLDK_ALERT_BY_THREAD_ID_PENDING_ENTRY PendingEntry = NULL;
    PETHREAD Thread;
    PKEVENT Event = NULL;
    NTSTATUS Status;
    KIRQL OldIrql;

    PAGED_CODE();

    Status = PsLookupThreadByThreadId( ThreadId,
                                       &Thread );
    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    OldIrql = ExAcquireSpinLockExclusive( &LdkpAlertByThreadIdListLock );
    Event = LdkpRemoveAlertByThreadIdWaiterLocked( Thread );
    ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                OldIrql );

    if (Event) {
        ObDereferenceObject( Thread );
        KeSetEvent( Event,
                    IO_NO_INCREMENT,
                    FALSE );
        return STATUS_SUCCESS;
    }

    PendingEntry = ExAllocateFromNPagedLookasideList( &LdkpAlertByThreadIdPendingLookaside );
    if (! PendingEntry) {
        OldIrql = ExAcquireSpinLockExclusive( &LdkpAlertByThreadIdListLock );
        Event = LdkpRemoveAlertByThreadIdWaiterLocked( Thread );
        ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                    OldIrql );
        if (Event) {
            ObDereferenceObject( Thread );
            KeSetEvent( Event,
                        IO_NO_INCREMENT,
                        FALSE );
            return STATUS_SUCCESS;
        }
        ObDereferenceObject( Thread );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    PendingEntry->Thread = Thread;

    OldIrql = ExAcquireSpinLockExclusive( &LdkpAlertByThreadIdListLock );
    Event = LdkpRemoveAlertByThreadIdWaiterLocked( Thread );
    if (! Event) {
        if (LdkpHasPendingAlertByThreadIdLocked( Thread )) {
            ExFreeToNPagedLookasideList( &LdkpAlertByThreadIdPendingLookaside,
                                         PendingEntry );
            ObDereferenceObject( Thread );
            PendingEntry = NULL;
        } else {
            InsertTailList( &LdkpAlertByThreadIdPendingListHead,
                            &PendingEntry->Links );
            PendingEntry = NULL;
        }
    }
    ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                OldIrql );

    if (Event) {
        if (PendingEntry) {
            ExFreeToNPagedLookasideList( &LdkpAlertByThreadIdPendingLookaside,
                                         PendingEntry );
            ObDereferenceObject( Thread );
        }
        KeSetEvent( Event,
                    IO_NO_INCREMENT,
                    FALSE );
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
NtWaitForAlertByThreadId (
    _In_opt_ PVOID Address,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    LDK_ALERT_BY_THREAD_ID_ENTRY Entry;
    KEVENT Event;
    PLDK_TEB Teb;
    NTSTATUS Status;
    BOOLEAN Woken;
    KIRQL OldIrql;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Address);

    Teb = NtCurrentTeb();

    KeInitializeEvent( &Event,
                       SynchronizationEvent,
                       FALSE );

    RtlZeroMemory( &Entry,
                   sizeof(Entry) );
    Entry.Thread = PsGetCurrentThread();
    Entry.Event = &Event;
    Entry.InList = TRUE;

    OldIrql = ExAcquireSpinLockExclusive( &LdkpAlertByThreadIdListLock );
    if (LdkpAlertByThreadIdShuttingDown ||
        LDK_IS_SHUTDOWN_IN_PROGRESS) {
        ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                    OldIrql );
        return STATUS_DELETE_PENDING;
    }

    LdkpReferenceAlertByThreadIdWait();

    if (LdkpRemovePendingAlertByThreadIdLocked( PsGetCurrentThread() ) ||
        Teb->AlertByThreadIdPending) {
        Teb->AlertByThreadIdPending = FALSE;
        ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                    OldIrql );
        LdkpDereferenceAlertByThreadIdWait();
        return STATUS_ALERTED;
    }
    InsertTailList( &LdkpAlertByThreadIdListHead,
                    &Entry.Links );
    ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                OldIrql );

    Status = KeWaitForSingleObject( &Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    Timeout );

    OldIrql = ExAcquireSpinLockExclusive( &LdkpAlertByThreadIdListLock );
    Woken = Entry.Woken;
    if (Entry.InList) {
        Entry.InList = FALSE;
        RemoveEntryList( &Entry.Links );
    }
    ExReleaseSpinLockExclusive( &LdkpAlertByThreadIdListLock,
                                OldIrql );

    if (Woken &&
        Status != STATUS_SUCCESS) {
        Status = KeWaitForSingleObject( &Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL );
    }

    if (Status == STATUS_SUCCESS) {
        LdkpDereferenceAlertByThreadIdWait();
        return STATUS_ALERTED;
    }

    LdkpDereferenceAlertByThreadIdWait();
    return Status;
}
