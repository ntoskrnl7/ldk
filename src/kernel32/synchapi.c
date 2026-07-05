#include "winbase.h"
#include "../ntdll/ntdll.h"

#ifndef MAXIMUM_WAIT_OBJECTS
#define MAXIMUM_WAIT_OBJECTS 64
#endif

#ifndef THREAD_WAIT_OBJECTS
#define THREAD_WAIT_OBJECTS 3
#endif

NTSYSAPI
NTSTATUS
NTAPI
ZwCreateSemaphore(
    _Out_ PHANDLE SemaphoreHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ LONG InitialCount,
    _In_ LONG MaximumCount
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwReleaseSemaphore(
    _In_ HANDLE SemaphoreHandle,
    _In_ LONG ReleaseCount,
    _Out_opt_ PLONG PreviousCount
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateEventA)
#pragma alloc_text(PAGE, CreateEventW)
#pragma alloc_text(PAGE, CreateEventExW)
#pragma alloc_text(PAGE, OpenEventA)
#pragma alloc_text(PAGE, OpenEventA)
#pragma alloc_text(PAGE, SetEvent)
#pragma alloc_text(PAGE, ResetEvent)
#pragma alloc_text(PAGE, PulseEvent)
#pragma alloc_text(PAGE, CreateMutexA)
#pragma alloc_text(PAGE, CreateMutexW)
#pragma alloc_text(PAGE, CreateMutexExA)
#pragma alloc_text(PAGE, CreateMutexExW)
#pragma alloc_text(PAGE, OpenMutexA)
#pragma alloc_text(PAGE, OpenMutexW)
#pragma alloc_text(PAGE, ReleaseMutex)
#pragma alloc_text(PAGE, CreateSemaphoreExW)
#pragma alloc_text(PAGE, ReleaseSemaphore)
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
#pragma alloc_text(PAGE, SignalObjectAndWait)
#pragma alloc_text(PAGE, Sleep)
#pragma alloc_text(PAGE, SleepEx)
#pragma alloc_text(PAGE, WaitOnAddress)
#pragma alloc_text(PAGE, WakeByAddressSingle)
#pragma alloc_text(PAGE, WakeByAddressAll)
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
CreateEventExW (
    _In_opt_ LPSECURITY_ATTRIBUTES lpEventAttributes,
    _In_opt_ LPCWSTR lpName,
    _In_ DWORD dwFlags,
    _In_ DWORD dwDesiredAccess
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    DWORD ValidFlags = CREATE_EVENT_MANUAL_RESET | CREATE_EVENT_INITIAL_SET;

    PAGED_CODE();

    if (FlagOn(dwFlags, ~ValidFlags)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

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
                            (ACCESS_MASK)dwDesiredAccess,
                            &ObjectAttributes,
                            FlagOn(dwFlags, CREATE_EVENT_MANUAL_RESET) ?
                                NotificationEvent :
                                SynchronizationEvent,
                            (BOOLEAN)FlagOn(dwFlags, CREATE_EVENT_INITIAL_SET) );

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
_Ret_maybenull_
HANDLE
WINAPI
CreateMutexA (
    _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    _In_ BOOL bInitialOwner,
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

    return CreateMutexW( lpMutexAttributes,
                         bInitialOwner,
                         lpNameW );
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateMutexW (
    _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    _In_ BOOL bInitialOwner,
    _In_opt_ LPCWSTR lpName
    )
{
    PAGED_CODE();

    return CreateMutexExW( lpMutexAttributes,
                           lpName,
                           bInitialOwner ? CREATE_MUTEX_INITIAL_OWNER : 0,
                           MUTEX_ALL_ACCESS );
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateMutexExA (
    _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    _In_opt_ LPCSTR lpName,
    _In_ DWORD dwFlags,
    _In_ DWORD dwDesiredAccess
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

    return CreateMutexExW( lpMutexAttributes,
                           lpNameW,
                           dwFlags,
                           dwDesiredAccess );
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateMutexExW (
    _In_opt_ LPSECURITY_ATTRIBUTES lpMutexAttributes,
    _In_opt_ LPCWSTR lpName,
    _In_ DWORD dwFlags,
    _In_ DWORD dwDesiredAccess
    )
{
    BOOL InheritHandle;
    UNICODE_STRING Name;
    PCUNICODE_STRING NamePointer;
    HANDLE Handle;
    BOOLEAN AlreadyExists;

    PAGED_CODE();

    if ((dwFlags & ~CREATE_MUTEX_INITIAL_OWNER) != 0) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    NamePointer = NULL;
    if (ARGUMENT_PRESENT(lpName)) {
        RtlInitUnicodeString( &Name,
                              lpName );
        if (Name.Length != 0) {
            NamePointer = &Name;
        }
    }

    InheritHandle = ARGUMENT_PRESENT(lpMutexAttributes) &&
                    lpMutexAttributes->bInheritHandle;

    AlreadyExists = FALSE;
    Handle = LdkCreateMutantHandle( dwDesiredAccess,
                                    InheritHandle,
                                    (BOOLEAN)((dwFlags & CREATE_MUTEX_INITIAL_OWNER) != 0),
                                    NamePointer,
                                    &AlreadyExists );
    if (Handle != NULL) {
        SetLastError( AlreadyExists ? ERROR_ALREADY_EXISTS : ERROR_SUCCESS );
    }

    return Handle;
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenMutexA (
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCSTR lpName
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

    return OpenMutexW( dwDesiredAccess,
                       bInheritHandle,
                       lpNameW );
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenMutexW (
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ LPCWSTR lpName
    )
{
    UNICODE_STRING Name;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(lpName)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    RtlInitUnicodeString( &Name,
                          lpName );
    return LdkOpenMutantHandle( dwDesiredAccess,
                                bInheritHandle,
                                &Name );
}

WINBASEAPI
BOOL
WINAPI
ReleaseMutex (
    _In_ HANDLE hMutex
    )
{
    BOOL Handled;

    PAGED_CODE();

    if (LdkReleaseMutantHandle( hMutex,
                                &Handled )) {
        return TRUE;
    }

    if (! Handled) {
        SetLastError( ERROR_INVALID_HANDLE );
    }
    return FALSE;
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateSemaphoreExW (
    _In_opt_ LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    _In_ LONG lInitialCount,
    _In_ LONG lMaximumCount,
    _In_opt_ LPCWSTR lpName,
    _Reserved_ DWORD dwFlags,
    _In_ DWORD dwDesiredAccess
    )
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;

    PAGED_CODE();

    if (dwFlags != 0 ||
        lMaximumCount <= 0 ||
        lInitialCount < 0 ||
        lInitialCount > lMaximumCount) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

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
    if (ARGUMENT_PRESENT(lpSemaphoreAttributes)) {
        ObjectAttributes.SecurityDescriptor = lpSemaphoreAttributes->lpSecurityDescriptor;
        if (lpSemaphoreAttributes->bInheritHandle) {
            SetFlag(ObjectAttributes.Attributes, OBJ_INHERIT);
        }
    }

    Status = ZwCreateSemaphore( &Handle,
                                (ACCESS_MASK)dwDesiredAccess,
                                &ObjectAttributes,
                                lInitialCount,
                                lMaximumCount );
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
BOOL
WINAPI
ReleaseSemaphore (
    _In_ HANDLE hSemaphore,
    _In_ LONG lReleaseCount,
    _Out_opt_ LPLONG lpPreviousCount
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    Status = ZwReleaseSemaphore( hSemaphore,
                                 lReleaseCount,
                                 lpPreviousCount );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
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
    PVOID WaitObject;
    BOOLEAN IsLdkObject;

    Status = LdkReferenceProcessWaitObject( hHandle,
                                            &WaitObject,
                                            &IsLdkObject );
    if (NT_SUCCESS(Status)) {
        do {
            Status = KeWaitForSingleObject( WaitObject,
                                            Executive,
                                            KernelMode,
                                            (BOOLEAN)bAlertable,
                                            pTimeout );
            if (! NT_SUCCESS(Status)) {
                LdkSetLastNTError( Status );
                Status = WAIT_FAILED;
            }
        } while (bAlertable && (Status == STATUS_ALERTED));

        LdkDereferenceProcessWaitObject( WaitObject,
                                         IsLdkObject );
        return (DWORD)Status;
    }

    if (Status != STATUS_OBJECT_TYPE_MISMATCH) {
        LdkSetLastNTError( Status );
        return WAIT_FAILED;
    }

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

    if (nCount == 0 ||
        nCount > MAXIMUM_WAIT_OBJECTS) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return WAIT_FAILED;
    }

	LARGE_INTEGER Timeout;
    PLARGE_INTEGER pTimeout = LdkFormatTimeout( &Timeout,
                                                dwMilliseconds );
    BOOLEAN HasLdkHandle = FALSE;

    for (DWORD Index = 0; Index < nCount; Index++) {
        if (LdkIsProcessHandleCandidate( lpHandles[Index] )) {
            HasLdkHandle = TRUE;
            break;
        }
    }

    if (HasLdkHandle) {
        PVOID ObjectsTmp[8];
        BOOLEAN IsLdkObjectTmp[8];
        KWAIT_BLOCK WaitBlocksTmp[THREAD_WAIT_OBJECTS];
        PVOID *Objects = ObjectsTmp;
        PBOOLEAN IsLdkObject = IsLdkObjectTmp;
        PKWAIT_BLOCK WaitBlocks = WaitBlocksTmp;
        ULONG ReferencedCount = 0;
        NTSTATUS Status;

        if (nCount > RTL_NUMBER_OF(ObjectsTmp)) {
#pragma warning(disable:4996)
            Objects = ExAllocatePoolWithTag( NonPagedPool,
                                             sizeof(PVOID) * nCount,
                                             TAG_TMP_POOL );
            IsLdkObject = ExAllocatePoolWithTag( NonPagedPool,
                                                 sizeof(BOOLEAN) * nCount,
                                                 TAG_TMP_POOL );
#pragma warning(default:4996)
            if (Objects == NULL ||
                IsLdkObject == NULL) {
                if (Objects != NULL) {
                    ExFreePoolWithTag( Objects,
                                       TAG_TMP_POOL );
                }
                if (IsLdkObject != NULL) {
                    ExFreePoolWithTag( IsLdkObject,
                                       TAG_TMP_POOL );
                }
                LdkSetLastNTError( STATUS_NO_MEMORY );
                return WAIT_FAILED;
            }
        }

        if (nCount > THREAD_WAIT_OBJECTS) {
#pragma warning(disable:4996)
            WaitBlocks = ExAllocatePoolWithTag( NonPagedPool,
                                                sizeof(KWAIT_BLOCK) * nCount,
                                                TAG_TMP_POOL );
#pragma warning(default:4996)
            if (WaitBlocks == NULL) {
                if (Objects != ObjectsTmp) {
                    ExFreePoolWithTag( Objects,
                                       TAG_TMP_POOL );
                    ExFreePoolWithTag( IsLdkObject,
                                       TAG_TMP_POOL );
                }
                LdkSetLastNTError( STATUS_NO_MEMORY );
                return WAIT_FAILED;
            }
        }

        for (DWORD Index = 0; Index < nCount; Index++) {
            Status = LdkReferenceProcessWaitObject( lpHandles[Index],
                                                    &Objects[Index],
                                                    &IsLdkObject[Index] );
            if (Status == STATUS_OBJECT_TYPE_MISMATCH) {
                Status = ObReferenceObjectByHandle( lpHandles[Index],
                                                    SYNCHRONIZE,
                                                    NULL,
                                                    KernelMode,
                                                    &Objects[Index],
                                                    NULL );
                IsLdkObject[Index] = FALSE;
            }

            if (! NT_SUCCESS(Status)) {
                for (ULONG CleanupIndex = 0;
                     CleanupIndex < ReferencedCount;
                     CleanupIndex++) {
                    LdkDereferenceProcessWaitObject( Objects[CleanupIndex],
                                                     IsLdkObject[CleanupIndex] );
                }
                if (WaitBlocks != WaitBlocksTmp) {
                    ExFreePoolWithTag( WaitBlocks,
                                       TAG_TMP_POOL );
                }
                if (Objects != ObjectsTmp) {
                    ExFreePoolWithTag( Objects,
                                       TAG_TMP_POOL );
                    ExFreePoolWithTag( IsLdkObject,
                                       TAG_TMP_POOL );
                }
                LdkSetLastNTError( Status );
                return WAIT_FAILED;
            }

            ReferencedCount++;
        }

        do {
            Status = KeWaitForMultipleObjects( nCount,
                                               Objects,
                                               bWaitAll ? WaitAll : WaitAny,
                                               Executive,
                                               KernelMode,
                                               (BOOLEAN)bAlertable,
                                               pTimeout,
                                               WaitBlocks );
            if (! NT_SUCCESS(Status)) {
                LdkSetLastNTError( Status );
                Status = WAIT_FAILED;
            }
        } while (bAlertable && (Status == STATUS_ALERTED));

        for (ULONG Index = 0; Index < ReferencedCount; Index++) {
            LdkDereferenceProcessWaitObject( Objects[Index],
                                             IsLdkObject[Index] );
        }
        if (WaitBlocks != WaitBlocksTmp) {
            ExFreePoolWithTag( WaitBlocks,
                               TAG_TMP_POOL );
        }
        if (Objects != ObjectsTmp) {
            ExFreePoolWithTag( Objects,
                               TAG_TMP_POOL );
            ExFreePoolWithTag( IsLdkObject,
                               TAG_TMP_POOL );
        }

        return (DWORD)Status;
    }

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
DWORD
WINAPI
SignalObjectAndWait (
    _In_ HANDLE hObjectToSignal,
    _In_ HANDLE hObjectToWaitOn,
    _In_ DWORD dwMilliseconds,
    _In_ BOOL bAlertable
    )
{
    NTSTATUS Status;
    BOOL Handled;

    PAGED_CODE();

    if (LdkReleaseMutantHandle( hObjectToSignal,
                                &Handled )) {
        Status = STATUS_SUCCESS;
    } else if (Handled) {
        return WAIT_FAILED;
    } else {
        Status = ZwSetEvent( hObjectToSignal,
                             NULL );
    }
    if (Status == STATUS_OBJECT_TYPE_MISMATCH) {
        Status = ZwReleaseSemaphore( hObjectToSignal,
                                     1,
                                     NULL );
    }
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return WAIT_FAILED;
    }

    return WaitForSingleObjectEx( hObjectToWaitOn,
                                  dwMilliseconds,
                                  bAlertable );
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
BOOL
WINAPI
WaitOnAddress (
    _In_reads_bytes_(AddressSize) volatile VOID* Address,
    _In_reads_bytes_(AddressSize) PVOID CompareAddress,
    _In_ SIZE_T AddressSize,
    _In_opt_ DWORD dwMilliseconds
    )
{
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    PAGED_CODE();

    Status = RtlWaitOnAddress( Address,
                               CompareAddress,
                               AddressSize,
                               LdkFormatTimeout( &Timeout,
                                                 dwMilliseconds ) );
    if (Status == STATUS_TIMEOUT) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
VOID
WINAPI
WakeByAddressSingle (
    _In_ PVOID Address
    )
{
    PAGED_CODE();

    RtlWakeAddressSingle( Address );
}

WINBASEAPI
VOID
WINAPI
WakeByAddressAll (
    _In_ PVOID Address
    )
{
    PAGED_CODE();

    RtlWakeAddressAll( Address );
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
