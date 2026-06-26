#include "ntdll.h"



#ifndef STATUS_RESOURCE_NOT_OWNED
#define STATUS_RESOURCE_NOT_OWNED ((NTSTATUS)0xC0000264L)
#endif

#define LDK_SRWLOCK_EXCLUSIVE ((PVOID)(LONG_PTR)-1)



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlInitializeSRWLock)
#pragma alloc_text(PAGE, RtlAcquireSRWLockShared)
#pragma alloc_text(PAGE, RtlReleaseSRWLockShared)
#pragma alloc_text(PAGE, RtlTryAcquireSRWLockShared)
#pragma alloc_text(PAGE, RtlAcquireSRWLockExclusive)
#pragma alloc_text(PAGE, RtlReleaseSRWLockExclusive)
#pragma alloc_text(PAGE, RtlTryAcquireSRWLockExclusive)
#endif



PVOID
LdkpReadSRWLockValue (
    _In_ PRTL_SRWLOCK SRWLock
    )
{
    return *((volatile PVOID*)&SRWLock->Ptr);
}

VOID
LdkpWaitForSRWLockChange (
    _Inout_ PRTL_SRWLOCK SRWLock,
    _In_ PVOID CompareValue
    )
{
    RtlWaitOnAddress( &SRWLock->Ptr,
                      &CompareValue,
                      sizeof(CompareValue),
                      NULL );
}

VOID
NTAPI
RtlInitializeSRWLock (
    _Out_ PRTL_SRWLOCK SRWLock
    )
{
    PAGED_CODE();

    if (! ARGUMENT_PRESENT(SRWLock)) {
        return;
    }

    SRWLock->Ptr = NULL;
}

VOID
NTAPI
RtlAcquireSRWLockExclusive (
    _Inout_ PRTL_SRWLOCK SRWLock
    )
{
    PVOID PreviousState;

    PAGED_CODE();

    for (;;) {
        PreviousState = InterlockedCompareExchangePointer( &SRWLock->Ptr,
                                                           LDK_SRWLOCK_EXCLUSIVE,
                                                           NULL );
        if (PreviousState == NULL) {
            return;
        }

        LdkpWaitForSRWLockChange( SRWLock,
                                  PreviousState );
    }
}

BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive (
    _Inout_ PRTL_SRWLOCK SRWLock
    )
{
    PAGED_CODE();

    return InterlockedCompareExchangePointer( &SRWLock->Ptr,
                                              LDK_SRWLOCK_EXCLUSIVE,
                                              NULL ) == NULL;
}

VOID
NTAPI
RtlReleaseSRWLockExclusive (
    _Inout_ PRTL_SRWLOCK SRWLock
    )
{
    PAGED_CODE();

    if (InterlockedCompareExchangePointer( &SRWLock->Ptr,
                                           NULL,
                                           LDK_SRWLOCK_EXCLUSIVE ) != LDK_SRWLOCK_EXCLUSIVE) {
        RtlRaiseStatus( STATUS_RESOURCE_NOT_OWNED );
    }

    RtlWakeAddressAll( &SRWLock->Ptr );
}

VOID
NTAPI
RtlAcquireSRWLockShared (
    _Inout_ PRTL_SRWLOCK SRWLock
    )
{
    PVOID State;
    PVOID PreviousState;
    LONG_PTR Count;

    PAGED_CODE();

    for (;;) {
        State = LdkpReadSRWLockValue( SRWLock );
        Count = (LONG_PTR)State;
        if (Count >= 0) {
            PreviousState = InterlockedCompareExchangePointer( &SRWLock->Ptr,
                                                               (PVOID)(Count + 1),
                                                               State );
            if (PreviousState == State) {
                return;
            }
            if ((LONG_PTR)PreviousState >= 0) {
                continue;
            }
            State = PreviousState;
        }

        LdkpWaitForSRWLockChange( SRWLock,
                                  State );
    }
}

BOOLEAN
NTAPI
RtlTryAcquireSRWLockShared (
    _Inout_ PRTL_SRWLOCK SRWLock
    )
{
    PVOID State;
    LONG_PTR Count;

    PAGED_CODE();

    State = LdkpReadSRWLockValue( SRWLock );
    Count = (LONG_PTR)State;
    while (Count >= 0) {
        if (InterlockedCompareExchangePointer( &SRWLock->Ptr,
                                               (PVOID)(Count + 1),
                                               State ) == State) {
            return TRUE;
        }

        State = LdkpReadSRWLockValue( SRWLock );
        Count = (LONG_PTR)State;
    }

    return FALSE;
}

VOID
NTAPI
RtlReleaseSRWLockShared (
    _Inout_ PRTL_SRWLOCK SRWLock
    )
{
    PVOID State;
    LONG_PTR Count;
    LONG_PTR NewCount;

    PAGED_CODE();

    for (;;) {
        State = LdkpReadSRWLockValue( SRWLock );
        Count = (LONG_PTR)State;
        if (Count <= 0) {
            RtlRaiseStatus( STATUS_RESOURCE_NOT_OWNED );
        }

        NewCount = Count - 1;
        if (InterlockedCompareExchangePointer( &SRWLock->Ptr,
                                               (PVOID)NewCount,
                                               State ) == State) {
            if (NewCount == 0) {
                RtlWakeAddressAll( &SRWLock->Ptr );
            }
            return;
        }
    }
}
