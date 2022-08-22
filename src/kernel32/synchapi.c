#include "winbase.h"
#include "../ntdll/ntdll.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateEventA)
#pragma alloc_text(PAGE, CreateEventW)
#pragma alloc_text(PAGE, OpenEventA)
#pragma alloc_text(PAGE, OpenEventA)
#pragma alloc_text(PAGE, SetEvent)
#pragma alloc_text(PAGE, ResetEvent)
#pragma alloc_text(PAGE, PulseEvent)
#pragma alloc_text(PAGE, InitializeSRWLock)
#pragma alloc_text(PAGE, ReleaseSRWLockExclusive)
#pragma alloc_text(PAGE, ReleaseSRWLockShared)
#pragma alloc_text(PAGE, AcquireSRWLockExclusive)
#pragma alloc_text(PAGE, AcquireSRWLockShared)
#pragma alloc_text(PAGE, TryAcquireSRWLockExclusive)
#pragma alloc_text(PAGE, TryAcquireSRWLockShared)
#pragma alloc_text(PAGE, InitializeCriticalSection)
#pragma alloc_text(PAGE, EnterCriticalSection)
#pragma alloc_text(PAGE, TryEnterCriticalSection)
#pragma alloc_text(PAGE, LeaveCriticalSection)
#pragma alloc_text(PAGE, InitializeCriticalSectionEx)
#pragma alloc_text(PAGE, InitializeCriticalSectionAndSpinCount)
#pragma alloc_text(PAGE, DeleteCriticalSection)
#pragma alloc_text(PAGE, InitOnceInitialize)
#pragma alloc_text(PAGE, InitOnceExecuteOnce)
#pragma alloc_text(PAGE, InitOnceBeginInitialize)
#pragma alloc_text(PAGE, InitOnceComplete)
#pragma alloc_text(PAGE, InitializeConditionVariable)
#pragma alloc_text(PAGE, WakeConditionVariable)
#pragma alloc_text(PAGE, WakeAllConditionVariable)
#pragma alloc_text(PAGE, SleepConditionVariableCS)
#pragma alloc_text(PAGE, SleepConditionVariableSRW)
#pragma alloc_text(PAGE, WaitForSingleObject)
#pragma alloc_text(PAGE, WaitForMultipleObjects)
#pragma alloc_text(PAGE, WaitForSingleObjectEx)
#pragma alloc_text(PAGE, WaitForMultipleObjectsEx)
#pragma alloc_text(PAGE, Sleep)
#pragma alloc_text(PAGE, SleepEx)
#endif



WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateEventA (
    _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
    _In_ BOOL bManualReset,
    _In_ BOOL bInitialState,
    _In_opt_ LPCSTR lpName
    )
{
    PCWSTR lpNameW = NULL;

    PAGED_CODE();

    if (ARGUMENT_PRESENT(lpName)) {
        PUNICODE_STRING NameW = LdkAnsiStringToStaticUnicodeString( lpName );
        if (! NameW) {
            return NULL;
        }
        lpNameW = (PCWSTR)NameW->Buffer;
    }
	return CreateEventW( lpEventAttributes,
                         bManualReset,
                         bInitialState,
                         lpNameW );
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateEventW (
    _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
    _In_ BOOL bManualReset,
    _In_ BOOL bInitialState,
    _In_opt_ LPCWSTR lpName
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    
    PAGED_CODE();

    if (ARGUMENT_PRESENT(lpName)) {
        RtlInitUnicodeString( &Name,
                              lpName );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &Name,
                                    OBJ_OPENIF | OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );
    } else {
        InitializeObjectAttributes( &ObjectAttributes,
                                    NULL,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    NULL );
    }
	if (ARGUMENT_PRESENT(lpEventAttributes)) {
		ObjectAttributes.SecurityDescriptor = lpEventAttributes->lpSecurityDescriptor;
		if (lpEventAttributes->bInheritHandle) {
			SetFlag(ObjectAttributes.Attributes, OBJ_INHERIT);
		}
	}

    Status = ZwCreateEvent( &Handle,
                            EVENT_ALL_ACCESS,
                            &ObjectAttributes,
                            bManualReset ? NotificationEvent : SynchronizationEvent,
                            (BOOLEAN)bInitialState );

    if (NT_SUCCESS(Status)) {
        if (Status == STATUS_OBJECT_NAME_EXISTS) {
            SetLastError( ERROR_ALREADY_EXISTS );
        } else {
            SetLastError( ERROR_SUCCESS );
        }
        return Handle;
    }
    LdkSetLastNTError( Status );
    return NULL;
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenEventA (
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCSTR lpName
    )
{
    PUNICODE_STRING Name;

    PAGED_CODE();

    Name = LdkAnsiStringToStaticUnicodeString( lpName );
    if (Name == NULL) {
        return NULL;
    }
	return OpenEventW( dwDesiredAccess,
                       bInheritHandle,
                       (LPCWSTR)Name->Buffer );
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenEventW (
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCWSTR lpName
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    PAGED_CODE();

    RtlInitUnicodeString( &Name,
                          lpName );

    InitializeObjectAttributes( &ObjectAttributes,
                                &Name,
                                ((bInheritHandle ? OBJ_INHERIT : 0) | OBJ_KERNEL_HANDLE),
                                NULL,
                                NULL);

    Status = ZwOpenEvent( &Handle,
                          dwDesiredAccess,
                          &ObjectAttributes );

    if (NT_SUCCESS(Status)) {
        return Handle;
    }

    LdkSetLastNTError( Status );
    return NULL;
}

WINBASEAPI
BOOL
WINAPI
SetEvent (
    _In_ HANDLE hEvent
    )
{
    PAGED_CODE();

    NTSTATUS Status = ZwSetEvent( hEvent,
                                  NULL );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError(Status);
    return FALSE;
}


WINBASEAPI
BOOL
WINAPI
ResetEvent(
    _In_ HANDLE hEvent
    )
{
    PAGED_CODE();

    NTSTATUS Status = ZwClearEvent( hEvent );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError(Status);
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
PulseEvent (
    _In_ HANDLE hEvent
    )
{
	PAGED_CODE();

    NTSTATUS Status = ZwPulseEvent( hEvent,
									NULL );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError(Status);
    return FALSE;
}



WINBASEAPI
VOID
WINAPI
InitializeSRWLock (
    _Out_ PSRWLOCK SRWLock
    )
{
    PAGED_CODE();

    RtlInitializeSRWLock( SRWLock );
}

WINBASEAPI
_Releases_exclusive_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockExclusive (
    _Inout_ PSRWLOCK SRWLock
    )
{
    PAGED_CODE();

    RtlReleaseSRWLockExclusive( SRWLock );	
}

WINBASEAPI
_Releases_shared_lock_(*SRWLock)
VOID
WINAPI
ReleaseSRWLockShared (
    _Inout_ PSRWLOCK SRWLock
    )
{
    PAGED_CODE();

    RtlReleaseSRWLockShared( SRWLock );
}

WINBASEAPI
_Acquires_exclusive_lock_(*SRWLock)
VOID
WINAPI
AcquireSRWLockExclusive (
    _Inout_ PSRWLOCK SRWLock
    )
{
    PAGED_CODE();

    RtlAcquireSRWLockExclusive( SRWLock );
}

WINBASEAPI
_Acquires_shared_lock_(*SRWLock)
VOID
WINAPI
AcquireSRWLockShared (
    _Inout_ PSRWLOCK SRWLock
    )
{
    PAGED_CODE();

    RtlAcquireSRWLockShared( SRWLock );
}

WINBASEAPI
_When_(return!=0, _Acquires_exclusive_lock_(*SRWLock))
BOOLEAN
WINAPI
TryAcquireSRWLockExclusive (
    _Inout_ PSRWLOCK SRWLock
    )
{
    PAGED_CODE();

    return RtlTryAcquireSRWLockExclusive( SRWLock );
}

WINBASEAPI
_When_(return!=0, _Acquires_shared_lock_(*SRWLock))
BOOLEAN
WINAPI
TryAcquireSRWLockShared (
    _Inout_ PSRWLOCK SRWLock
    )
{
    PAGED_CODE();

    return RtlTryAcquireSRWLockShared( SRWLock );
}



#if (_WIN32_WINNT < 0x0600)
_Maybe_raises_SEH_exception_
WINBASEAPI
VOID
WINAPI
InitializeCriticalSection (
    _Out_ LPCRITICAL_SECTION lpCriticalSection
    )
#else
WINBASEAPI
VOID
WINAPI
InitializeCriticalSection (
    _Out_ LPCRITICAL_SECTION lpCriticalSection
    )
#endif  // (_WIN32_WINNT < 0x0600)
{
    PAGED_CODE();

    NTSTATUS Status;
    Status = RtlInitializeCriticalSection( lpCriticalSection );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
    }
}

WINBASEAPI
VOID
WINAPI
EnterCriticalSection (
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
    PAGED_CODE();

    NTSTATUS Status;
    Status = RtlEnterCriticalSection( lpCriticalSection );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
    }
}

WINBASEAPI
BOOL
WINAPI
TryEnterCriticalSection (
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
    PAGED_CODE();

    return (BOOL)RtlTryEnterCriticalSection( lpCriticalSection );
}

WINBASEAPI
VOID
WINAPI
LeaveCriticalSection (
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
    PAGED_CODE();

    NTSTATUS Status;
    Status = RtlLeaveCriticalSection( lpCriticalSection );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
    }
}

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
InitializeCriticalSectionEx (
    _Out_ LPCRITICAL_SECTION lpCriticalSection,
    _In_ DWORD dwSpinCount,
	_In_ DWORD dwFlags
    )
{
    PAGED_CODE();

	UNREFERENCED_PARAMETER(dwFlags);

    NTSTATUS Status;
    Status = RtlInitializeCriticalSectionEx( lpCriticalSection,
                                             dwSpinCount,
                                             dwFlags );
    if (NT_SUCCESS( Status )) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
InitializeCriticalSectionAndSpinCount (
    _Out_ LPCRITICAL_SECTION lpCriticalSection,
    _In_ DWORD dwSpinCount
    )
{
    PAGED_CODE();

    NTSTATUS Status;
    Status = RtlInitializeCriticalSectionAndSpinCount( lpCriticalSection,
                                                       dwSpinCount );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
VOID
WINAPI
DeleteCriticalSection (
    _Inout_ LPCRITICAL_SECTION lpCriticalSection
    )
{
    PAGED_CODE();

    NTSTATUS Status;
    Status = RtlDeleteCriticalSection( lpCriticalSection );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
    }
}



WINBASEAPI
VOID
WINAPI
InitOnceInitialize (
    _Out_ PINIT_ONCE InitOnce
    )
{
    PAGED_CODE();

    RtlRunOnceInitialize( InitOnce );
}

WINBASEAPI
BOOL
WINAPI
InitOnceExecuteOnce (
    _Inout_ PINIT_ONCE InitOnce,
    _In_ __callback PINIT_ONCE_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ LPVOID * Context
    )
{
    PAGED_CODE();

    NTSTATUS Status;
    try {
        Status = RtlRunOnceExecuteOnce( InitOnce,
                                        (PRTL_RUN_ONCE_INIT_FN)InitFn,
                                        Parameter,
                                        Context );
        if (NT_SUCCESS(Status)) {
            return TRUE;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
InitOnceBeginInitialize (
    _Inout_ LPINIT_ONCE lpInitOnce,
    _In_ DWORD dwFlags,
    _Out_ PBOOL fPending,
    _Outptr_opt_result_maybenull_ LPVOID* lpContext
    )
{
    PAGED_CODE();

    NTSTATUS Status = RtlRunOnceBeginInitialize( lpInitOnce,
                                                 dwFlags,
                                                 lpContext );
    if (NT_SUCCESS(Status)) {
        *fPending = (Status == STATUS_PENDING);
        return TRUE;
    }
    LdkSetLastNTError(Status);
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
InitOnceComplete (
    _Inout_ LPINIT_ONCE lpInitOnce,
    _In_ DWORD dwFlags,
    _In_opt_ LPVOID lpContext
    )
{
    PAGED_CODE();

    NTSTATUS Status = RtlRunOnceComplete( lpInitOnce,
                                          dwFlags,
                                          lpContext );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}



WINBASEAPI
VOID
WINAPI
InitializeConditionVariable (
    _Out_ PCONDITION_VARIABLE ConditionVariable
    )
{
    PAGED_CODE();

    RtlInitializeConditionVariable( ConditionVariable );
}

WINBASEAPI
VOID
WINAPI
WakeConditionVariable (
    _Inout_ PCONDITION_VARIABLE ConditionVariable
    )
{
    PAGED_CODE();

    RtlWakeConditionVariable( ConditionVariable );
}

WINBASEAPI
VOID
WINAPI
WakeAllConditionVariable (
    _Inout_ PCONDITION_VARIABLE ConditionVariable
    )
{
    PAGED_CODE();

    RtlWakeAllConditionVariable( ConditionVariable );
}

WINBASEAPI
BOOL
WINAPI
SleepConditionVariableCS (
    _Inout_ PCONDITION_VARIABLE ConditionVariable,
    _Inout_ PCRITICAL_SECTION CriticalSection,
    _In_ DWORD dwMilliseconds
    )
{
    PAGED_CODE();

    LARGE_INTEGER Timeout;
    NTSTATUS Status = RtlSleepConditionVariableCS( ConditionVariable,
                                                   CriticalSection,
                                                   LdkFormatTimeout( &Timeout,
                                                                     dwMilliseconds ) );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
SleepConditionVariableSRW (
    _Inout_ PCONDITION_VARIABLE ConditionVariable,
    _Inout_ PSRWLOCK SRWLock,
    _In_ DWORD dwMilliseconds,
    _In_ ULONG Flags
    )
{
    PAGED_CODE();

    LARGE_INTEGER Timeout;
    NTSTATUS Status = RtlSleepConditionVariableSRW( ConditionVariable,
                                                    SRWLock,
                                                    LdkFormatTimeout( &Timeout,
                                                                      dwMilliseconds ),
                                                    Flags );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}



WINBASEAPI
DWORD
WINAPI
WaitForSingleObject (
    _In_ HANDLE hHandle,
    _In_ DWORD dwMilliseconds
    )
{
    PAGED_CODE();

	return WaitForSingleObjectEx( hHandle,
                                  dwMilliseconds,
                                  FALSE );
}

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjects (
    _In_ DWORD nCount,
    _In_reads_(nCount) CONST HANDLE* lpHandles,
    _In_ BOOL bWaitAll,
    _In_ DWORD dwMilliseconds
    )
{
    PAGED_CODE();

    return WaitForMultipleObjectsEx( nCount,
                                     lpHandles,
                                     bWaitAll,
                                     dwMilliseconds,
                                     FALSE );
}

WINBASEAPI
DWORD
WINAPI
WaitForSingleObjectEx (
    _In_ HANDLE hHandle,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    )
{
    PAGED_CODE();

    NTSTATUS Status;
	LARGE_INTEGER Timeout;
    PLARGE_INTEGER pTimeout = LdkFormatTimeout( &Timeout,
                                                dwMilliseconds );

    do {
        Status = ZwWaitForSingleObject( hHandle,
                                        (BOOLEAN)bAlertable,
                                        pTimeout );
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            Status = WAIT_FAILED;
        }
    } while (bAlertable && (Status == STATUS_ALERTED));

	return (DWORD)Status;
}

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjectsEx (
    _In_ DWORD nCount,
    _In_reads_(nCount) CONST HANDLE* lpHandles,
    _In_ BOOL bWaitAll,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    )
{
    PAGED_CODE();

	LARGE_INTEGER Timeout;
    PLARGE_INTEGER pTimeout = LdkFormatTimeout( &Timeout,
                                                dwMilliseconds );
    HANDLE HandlesTmp[8];
    PHANDLE Handles;
    if (nCount > 8) {
#pragma warning(disable:4996)
        Handles = ExAllocatePoolWithTag( PagedPool,
                                         sizeof(HANDLE) * nCount,
                                         TAG_TMP_POOL );
#pragma warning(default:4996)
        if (!Handles) {
            LdkSetLastNTError(STATUS_NO_MEMORY);
            return WAIT_FAILED;
        }
    } else {
        Handles = HandlesTmp;
    }
    RtlCopyMemory( Handles,
                   lpHandles,
                   sizeof(HANDLE) * nCount);

    NTSTATUS Status;
    do {
        Status = ZwWaitForMultipleObjects( nCount,
                                           Handles,
                                           bWaitAll ? WaitAll : WaitAny,
                                           (BOOLEAN)bAlertable,
                                           pTimeout);
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            Status = WAIT_FAILED;
        }
    } while (bAlertable && (Status == STATUS_ALERTED));

    if (Handles != HandlesTmp) {
        ExFreePoolWithTag( Handles,
                           TAG_TMP_POOL );
    }
	return (DWORD)Status;
}



WINBASEAPI
VOID
WINAPI
Sleep (
    _In_ DWORD dwMilliseconds
    )
{
    PAGED_CODE();

	SleepEx( dwMilliseconds,
             FALSE );
}

WINBASEAPI
DWORD
WINAPI
SleepEx (
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    )
{
    PAGED_CODE();

	LARGE_INTEGER Timeout;
    PLARGE_INTEGER pTimeout = LdkFormatTimeout( &Timeout,
                                                dwMilliseconds );
	if (pTimeout == NULL) {
        Timeout.LowPart = 0x0;
        Timeout.HighPart = 0x80000000;
        pTimeout = &Timeout;
	}

    NTSTATUS Status;
	do {
        Status = KeDelayExecutionThread( KernelMode,
                                         (BOOLEAN)bAlertable,
                                         pTimeout );
	} while (bAlertable && (Status == STATUS_ALERTED));

	return Status == STATUS_USER_APC ? WAIT_IO_COMPLETION : 0;
}