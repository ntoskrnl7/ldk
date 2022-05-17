#include "ntdll.h"



NTSTATUS
LdkpInitializeKeyedEventList (
    VOID
    );

VOID
LdkpTerminateKeyedEventList(
    VOID
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeKeyedEventList)
#pragma alloc_text(PAGE, NtCreateKeyedEvent)
#pragma alloc_text(PAGE, NtOpenKeyedEvent)
#endif



#define TAG_KEYED_EVENT_WAIT_ENTRY  'eWeK'

typedef struct _LDK_KEYED_EVENT_WAIT_ENTRY {
    LIST_ENTRY Links;
    PVOID Key;
    PLDK_TEB Teb;
} LDK_KEYED_EVENT_WAIT_ENTRY, * PLDK_KEYED_EVENT_WAIT_ENTRY;

EX_SPIN_LOCK LdkpKeyedEventWaitListLock;
LIST_ENTRY LdkpKeyedEventWaitListHead;
NPAGED_LOOKASIDE_LIST LdkpKeyedEventWaitLookaside;

NTSTATUS
LdkpInitializeKeyedEventList (
    VOID
    )
{
    PAGED_CODE();

    LdkpKeyedEventWaitListLock = 0;

    InitializeListHead( &LdkpKeyedEventWaitListHead );
    ExInitializeNPagedLookasideList( &LdkpKeyedEventWaitLookaside,
                                     NULL,
                                     NULL,
                                     0,
                                     sizeof(LDK_KEYED_EVENT_WAIT_ENTRY),
                                     TAG_KEYED_EVENT_WAIT_ENTRY,
                                     0 );
    return STATUS_SUCCESS;
}

VOID
LdkpTerminateKeyedEventList(
    VOID
    )
{
    KIRQL OldIrql = ExAcquireSpinLockExclusive( &LdkpKeyedEventWaitListLock );
    PLIST_ENTRY current;
    PLIST_ENTRY next;
    LIST_FOR_EACH_SAFE(current, next, &LdkpKeyedEventWaitListHead) {
        PLDK_KEYED_EVENT_WAIT_ENTRY entry = CONTAINING_RECORD( current, LDK_KEYED_EVENT_WAIT_ENTRY, Links );
        if (entry->Teb) {
            KeReleaseSemaphore( &entry->Teb->KeyedWaitSemaphore,
                                SEMAPHORE_INCREMENT,
                                1,
                                FALSE );
            entry->Teb = NULL;
        }
        entry->Key = NULL;
        RemoveEntryList( &entry->Links );
        ExFreeToNPagedLookasideList( &LdkpKeyedEventWaitLookaside,
                                     entry );
    }
    ExReleaseSpinLockExclusive( &LdkpKeyedEventWaitListLock,
                                OldIrql );

    ExDeleteNPagedLookasideList( &LdkpKeyedEventWaitLookaside );
}



NTSTATUS
NTAPI
NtCreateKeyedEvent (
    _Out_ PHANDLE KeyedEventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(DesiredAccess);
    UNREFERENCED_PARAMETER(Flags);
    PAGED_CODE();

    if (ARGUMENT_PRESENT(ObjectAttributes)) {
        SetFlag( ObjectAttributes->Attributes, OBJ_KERNEL_HANDLE );
    }
    return ZwCreateEvent( KeyedEventHandle,
                          EVENT_ALL_ACCESS,
                          ObjectAttributes,
                          NotificationEvent,
                          FALSE );
}

NTSTATUS
NTAPI
NtOpenKeyedEvent (
    _Out_ PHANDLE KeyedEventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    )
{
    UNREFERENCED_PARAMETER(DesiredAccess);
    PAGED_CODE();

    if (ARGUMENT_PRESENT(ObjectAttributes)) {
        SetFlag( ObjectAttributes->Attributes, OBJ_KERNEL_HANDLE );
    }
    return ZwOpenEvent( KeyedEventHandle,
                        MAXIMUM_ALLOWED,
                        ObjectAttributes );
}

NTSTATUS
NTAPI
NtWaitForKeyedEvent (
    _In_ HANDLE KeyedEventHandle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    PLDK_TEB teb = NtCurrentTeb();
    PLDK_KEYED_EVENT_WAIT_ENTRY entry = ExAllocateFromNPagedLookasideList( &LdkpKeyedEventWaitLookaside );
    if (!entry) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KIRQL OldIrql = ExAcquireSpinLockExclusive( &LdkpKeyedEventWaitListLock );
    entry->Key = Key;
    entry->Teb = teb;
    InsertTailList( &LdkpKeyedEventWaitListHead,
                    &entry->Links );
    ExReleaseSpinLockExclusive( &LdkpKeyedEventWaitListLock,
                                OldIrql );

    ZwPulseEvent( KeyedEventHandle,
                  NULL );
    
    teb->KeyedWaitValue = Key;
    NTSTATUS Status;
    Status = KeWaitForSingleObject( &teb->KeyedWaitSemaphore,
                                    Executive,
                                    KernelMode,
                                    Alertable,
                                    Timeout );
    if (NT_SUCCESS(Status)) {
    } else {
        KdBreakPoint();
    }
    return Status;
}

NTSTATUS
NTAPI
NtReleaseKeyedEvent (
    _In_ HANDLE KeyedEventHandle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    PLDK_TEB Found = NULL;

    do {
        KIRQL OldIrql = ExAcquireSpinLockShared( &LdkpKeyedEventWaitListLock );
        PLIST_ENTRY Current;
        PLIST_ENTRY Next;
        LIST_FOR_EACH_SAFE(Current, Next, &LdkpKeyedEventWaitListHead) {
            PLDK_KEYED_EVENT_WAIT_ENTRY Entry = CONTAINING_RECORD( Current, LDK_KEYED_EVENT_WAIT_ENTRY, Links );
            if (Entry->Key == Key) {
                while (!ExTryConvertSharedSpinLockExclusive( &LdkpKeyedEventWaitListLock )) {
                    YieldProcessor();
                }
                Found = Entry->Teb;
                Entry->Teb = NULL;
                Entry->Key = NULL;
                RemoveEntryList( &Entry->Links );
                ExFreeToNPagedLookasideList( &LdkpKeyedEventWaitLookaside,
                                             Entry );
                break;
            }
        }
        if (Found) {
            ExReleaseSpinLockExclusive( &LdkpKeyedEventWaitListLock,
                                        OldIrql );
            break;
        } else {
            ExReleaseSpinLockShared( &LdkpKeyedEventWaitListLock,
                                     OldIrql );
        }        
        Status = ZwWaitForSingleObject( KeyedEventHandle,
                                        Alertable,
                                        Timeout );
    } while (NT_SUCCESS(Status));

    if (! NT_SUCCESS(Status)) {
        KdBreakPoint();
        return Status;
    }

    KeReleaseSemaphore( &Found->KeyedWaitSemaphore,
                        SEMAPHORE_INCREMENT,
                        1,
                        FALSE );

    return ZwPulseEvent( KeyedEventHandle,
                         NULL );
}
