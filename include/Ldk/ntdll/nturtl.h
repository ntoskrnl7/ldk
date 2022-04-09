#pragma once

EXTERN_C_START

VOID
NTAPI
RtlInitializeSRWLock(
    _Out_ PRTL_SRWLOCK SRWLock
    );

VOID
NTAPI
RtlAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

VOID
NTAPI
RtlReleaseSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

VOID
NTAPI
RtlAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

VOID
NTAPI
RtlReleaseSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

BOOLEAN
NTAPI
RtlTryAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );



NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
    );

NTSTATUS
NTAPI
RtlInitializeCriticalSectionEx(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount,
    _In_ ULONG Flags
    );

NTSTATUS
NTAPI
RtlDeleteCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

LONG
NTAPI
RtlGetCriticalSectionRecursionCount(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSTATUS
NTAPI
RtlEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

BOOLEAN
NTAPI
RtlTryEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );


VOID
NTAPI
RtlInitializeConditionVariable(
    _Out_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

VOID
NTAPI
RtlWakeConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

VOID
NTAPI
RtlWakeAllConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

NTSTATUS
NTAPI
RtlSleepConditionVariableCS(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_opt_ const LARGE_INTEGER* TimeOut
    );

NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_SRWLOCK SRWLock,
    _In_opt_ const LARGE_INTEGER* TimeOut,
    _In_ ULONG Flags
    );



HANDLE
NTAPI
RtlGetProcessHeap(
    VOID
    );



#define WT_EXECUTEDEFAULT       0x00000000                           // winnt
#define WT_EXECUTEINIOTHREAD    0x00000001                           // winnt
#define WT_EXECUTEINUITHREAD    0x00000002                           // winnt
#define WT_EXECUTEINWAITTHREAD  0x00000004                           // winnt
#define WT_EXECUTEONLYONCE      0x00000008                           // winnt
#define WT_EXECUTEINTIMERTHREAD 0x00000020                           // winnt
#define WT_EXECUTELONGFUNCTION  0x00000010                           // winnt
#define WT_EXECUTEINPERSISTENTIOTHREAD  0x00000040                   // winnt
#define WT_EXECUTEINPERSISTENTTHREAD 0x00000080                      // winnt
#define WT_TRANSFER_IMPERSONATION 0x00000100                         // winnt
#define WT_SET_MAX_THREADPOOL_THREADS(Flags, Limit)  ((Flags) |= (Limit)<<16) // winnt

typedef VOID (NTAPI * WORKERCALLBACKFUNC) (PVOID );                 // winnt

NTSTATUS
NTAPI
RtlQueueWorkItem(
    _In_ WORKERCALLBACKFUNC Function,
    _In_ PVOID Context,
    _In_ ULONG Flags
    );

EXTERN_C_END