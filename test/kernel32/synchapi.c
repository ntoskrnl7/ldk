#if _KERNEL_MODE
#include <Ldk/Windows.h>
#include <Ldk/ntdll.h>

BOOLEAN
WaitOnAddressTest (
    VOID
    );

DWORD
WINAPI
AlertByThreadIdTestThreadProc (
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
WaitOnAddressTestThreadProc (
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
AlertByThreadIdStressThreadProc (
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
AlertByThreadIdTimeoutStressThreadProc (
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
WaitOnAddressStressThreadProc (
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
WaitOnAddressTimeoutThreadProc (
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
RtlWaitOnAddressStressThreadProc (
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
RtlWaitOnAddressTimeoutThreadProc (
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
WaitOnAddressMixedThreadProc (
    LPVOID lpThreadParameter
    );

BOOLEAN
AlertByThreadIdStressTest (
    VOID
    );

BOOLEAN
NtWaitForAlertByThreadIdTimeoutStressTest (
    VOID
    );

BOOLEAN
WaitOnAddressWakeAllStressTest (
    VOID
    );

BOOLEAN
WaitOnAddressWakeSingleStressTest (
    VOID
    );

BOOLEAN
WaitOnAddressTimeoutStressTest (
    VOID
    );

BOOLEAN
RtlWaitOnAddressWakeAllStressTest (
    VOID
    );

BOOLEAN
RtlWaitOnAddressWakeSingleStressTest (
    VOID
    );

BOOLEAN
RtlWaitOnAddressTimeoutStressTest (
    VOID
    );

BOOLEAN
WaitOnAddressAddressIsolationStressTest (
    VOID
    );

BOOLEAN
WaitOnAddressRepeatedDurabilityStressTest (
    VOID
    );

BOOLEAN
WaitOnAddressSizeMatrixTest (
    VOID
    );

BOOLEAN
WaitOnAddressWaitForCounter (
    _In_ volatile LONG* Counter,
    _In_ LONG Expected,
    _In_ DWORD TimeoutMilliseconds
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, WaitOnAddressTest)
#pragma alloc_text(PAGE, AlertByThreadIdTestThreadProc)
#pragma alloc_text(PAGE, WaitOnAddressTestThreadProc)
#pragma alloc_text(PAGE, AlertByThreadIdStressThreadProc)
#pragma alloc_text(PAGE, AlertByThreadIdTimeoutStressThreadProc)
#pragma alloc_text(PAGE, WaitOnAddressStressThreadProc)
#pragma alloc_text(PAGE, WaitOnAddressTimeoutThreadProc)
#pragma alloc_text(PAGE, RtlWaitOnAddressStressThreadProc)
#pragma alloc_text(PAGE, RtlWaitOnAddressTimeoutThreadProc)
#pragma alloc_text(PAGE, WaitOnAddressMixedThreadProc)
#pragma alloc_text(PAGE, AlertByThreadIdStressTest)
#pragma alloc_text(PAGE, NtWaitForAlertByThreadIdTimeoutStressTest)
#pragma alloc_text(PAGE, WaitOnAddressWakeAllStressTest)
#pragma alloc_text(PAGE, WaitOnAddressWakeSingleStressTest)
#pragma alloc_text(PAGE, WaitOnAddressTimeoutStressTest)
#pragma alloc_text(PAGE, RtlWaitOnAddressWakeAllStressTest)
#pragma alloc_text(PAGE, RtlWaitOnAddressWakeSingleStressTest)
#pragma alloc_text(PAGE, RtlWaitOnAddressTimeoutStressTest)
#pragma alloc_text(PAGE, WaitOnAddressAddressIsolationStressTest)
#pragma alloc_text(PAGE, WaitOnAddressRepeatedDurabilityStressTest)
#pragma alloc_text(PAGE, WaitOnAddressSizeMatrixTest)
#pragma alloc_text(PAGE, WaitOnAddressWaitForCounter)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <stdio.h>
#include <winternl.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef STATUS_ALERTED
#define STATUS_ALERTED ((NTSTATUS)0x00000101)
#endif

#ifndef STATUS_TIMEOUT
#define STATUS_TIMEOUT ((NTSTATUS)0x00000102)
#endif

NTSYSAPI
NTSTATUS
NTAPI
NtAlertThreadByThreadId (
    _In_ HANDLE ThreadId
    );

NTSYSAPI
NTSTATUS
NTAPI
NtWaitForAlertByThreadId (
    _In_opt_ PVOID Address,
    _In_opt_ PLARGE_INTEGER Timeout
    );

#pragma comment(lib, "ntdll.lib")
#define PAGED_CODE()
#endif

#ifndef STATUS_INVALID_PARAMETER
#define STATUS_INVALID_PARAMETER ((NTSTATUS)0xC000000D)
#endif

typedef struct _ALERT_BY_THREAD_ID_TEST_CONTEXT {
    volatile LONG Ready;
    volatile LONG Woken;
    NTSTATUS WaitStatus;
    DWORD ThreadId;
} ALERT_BY_THREAD_ID_TEST_CONTEXT, *PALERT_BY_THREAD_ID_TEST_CONTEXT;

typedef struct _WAIT_ON_ADDRESS_TEST_CONTEXT {
    volatile LONG Value;
    volatile LONG ReadyCount;
    volatile LONG WakeCount;
} WAIT_ON_ADDRESS_TEST_CONTEXT, *PWAIT_ON_ADDRESS_TEST_CONTEXT;

#define ALERT_BY_THREAD_ID_STRESS_THREADS 16
#define ALERT_BY_THREAD_ID_TIMEOUT_STRESS_THREADS 8
#define WAIT_ON_ADDRESS_STRESS_THREADS 16
#define WAIT_ON_ADDRESS_SINGLE_STRESS_THREADS 8
#define WAIT_ON_ADDRESS_TIMEOUT_STRESS_THREADS 8
#define WAIT_ON_ADDRESS_MIXED_ADDRESS_COUNT 4
#define WAIT_ON_ADDRESS_MIXED_WAITERS_PER_ADDRESS 4
#define WAIT_ON_ADDRESS_MIXED_THREADS \
    (WAIT_ON_ADDRESS_MIXED_ADDRESS_COUNT * WAIT_ON_ADDRESS_MIXED_WAITERS_PER_ADDRESS)
#define WAIT_ON_ADDRESS_DURABILITY_ROUNDS 12
#define WAIT_ON_ADDRESS_DURABILITY_THREADS 8

typedef struct _ALERT_BY_THREAD_ID_STRESS_CONTEXT {
    volatile LONG ReadyCount;
    volatile LONG Start;
    volatile LONG WokenCount;
    volatile LONG TimeoutCount;
    volatile LONG FailedCount;
    volatile LONG AlertFailedCount;
    DWORD ThreadIds[ALERT_BY_THREAD_ID_STRESS_THREADS];
    NTSTATUS Statuses[ALERT_BY_THREAD_ID_STRESS_THREADS];
} ALERT_BY_THREAD_ID_STRESS_CONTEXT, *PALERT_BY_THREAD_ID_STRESS_CONTEXT;

typedef struct _ALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT {
    PALERT_BY_THREAD_ID_STRESS_CONTEXT Context;
    LONG Index;
} ALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT, *PALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT;

typedef struct _WAIT_ON_ADDRESS_STRESS_CONTEXT {
    volatile LONG Value;
    volatile LONG ReadyCount;
    volatile LONG WakeCount;
    volatile LONG TimeoutCount;
    volatile LONG FailedCount;
    NTSTATUS Statuses[WAIT_ON_ADDRESS_STRESS_THREADS];
    DWORD LastErrors[WAIT_ON_ADDRESS_STRESS_THREADS];
} WAIT_ON_ADDRESS_STRESS_CONTEXT, *PWAIT_ON_ADDRESS_STRESS_CONTEXT;

typedef struct _WAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT {
    PWAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    LONG Index;
} WAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT, *PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT;

typedef struct _WAIT_ON_ADDRESS_MIXED_CONTEXT {
    volatile LONG Values[WAIT_ON_ADDRESS_MIXED_ADDRESS_COUNT];
    volatile LONG ReadyCount;
    volatile LONG WakeCount;
    volatile LONG FailedCount;
    volatile LONG WakeCounts[WAIT_ON_ADDRESS_MIXED_ADDRESS_COUNT];
    NTSTATUS Statuses[WAIT_ON_ADDRESS_MIXED_THREADS];
    DWORD LastErrors[WAIT_ON_ADDRESS_MIXED_THREADS];
} WAIT_ON_ADDRESS_MIXED_CONTEXT, *PWAIT_ON_ADDRESS_MIXED_CONTEXT;

typedef struct _WAIT_ON_ADDRESS_MIXED_THREAD_CONTEXT {
    PWAIT_ON_ADDRESS_MIXED_CONTEXT Context;
    LONG Index;
    LONG AddressIndex;
    BOOLEAN UseRtl;
} WAIT_ON_ADDRESS_MIXED_THREAD_CONTEXT, *PWAIT_ON_ADDRESS_MIXED_THREAD_CONTEXT;

DWORD
WINAPI
AlertByThreadIdTestThreadProc (
    LPVOID lpThreadParameter
    )
{
    PALERT_BY_THREAD_ID_TEST_CONTEXT Context;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;

    PAGED_CODE();

    Context = (PALERT_BY_THREAD_ID_TEST_CONTEXT)lpThreadParameter;
    Context->ThreadId = GetCurrentThreadId();
    InterlockedExchange( (PLONG)&Context->Ready,
                         1 );

    Timeout.QuadPart = -1000 * 10000;
    Status = NtWaitForAlertByThreadId( NULL,
                                       &Timeout );
    Context->WaitStatus = Status;
    if (Status == STATUS_ALERTED) {
        InterlockedExchange( (PLONG)&Context->Woken,
                             1 );
        return 0;
    }

    return 1;
}

DWORD
WINAPI
WaitOnAddressTestThreadProc (
    LPVOID lpThreadParameter
    )
{
    PWAIT_ON_ADDRESS_TEST_CONTEXT Context;
    LONG Compare;

    PAGED_CODE();

    Context = (PWAIT_ON_ADDRESS_TEST_CONTEXT)lpThreadParameter;
    Compare = 0;

    InterlockedIncrement( (PLONG)&Context->ReadyCount );
    if (WaitOnAddress( &Context->Value,
                       &Compare,
                       sizeof(Context->Value),
                       1000 )) {
        InterlockedIncrement( (PLONG)&Context->WakeCount );
        return 0;
    }

    return 1;
}

DWORD
WINAPI
AlertByThreadIdStressThreadProc (
    LPVOID lpThreadParameter
    )
{
    PALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT ThreadContext;
    PALERT_BY_THREAD_ID_STRESS_CONTEXT Context;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    LONG Index;

    PAGED_CODE();

    ThreadContext = (PALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT)lpThreadParameter;
    Context = ThreadContext->Context;
    Index = ThreadContext->Index;

    Context->ThreadIds[Index] = GetCurrentThreadId();
    InterlockedIncrement( (PLONG)&Context->ReadyCount );

    while (! Context->Start) {
        YieldProcessor();
    }

    Timeout.QuadPart = -5000LL * 10000;
    Status = NtWaitForAlertByThreadId( NULL,
                                       &Timeout );
    Context->Statuses[Index] = Status;

    if (Status == STATUS_ALERTED) {
        InterlockedIncrement( (PLONG)&Context->WokenCount );
        return 0;
    }

    InterlockedIncrement( (PLONG)&Context->FailedCount );
    return 1;
}

DWORD
WINAPI
AlertByThreadIdTimeoutStressThreadProc (
    LPVOID lpThreadParameter
    )
{
    PALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT ThreadContext;
    PALERT_BY_THREAD_ID_STRESS_CONTEXT Context;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    LONG Index;

    PAGED_CODE();

    ThreadContext = (PALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT)lpThreadParameter;
    Context = ThreadContext->Context;
    Index = ThreadContext->Index;

    Context->ThreadIds[Index] = GetCurrentThreadId();
    InterlockedIncrement( (PLONG)&Context->ReadyCount );

    while (! Context->Start) {
        YieldProcessor();
    }

    Timeout.QuadPart = -20LL * 10000;
    Status = NtWaitForAlertByThreadId( NULL,
                                       &Timeout );
    Context->Statuses[Index] = Status;

    if (Status == STATUS_TIMEOUT) {
        InterlockedIncrement( (PLONG)&Context->TimeoutCount );
        return 0;
    }

    InterlockedIncrement( (PLONG)&Context->FailedCount );
    return 1;
}

DWORD
WINAPI
WaitOnAddressStressThreadProc (
    LPVOID lpThreadParameter
    )
{
    PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContext;
    PWAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    LONG Compare;
    LONG Index;

    PAGED_CODE();

    ThreadContext = (PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT)lpThreadParameter;
    Context = ThreadContext->Context;
    Index = ThreadContext->Index;
    Compare = 0;

    InterlockedIncrement( (PLONG)&Context->ReadyCount );
    if (WaitOnAddress( &Context->Value,
                       &Compare,
                       sizeof(Context->Value),
                       5000 )) {
        InterlockedIncrement( (PLONG)&Context->WakeCount );
        return 0;
    }

    Context->LastErrors[Index] = GetLastError();
    InterlockedIncrement( (PLONG)&Context->FailedCount );
    return 1;
}

DWORD
WINAPI
WaitOnAddressTimeoutThreadProc (
    LPVOID lpThreadParameter
    )
{
    PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContext;
    PWAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    LONG Compare;
    LONG Index;

    PAGED_CODE();

    ThreadContext = (PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT)lpThreadParameter;
    Context = ThreadContext->Context;
    Index = ThreadContext->Index;
    Compare = 0;

    InterlockedIncrement( (PLONG)&Context->ReadyCount );
    if (! WaitOnAddress( &Context->Value,
                         &Compare,
                         sizeof(Context->Value),
                         20 )) {
        Context->LastErrors[Index] = GetLastError();
        InterlockedIncrement( (PLONG)&Context->TimeoutCount );
        return 0;
    }

    InterlockedIncrement( (PLONG)&Context->FailedCount );
    return 1;
}

DWORD
WINAPI
RtlWaitOnAddressStressThreadProc (
    LPVOID lpThreadParameter
    )
{
    PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContext;
    PWAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    LONG Compare;
    LONG Index;

    PAGED_CODE();

    ThreadContext = (PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT)lpThreadParameter;
    Context = ThreadContext->Context;
    Index = ThreadContext->Index;
    Compare = 0;

    InterlockedIncrement( (PLONG)&Context->ReadyCount );
    Timeout.QuadPart = -5000LL * 10000;
    Status = RtlWaitOnAddress( &Context->Value,
                               &Compare,
                               sizeof(Context->Value),
                               &Timeout );
    Context->Statuses[Index] = Status;
    if (Status == STATUS_SUCCESS) {
        InterlockedIncrement( (PLONG)&Context->WakeCount );
        return 0;
    }

    InterlockedIncrement( (PLONG)&Context->FailedCount );
    return 1;
}

DWORD
WINAPI
RtlWaitOnAddressTimeoutThreadProc (
    LPVOID lpThreadParameter
    )
{
    PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContext;
    PWAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    LONG Compare;
    LONG Index;

    PAGED_CODE();

    ThreadContext = (PWAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT)lpThreadParameter;
    Context = ThreadContext->Context;
    Index = ThreadContext->Index;
    Compare = 0;

    InterlockedIncrement( (PLONG)&Context->ReadyCount );
    Timeout.QuadPart = -20LL * 10000;
    Status = RtlWaitOnAddress( &Context->Value,
                               &Compare,
                               sizeof(Context->Value),
                               &Timeout );
    Context->Statuses[Index] = Status;

    if (Status == STATUS_TIMEOUT) {
        InterlockedIncrement( (PLONG)&Context->TimeoutCount );
        return 0;
    }

    InterlockedIncrement( (PLONG)&Context->FailedCount );
    return 1;
}

DWORD
WINAPI
WaitOnAddressMixedThreadProc (
    LPVOID lpThreadParameter
    )
{
    PWAIT_ON_ADDRESS_MIXED_THREAD_CONTEXT ThreadContext;
    PWAIT_ON_ADDRESS_MIXED_CONTEXT Context;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    LONG Compare;
    LONG Index;
    LONG AddressIndex;

    PAGED_CODE();

    ThreadContext = (PWAIT_ON_ADDRESS_MIXED_THREAD_CONTEXT)lpThreadParameter;
    Context = ThreadContext->Context;
    Index = ThreadContext->Index;
    AddressIndex = ThreadContext->AddressIndex;
    Compare = 0;

    InterlockedIncrement( (PLONG)&Context->ReadyCount );

    if (ThreadContext->UseRtl) {
        Timeout.QuadPart = -3000LL * 10000;
        Status = RtlWaitOnAddress( &Context->Values[AddressIndex],
                                   &Compare,
                                   sizeof(Context->Values[AddressIndex]),
                                   &Timeout );
        Context->Statuses[Index] = Status;
        if (Status != STATUS_SUCCESS) {
            InterlockedIncrement( (PLONG)&Context->FailedCount );
            return 1;
        }
    } else {
        if (! WaitOnAddress( &Context->Values[AddressIndex],
                             &Compare,
                             sizeof(Context->Values[AddressIndex]),
                             3000 )) {
            Context->LastErrors[Index] = GetLastError();
            InterlockedIncrement( (PLONG)&Context->FailedCount );
            return 1;
        }
        Context->Statuses[Index] = STATUS_SUCCESS;
    }

    InterlockedIncrement( (PLONG)&Context->WakeCounts[AddressIndex] );
    InterlockedIncrement( (PLONG)&Context->WakeCount );
    return 0;
}

BOOLEAN
WaitOnAddressWaitForCounter (
    _In_ volatile LONG* Counter,
    _In_ LONG Expected,
    _In_ DWORD TimeoutMilliseconds
    )
{
    DWORD Elapsed;

    PAGED_CODE();

    for (Elapsed = 0; Elapsed < TimeoutMilliseconds; Elapsed++) {
        if (*Counter == Expected) {
            return TRUE;
        }
        Sleep( 1 );
    }

    return (*Counter == Expected);
}

BOOLEAN
AlertByThreadIdStressTest (
    VOID
    )
{
    ALERT_BY_THREAD_ID_STRESS_CONTEXT Context;
    ALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT ThreadContexts[ALERT_BY_THREAD_ID_STRESS_THREADS];
    HANDLE Threads[ALERT_BY_THREAD_ID_STRESS_THREADS] = { NULL, };
    DWORD WaitResult;
    NTSTATUS Status;
    LONG Index;
    LONG CreatedCount = 0;
    LONG FirstBadIndex = -1;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       AlertByThreadIdStressThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] Alert stress CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    while (Context.ReadyCount < CreatedCount) {
        YieldProcessor();
    }

    for (Index = 0; Index < CreatedCount; Index += 2) {
        Status = NtAlertThreadByThreadId( (HANDLE)(ULONG_PTR)Context.ThreadIds[Index] );
        if (! NT_SUCCESS(Status)) {
            Context.Statuses[Index] = Status;
            InterlockedIncrement( (PLONG)&Context.AlertFailedCount );
        }
    }

    InterlockedExchange( (PLONG)&Context.Start,
                         1 );
    Sleep( 20 );

    for (Index = 1; Index < CreatedCount; Index += 2) {
        Status = NtAlertThreadByThreadId( (HANDLE)(ULONG_PTR)Context.ThreadIds[Index] );
        if (! NT_SUCCESS(Status)) {
            Context.Statuses[Index] = Status;
            InterlockedIncrement( (PLONG)&Context.AlertFailedCount );
        }
    }

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         7000 );

    for (Index = 0; Index < CreatedCount; Index++) {
        if (Context.Statuses[Index] != STATUS_ALERTED &&
            FirstBadIndex < 0) {
            FirstBadIndex = Index;
        }
    }

    if (WaitResult != WAIT_OBJECT_0 ||
        Context.WokenCount != CreatedCount ||
        Context.FailedCount != 0 ||
        Context.AlertFailedCount != 0) {
        fprintf(stderr,
                "[Failed] Alert stress Wait = 0x%08x Ready = %ld Woken = %ld Failed = %ld AlertFailed = %ld FirstBadIndex = %ld FirstBadStatus = 0x%08x\n",
                WaitResult,
                Context.ReadyCount,
                Context.WokenCount,
                Context.FailedCount,
                Context.AlertFailedCount,
                FirstBadIndex,
                FirstBadIndex >= 0 ? Context.Statuses[FirstBadIndex] : STATUS_SUCCESS);
        for (Index = 0; Index < CreatedCount; Index++) {
            if (Context.Statuses[Index] != STATUS_ALERTED) {
                fprintf(stderr,
                        "[Failed] AlertStressBad I=%ld S=0x%08x TID=%lu\n",
                        Index,
                        Context.Statuses[Index],
                        Context.ThreadIds[Index]);
            }
        }
        goto Cleanup;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] Alert stress\n\n");
    return TRUE;

Cleanup:
    InterlockedExchange( (PLONG)&Context.Start,
                         1 );
    for (Index = 0; Index < CreatedCount; Index++) {
        if (Context.ThreadIds[Index]) {
            NtAlertThreadByThreadId( (HANDLE)(ULONG_PTR)Context.ThreadIds[Index] );
        }
    }
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
NtWaitForAlertByThreadIdTimeoutStressTest (
    VOID
    )
{
    ALERT_BY_THREAD_ID_STRESS_CONTEXT Context;
    ALERT_BY_THREAD_ID_STRESS_THREAD_CONTEXT ThreadContexts[ALERT_BY_THREAD_ID_TIMEOUT_STRESS_THREADS];
    HANDLE Threads[ALERT_BY_THREAD_ID_TIMEOUT_STRESS_THREADS] = { NULL, };
    DWORD WaitResult;
    LONG Index;
    LONG CreatedCount = 0;
    LONG FirstBadIndex = -1;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       AlertByThreadIdTimeoutStressThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] NtWait timeout stress CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    while (Context.ReadyCount < CreatedCount) {
        YieldProcessor();
    }

    InterlockedExchange( (PLONG)&Context.Start,
                         1 );

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         7000 );

    for (Index = 0; Index < CreatedCount; Index++) {
        if (Context.Statuses[Index] != STATUS_TIMEOUT &&
            FirstBadIndex < 0) {
            FirstBadIndex = Index;
        }
    }

    if (WaitResult != WAIT_OBJECT_0 ||
        Context.TimeoutCount != CreatedCount ||
        Context.FailedCount != 0) {
        fprintf(stderr,
                "[Failed] NtWait timeout stress Wait = 0x%08x Ready = %ld Timeout = %ld Failed = %ld FirstBadIndex = %ld FirstBadStatus = 0x%08x\n",
                WaitResult,
                Context.ReadyCount,
                Context.TimeoutCount,
                Context.FailedCount,
                FirstBadIndex,
                FirstBadIndex >= 0 ? Context.Statuses[FirstBadIndex] : STATUS_SUCCESS);
        goto Cleanup;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] NtWait timeout stress\n\n");
    return TRUE;

Cleanup:
    InterlockedExchange( (PLONG)&Context.Start,
                         1 );
    for (Index = 0; Index < CreatedCount; Index++) {
        if (Context.ThreadIds[Index]) {
            NtAlertThreadByThreadId( (HANDLE)(ULONG_PTR)Context.ThreadIds[Index] );
        }
    }
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
WaitOnAddressWakeAllStressTest (
    VOID
    )
{
    WAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    WAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContexts[WAIT_ON_ADDRESS_STRESS_THREADS];
    HANDLE Threads[WAIT_ON_ADDRESS_STRESS_THREADS] = { NULL, };
    DWORD WaitResult;
    LONG Index;
    LONG CreatedCount = 0;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       WaitOnAddressStressThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] WaitOnAddress wake-all stress CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    while (Context.ReadyCount < CreatedCount) {
        YieldProcessor();
    }

    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    WakeByAddressAll( (PVOID)&Context.Value );

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         7000 );

    if (WaitResult != WAIT_OBJECT_0 ||
        Context.WakeCount != CreatedCount ||
        Context.FailedCount != 0) {
        fprintf(stderr,
                "[Failed] WaitOnAddress wake-all stress Wait = 0x%08x Ready = %ld Wake = %ld Failed = %ld FirstError = %lu Value = %ld\n",
                WaitResult,
                Context.ReadyCount,
                Context.WakeCount,
                Context.FailedCount,
                Context.LastErrors[0],
                Context.Value);
        goto Cleanup;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] WaitOnAddress wake-all stress\n\n");
    return TRUE;

Cleanup:
    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    WakeByAddressAll( (PVOID)&Context.Value );
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
WaitOnAddressWakeSingleStressTest (
    VOID
    )
{
    WAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    WAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContexts[WAIT_ON_ADDRESS_SINGLE_STRESS_THREADS];
    HANDLE Threads[WAIT_ON_ADDRESS_SINGLE_STRESS_THREADS] = { NULL, };
    DWORD WaitResult;
    BOOLEAN SingleCompleted;
    LONG Attempt;
    LONG Index;
    LONG CreatedCount = 0;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       WaitOnAddressStressThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] WaitOnAddress wake-single stress CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    while (Context.ReadyCount < CreatedCount) {
        YieldProcessor();
    }
    Sleep( 20 );

    for (Attempt = 0;
         Attempt < 10000 && Context.WakeCount < CreatedCount;
         Attempt++) {
        WakeByAddressSingle( (PVOID)&Context.Value );
        if ((Attempt % 64) == 0) {
            Sleep( 1 );
        } else {
            YieldProcessor();
        }
    }

    SingleCompleted = (Context.WakeCount == CreatedCount);
    if (! SingleCompleted) {
        InterlockedExchange( (PLONG)&Context.Value,
                             1 );
        WakeByAddressAll( (PVOID)&Context.Value );
    }

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         7000 );

    if (! SingleCompleted ||
        WaitResult != WAIT_OBJECT_0 ||
        Context.WakeCount != CreatedCount ||
        Context.FailedCount != 0) {
        fprintf(stderr,
                "[Failed] WaitOnAddress wake-single stress Wait = 0x%08x SingleCompleted = %d Ready = %ld Wake = %ld Failed = %ld Attempts = %ld FirstError = %lu Value = %ld\n",
                WaitResult,
                SingleCompleted,
                Context.ReadyCount,
                Context.WakeCount,
                Context.FailedCount,
                Attempt,
                Context.LastErrors[0],
                Context.Value);
        goto CleanupCloseOnly;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] WaitOnAddress wake-single stress\n\n");
    return TRUE;

Cleanup:
    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    WakeByAddressAll( (PVOID)&Context.Value );
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
CleanupCloseOnly:
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
WaitOnAddressTimeoutStressTest (
    VOID
    )
{
    WAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    WAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContexts[WAIT_ON_ADDRESS_TIMEOUT_STRESS_THREADS];
    HANDLE Threads[WAIT_ON_ADDRESS_TIMEOUT_STRESS_THREADS] = { NULL, };
    DWORD WaitResult;
    LONG Index;
    LONG CreatedCount = 0;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       WaitOnAddressTimeoutThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] WaitOnAddress timeout stress CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         7000 );

    if (WaitResult != WAIT_OBJECT_0 ||
        Context.TimeoutCount != CreatedCount ||
        Context.FailedCount != 0) {
        fprintf(stderr,
                "[Failed] WaitOnAddress timeout stress Wait = 0x%08x Ready = %ld Timeout = %ld Failed = %ld FirstError = %lu Value = %ld\n",
                WaitResult,
                Context.ReadyCount,
                Context.TimeoutCount,
                Context.FailedCount,
                Context.LastErrors[0],
                Context.Value);
        goto Cleanup;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] WaitOnAddress timeout stress\n\n");
    return TRUE;

Cleanup:
    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    WakeByAddressAll( (PVOID)&Context.Value );
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
RtlWaitOnAddressWakeAllStressTest (
    VOID
    )
{
    WAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    WAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContexts[WAIT_ON_ADDRESS_STRESS_THREADS];
    HANDLE Threads[WAIT_ON_ADDRESS_STRESS_THREADS] = { NULL, };
    DWORD WaitResult;
    LONG Index;
    LONG CreatedCount = 0;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       RtlWaitOnAddressStressThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] RtlWaitOnAddress stress CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    while (Context.ReadyCount < CreatedCount) {
        YieldProcessor();
    }

    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    RtlWakeAddressAll( (PVOID)&Context.Value );

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         7000 );

    if (WaitResult != WAIT_OBJECT_0 ||
        Context.WakeCount != CreatedCount ||
        Context.FailedCount != 0) {
        fprintf(stderr,
                "[Failed] RtlWaitOnAddress stress Wait = 0x%08x Ready = %ld Wake = %ld Failed = %ld FirstStatus = 0x%08x Value = %ld\n",
                WaitResult,
                Context.ReadyCount,
                Context.WakeCount,
                Context.FailedCount,
                Context.Statuses[0],
                Context.Value);
        goto Cleanup;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] RtlWaitOnAddress wake-all stress\n\n");
    return TRUE;

Cleanup:
    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    RtlWakeAddressAll( (PVOID)&Context.Value );
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
RtlWaitOnAddressWakeSingleStressTest (
    VOID
    )
{
    WAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    WAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContexts[WAIT_ON_ADDRESS_SINGLE_STRESS_THREADS];
    HANDLE Threads[WAIT_ON_ADDRESS_SINGLE_STRESS_THREADS] = { NULL, };
    DWORD WaitResult;
    BOOLEAN SingleCompleted;
    LONG Attempt;
    LONG Index;
    LONG CreatedCount = 0;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       RtlWaitOnAddressStressThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] RtlWaitOnAddress wake-single stress CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    while (Context.ReadyCount < CreatedCount) {
        YieldProcessor();
    }
    Sleep( 20 );

    for (Attempt = 0;
         Attempt < 10000 && Context.WakeCount < CreatedCount;
         Attempt++) {
        RtlWakeAddressSingle( (PVOID)&Context.Value );
        if ((Attempt % 64) == 0) {
            Sleep( 1 );
        } else {
            YieldProcessor();
        }
    }

    SingleCompleted = (Context.WakeCount == CreatedCount);
    if (! SingleCompleted) {
        InterlockedExchange( (PLONG)&Context.Value,
                             1 );
        RtlWakeAddressAll( (PVOID)&Context.Value );
    }

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         7000 );

    if (! SingleCompleted ||
        WaitResult != WAIT_OBJECT_0 ||
        Context.WakeCount != CreatedCount ||
        Context.FailedCount != 0) {
        fprintf(stderr,
                "[Failed] RtlWaitOnAddress wake-single stress Wait = 0x%08x SingleCompleted = %d Ready = %ld Wake = %ld Failed = %ld Attempts = %ld FirstStatus = 0x%08x Value = %ld\n",
                WaitResult,
                SingleCompleted,
                Context.ReadyCount,
                Context.WakeCount,
                Context.FailedCount,
                Attempt,
                Context.Statuses[0],
                Context.Value);
        goto CleanupCloseOnly;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] RtlWaitOnAddress wake-single stress\n\n");
    return TRUE;

Cleanup:
    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    RtlWakeAddressAll( (PVOID)&Context.Value );
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
CleanupCloseOnly:
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
RtlWaitOnAddressTimeoutStressTest (
    VOID
    )
{
    WAIT_ON_ADDRESS_STRESS_CONTEXT Context;
    WAIT_ON_ADDRESS_STRESS_THREAD_CONTEXT ThreadContexts[WAIT_ON_ADDRESS_TIMEOUT_STRESS_THREADS];
    HANDLE Threads[WAIT_ON_ADDRESS_TIMEOUT_STRESS_THREADS] = { NULL, };
    DWORD WaitResult;
    LONG Index;
    LONG CreatedCount = 0;
    LONG FirstBadIndex = -1;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       RtlWaitOnAddressTimeoutThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] RtlWaitOnAddress timeout stress CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         7000 );

    for (Index = 0; Index < CreatedCount; Index++) {
        if (Context.Statuses[Index] != STATUS_TIMEOUT &&
            FirstBadIndex < 0) {
            FirstBadIndex = Index;
        }
    }

    if (WaitResult != WAIT_OBJECT_0 ||
        Context.TimeoutCount != CreatedCount ||
        Context.FailedCount != 0) {
        fprintf(stderr,
                "[Failed] RtlWaitOnAddress timeout stress Wait = 0x%08x Ready = %ld Timeout = %ld Failed = %ld FirstBadIndex = %ld FirstBadStatus = 0x%08x Value = %ld\n",
                WaitResult,
                Context.ReadyCount,
                Context.TimeoutCount,
                Context.FailedCount,
                FirstBadIndex,
                FirstBadIndex >= 0 ? Context.Statuses[FirstBadIndex] : STATUS_SUCCESS,
                Context.Value);
        goto Cleanup;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] RtlWaitOnAddress timeout stress\n\n");
    return TRUE;

Cleanup:
    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    RtlWakeAddressAll( (PVOID)&Context.Value );
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
WaitOnAddressAddressIsolationStressTest (
    VOID
    )
{
    WAIT_ON_ADDRESS_MIXED_CONTEXT Context;
    WAIT_ON_ADDRESS_MIXED_THREAD_CONTEXT ThreadContexts[WAIT_ON_ADDRESS_MIXED_THREADS];
    HANDLE Threads[WAIT_ON_ADDRESS_MIXED_THREADS] = { NULL, };
    DWORD WaitResult;
    LONG AddressIndex;
    LONG CreatedCount = 0;
    LONG FirstBadIndex = -1;
    LONG Index;
    LONG OtherAddressIndex;

    PAGED_CODE();

    RtlZeroMemory( &Context,
                   sizeof(Context) );

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        ThreadContexts[Index].Context = &Context;
        ThreadContexts[Index].Index = Index;
        ThreadContexts[Index].AddressIndex = Index % WAIT_ON_ADDRESS_MIXED_ADDRESS_COUNT;
        ThreadContexts[Index].UseRtl = (BOOLEAN)((Index & 1) != 0);
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       WaitOnAddressMixedThreadProc,
                                       &ThreadContexts[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] WaitOnAddress address-isolation CreateThread[%ld] ErrorCode = %d\n",
                    Index,
                    GetLastError());
            goto Cleanup;
        }
        CreatedCount++;
    }

    while (Context.ReadyCount < CreatedCount) {
        YieldProcessor();
    }
    Sleep( 20 );

    for (AddressIndex = 0;
         AddressIndex < WAIT_ON_ADDRESS_MIXED_ADDRESS_COUNT;
         AddressIndex++) {
        for (OtherAddressIndex = AddressIndex + 1;
             OtherAddressIndex < WAIT_ON_ADDRESS_MIXED_ADDRESS_COUNT;
             OtherAddressIndex++) {
            if (Context.WakeCounts[OtherAddressIndex] != 0) {
                fprintf(stderr,
                        "[Failed] WaitOnAddress address-isolation early wake Address = %ld OtherAddress = %ld OtherWake = %ld\n",
                        AddressIndex,
                        OtherAddressIndex,
                        Context.WakeCounts[OtherAddressIndex]);
                goto Cleanup;
            }
        }

        InterlockedExchange( (PLONG)&Context.Values[AddressIndex],
                             1 );
        if ((AddressIndex & 1) == 0) {
            WakeByAddressAll( (PVOID)&Context.Values[AddressIndex] );
        } else {
            RtlWakeAddressAll( (PVOID)&Context.Values[AddressIndex] );
        }

        if (! WaitOnAddressWaitForCounter( &Context.WakeCounts[AddressIndex],
                                           WAIT_ON_ADDRESS_MIXED_WAITERS_PER_ADDRESS,
                                           1000 )) {
            fprintf(stderr,
                    "[Failed] WaitOnAddress address-isolation wake Address = %ld Wake = %ld Expected = %ld TotalWake = %ld Failed = %ld\n",
                    AddressIndex,
                    Context.WakeCounts[AddressIndex],
                    (LONG)WAIT_ON_ADDRESS_MIXED_WAITERS_PER_ADDRESS,
                    Context.WakeCount,
                    Context.FailedCount);
            goto Cleanup;
        }
    }

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         5000 );

    for (Index = 0; Index < CreatedCount; Index++) {
        if (Context.Statuses[Index] != STATUS_SUCCESS &&
            FirstBadIndex < 0) {
            FirstBadIndex = Index;
        }
    }

    if (WaitResult != WAIT_OBJECT_0 ||
        Context.WakeCount != CreatedCount ||
        Context.FailedCount != 0) {
        fprintf(stderr,
                "[Failed] WaitOnAddress address-isolation Wait = 0x%08x Ready = %ld Wake = %ld Failed = %ld FirstBadIndex = %ld FirstStatus = 0x%08x FirstError = %lu\n",
                WaitResult,
                Context.ReadyCount,
                Context.WakeCount,
                Context.FailedCount,
                FirstBadIndex,
                FirstBadIndex >= 0 ? Context.Statuses[FirstBadIndex] : STATUS_SUCCESS,
                FirstBadIndex >= 0 ? Context.LastErrors[FirstBadIndex] : ERROR_SUCCESS);
        goto Cleanup;
    }

    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("[Success] WaitOnAddress address-isolation stress\n\n");
    return TRUE;

Cleanup:
    for (AddressIndex = 0;
         AddressIndex < WAIT_ON_ADDRESS_MIXED_ADDRESS_COUNT;
         AddressIndex++) {
        InterlockedExchange( (PLONG)&Context.Values[AddressIndex],
                             1 );
        WakeByAddressAll( (PVOID)&Context.Values[AddressIndex] );
        RtlWakeAddressAll( (PVOID)&Context.Values[AddressIndex] );
    }
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
WaitOnAddressRepeatedDurabilityStressTest (
    VOID
    )
{
    WAIT_ON_ADDRESS_MIXED_CONTEXT Context;
    WAIT_ON_ADDRESS_MIXED_THREAD_CONTEXT ThreadContexts[WAIT_ON_ADDRESS_DURABILITY_THREADS];
    HANDLE Threads[WAIT_ON_ADDRESS_DURABILITY_THREADS] = { NULL, };
    DWORD WaitResult;
    BOOLEAN SingleCompleted;
    LONG Attempt;
    LONG CreatedCount;
    LONG FirstBadIndex;
    LONG Index;
    LONG Round;

    PAGED_CODE();

    for (Round = 0; Round < WAIT_ON_ADDRESS_DURABILITY_ROUNDS; Round++) {
        RtlZeroMemory( &Context,
                       sizeof(Context) );
        RtlZeroMemory( Threads,
                       sizeof(Threads) );
        CreatedCount = 0;
        FirstBadIndex = -1;

        for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
            ThreadContexts[Index].Context = &Context;
            ThreadContexts[Index].Index = Index;
            ThreadContexts[Index].AddressIndex = 0;
            ThreadContexts[Index].UseRtl = (BOOLEAN)(((Round + Index) & 1) != 0);
            Threads[Index] = CreateThread( NULL,
                                           0,
                                           WaitOnAddressMixedThreadProc,
                                           &ThreadContexts[Index],
                                           0,
                                           NULL );
            if (! Threads[Index]) {
                fprintf(stderr,
                        "[Failed] WaitOnAddress durability CreateThread Round = %ld Index = %ld ErrorCode = %d\n",
                        Round,
                        Index,
                        GetLastError());
                goto CleanupRound;
            }
            CreatedCount++;
        }

        while (Context.ReadyCount < CreatedCount) {
            YieldProcessor();
        }
        Sleep( 20 );

        if ((Round & 1) == 0) {
            InterlockedExchange( (PLONG)&Context.Values[0],
                                 1 );
            if ((Round & 2) == 0) {
                WakeByAddressAll( (PVOID)&Context.Values[0] );
            } else {
                RtlWakeAddressAll( (PVOID)&Context.Values[0] );
            }
        } else {
            for (Attempt = 0;
                 Attempt < 5000 && Context.WakeCount < CreatedCount;
                 Attempt++) {
                if ((Round & 2) == 0) {
                    WakeByAddressSingle( (PVOID)&Context.Values[0] );
                } else {
                    RtlWakeAddressSingle( (PVOID)&Context.Values[0] );
                }
                if ((Attempt % 64) == 0) {
                    Sleep( 1 );
                } else {
                    YieldProcessor();
                }
            }

            SingleCompleted = (Context.WakeCount == CreatedCount);
            if (! SingleCompleted) {
                InterlockedExchange( (PLONG)&Context.Values[0],
                                     1 );
                WakeByAddressAll( (PVOID)&Context.Values[0] );
                RtlWakeAddressAll( (PVOID)&Context.Values[0] );
            }
        }

        WaitResult = WaitForMultipleObjects( CreatedCount,
                                             Threads,
                                             TRUE,
                                             5000 );

        for (Index = 0; Index < CreatedCount; Index++) {
            if (Context.Statuses[Index] != STATUS_SUCCESS &&
                FirstBadIndex < 0) {
                FirstBadIndex = Index;
            }
        }

        if (WaitResult != WAIT_OBJECT_0 ||
            Context.WakeCount != CreatedCount ||
            Context.FailedCount != 0) {
            fprintf(stderr,
                    "[Failed] WaitOnAddress durability Round = %ld Wait = 0x%08x Ready = %ld Wake = %ld Failed = %ld FirstBadIndex = %ld FirstStatus = 0x%08x FirstError = %lu Value = %ld\n",
                    Round,
                    WaitResult,
                    Context.ReadyCount,
                    Context.WakeCount,
                    Context.FailedCount,
                    FirstBadIndex,
                    FirstBadIndex >= 0 ? Context.Statuses[FirstBadIndex] : STATUS_SUCCESS,
                    FirstBadIndex >= 0 ? Context.LastErrors[FirstBadIndex] : ERROR_SUCCESS,
                    Context.Values[0]);
            goto CleanupRound;
        }

        for (Index = 0; Index < CreatedCount; Index++) {
            CloseHandle( Threads[Index] );
            Threads[Index] = NULL;
        }
    }

    printf("[Success] WaitOnAddress repeated durability stress Rounds = %ld Threads = %ld\n\n",
           (LONG)WAIT_ON_ADDRESS_DURABILITY_ROUNDS,
           (LONG)WAIT_ON_ADDRESS_DURABILITY_THREADS);
    return TRUE;

CleanupRound:
    InterlockedExchange( (PLONG)&Context.Values[0],
                         1 );
    WakeByAddressAll( (PVOID)&Context.Values[0] );
    RtlWakeAddressAll( (PVOID)&Context.Values[0] );
    if (CreatedCount) {
        WaitForMultipleObjects( CreatedCount,
                                Threads,
                                TRUE,
                                2000 );
    }
    for (Index = 0; Index < CreatedCount; Index++) {
        CloseHandle( Threads[Index] );
    }
    return FALSE;
}

BOOLEAN
WaitOnAddressSizeMatrixTest (
    VOID
    )
{
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    UCHAR Value8;
    UCHAR Compare8;
    USHORT Value16;
    USHORT Compare16;
    ULONG Value32;
    ULONG Compare32;
    ULONGLONG Value64;
    ULONGLONG Compare64;

    PAGED_CODE();

    Timeout.QuadPart = 0;

    Value8 = 1;
    Compare8 = 0;
    Status = RtlWaitOnAddress( &Value8,
                               &Compare8,
                               sizeof(Value8),
                               &Timeout );
    if (Status != STATUS_SUCCESS) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix immediate UCHAR Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    Value16 = 1;
    Compare16 = 0;
    Status = RtlWaitOnAddress( &Value16,
                               &Compare16,
                               sizeof(Value16),
                               &Timeout );
    if (Status != STATUS_SUCCESS) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix immediate USHORT Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    Value32 = 1;
    Compare32 = 0;
    Status = RtlWaitOnAddress( &Value32,
                               &Compare32,
                               sizeof(Value32),
                               &Timeout );
    if (Status != STATUS_SUCCESS) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix immediate ULONG Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    Value64 = 1;
    Compare64 = 0;
    Status = RtlWaitOnAddress( &Value64,
                               &Compare64,
                               sizeof(Value64),
                               &Timeout );
    if (Status != STATUS_SUCCESS) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix immediate ULONGLONG Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    Value8 = 0;
    Compare8 = 0;
    Status = RtlWaitOnAddress( &Value8,
                               &Compare8,
                               sizeof(Value8),
                               &Timeout );
    if (Status != STATUS_TIMEOUT) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix timeout UCHAR Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    Value16 = 0;
    Compare16 = 0;
    Status = RtlWaitOnAddress( &Value16,
                               &Compare16,
                               sizeof(Value16),
                               &Timeout );
    if (Status != STATUS_TIMEOUT) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix timeout USHORT Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    Value32 = 0;
    Compare32 = 0;
    Status = RtlWaitOnAddress( &Value32,
                               &Compare32,
                               sizeof(Value32),
                               &Timeout );
    if (Status != STATUS_TIMEOUT) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix timeout ULONG Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    Value64 = 0;
    Compare64 = 0;
    Status = RtlWaitOnAddress( &Value64,
                               &Compare64,
                               sizeof(Value64),
                               &Timeout );
    if (Status != STATUS_TIMEOUT) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix timeout ULONGLONG Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    Status = RtlWaitOnAddress( &Value32,
                               &Compare32,
                               3,
                               &Timeout );
    if (Status != STATUS_INVALID_PARAMETER) {
        fprintf(stderr,
                "[Failed] WaitOnAddress size matrix invalid size Status = 0x%08x\n",
                Status);
        return FALSE;
    }

    if (! WaitOnAddress( &Value8,
                         &Compare8,
                         sizeof(Value8),
                         0 )) {
        printf("[Success] WaitOnAddress size matrix\n\n");
        return TRUE;
    }

    fprintf(stderr,
            "[Failed] WaitOnAddress size matrix wrapper timeout Value8 = %u Compare8 = %u\n",
            Value8,
            Compare8);
    return FALSE;
}

BOOLEAN
WaitOnAddressTest (
    VOID
    )
{
    ALERT_BY_THREAD_ID_TEST_CONTEXT AlertContext;
    WAIT_ON_ADDRESS_TEST_CONTEXT Context;
    HANDLE Thread;
    HANDLE Threads[3];
    DWORD ExitCode;
    DWORD WaitResult;
    BOOL ExitCodeResult;
    NTSTATUS Status;
    LONG Compare;
    BOOLEAN Result = TRUE;
    int Index;

    PAGED_CODE();

    printf("WaitOnAddress Test\n");

    printf("Test NtAlertThreadByThreadId/NtWaitForAlertByThreadId\n");
    RtlZeroMemory( &AlertContext,
                   sizeof(AlertContext) );
    Thread = CreateThread( NULL,
                           0,
                           AlertByThreadIdTestThreadProc,
                           &AlertContext,
                           0,
                           NULL );
    if (! Thread) {
        fprintf(stderr, "[Failed] CreateThread(Alert) ErrorCode = %d\n", GetLastError());
        return FALSE;
    }

    while (! AlertContext.Ready) {
        YieldProcessor();
    }

    Status = NtAlertThreadByThreadId( (HANDLE)(ULONG_PTR)AlertContext.ThreadId );
    WaitResult = WaitForSingleObject( Thread,
                                      2000 );
    ExitCode = (DWORD)-1;
    ExitCodeResult = GetExitCodeThread( Thread,
                                        &ExitCode );

    if (! NT_SUCCESS(Status) ||
        WaitResult != WAIT_OBJECT_0 ||
        ! ExitCodeResult ||
        ExitCode != 0 ||
        AlertContext.Woken != 1) {
        fprintf(stderr,
                "[Failed] AlertBasic A=0x%08x W=0x%08x X=%d EC=%lu R=%ld WK=%ld WS=0x%08x TID=%lu\n",
                Status,
                WaitResult,
                ExitCodeResult,
                ExitCode,
                AlertContext.Ready,
                AlertContext.Woken,
                AlertContext.WaitStatus,
                AlertContext.ThreadId);
        Result = FALSE;
    } else {
        printf("[Success] Test NtAlertThreadByThreadId/NtWaitForAlertByThreadId\n\n");
    }
    CloseHandle( Thread );

    printf("Test WaitOnAddress immediate success\n");
    RtlZeroMemory( &Context,
                   sizeof(Context) );
    Context.Value = 1;
    Compare = 0;
    if (WaitOnAddress( &Context.Value,
                       &Compare,
                       sizeof(Context.Value),
                       1000 )) {
        printf("[Success] Test WaitOnAddress immediate success\n\n");
    } else {
        fprintf(stderr,
                "[Failed] Test WaitOnAddress immediate success ErrorCode = %d Value = %ld Compare = %ld\n",
                GetLastError(),
                Context.Value,
                Compare);
        Result = FALSE;
    }

    printf("Test WaitOnAddress timeout\n");
    RtlZeroMemory( &Context,
                   sizeof(Context) );
    Compare = 0;
    if (! WaitOnAddress( &Context.Value,
                         &Compare,
                         sizeof(Context.Value),
                         10 )) {
        printf("[Success] Test WaitOnAddress timeout\n\n");
    } else {
        fprintf(stderr,
                "[Failed] Test WaitOnAddress timeout ErrorCode = %d Value = %ld Compare = %ld\n",
                GetLastError(),
                Context.Value,
                Compare);
        Result = FALSE;
    }

    printf("Test WakeByAddressSingle\n");
    RtlZeroMemory( &Context,
                   sizeof(Context) );
    Thread = CreateThread( NULL,
                           0,
                           WaitOnAddressTestThreadProc,
                           &Context,
                           0,
                           NULL );
    if (! Thread) {
        fprintf(stderr, "[Failed] CreateThread(Single) ErrorCode = %d\n", GetLastError());
        return FALSE;
    }

    while (Context.ReadyCount < 1) {
        YieldProcessor();
    }

    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    WakeByAddressSingle( (PVOID)&Context.Value );

    WaitResult = WaitForSingleObject( Thread,
                                      2000 );
    ExitCode = (DWORD)-1;
    ExitCodeResult = GetExitCodeThread( Thread,
                                        &ExitCode );

    if (WaitResult != WAIT_OBJECT_0 ||
        ! ExitCodeResult ||
        ExitCode != 0 ||
        Context.WakeCount != 1) {
        fprintf(stderr,
                "[Failed] Test WakeByAddressSingle Wait = 0x%08x ExitCodeResult = %d ExitCode = %d ReadyCount = %ld WakeCount = %ld Value = %ld\n",
                WaitResult,
                ExitCodeResult,
                ExitCode,
                Context.ReadyCount,
                Context.WakeCount,
                Context.Value);
        Result = FALSE;
    } else {
        printf("[Success] Test WakeByAddressSingle\n\n");
    }
    CloseHandle( Thread );

    printf("Test WakeByAddressAll\n");
    RtlZeroMemory( &Context,
                   sizeof(Context) );
    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       WaitOnAddressTestThreadProc,
                                       &Context,
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] CreateThread(All)[%d] ErrorCode = %d ReadyCount = %ld\n",
                    Index,
                    GetLastError(),
                    Context.ReadyCount);
            return FALSE;
        }
    }

    while (Context.ReadyCount < RTL_NUMBER_OF(Threads)) {
        YieldProcessor();
    }

    InterlockedExchange( (PLONG)&Context.Value,
                         1 );
    WakeByAddressAll( (PVOID)&Context.Value );

    WaitResult = WaitForMultipleObjects( RTL_NUMBER_OF(Threads),
                                         Threads,
                                         TRUE,
                                         2000 );

    if (WaitResult != WAIT_OBJECT_0 ||
        Context.WakeCount != RTL_NUMBER_OF(Threads)) {
        fprintf(stderr,
                "[Failed] Test WakeByAddressAll Wait = 0x%08x ReadyCount = %ld WakeCount = %ld Value = %ld ExpectedWakeCount = %llu\n",
                WaitResult,
                Context.ReadyCount,
                Context.WakeCount,
                Context.Value,
                (ULONGLONG)RTL_NUMBER_OF(Threads));
        Result = FALSE;
    } else {
        printf("[Success] Test WakeByAddressAll\n\n");
    }

    for (Index = 0; Index < RTL_NUMBER_OF(Threads); Index++) {
        CloseHandle( Threads[Index] );
    }

    printf("Test NtAlertThreadByThreadId/NtWaitForAlertByThreadId stress\n");
    if (! AlertByThreadIdStressTest()) {
        Result = FALSE;
    }

    printf("Test NtWaitForAlertByThreadId timeout stress\n");
    if (! NtWaitForAlertByThreadIdTimeoutStressTest()) {
        Result = FALSE;
    }

    printf("Test WaitOnAddress WakeByAddressAll stress\n");
    if (! WaitOnAddressWakeAllStressTest()) {
        Result = FALSE;
    }

    printf("Test WaitOnAddress WakeByAddressSingle stress\n");
    if (! WaitOnAddressWakeSingleStressTest()) {
        Result = FALSE;
    }

    printf("Test WaitOnAddress timeout stress\n");
    if (! WaitOnAddressTimeoutStressTest()) {
        Result = FALSE;
    }

    printf("Test RtlWaitOnAddress/RtlWakeAddressAll stress\n");
    if (! RtlWaitOnAddressWakeAllStressTest()) {
        Result = FALSE;
    }

    printf("Test RtlWaitOnAddress/RtlWakeAddressSingle stress\n");
    if (! RtlWaitOnAddressWakeSingleStressTest()) {
        Result = FALSE;
    }

    printf("Test RtlWaitOnAddress timeout stress\n");
    if (! RtlWaitOnAddressTimeoutStressTest()) {
        Result = FALSE;
    }

    printf("Test WaitOnAddress address-isolation stress\n");
    if (! WaitOnAddressAddressIsolationStressTest()) {
        Result = FALSE;
    }

    printf("Test WaitOnAddress repeated durability stress\n");
    if (! WaitOnAddressRepeatedDurabilityStressTest()) {
        Result = FALSE;
    }

    printf("Test WaitOnAddress size matrix\n");
    if (! WaitOnAddressSizeMatrixTest()) {
        Result = FALSE;
    }

    return Result;
}
