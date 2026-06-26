#include "ntdll.h"



#define LDK_CONDITION_VARIABLE_WAKE_BATCH 64

typedef struct _LDK_CONDITION_VARIABLE_WAIT_BLOCK {
    LIST_ENTRY Links;
    PRTL_CONDITION_VARIABLE ConditionVariable;
    KEVENT Event;
    BOOLEAN InList;
    BOOLEAN Woken;
} LDK_CONDITION_VARIABLE_WAIT_BLOCK, *PLDK_CONDITION_VARIABLE_WAIT_BLOCK;

EX_SPIN_LOCK LdkpConditionVariableLock;
LIST_ENTRY LdkpConditionVariableWaitList;
LONG LdkpConditionVariableInitialized;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlInitializeConditionVariable)
#pragma alloc_text(PAGE, RtlWakeConditionVariable)
#pragma alloc_text(PAGE, RtlWakeAllConditionVariable)
#pragma alloc_text(PAGE, RtlSleepConditionVariableCS)
#pragma alloc_text(PAGE, RtlSleepConditionVariableSRW)
#endif



VOID
LdkpEnsureConditionVariableList (
    VOID
    )
{
    LONG State;

    State = *((volatile LONG*)&LdkpConditionVariableInitialized);
    if (State == 2) {
        return;
    }

    if (InterlockedCompareExchange( &LdkpConditionVariableInitialized,
                                    1,
                                    0 ) == 0) {
        LdkpConditionVariableLock = 0;
        InitializeListHead( &LdkpConditionVariableWaitList );
        InterlockedExchange( &LdkpConditionVariableInitialized,
                             2 );
        return;
    }

    while (*((volatile LONG*)&LdkpConditionVariableInitialized) != 2) {
        YieldProcessor();
    }
}

ULONG
LdkpCollectConditionVariableWaiters (
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _In_ BOOLEAN WakeAll,
    _Out_writes_(WaiterCount) PLDK_CONDITION_VARIABLE_WAIT_BLOCK* Waiters,
    _In_ ULONG WaiterCount
    )
{
    ULONG Count = 0;
    PLIST_ENTRY Current;
    KIRQL OldIrql;

    OldIrql = ExAcquireSpinLockExclusive( &LdkpConditionVariableLock );
    Current = LdkpConditionVariableWaitList.Flink;
    while (Current != &LdkpConditionVariableWaitList &&
           Count < WaiterCount) {
        PLDK_CONDITION_VARIABLE_WAIT_BLOCK WaitBlock;
        PLIST_ENTRY Next;

        Next = Current->Flink;
        WaitBlock = CONTAINING_RECORD( Current,
                                        LDK_CONDITION_VARIABLE_WAIT_BLOCK,
                                        Links );
        if (WaitBlock->ConditionVariable == ConditionVariable) {
            WaitBlock->InList = FALSE;
            WaitBlock->Woken = TRUE;
            RemoveEntryList( &WaitBlock->Links );
            Waiters[Count++] = WaitBlock;
            if (! WakeAll) {
                break;
            }
        }

        Current = Next;
    }
    ExReleaseSpinLockExclusive( &LdkpConditionVariableLock,
                                OldIrql );

    return Count;
}

VOID
LdkpWakeConditionVariableWaiters (
    _In_reads_(WaiterCount) PLDK_CONDITION_VARIABLE_WAIT_BLOCK* Waiters,
    _In_ ULONG WaiterCount
    )
{
    ULONG Index;

    for (Index = 0; Index < WaiterCount; Index++) {
        KeSetEvent( &Waiters[Index]->Event,
                    IO_NO_INCREMENT,
                    FALSE );
    }
}

NTSTATUS
LdkpSleepConditionVariable (
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_opt_ PRTL_CRITICAL_SECTION CriticalSection,
    _Inout_opt_ PRTL_SRWLOCK SRWLock,
    _In_opt_ const LARGE_INTEGER* TimeOut,
    _In_ ULONG Flags
    )
{
    LDK_CONDITION_VARIABLE_WAIT_BLOCK WaitBlock;
    NTSTATUS Status;
    BOOLEAN Woken;
    KIRQL OldIrql;

    LdkpEnsureConditionVariableList();

    RtlZeroMemory( &WaitBlock,
                   sizeof(WaitBlock) );
    WaitBlock.ConditionVariable = ConditionVariable;
    WaitBlock.InList = TRUE;
    KeInitializeEvent( &WaitBlock.Event,
                       SynchronizationEvent,
                       FALSE );

    OldIrql = ExAcquireSpinLockExclusive( &LdkpConditionVariableLock );
    InsertTailList( &LdkpConditionVariableWaitList,
                    &WaitBlock.Links );
    ExReleaseSpinLockExclusive( &LdkpConditionVariableLock,
                                OldIrql );

    if (CriticalSection) {
        RtlLeaveCriticalSection( CriticalSection );
    } else if (Flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED) {
        RtlReleaseSRWLockShared( SRWLock );
    } else {
        RtlReleaseSRWLockExclusive( SRWLock );
    }

    Status = KeWaitForSingleObject( &WaitBlock.Event,
                                    Executive,
                                    KernelMode,
                                    FALSE,
                                    (PLARGE_INTEGER)TimeOut );

    OldIrql = ExAcquireSpinLockExclusive( &LdkpConditionVariableLock );
    Woken = WaitBlock.Woken;
    if (WaitBlock.InList) {
        WaitBlock.InList = FALSE;
        RemoveEntryList( &WaitBlock.Links );
    }
    ExReleaseSpinLockExclusive( &LdkpConditionVariableLock,
                                OldIrql );

    if (Woken &&
        Status != STATUS_SUCCESS) {
        Status = KeWaitForSingleObject( &WaitBlock.Event,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        NULL );
    }

    if (CriticalSection) {
        RtlEnterCriticalSection( CriticalSection );
    } else if (Flags & RTL_CONDITION_VARIABLE_LOCKMODE_SHARED) {
        RtlAcquireSRWLockShared( SRWLock );
    } else {
        RtlAcquireSRWLockExclusive( SRWLock );
    }

    return Status;
}

VOID
NTAPI
RtlInitializeConditionVariable (
    _Out_ PRTL_CONDITION_VARIABLE ConditionVariable
    )
{
    PAGED_CODE();

    if (! ARGUMENT_PRESENT(ConditionVariable)) {
        return;
    }

    LdkpEnsureConditionVariableList();
    ConditionVariable->Ptr = NULL;
}

VOID
NTAPI
RtlWakeConditionVariable (
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    )
{
    PLDK_CONDITION_VARIABLE_WAIT_BLOCK Waiter;
    ULONG Count;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(ConditionVariable)) {
        return;
    }

    LdkpEnsureConditionVariableList();
    Count = LdkpCollectConditionVariableWaiters( ConditionVariable,
                                                 FALSE,
                                                 &Waiter,
                                                 1 );
    LdkpWakeConditionVariableWaiters( &Waiter,
                                      Count );
}

VOID
NTAPI
RtlWakeAllConditionVariable (
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    )
{
    PLDK_CONDITION_VARIABLE_WAIT_BLOCK Waiters[LDK_CONDITION_VARIABLE_WAKE_BATCH];
    ULONG Count;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(ConditionVariable)) {
        return;
    }

    LdkpEnsureConditionVariableList();
    do {
        Count = LdkpCollectConditionVariableWaiters( ConditionVariable,
                                                    TRUE,
                                                    Waiters,
                                                    RTL_NUMBER_OF(Waiters) );
        LdkpWakeConditionVariableWaiters( Waiters,
                                          Count );
    } while (Count == RTL_NUMBER_OF(Waiters));
}

NTSTATUS
NTAPI
RtlSleepConditionVariableCS (
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_opt_ const LARGE_INTEGER* TimeOut
    )
{
    PAGED_CODE();

    if (! ARGUMENT_PRESENT(ConditionVariable) ||
        ! ARGUMENT_PRESENT(CriticalSection)) {
        return STATUS_INVALID_PARAMETER;
    }

    return LdkpSleepConditionVariable( ConditionVariable,
                                       CriticalSection,
                                       NULL,
                                       TimeOut,
                                       0 );
}

NTSTATUS
NTAPI
RtlSleepConditionVariableSRW (
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_SRWLOCK SRWLock,
    _In_opt_ const LARGE_INTEGER* TimeOut,
    _In_ ULONG Flags
    )
{
    PAGED_CODE();

    if (! ARGUMENT_PRESENT(ConditionVariable) ||
        ! ARGUMENT_PRESENT(SRWLock)) {
        return STATUS_INVALID_PARAMETER;
    }

    return LdkpSleepConditionVariable( ConditionVariable,
                                       NULL,
                                       SRWLock,
                                       TimeOut,
                                       Flags );
}
