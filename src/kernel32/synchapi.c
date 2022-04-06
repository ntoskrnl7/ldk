#include "winbase.h"

#include "../ntdll/ntdll.h"
#include "../nt/zwapi.h"

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
VOID
WINAPI
InitializeSRWLock(
    _Out_ PSRWLOCK SRWLock
    )
{
    RtlInitializeSRWLock(SRWLock);
}

WINBASEAPI
_Releases_exclusive_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    )
{
    RtlReleaseSRWLockExclusive(SRWLock);	
}

WINBASEAPI
_Releases_shared_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    )
{
    RtlReleaseSRWLockShared(SRWLock);
}

WINBASEAPI
_Acquires_exclusive_lock_(*SRWLock)
VOID
WINAPI
AcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    )
{
    RtlAcquireSRWLockExclusive(SRWLock);
}

WINBASEAPI
_Acquires_shared_lock_(*SRWLock)
VOID
WINAPI
AcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    )
{
    RtlAcquireSRWLockShared(SRWLock);
}

WINBASEAPI
_When_(return!=0, _Acquires_exclusive_lock_(*SRWLock))
BOOLEAN
WINAPI
TryAcquireSRWLockExclusive(
    _Inout_ PSRWLOCK SRWLock
    )
{
    return RtlTryAcquireSRWLockExclusive(SRWLock);
}

WINBASEAPI
_When_(return!=0, _Acquires_shared_lock_(*SRWLock))
BOOLEAN
WINAPI
TryAcquireSRWLockShared(
    _Inout_ PSRWLOCK SRWLock
    )
{
    return RtlTryAcquireSRWLockShared(SRWLock);
}
#endif // (_WIN32_WINNT >= 0x0600)



#if (_WIN32_WINNT < 0x0600)
_Maybe_raises_SEH_exception_
WINBASEAPI
VOID
WINAPI
InitializeCriticalSection(
    _Out_ LPCRITICAL_SECTION lpCriticalSection
    )
#else
WINBASEAPI
VOID
WINAPI
InitializeCriticalSection(
    _Out_ LPCRITICAL_SECTION lpCriticalSection
    )
#endif  // (_WIN32_WINNT < 0x0600)
{
    NTSTATUS status;
    status = RtlInitializeCriticalSection(lpCriticalSection);
    if (!NT_SUCCESS(status)) {
        BaseSetLastNTError(status);
    }
}

WINBASEAPI
VOID
WINAPI
EnterCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
    NTSTATUS status;
    status = RtlEnterCriticalSection(lpCriticalSection);
    if (!NT_SUCCESS(status)) {
        BaseSetLastNTError(status);
    }
}

WINBASEAPI
BOOL
WINAPI
TryEnterCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
    return (BOOL)RtlTryEnterCriticalSection(lpCriticalSection);
}

WINBASEAPI
VOID
WINAPI
LeaveCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
    NTSTATUS status;
    status = RtlLeaveCriticalSection(lpCriticalSection);
    if (!NT_SUCCESS(status)) {
        BaseSetLastNTError(status);
    }
}

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
InitializeCriticalSectionEx(
    _Out_ LPCRITICAL_SECTION lpCriticalSection,
    _In_ DWORD dwSpinCount,
	_In_ DWORD dwFlags
    )
{
	UNREFERENCED_PARAMETER(dwFlags);

    NTSTATUS status;
    status = RtlInitializeCriticalSectionEx(lpCriticalSection, dwSpinCount, dwFlags);
    if (NT_SUCCESS(status)) {
        return TRUE;
    }
    BaseSetLastNTError(status);
    return FALSE;
}

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
InitializeCriticalSectionAndSpinCount(
    _Out_ LPCRITICAL_SECTION lpCriticalSection,
    _In_ DWORD dwSpinCount
    )
{
    NTSTATUS status;
    status = RtlInitializeCriticalSectionAndSpinCount(lpCriticalSection, dwSpinCount);
    if (NT_SUCCESS(status)) {
        return TRUE;
    }
    BaseSetLastNTError(status);
    return FALSE;
}

WINBASEAPI
VOID
WINAPI
DeleteCriticalSection(
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
    NTSTATUS status;
    status = RtlDeleteCriticalSection(lpCriticalSection);
    if (!NT_SUCCESS(status)) {
        BaseSetLastNTError(status);
    }
}



WINBASEAPI
BOOL
WINAPI
InitOnceExecuteOnce(
    _Inout_ PINIT_ONCE InitOnce,
    _In_ __callback PINIT_ONCE_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ LPVOID * Context
    )
{
    NTSTATUS status;
    try {
        status = RtlRunOnceExecuteOnce(InitOnce, (PRTL_RUN_ONCE_INIT_FN)InitFn, Parameter, Context);
        if (NT_SUCCESS(status)) {
            return TRUE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        status = GetExceptionCode();
    }

    BaseSetLastNTError(status);
    return FALSE;
}



WINBASEAPI
VOID
WINAPI
InitializeConditionVariable(
    _Out_ PCONDITION_VARIABLE ConditionVariable
    )
{
    RtlInitializeConditionVariable(ConditionVariable);
}

WINBASEAPI
VOID
WINAPI
WakeConditionVariable(
    _Inout_ PCONDITION_VARIABLE ConditionVariable
    )
{
    RtlWakeConditionVariable(ConditionVariable);
}

WINBASEAPI
VOID
WINAPI
WakeAllConditionVariable(
    _Inout_ PCONDITION_VARIABLE ConditionVariable
    )
{
    RtlWakeAllConditionVariable(ConditionVariable);
}

WINBASEAPI
BOOL
WINAPI
SleepConditionVariableCS(
    _Inout_ PCONDITION_VARIABLE ConditionVariable,
    _Inout_ PCRITICAL_SECTION CriticalSection,
    _In_ DWORD dwMilliseconds
    )
{
    LARGE_INTEGER Timeout;

    NTSTATUS status;
    status = RtlSleepConditionVariableCS(ConditionVariable, CriticalSection, BaseFormatTimeout(&Timeout, dwMilliseconds));
    if (NT_SUCCESS(status)) {
        return TRUE;
    }
    BaseSetLastNTError(status);
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
SleepConditionVariableSRW(
    _Inout_ PCONDITION_VARIABLE ConditionVariable,
    _Inout_ PSRWLOCK SRWLock,
    _In_ DWORD dwMilliseconds,
    _In_ ULONG Flags
    )
{
    LARGE_INTEGER Timeout;

    NTSTATUS status;
    status = RtlSleepConditionVariableSRW(ConditionVariable, SRWLock, BaseFormatTimeout(&Timeout, dwMilliseconds), Flags);
    if (NT_SUCCESS(status)) {
        return TRUE;
    }
    BaseSetLastNTError(status);
    return FALSE;
}



WINBASEAPI
DWORD
WINAPI
WaitForSingleObject(
    _In_ HANDLE hHandle,
    _In_ DWORD dwMilliseconds
    )
{
	return WaitForSingleObjectEx(hHandle, dwMilliseconds, FALSE);
}

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjects(
    _In_ DWORD nCount,
    _In_reads_(nCount) CONST HANDLE* lpHandles,
    _In_ BOOL bWaitAll,
    _In_ DWORD dwMilliseconds
    )
{
    return WaitForMultipleObjectsEx(nCount, lpHandles, bWaitAll, dwMilliseconds, FALSE);
}

WINBASEAPI
DWORD
WINAPI
WaitForSingleObjectEx(
    _In_ HANDLE hHandle,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    )
{
	LARGE_INTEGER timeout;
    PLARGE_INTEGER pTimeout = BaseFormatTimeout(&timeout, dwMilliseconds);

    NTSTATUS status;
    do {
        status = ZwWaitForSingleObject(hHandle, (BOOLEAN)bAlertable, pTimeout);
        if (!NT_SUCCESS(status)) {
            BaseSetLastNTError(status);
            status = WAIT_FAILED;
        }
    } while (bAlertable && (status == STATUS_ALERTED));

	return (DWORD)status;
}

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjectsEx(
    _In_ DWORD nCount,
    _In_reads_(nCount) CONST HANDLE* lpHandles,
    _In_ BOOL bWaitAll,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    )
{
	LARGE_INTEGER timeout;
    PLARGE_INTEGER pTimeout = BaseFormatTimeout(&timeout, dwMilliseconds);
    HANDLE handlesTmp[8];
    PHANDLE handles;
    if (nCount > 8) {
        handles = ExAllocatePoolWithTag(PagedPool, sizeof(HANDLE) * nCount, TAG_TMP_POOL);
        if (!handles) {
            BaseSetLastNTError(STATUS_NO_MEMORY);
            return WAIT_FAILED;
        }
    } else {
        handles = handlesTmp;
    }
    RtlCopyMemory(handles, lpHandles, sizeof(HANDLE) * nCount);

    NTSTATUS status;
    do {
        status = ZwWaitForMultipleObjects(nCount, handles, bWaitAll ? WaitAll : WaitAny, (BOOLEAN)bAlertable, pTimeout);
        if (!NT_SUCCESS(status)) {
            BaseSetLastNTError(status);
            status = WAIT_FAILED;
        }
    } while (bAlertable && (status == STATUS_ALERTED));

    if (handles != handlesTmp) {
        ExFreePoolWithTag(handles, TAG_TMP_POOL);
    }
	return (DWORD)status;
}


WINBASEAPI
VOID
WINAPI
Sleep(
    _In_ DWORD dwMilliseconds
    )
{
	SleepEx(dwMilliseconds, FALSE);
}

WINBASEAPI
DWORD
WINAPI
SleepEx(
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    )
{
	LARGE_INTEGER Timeout;
    PLARGE_INTEGER pTimeout = BaseFormatTimeout(&Timeout, dwMilliseconds);
	if (!pTimeout) {
        Timeout.LowPart = 0x0;
        Timeout.HighPart = 0x80000000;
        pTimeout = &Timeout;
	}

    NTSTATUS status;
	do {
        status = KeDelayExecutionThread(KernelMode, (BOOLEAN)bAlertable, pTimeout);
	} while (bAlertable && (status == STATUS_ALERTED));

	return status == STATUS_USER_APC ? WAIT_IO_COMPLETION : 0;
}