#pragma once

EXTERN_C_START

//
// Define the slim R/W lock.
//

#define SRWLOCK_INIT RTL_SRWLOCK_INIT

typedef RTL_SRWLOCK SRWLOCK, *PSRWLOCK;

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
VOID
WINAPI
InitializeSRWLock(
    _Out_ PSRWLOCK SRWLock
    );

WINBASEAPI
_Releases_exclusive_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

WINBASEAPI
_Releases_shared_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

WINBASEAPI
_Acquires_exclusive_lock_(*SRWLock)
VOID
WINAPI
AcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

WINBASEAPI
_Acquires_shared_lock_(*SRWLock)
VOID
WINAPI
AcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

WINBASEAPI
_When_(return!=0, _Acquires_exclusive_lock_(*SRWLock))
BOOLEAN
WINAPI
TryAcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    );

WINBASEAPI
_When_(return!=0, _Acquires_shared_lock_(*SRWLock))
BOOLEAN
WINAPI
TryAcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    );

#endif // (_WIN32_WINNT >= 0x0600)


//
// Define the critical section.
//

#if (_WIN32_WINNT < 0x0600)

_Maybe_raises_SEH_exception_
WINBASEAPI
VOID
WINAPI
InitializeCriticalSection(
    _Out_ LPCRITICAL_SECTION lpCriticalSection
    );

#else

WINBASEAPI
VOID
WINAPI
InitializeCriticalSection(
    _Out_ LPCRITICAL_SECTION lpCriticalSection
    );

#endif  // (_WIN32_WINNT < 0x0600)



WINBASEAPI
VOID
WINAPI
EnterCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

WINBASEAPI
BOOL
WINAPI
TryEnterCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

WINBASEAPI
VOID
WINAPI
LeaveCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
InitializeCriticalSectionAndSpinCount(
    _Out_ LPCRITICAL_SECTION lpCriticalSection,
    _In_ DWORD dwSpinCount
    );

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
InitializeCriticalSectionEx(
    _Out_ LPCRITICAL_SECTION lpCriticalSection,
    _In_ DWORD dwSpinCount,
	_In_ DWORD dwFlags
    );

WINBASEAPI
VOID
WINAPI
DeleteCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    );



//
// Define one-time initialization primitive
//

typedef RTL_RUN_ONCE INIT_ONCE;
typedef PRTL_RUN_ONCE PINIT_ONCE;
typedef PRTL_RUN_ONCE LPINIT_ONCE;

#define INIT_ONCE_STATIC_INIT   RTL_RUN_ONCE_INIT

typedef
BOOL
(WINAPI *PINIT_ONCE_FN) (
    _Inout_ PINIT_ONCE InitOnce,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ PVOID *Context
    );

WINBASEAPI
BOOL
WINAPI
InitOnceExecuteOnce(
    _Inout_ PINIT_ONCE InitOnce,
    _In_ __callback PINIT_ONCE_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ LPVOID * Context
    );



//
// Define condition variable
//

typedef RTL_CONDITION_VARIABLE CONDITION_VARIABLE, *PCONDITION_VARIABLE;

//
// Static initializer for the condition variable
//

#define CONDITION_VARIABLE_INIT RTL_CONDITION_VARIABLE_INIT

//
// Flags for condition variables
//

#define CONDITION_VARIABLE_LOCKMODE_SHARED RTL_CONDITION_VARIABLE_LOCKMODE_SHARED

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
VOID
WINAPI
InitializeConditionVariable(
    _Out_ PCONDITION_VARIABLE ConditionVariable
    );

WINBASEAPI
VOID
WINAPI
WakeConditionVariable(
    _Inout_ PCONDITION_VARIABLE ConditionVariable
    );

WINBASEAPI
VOID
WINAPI
WakeAllConditionVariable(
    _Inout_ PCONDITION_VARIABLE ConditionVariable
    );

WINBASEAPI
BOOL
WINAPI
SleepConditionVariableCS(
    _Inout_ PCONDITION_VARIABLE ConditionVariable,
    _Inout_ PCRITICAL_SECTION CriticalSection,
    _In_ DWORD dwMilliseconds
    );

WINBASEAPI
BOOL
WINAPI
SleepConditionVariableSRW(
    _Inout_ PCONDITION_VARIABLE ConditionVariable,
    _Inout_ PSRWLOCK SRWLock,
    _In_ DWORD dwMilliseconds,
    _In_ ULONG Flags
    );

#endif // (_WIN32_WINNT >= 0x0600)






WINBASEAPI
DWORD
WINAPI
WaitForSingleObject(
    _In_ HANDLE hHandle,
    _In_ DWORD dwMilliseconds
    );

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjects(
    _In_ DWORD nCount,
    _In_reads_(nCount) CONST HANDLE* lpHandles,
    _In_ BOOL bWaitAll,
    _In_ DWORD dwMilliseconds
    );

WINBASEAPI
DWORD
WINAPI
WaitForSingleObjectEx(
    _In_ HANDLE hHandle,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    );

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjectsEx(
    _In_ DWORD nCount,
    _In_reads_(nCount) CONST HANDLE* lpHandles,
    _In_ BOOL bWaitAll,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    );



WINBASEAPI
DWORD
WINAPI
SleepEx(
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    );

WINBASEAPI
VOID
WINAPI
Sleep(
    _In_ DWORD dwMilliseconds
    );

EXTERN_C_END