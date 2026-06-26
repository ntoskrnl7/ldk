#include "ntdll.h"



#define LDK_WAIT_ON_ADDRESS_WAKE_BATCH 64

typedef struct _LDK_WAIT_ON_ADDRESS_ENTRY {
    LIST_ENTRY Links;
    PVOID Address;
    HANDLE ThreadId;
    BOOLEAN InList;
    BOOLEAN Woken;
} LDK_WAIT_ON_ADDRESS_ENTRY, *PLDK_WAIT_ON_ADDRESS_ENTRY;

EX_SPIN_LOCK LdkpWaitOnAddressListLock;
LIST_ENTRY LdkpWaitOnAddressListHead;
KEVENT LdkpWaitOnAddressDrainedEvent;
LONG LdkpWaitOnAddressWaitCount;
BOOLEAN LdkpWaitOnAddressShuttingDown;



NTSTATUS
LdkpInitializeWaitOnAddressList (
    VOID
    );

VOID
LdkpTerminateWaitOnAddressList(
    VOID
    );

VOID
LdkpReferenceWaitOnAddressWait (
    VOID
    );

VOID
LdkpDereferenceWaitOnAddressWait (
    VOID
    );

VOID
LdkpWakeAllAlertByThreadIdWaiters (
    VOID
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeWaitOnAddressList)
#pragma alloc_text(PAGE, RtlWakeAddressSingle)
#pragma alloc_text(PAGE, RtlWakeAddressAll)
#endif



BOOLEAN
LdkpWaitOnAddressValuesMatch (
    _In_reads_bytes_(AddressSize) volatile VOID* Address,
    _In_reads_bytes_(AddressSize) PVOID CompareAddress,
    _In_ SIZE_T AddressSize
    )
{
    switch (AddressSize) {
    case sizeof(UCHAR):
        return *((volatile UCHAR*)Address) == *((PUCHAR)CompareAddress);
    case sizeof(USHORT):
        return *((volatile USHORT*)Address) == *((PUSHORT)CompareAddress);
    case sizeof(ULONG):
        return *((volatile ULONG*)Address) == *((PULONG)CompareAddress);
    case sizeof(ULONGLONG):
        return *((volatile ULONGLONG*)Address) == *((PULONGLONG)CompareAddress);
    default:
        return FALSE;
    }
}

ULONG
LdkpCollectWaitOnAddressThreadIds (
    _In_opt_ PVOID Address,
    _In_ BOOLEAN WakeAll,
    _Out_writes_(ThreadIdCount) PHANDLE ThreadIds,
    _In_ ULONG ThreadIdCount
    )
{
    ULONG Count = 0;
    PLIST_ENTRY Current;
    KIRQL OldIrql;

    OldIrql = ExAcquireSpinLockExclusive( &LdkpWaitOnAddressListLock );
    Current = LdkpWaitOnAddressListHead.Flink;
    while (Current != &LdkpWaitOnAddressListHead && Count < ThreadIdCount) {
        PLDK_WAIT_ON_ADDRESS_ENTRY Entry;
        PLIST_ENTRY Next;

        Next = Current->Flink;
        Entry = CONTAINING_RECORD( Current,
                                   LDK_WAIT_ON_ADDRESS_ENTRY,
                                   Links );
        if (! Address || Entry->Address == Address) {
            Entry->InList = FALSE;
            Entry->Woken = TRUE;
            RemoveEntryList( &Entry->Links );
            ThreadIds[Count++] = Entry->ThreadId;
            if (! WakeAll) {
                break;
            }
        }

        Current = Next;
    }
    ExReleaseSpinLockExclusive( &LdkpWaitOnAddressListLock,
                                OldIrql );

    return Count;
}

VOID
LdkpAlertThreadIds (
    _In_reads_(ThreadIdCount) PHANDLE ThreadIds,
    _In_ ULONG ThreadIdCount
    )
{
    ULONG Index;

    for (Index = 0; Index < ThreadIdCount; Index++) {
        NtAlertThreadByThreadId( ThreadIds[Index] );
    }
}

NTSTATUS
LdkpInitializeWaitOnAddressList (
    VOID
    )
{
    PAGED_CODE();

    LdkpWaitOnAddressListLock = 0;
    InitializeListHead( &LdkpWaitOnAddressListHead );
    KeInitializeEvent( &LdkpWaitOnAddressDrainedEvent,
                       NotificationEvent,
                       TRUE );
    LdkpWaitOnAddressWaitCount = 0;
    LdkpWaitOnAddressShuttingDown = FALSE;
    return STATUS_SUCCESS;
}

VOID
LdkpTerminateWaitOnAddressList(
    VOID
    )
{
    HANDLE ThreadIds[LDK_WAIT_ON_ADDRESS_WAKE_BATCH];
    ULONG Count;
    KIRQL OldIrql;

    PAGED_CODE();

    OldIrql = ExAcquireSpinLockExclusive( &LdkpWaitOnAddressListLock );
    LdkpWaitOnAddressShuttingDown = TRUE;
    ExReleaseSpinLockExclusive( &LdkpWaitOnAddressListLock,
                                OldIrql );

    do {
        Count = LdkpCollectWaitOnAddressThreadIds( NULL,
                                                   TRUE,
                                                   ThreadIds,
                                                   RTL_NUMBER_OF(ThreadIds) );
        LdkpAlertThreadIds( ThreadIds,
                            Count );
    } while (Count == RTL_NUMBER_OF(ThreadIds));

    LdkpWakeAllAlertByThreadIdWaiters();

    KeWaitForSingleObject( &LdkpWaitOnAddressDrainedEvent,
                           Executive,
                           KernelMode,
                           FALSE,
                           NULL );
}

VOID
LdkpReferenceWaitOnAddressWait (
    VOID
    )
{
    if (InterlockedIncrement( &LdkpWaitOnAddressWaitCount ) == 1) {
        KeClearEvent( &LdkpWaitOnAddressDrainedEvent );
    }
}

VOID
LdkpDereferenceWaitOnAddressWait (
    VOID
    )
{
    if (InterlockedDecrement( &LdkpWaitOnAddressWaitCount ) == 0) {
        KeSetEvent( &LdkpWaitOnAddressDrainedEvent,
                    IO_NO_INCREMENT,
                    FALSE );
    }
}

NTSTATUS
NTAPI
RtlWaitOnAddress (
    _In_reads_bytes_(AddressSize) volatile VOID* Address,
    _In_reads_bytes_(AddressSize) PVOID CompareAddress,
    _In_ SIZE_T AddressSize,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    LDK_WAIT_ON_ADDRESS_ENTRY Entry;
    NTSTATUS Status;
    BOOLEAN Woken;
    KIRQL OldIrql;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(Address) ||
        ! ARGUMENT_PRESENT(CompareAddress) ||
        (AddressSize != sizeof(UCHAR) &&
         AddressSize != sizeof(USHORT) &&
         AddressSize != sizeof(ULONG) &&
         AddressSize != sizeof(ULONGLONG))) {
        return STATUS_INVALID_PARAMETER;
    }

    if (! LdkpWaitOnAddressValuesMatch( Address,
                                        CompareAddress,
                                        AddressSize )) {
        return STATUS_SUCCESS;
    }

    RtlZeroMemory( &Entry,
                   sizeof(Entry) );
    Entry.Address = (PVOID)Address;
    Entry.ThreadId = PsGetCurrentThreadId();
    Entry.InList = TRUE;

    OldIrql = ExAcquireSpinLockExclusive( &LdkpWaitOnAddressListLock );
    if (LdkpWaitOnAddressShuttingDown ||
        LDK_IS_SHUTDOWN_IN_PROGRESS) {
        ExReleaseSpinLockExclusive( &LdkpWaitOnAddressListLock,
                                    OldIrql );
        return STATUS_DELETE_PENDING;
    }

    if (! LdkpWaitOnAddressValuesMatch( Address,
                                        CompareAddress,
                                        AddressSize )) {
        Entry.InList = FALSE;
        ExReleaseSpinLockExclusive( &LdkpWaitOnAddressListLock,
                                    OldIrql );
        return STATUS_SUCCESS;
    }

    LdkpReferenceWaitOnAddressWait();

    InsertTailList( &LdkpWaitOnAddressListHead,
                    &Entry.Links );

    if (! LdkpWaitOnAddressValuesMatch( Address,
                                        CompareAddress,
                                        AddressSize )) {
        Entry.InList = FALSE;
        RemoveEntryList( &Entry.Links );
        ExReleaseSpinLockExclusive( &LdkpWaitOnAddressListLock,
                                    OldIrql );
        LdkpDereferenceWaitOnAddressWait();
        return STATUS_SUCCESS;
    }
    ExReleaseSpinLockExclusive( &LdkpWaitOnAddressListLock,
                                OldIrql );

    Status = NtWaitForAlertByThreadId( NULL,
                                       Timeout );

    OldIrql = ExAcquireSpinLockExclusive( &LdkpWaitOnAddressListLock );
    Woken = Entry.Woken;
    if (Entry.InList) {
        Entry.InList = FALSE;
        RemoveEntryList( &Entry.Links );
    }
    ExReleaseSpinLockExclusive( &LdkpWaitOnAddressListLock,
                                OldIrql );

    if (Woken &&
        Status != STATUS_ALERTED &&
        Status != STATUS_SUCCESS) {
        Status = NtWaitForAlertByThreadId( NULL,
                                           NULL );
    }

    if (Status == STATUS_ALERTED) {
        Status = STATUS_SUCCESS;
    }

    LdkpDereferenceWaitOnAddressWait();
    return Status;
}

VOID
NTAPI
RtlWakeAddressSingle (
    _In_ PVOID Address
    )
{
    HANDLE ThreadId;
    ULONG Count;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(Address)) {
        return;
    }

    Count = LdkpCollectWaitOnAddressThreadIds( Address,
                                               FALSE,
                                               &ThreadId,
                                               1 );
    if (Count) {
        LdkpAlertThreadIds( &ThreadId,
                            Count );
    }
}

VOID
NTAPI
RtlWakeAddressAll (
    _In_ PVOID Address
    )
{
    HANDLE ThreadIds[LDK_WAIT_ON_ADDRESS_WAKE_BATCH];
    ULONG Count;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(Address)) {
        return;
    }

    do {
        Count = LdkpCollectWaitOnAddressThreadIds( Address,
                                                   TRUE,
                                                   ThreadIds,
                                                   RTL_NUMBER_OF(ThreadIds) );
        LdkpAlertThreadIds( ThreadIds,
                            Count );
    } while (Count == RTL_NUMBER_OF(ThreadIds));
}
