#include "ntdll.h"
#include "../ldk.h"



#ifndef STATUS_RESOURCE_NOT_OWNED
#define STATUS_RESOURCE_NOT_OWNED ((NTSTATUS)0xC0000264L)
#endif

#define TAG_CRITICAL_SECTION_STATE 'sCdL'



FAST_MUTEX LdkpCriticalSectionLock;
LONG LdkpCriticalSectionLockInitialized;

typedef struct _LDK_CRITICAL_SECTION_WAIT_BLOCK {
    LIST_ENTRY Links;
    HANDLE Owner;
    KEVENT Event;
    BOOLEAN InList;
} LDK_CRITICAL_SECTION_WAIT_BLOCK, *PLDK_CRITICAL_SECTION_WAIT_BLOCK;

typedef struct _LDK_CRITICAL_SECTION_STATE {
    LIST_ENTRY WaitList;
} LDK_CRITICAL_SECTION_STATE, *PLDK_CRITICAL_SECTION_STATE;



VOID
NTAPI
RtlpDeleteDeferedCriticalSection (
    VOID
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlInitializeCriticalSectionEx)
#pragma alloc_text(PAGE, RtlpDeleteDeferedCriticalSection)
#pragma alloc_text(PAGE, RtlInitializeCriticalSection)
#pragma alloc_text(PAGE, RtlInitializeCriticalSectionAndSpinCount)
#pragma alloc_text(PAGE, RtlGetCriticalSectionRecursionCount)
#endif



HANDLE
LdkpCurrentCriticalSectionOwner (
    VOID
    )
{
    return NtCurrentTeb()->ClientId.UniqueThread;
}

NTSTATUS
LdkpEnsureCriticalSectionState (
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    PLDK_CRITICAL_SECTION_STATE State;
    PVOID PreviousState;

    if (CriticalSection->LockSemaphore) {
        return STATUS_SUCCESS;
    }

#pragma warning(suppress:4996)
    State = ExAllocatePoolWithTag( NonPagedPool,
                                   sizeof(*State),
                                   TAG_CRITICAL_SECTION_STATE );
    if (! State) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    InitializeListHead( &State->WaitList );

    PreviousState = InterlockedCompareExchangePointer( &CriticalSection->LockSemaphore,
                                                       State,
                                                       NULL );
    if (PreviousState) {
        ExFreePoolWithTag( State,
                           TAG_CRITICAL_SECTION_STATE );
    }

    return STATUS_SUCCESS;
}

VOID
LdkpRaiseCriticalSectionStatus (
    _In_ NTSTATUS Status
    )
{
    RtlRaiseStatus( Status );
}

VOID
LdkpEnsureCriticalSectionLock (
    VOID
    )
{
    LONG State;

    State = *((volatile LONG*)&LdkpCriticalSectionLockInitialized);
    if (State == 2) {
        return;
    }

    if (InterlockedCompareExchange( &LdkpCriticalSectionLockInitialized,
                                    1,
                                    0 ) == 0) {
        ExInitializeFastMutex( &LdkpCriticalSectionLock );
        InterlockedExchange( &LdkpCriticalSectionLockInitialized,
                             2 );
        return;
    }

    while (*((volatile LONG*)&LdkpCriticalSectionLockInitialized) != 2) {
        YieldProcessor();
    }
}

VOID
LdkpAcquireCriticalSectionLock (
    VOID
    )
{
    LdkpEnsureCriticalSectionLock();
    ExAcquireFastMutex( &LdkpCriticalSectionLock );
}

VOID
LdkpReleaseCriticalSectionLock (
    VOID
    )
{
    ExReleaseFastMutex( &LdkpCriticalSectionLock );
}

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

NTSTATUS
NTAPI
RtlInitializeCriticalSection (
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    PAGED_CODE();

    return RtlInitializeCriticalSectionAndSpinCount( CriticalSection,
                                                     0 );
}

NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount (
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
    )
{
    PAGED_CODE();

    if (! ARGUMENT_PRESENT(CriticalSection)) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlZeroMemory( CriticalSection,
                   sizeof(*CriticalSection) );
    CriticalSection->LockCount = -1;
    CriticalSection->SpinCount = SpinCount;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlDeleteCriticalSection (
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    PVOID State;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(CriticalSection)) {
        return STATUS_INVALID_PARAMETER;
    }

    LdkpAcquireCriticalSectionLock();
    State = CriticalSection->LockSemaphore;
    RtlZeroMemory( CriticalSection,
                   sizeof(*CriticalSection) );
    CriticalSection->LockCount = -1;
    LdkpReleaseCriticalSectionLock();

    if (State) {
        ExFreePoolWithTag( State,
                           TAG_CRITICAL_SECTION_STATE );
    }

    return STATUS_SUCCESS;
}

LONG
NTAPI
RtlGetCriticalSectionRecursionCount (
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    PAGED_CODE();

    if (! ARGUMENT_PRESENT(CriticalSection)) {
        return 0;
    }

    return CriticalSection->RecursionCount;
}

NTSTATUS
NTAPI
RtlEnterCriticalSection (
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    LDK_CRITICAL_SECTION_WAIT_BLOCK WaitBlock;
    PLDK_CRITICAL_SECTION_STATE State;
    HANDLE Owner;
    NTSTATUS Status;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(CriticalSection)) {
        return STATUS_INVALID_PARAMETER;
    }

    Owner = LdkpCurrentCriticalSectionOwner();
    RtlZeroMemory( &WaitBlock,
                   sizeof(WaitBlock) );
    WaitBlock.Owner = Owner;
    WaitBlock.InList = TRUE;
    KeInitializeEvent( &WaitBlock.Event,
                       SynchronizationEvent,
                       FALSE );

    for (;;) {
        LdkpAcquireCriticalSectionLock();
        if (! CriticalSection->OwningThread) {
            CriticalSection->OwningThread = Owner;
            CriticalSection->LockCount = 0;
            CriticalSection->RecursionCount = 1;
            LdkpReleaseCriticalSectionLock();
            return STATUS_SUCCESS;
        }

        if (CriticalSection->OwningThread == Owner) {
            CriticalSection->RecursionCount++;
            LdkpReleaseCriticalSectionLock();
            return STATUS_SUCCESS;
        }

        State = (PLDK_CRITICAL_SECTION_STATE)CriticalSection->LockSemaphore;
        if (! State) {
            LdkpReleaseCriticalSectionLock();

            Status = LdkpEnsureCriticalSectionState( CriticalSection );
            if (! NT_SUCCESS(Status)) {
                return Status;
            }
            continue;
        }

        CriticalSection->LockCount++;
        InsertTailList( &State->WaitList,
                        &WaitBlock.Links );
        LdkpReleaseCriticalSectionLock();

        Status = KeWaitForSingleObject( &WaitBlock.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL );
        if (NT_SUCCESS(Status)) {
            return STATUS_SUCCESS;
        }

        LdkpAcquireCriticalSectionLock();
        if (WaitBlock.InList) {
            WaitBlock.InList = FALSE;
            RemoveEntryList( &WaitBlock.Links );
            CriticalSection->LockCount--;
        }
        LdkpReleaseCriticalSectionLock();

        return Status;
    }
}

BOOLEAN
NTAPI
RtlTryEnterCriticalSection (
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    HANDLE Owner;
    BOOLEAN Acquired;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(CriticalSection)) {
        return FALSE;
    }

    Owner = LdkpCurrentCriticalSectionOwner();

    LdkpAcquireCriticalSectionLock();
    if (CriticalSection->OwningThread == Owner) {
        CriticalSection->RecursionCount++;
        LdkpReleaseCriticalSectionLock();
        return TRUE;
    }

    Acquired = (CriticalSection->OwningThread == NULL);
    if (Acquired) {
        CriticalSection->OwningThread = Owner;
        CriticalSection->LockCount = 0;
        CriticalSection->RecursionCount = 1;
    }
    LdkpReleaseCriticalSectionLock();

    return Acquired;
}

NTSTATUS
NTAPI
RtlLeaveCriticalSection (
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    )
{
    PLDK_CRITICAL_SECTION_STATE State;
    PLDK_CRITICAL_SECTION_WAIT_BLOCK WaitBlock = NULL;
    HANDLE Owner;
    BOOLEAN RaiseNotOwned = FALSE;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(CriticalSection)) {
        return STATUS_INVALID_PARAMETER;
    }

    Owner = LdkpCurrentCriticalSectionOwner();
    LdkpAcquireCriticalSectionLock();
    if (CriticalSection->OwningThread != Owner ||
        CriticalSection->RecursionCount <= 0) {
        RaiseNotOwned = TRUE;
    } else {
        CriticalSection->RecursionCount--;
        if (CriticalSection->RecursionCount == 0) {
            State = (PLDK_CRITICAL_SECTION_STATE)CriticalSection->LockSemaphore;
            if (State &&
                ! IsListEmpty( &State->WaitList )) {
                WaitBlock = CONTAINING_RECORD( RemoveHeadList( &State->WaitList ),
                                               LDK_CRITICAL_SECTION_WAIT_BLOCK,
                                               Links );
                WaitBlock->InList = FALSE;
                CriticalSection->OwningThread = WaitBlock->Owner;
                CriticalSection->RecursionCount = 1;
                CriticalSection->LockCount--;
            } else {
                CriticalSection->OwningThread = NULL;
                CriticalSection->LockCount = -1;
            }
        }
    }
    LdkpReleaseCriticalSectionLock();

    if (RaiseNotOwned) {
        LdkpRaiseCriticalSectionStatus( STATUS_RESOURCE_NOT_OWNED );
    }

    if (WaitBlock) {
        KeSetEvent( &WaitBlock->Event,
                    IO_NO_INCREMENT,
                    FALSE );
    }

    return STATUS_SUCCESS;
}

VOID
NTAPI
RtlpDeleteDeferedCriticalSection (
    VOID
    )
{
    PAGED_CODE();
}
