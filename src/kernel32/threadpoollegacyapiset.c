#include "winbase.h"

#include "../ntdll/ntdll.h"


#define TAG_LEGACY_TIMER_QUEUE_TIMER 'tQlL'
#define TAG_LEGACY_REGISTERED_WAIT   'wRlL'

#define LDK_LEGACY_TIMER_QUEUE_TIMER_MAGIC 'tqlL'
#define LDK_LEGACY_REGISTERED_WAIT_MAGIC   'wrlL'

typedef struct _LDK_LEGACY_TIMER_QUEUE_TIMER {
    ULONG Magic;
    PTP_TIMER Timer;
    WAITORTIMERCALLBACK Callback;
    PVOID Context;
    ULONG Flags;
} LDK_LEGACY_TIMER_QUEUE_TIMER, *PLDK_LEGACY_TIMER_QUEUE_TIMER;

typedef struct _LDK_LEGACY_REGISTERED_WAIT {
    ULONG Magic;
    PTP_WAIT Wait;
    HANDLE WaitHandle;
    WAITORTIMERCALLBACK Callback;
    PVOID Context;
    ULONG TimeoutMilliseconds;
    ULONG Flags;
    FILETIME TimeoutFileTime;
    LONG DeleteRequested;
} LDK_LEGACY_REGISTERED_WAIT, *PLDK_LEGACY_REGISTERED_WAIT;

VOID
LdkpLegacyRelativeFileTime (
    _Out_ PFILETIME FileTime,
    _In_ ULONG Milliseconds
    )
{
    LARGE_INTEGER DueTime;

    DueTime.QuadPart = -((LONGLONG)Milliseconds * 10000);
    FileTime->dwLowDateTime = DueTime.LowPart;
    FileTime->dwHighDateTime = DueTime.HighPart;
}

VOID
NTAPI
LdkpLegacyTimerQueueTimerCallback (
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_TIMER Timer
    )
{
    PLDK_LEGACY_TIMER_QUEUE_TIMER LegacyTimer;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Timer);

    LegacyTimer = (PLDK_LEGACY_TIMER_QUEUE_TIMER)Context;
    if (LegacyTimer == NULL ||
        LegacyTimer->Magic != LDK_LEGACY_TIMER_QUEUE_TIMER_MAGIC) {
        return;
    }

    LegacyTimer->Callback( LegacyTimer->Context,
                           TRUE );
}

VOID
NTAPI
LdkpLegacyRegisteredWaitCallback (
    _Inout_ PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID Context,
    _Inout_ PTP_WAIT Wait,
    _In_ TP_WAIT_RESULT WaitResult
    )
{
    PLDK_LEGACY_REGISTERED_WAIT LegacyWait;
    PFILETIME TimeoutFileTime;

    UNREFERENCED_PARAMETER(Instance);

    LegacyWait = (PLDK_LEGACY_REGISTERED_WAIT)Context;
    if (LegacyWait == NULL ||
        LegacyWait->Magic != LDK_LEGACY_REGISTERED_WAIT_MAGIC) {
        return;
    }

    LegacyWait->Callback( LegacyWait->Context,
                          WaitResult == WAIT_TIMEOUT );

    if (FlagOn( LegacyWait->Flags,
                WT_EXECUTEONLYONCE ) ||
        InterlockedCompareExchange( &LegacyWait->DeleteRequested,
                                    0,
                                    0 ) != 0) {
        return;
    }

    TimeoutFileTime = (LegacyWait->TimeoutMilliseconds == INFINITE) ?
        NULL :
        &LegacyWait->TimeoutFileTime;
    SetThreadpoolWait( Wait,
                       LegacyWait->WaitHandle,
                       TimeoutFileTime );
}


WINBASEAPI
BOOL
WINAPI
QueueUserWorkItem (
    _In_ LPTHREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_ ULONG Flags
    )
{
    PAGED_CODE();

    NTSTATUS Status = RtlQueueWorkItem( (WORKERCALLBACKFUNC)Function,
                                        Context,
                                        Flags );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
CreateTimerQueueTimer (
    _Outptr_ PHANDLE phNewTimer,
    _In_opt_ HANDLE TimerQueue,
    _In_ WAITORTIMERCALLBACK Callback,
    _In_opt_ PVOID Parameter,
    _In_ DWORD DueTime,
    _In_ DWORD Period,
    _In_ ULONG Flags
    )
{
    PLDK_LEGACY_TIMER_QUEUE_TIMER LegacyTimer;
    FILETIME DueTimeFileTime;

    PAGED_CODE();

    if (phNewTimer == NULL ||
        Callback == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *phNewTimer = NULL;

    if (TimerQueue != NULL) {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

#pragma warning(suppress:4996)
    LegacyTimer = ExAllocatePoolWithTag( NonPagedPool,
                                         sizeof(*LegacyTimer),
                                         TAG_LEGACY_TIMER_QUEUE_TIMER );
    if (LegacyTimer == NULL) {
        LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
        return FALSE;
    }
    RtlZeroMemory( LegacyTimer,
                   sizeof(*LegacyTimer) );

    LegacyTimer->Magic = LDK_LEGACY_TIMER_QUEUE_TIMER_MAGIC;
    LegacyTimer->Callback = Callback;
    LegacyTimer->Context = Parameter;
    LegacyTimer->Flags = Flags;
    LegacyTimer->Timer = CreateThreadpoolTimer( LdkpLegacyTimerQueueTimerCallback,
                                                LegacyTimer,
                                                NULL );
    if (LegacyTimer->Timer == NULL) {
        ExFreePoolWithTag( LegacyTimer,
                           TAG_LEGACY_TIMER_QUEUE_TIMER );
        return FALSE;
    }

    LdkpLegacyRelativeFileTime( &DueTimeFileTime,
                                DueTime );
    SetThreadpoolTimer( LegacyTimer->Timer,
                        &DueTimeFileTime,
                        Period,
                        0 );

    *phNewTimer = (HANDLE)LegacyTimer;
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
DeleteTimerQueueTimer (
    _In_opt_ HANDLE TimerQueue,
    _In_ HANDLE Timer,
    _In_opt_ HANDLE CompletionEvent
    )
{
    PLDK_LEGACY_TIMER_QUEUE_TIMER LegacyTimer;

    PAGED_CODE();

    if (TimerQueue != NULL) {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    LegacyTimer = (PLDK_LEGACY_TIMER_QUEUE_TIMER)Timer;
    if (LegacyTimer == NULL ||
        LegacyTimer->Magic != LDK_LEGACY_TIMER_QUEUE_TIMER_MAGIC ||
        LegacyTimer->Timer == NULL) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    SetThreadpoolTimer( LegacyTimer->Timer,
                        NULL,
                        0,
                        0 );
    WaitForThreadpoolTimerCallbacks( LegacyTimer->Timer,
                                     TRUE );
    CloseThreadpoolTimer( LegacyTimer->Timer );
    LegacyTimer->Magic = 0;
    ExFreePoolWithTag( LegacyTimer,
                       TAG_LEGACY_TIMER_QUEUE_TIMER );

    if (CompletionEvent != NULL &&
        CompletionEvent != INVALID_HANDLE_VALUE) {
        SetEvent( CompletionEvent );
    }

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
RegisterWaitForSingleObject (
    _Outptr_ PHANDLE phNewWaitObject,
    _In_ HANDLE hObject,
    _In_ WAITORTIMERCALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_ ULONG dwMilliseconds,
    _In_ ULONG dwFlags
    )
{
    PLDK_LEGACY_REGISTERED_WAIT LegacyWait;
    PFILETIME TimeoutFileTime;
    PVOID WaitObject;
    NTSTATUS Status;

    PAGED_CODE();

    if (phNewWaitObject == NULL ||
        hObject == NULL ||
        Callback == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *phNewWaitObject = NULL;

    if (LdkIsProcessHandleCandidate( hObject )) {
        SetLastError( ERROR_NOT_SUPPORTED );
        return FALSE;
    }

    Status = ObReferenceObjectByHandle( hObject,
                                        SYNCHRONIZE,
                                        NULL,
                                        KernelMode,
                                        &WaitObject,
                                        NULL );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }
    ObDereferenceObject( WaitObject );

#pragma warning(suppress:4996)
    LegacyWait = ExAllocatePoolWithTag( NonPagedPool,
                                        sizeof(*LegacyWait),
                                        TAG_LEGACY_REGISTERED_WAIT );
    if (LegacyWait == NULL) {
        LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
        return FALSE;
    }
    RtlZeroMemory( LegacyWait,
                   sizeof(*LegacyWait) );

    LegacyWait->Magic = LDK_LEGACY_REGISTERED_WAIT_MAGIC;
    LegacyWait->WaitHandle = hObject;
    LegacyWait->Callback = Callback;
    LegacyWait->Context = Context;
    LegacyWait->TimeoutMilliseconds = dwMilliseconds;
    LegacyWait->Flags = dwFlags;
    if (dwMilliseconds != INFINITE) {
        LdkpLegacyRelativeFileTime( &LegacyWait->TimeoutFileTime,
                                    dwMilliseconds );
        TimeoutFileTime = &LegacyWait->TimeoutFileTime;
    } else {
        TimeoutFileTime = NULL;
    }

    LegacyWait->Wait = CreateThreadpoolWait( LdkpLegacyRegisteredWaitCallback,
                                             LegacyWait,
                                             NULL );
    if (LegacyWait->Wait == NULL) {
        ExFreePoolWithTag( LegacyWait,
                           TAG_LEGACY_REGISTERED_WAIT );
        return FALSE;
    }

    SetThreadpoolWait( LegacyWait->Wait,
                       hObject,
                       TimeoutFileTime );

    *phNewWaitObject = (HANDLE)LegacyWait;
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
UnregisterWaitEx (
    _In_ HANDLE WaitHandle,
    _In_opt_ HANDLE CompletionEvent
    )
{
    PLDK_LEGACY_REGISTERED_WAIT LegacyWait;

    PAGED_CODE();

    LegacyWait = (PLDK_LEGACY_REGISTERED_WAIT)WaitHandle;
    if (LegacyWait == NULL ||
        LegacyWait->Magic != LDK_LEGACY_REGISTERED_WAIT_MAGIC ||
        LegacyWait->Wait == NULL) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    InterlockedExchange( &LegacyWait->DeleteRequested,
                         1 );
    SetThreadpoolWait( LegacyWait->Wait,
                       NULL,
                       NULL );
    WaitForThreadpoolWaitCallbacks( LegacyWait->Wait,
                                    TRUE );
    CloseThreadpoolWait( LegacyWait->Wait );
    LegacyWait->Magic = 0;
    ExFreePoolWithTag( LegacyWait,
                       TAG_LEGACY_REGISTERED_WAIT );

    if (CompletionEvent != NULL &&
        CompletionEvent != INVALID_HANDLE_VALUE) {
        SetEvent( CompletionEvent );
    }

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
UnregisterWait (
    _In_ HANDLE WaitHandle
    )
{
    PAGED_CODE();

    return UnregisterWaitEx( WaitHandle,
                             NULL );
}
