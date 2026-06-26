#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
ConditionVariableTest (
    VOID
    );

DWORD
WINAPI
CondTestThreadProc(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
CriticalSectionCounterThreadProc(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
SRWLockCounterThreadProc(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
CondSRWTestThreadProc(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
CriticalSectionStressThreadProc(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
ConditionVariableCSConsumerThreadProc(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
SRWLockMixedStressThreadProc(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
ConditionVariableSRWExclusiveConsumerThreadProc(
    LPVOID lpThreadParameter
    );

BOOLEAN
CriticalSectionRegressionTest(
    VOID
    );

BOOLEAN
CriticalSectionContentionStressTest(
    VOID
    );

BOOLEAN
SRWLockRegressionTest(
    VOID
    );

BOOLEAN
SRWLockMixedStressTest(
    VOID
    );

BOOLEAN
ConditionVariableCSProducerConsumerStressTest(
    VOID
    );

BOOLEAN
ConditionVariableSRWRegressionTest(
    VOID
    );

BOOLEAN
ConditionVariableSRWExclusiveStressTest(
    VOID
    );

BOOLEAN
SyncWaitForCounter(
    _In_ volatile LONG* Counter,
    _In_ LONG Expected,
    _In_ DWORD TimeoutMilliseconds
    );

VOID
SyncWaitForThreads(
    _In_reads_(Count) HANDLE* Threads,
    _In_ LONG Count
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ConditionVariableTest)
#pragma alloc_text(PAGE, CondTestThreadProc)
#pragma alloc_text(PAGE, CriticalSectionCounterThreadProc)
#pragma alloc_text(PAGE, SRWLockCounterThreadProc)
#pragma alloc_text(PAGE, CondSRWTestThreadProc)
#pragma alloc_text(PAGE, CriticalSectionStressThreadProc)
#pragma alloc_text(PAGE, ConditionVariableCSConsumerThreadProc)
#pragma alloc_text(PAGE, SRWLockMixedStressThreadProc)
#pragma alloc_text(PAGE, ConditionVariableSRWExclusiveConsumerThreadProc)
#pragma alloc_text(PAGE, CriticalSectionRegressionTest)
#pragma alloc_text(PAGE, CriticalSectionContentionStressTest)
#pragma alloc_text(PAGE, SRWLockRegressionTest)
#pragma alloc_text(PAGE, SRWLockMixedStressTest)
#pragma alloc_text(PAGE, ConditionVariableCSProducerConsumerStressTest)
#pragma alloc_text(PAGE, ConditionVariableSRWRegressionTest)
#pragma alloc_text(PAGE, ConditionVariableSRWExclusiveStressTest)
#pragma alloc_text(PAGE, SyncWaitForCounter)
#pragma alloc_text(PAGE, SyncWaitForThreads)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <stdio.h>

#define PAGED_CODE()
#define DbgPrint printf
#endif

#define COND_TEST_THREAD_COUNT 5
#define SYNC_COUNTER_THREAD_COUNT 8
#define SYNC_COUNTER_ITERATIONS 256
#define COND_SRW_TEST_THREAD_COUNT 6
#define SYNC_STRESS_THREAD_COUNT 12
#define SYNC_STRESS_ITERATIONS 384
#define SYNC_STRESS_TIMEOUT 120000
#define COND_STRESS_CONSUMER_COUNT 8
#define COND_STRESS_WORK_ITEMS 256
#define SRW_MIXED_THREAD_COUNT 12
#define SRW_MIXED_ITERATIONS 384

#define RECORD_SYNC_FAILURE(_Failure) \
    InterlockedCompareExchange( (PLONG)(_Failure), __LINE__, 0 )

typedef struct _COND_TEST_THREAD_PARAM {
    PCSTR name;
    BOOL ready;
    LONG processed;
    CONDITION_VARIABLE cv;
    CRITICAL_SECTION cs;
} COND_TEST_THREAD_PARAM, *PCOND_TEST_THREAD_PARAM;

typedef struct _CRITICAL_SECTION_COUNTER_PARAM {
    PCRITICAL_SECTION CriticalSection;
    volatile LONG ReadyCount;
    LONG Counter;
} CRITICAL_SECTION_COUNTER_PARAM, *PCRITICAL_SECTION_COUNTER_PARAM;

typedef struct _SRWLOCK_COUNTER_PARAM {
    PSRWLOCK SRWLock;
    volatile LONG ReadyCount;
    LONG Counter;
} SRWLOCK_COUNTER_PARAM, *PSRWLOCK_COUNTER_PARAM;

typedef struct _COND_SRW_TEST_PARAM {
    SRWLOCK SRWLock;
    CONDITION_VARIABLE ConditionVariable;
    BOOL Ready;
    volatile LONG ReadyCount;
    volatile LONG Processed;
} COND_SRW_TEST_PARAM, *PCOND_SRW_TEST_PARAM;

typedef struct _CRITICAL_SECTION_STRESS_PARAM {
    CRITICAL_SECTION CriticalSection;
    volatile LONG ReadyCount;
    volatile LONG Start;
    volatile LONG InsideCount;
    volatile LONG FailureLine;
    LONG Counter;
} CRITICAL_SECTION_STRESS_PARAM, *PCRITICAL_SECTION_STRESS_PARAM;

typedef struct _COND_CS_STRESS_PARAM {
    CRITICAL_SECTION CriticalSection;
    CONDITION_VARIABLE ConditionVariable;
    volatile LONG ReadyCount;
    volatile LONG FailureLine;
    LONG WorkItems;
    LONG Produced;
    LONG Consumed;
    BOOL Done;
} COND_CS_STRESS_PARAM, *PCOND_CS_STRESS_PARAM;

typedef struct _SRW_MIXED_STRESS_PARAM {
    SRWLOCK SRWLock;
    volatile LONG ReadyCount;
    volatile LONG Start;
    volatile LONG ActiveReaders;
    volatile LONG ActiveWriters;
    volatile LONG FailureLine;
    volatile LONG ReadChecks;
    LONG Value;
    LONG Writes;
} SRW_MIXED_STRESS_PARAM, *PSRW_MIXED_STRESS_PARAM;

typedef struct _SRW_MIXED_THREAD_PARAM {
    PSRW_MIXED_STRESS_PARAM Context;
    LONG Index;
    BOOL Writer;
} SRW_MIXED_THREAD_PARAM, *PSRW_MIXED_THREAD_PARAM;

typedef struct _COND_SRW_EXCLUSIVE_STRESS_PARAM {
    SRWLOCK SRWLock;
    CONDITION_VARIABLE ConditionVariable;
    volatile LONG ReadyCount;
    volatile LONG FailureLine;
    LONG WorkItems;
    LONG Produced;
    LONG Consumed;
    BOOL Done;
} COND_SRW_EXCLUSIVE_STRESS_PARAM, *PCOND_SRW_EXCLUSIVE_STRESS_PARAM;

BOOLEAN
SyncWaitForCounter (
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

VOID
SyncWaitForThreads (
    _In_reads_(Count) HANDLE* Threads,
    _In_ LONG Count
    )
{
    LONG Index;

    PAGED_CODE();

    for (Index = 0; Index < Count; Index++) {
        if (Threads[Index]) {
            WaitForSingleObject( Threads[Index],
                                 INFINITE );
        }
    }
}

DWORD
WINAPI
CondTestThreadProc (
    LPVOID lpThreadParameter
    )
{
    PCOND_TEST_THREAD_PARAM param = (PCOND_TEST_THREAD_PARAM)lpThreadParameter;

    PAGED_CODE();

    DbgPrint("%s - %d - start\n", param->name, GetCurrentThreadId());

    EnterCriticalSection(&param->cs);
    while (!param->ready) {
        SleepConditionVariableCS(&param->cv, &param->cs, INFINITE);
    }

    DbgPrint("%s - %d - processed (%d)\n", param->name, GetCurrentThreadId(), InterlockedIncrement(&param->processed));

    LeaveCriticalSection(&param->cs);

    DbgPrint("%s - %d - end\n", param->name, GetCurrentThreadId());

    WakeConditionVariable(&param->cv);
    return 0;
}

DWORD
WINAPI
CriticalSectionCounterThreadProc (
    LPVOID lpThreadParameter
    )
{
    PCRITICAL_SECTION_COUNTER_PARAM Param;
    LONG Index;

    PAGED_CODE();

    Param = (PCRITICAL_SECTION_COUNTER_PARAM)lpThreadParameter;
    InterlockedIncrement( (PLONG)&Param->ReadyCount );

    for (Index = 0; Index < SYNC_COUNTER_ITERATIONS; Index++) {
        EnterCriticalSection( Param->CriticalSection );
        Param->Counter++;
        LeaveCriticalSection( Param->CriticalSection );
    }

    return 0;
}

DWORD
WINAPI
SRWLockCounterThreadProc (
    LPVOID lpThreadParameter
    )
{
    PSRWLOCK_COUNTER_PARAM Param;
    LONG Index;

    PAGED_CODE();

    Param = (PSRWLOCK_COUNTER_PARAM)lpThreadParameter;
    InterlockedIncrement( (PLONG)&Param->ReadyCount );

    for (Index = 0; Index < SYNC_COUNTER_ITERATIONS; Index++) {
        AcquireSRWLockExclusive( Param->SRWLock );
        Param->Counter++;
        ReleaseSRWLockExclusive( Param->SRWLock );
    }

    return 0;
}

DWORD
WINAPI
CondSRWTestThreadProc (
    LPVOID lpThreadParameter
    )
{
    PCOND_SRW_TEST_PARAM Param;

    PAGED_CODE();

    Param = (PCOND_SRW_TEST_PARAM)lpThreadParameter;

    AcquireSRWLockShared( &Param->SRWLock );
    InterlockedIncrement( (PLONG)&Param->ReadyCount );
    while (! Param->Ready) {
        SleepConditionVariableSRW( &Param->ConditionVariable,
                                   &Param->SRWLock,
                                   INFINITE,
                                   CONDITION_VARIABLE_LOCKMODE_SHARED );
    }
    InterlockedIncrement( (PLONG)&Param->Processed );
    ReleaseSRWLockShared( &Param->SRWLock );

    return 0;
}

DWORD
WINAPI
CriticalSectionStressThreadProc (
    LPVOID lpThreadParameter
    )
{
    PCRITICAL_SECTION_STRESS_PARAM Param;
    LONG Index;
    LONG InsideCount;

    PAGED_CODE();

    Param = (PCRITICAL_SECTION_STRESS_PARAM)lpThreadParameter;
    InterlockedIncrement( (PLONG)&Param->ReadyCount );

    while (! Param->Start) {
        YieldProcessor();
    }

    for (Index = 0; Index < SYNC_STRESS_ITERATIONS; Index++) {
        if ((Index & 3) == 0) {
            while (! TryEnterCriticalSection( &Param->CriticalSection )) {
                YieldProcessor();
            }
        } else {
            EnterCriticalSection( &Param->CriticalSection );
        }

        InsideCount = InterlockedIncrement( (PLONG)&Param->InsideCount );
        if (InsideCount != 1) {
            RECORD_SYNC_FAILURE( &Param->FailureLine );
        }

        Param->Counter++;

        if ((Index % 5) == 0) {
            EnterCriticalSection( &Param->CriticalSection );
            Param->Counter++;
            LeaveCriticalSection( &Param->CriticalSection );
        }

        if ((Index & 15) == 0) {
            Sleep( 0 );
        }

        InsideCount = InterlockedDecrement( (PLONG)&Param->InsideCount );
        if (InsideCount != 0) {
            RECORD_SYNC_FAILURE( &Param->FailureLine );
        }

        LeaveCriticalSection( &Param->CriticalSection );

        if ((Index & 7) == 0) {
            YieldProcessor();
        }
    }

    return Param->FailureLine ? 1 : 0;
}

DWORD
WINAPI
ConditionVariableCSConsumerThreadProc (
    LPVOID lpThreadParameter
    )
{
    PCOND_CS_STRESS_PARAM Param;

    PAGED_CODE();

    Param = (PCOND_CS_STRESS_PARAM)lpThreadParameter;

    EnterCriticalSection( &Param->CriticalSection );
    InterlockedIncrement( (PLONG)&Param->ReadyCount );

    for (;;) {
        while (Param->WorkItems == 0 &&
               ! Param->Done) {
            if (! SleepConditionVariableCS( &Param->ConditionVariable,
                                            &Param->CriticalSection,
                                            25 )) {
                if (GetLastError() != ERROR_TIMEOUT) {
                    RECORD_SYNC_FAILURE( &Param->FailureLine );
                    LeaveCriticalSection( &Param->CriticalSection );
                    return 1;
                }
            }
        }

        if (Param->WorkItems == 0 &&
            Param->Done) {
            break;
        }

        Param->WorkItems--;
        Param->Consumed++;
        LeaveCriticalSection( &Param->CriticalSection );
        YieldProcessor();
        EnterCriticalSection( &Param->CriticalSection );
    }

    LeaveCriticalSection( &Param->CriticalSection );
    return Param->FailureLine ? 1 : 0;
}

DWORD
WINAPI
SRWLockMixedStressThreadProc (
    LPVOID lpThreadParameter
    )
{
    PSRW_MIXED_THREAD_PARAM ThreadParam;
    PSRW_MIXED_STRESS_PARAM Param;
    LONG Index;
    LONG Active;
    LONG Snapshot;

    PAGED_CODE();

    ThreadParam = (PSRW_MIXED_THREAD_PARAM)lpThreadParameter;
    Param = ThreadParam->Context;
    InterlockedIncrement( (PLONG)&Param->ReadyCount );

    while (! Param->Start) {
        YieldProcessor();
    }

    for (Index = 0; Index < SRW_MIXED_ITERATIONS; Index++) {
        if (ThreadParam->Writer) {
            if ((Index & 3) == 0) {
                while (! TryAcquireSRWLockExclusive( &Param->SRWLock )) {
                    YieldProcessor();
                }
            } else {
                AcquireSRWLockExclusive( &Param->SRWLock );
            }

            Active = InterlockedIncrement( (PLONG)&Param->ActiveWriters );
            if (Active != 1 ||
                Param->ActiveReaders != 0) {
                RECORD_SYNC_FAILURE( &Param->FailureLine );
            }

            Param->Value++;
            Param->Writes++;

            if ((Index & 15) == 0) {
                Sleep( 0 );
            }

            Active = InterlockedDecrement( (PLONG)&Param->ActiveWriters );
            if (Active != 0) {
                RECORD_SYNC_FAILURE( &Param->FailureLine );
            }

            ReleaseSRWLockExclusive( &Param->SRWLock );
        } else {
            if ((Index & 3) == 0) {
                while (! TryAcquireSRWLockShared( &Param->SRWLock )) {
                    YieldProcessor();
                }
            } else {
                AcquireSRWLockShared( &Param->SRWLock );
            }

            Active = InterlockedIncrement( (PLONG)&Param->ActiveReaders );
            Snapshot = Param->Value;
            if (Active <= 0 ||
                Param->ActiveWriters != 0) {
                RECORD_SYNC_FAILURE( &Param->FailureLine );
            }

            if ((Index & 15) == 0) {
                Sleep( 0 );
            }

            if (Param->Value != Snapshot) {
                RECORD_SYNC_FAILURE( &Param->FailureLine );
            }

            InterlockedIncrement( (PLONG)&Param->ReadChecks );
            Active = InterlockedDecrement( (PLONG)&Param->ActiveReaders );
            if (Active < 0) {
                RECORD_SYNC_FAILURE( &Param->FailureLine );
            }

            ReleaseSRWLockShared( &Param->SRWLock );
        }

        if ((Index & 7) == 0) {
            YieldProcessor();
        }
    }

    return Param->FailureLine ? 1 : 0;
}

DWORD
WINAPI
ConditionVariableSRWExclusiveConsumerThreadProc (
    LPVOID lpThreadParameter
    )
{
    PCOND_SRW_EXCLUSIVE_STRESS_PARAM Param;

    PAGED_CODE();

    Param = (PCOND_SRW_EXCLUSIVE_STRESS_PARAM)lpThreadParameter;

    AcquireSRWLockExclusive( &Param->SRWLock );
    InterlockedIncrement( (PLONG)&Param->ReadyCount );

    for (;;) {
        while (Param->WorkItems == 0 &&
               ! Param->Done) {
            if (! SleepConditionVariableSRW( &Param->ConditionVariable,
                                             &Param->SRWLock,
                                             25,
                                             0 )) {
                if (GetLastError() != ERROR_TIMEOUT) {
                    RECORD_SYNC_FAILURE( &Param->FailureLine );
                    ReleaseSRWLockExclusive( &Param->SRWLock );
                    return 1;
                }
            }
        }

        if (Param->WorkItems == 0 &&
            Param->Done) {
            break;
        }

        Param->WorkItems--;
        Param->Consumed++;
        ReleaseSRWLockExclusive( &Param->SRWLock );
        YieldProcessor();
        AcquireSRWLockExclusive( &Param->SRWLock );
    }

    ReleaseSRWLockExclusive( &Param->SRWLock );
    return Param->FailureLine ? 1 : 0;
}

BOOLEAN
CriticalSectionRegressionTest (
    VOID
    )
{
    CRITICAL_SECTION CriticalSection;
    CRITICAL_SECTION_COUNTER_PARAM Param;
    HANDLE Threads[SYNC_COUNTER_THREAD_COUNT] = { NULL, };
    DWORD WaitResult;
    LONG CreatedCount = 0;
    LONG Index;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    InitializeCriticalSection( &CriticalSection );

    if (! TryEnterCriticalSection( &CriticalSection )) {
        fprintf(stderr, "[Failed] TryEnterCriticalSection first entry\n");
        Result = FALSE;
        goto Cleanup;
    }

    if (! TryEnterCriticalSection( &CriticalSection )) {
        fprintf(stderr, "[Failed] TryEnterCriticalSection recursive entry\n");
        Result = FALSE;
        LeaveCriticalSection( &CriticalSection );
        goto Cleanup;
    }

    LeaveCriticalSection( &CriticalSection );
    LeaveCriticalSection( &CriticalSection );

    RtlZeroMemory( &Param,
                   sizeof(Param) );
    Param.CriticalSection = &CriticalSection;

    for (Index = 0; Index < SYNC_COUNTER_THREAD_COUNT; Index++) {
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       CriticalSectionCounterThreadProc,
                                       &Param,
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] CriticalSection CreateThread[%ld] ErrorCode = %lu\n",
                    Index,
                    GetLastError());
            Result = FALSE;
            goto Cleanup;
        }
        CreatedCount++;
    }

    WaitResult = WaitForMultipleObjects( SYNC_COUNTER_THREAD_COUNT,
                                         Threads,
                                         TRUE,
                                         5000 );
    if (WaitResult != WAIT_OBJECT_0 ||
        Param.Counter != SYNC_COUNTER_THREAD_COUNT * SYNC_COUNTER_ITERATIONS) {
        fprintf(stderr,
                "[Failed] CriticalSection counter Wait = 0x%08x Ready = %ld Counter = %ld Expected = %ld\n",
                WaitResult,
                Param.ReadyCount,
                Param.Counter,
                (LONG)(SYNC_COUNTER_THREAD_COUNT * SYNC_COUNTER_ITERATIONS));
        Result = FALSE;
    }

Cleanup:
    if (CreatedCount) {
        SyncWaitForThreads( Threads,
                            CreatedCount );
    }
    for (Index = 0; Index < SYNC_COUNTER_THREAD_COUNT; Index++) {
        if (Threads[Index]) {
            CloseHandle( Threads[Index] );
        }
    }
    DeleteCriticalSection( &CriticalSection );
    return Result;
}

BOOLEAN
CriticalSectionContentionStressTest (
    VOID
    )
{
    CRITICAL_SECTION_STRESS_PARAM Param;
    HANDLE Threads[SYNC_STRESS_THREAD_COUNT] = { NULL, };
    DWORD WaitResult;
    LONG CreatedCount = 0;
    LONG ExpectedCounter;
    LONG RecursiveIterations;
    LONG Index;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    RtlZeroMemory( &Param,
                   sizeof(Param) );
    InitializeCriticalSection( &Param.CriticalSection );

    for (Index = 0; Index < SYNC_STRESS_THREAD_COUNT; Index++) {
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       CriticalSectionStressThreadProc,
                                       &Param,
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] CriticalSection stress CreateThread[%ld] ErrorCode = %lu\n",
                    Index,
                    GetLastError());
            Result = FALSE;
            goto Cleanup;
        }
        CreatedCount++;
    }

    if (! SyncWaitForCounter( &Param.ReadyCount,
                              CreatedCount,
                              5000 )) {
        fprintf(stderr,
                "[Failed] CriticalSection stress ready Ready = %ld Expected = %ld\n",
                Param.ReadyCount,
                CreatedCount);
        Result = FALSE;
        goto Cleanup;
    }

    InterlockedExchange( (PLONG)&Param.Start,
                         1 );

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         SYNC_STRESS_TIMEOUT );

    RecursiveIterations = ((SYNC_STRESS_ITERATIONS - 1) / 5) + 1;
    ExpectedCounter = CreatedCount * (SYNC_STRESS_ITERATIONS + RecursiveIterations);
    if (WaitResult != WAIT_OBJECT_0 ||
        Param.Counter != ExpectedCounter ||
        Param.InsideCount != 0 ||
        Param.FailureLine != 0) {
        fprintf(stderr,
                "[Failed] CriticalSection stress Wait = 0x%08x Ready = %ld Counter = %ld Expected = %ld Inside = %ld FailureLine = %ld\n",
                WaitResult,
                Param.ReadyCount,
                Param.Counter,
                ExpectedCounter,
                Param.InsideCount,
                Param.FailureLine);
        Result = FALSE;
    } else {
        printf("[Success] CriticalSection contention stress\n\n");
    }

Cleanup:
    InterlockedExchange( (PLONG)&Param.Start,
                         1 );
    if (CreatedCount) {
        SyncWaitForThreads( Threads,
                            CreatedCount );
    }
    for (Index = 0; Index < SYNC_STRESS_THREAD_COUNT; Index++) {
        if (Threads[Index]) {
            CloseHandle( Threads[Index] );
        }
    }
    DeleteCriticalSection( &Param.CriticalSection );
    return Result;
}

BOOLEAN
SRWLockRegressionTest (
    VOID
    )
{
    SRWLOCK SRWLock;
    SRWLOCK_COUNTER_PARAM Param;
    HANDLE Threads[SYNC_COUNTER_THREAD_COUNT] = { NULL, };
    DWORD WaitResult;
    LONG CreatedCount = 0;
    LONG Index;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    InitializeSRWLock( &SRWLock );

    AcquireSRWLockShared( &SRWLock );
    if (TryAcquireSRWLockExclusive( &SRWLock )) {
        fprintf(stderr, "[Failed] TryAcquireSRWLockExclusive while shared\n");
        ReleaseSRWLockExclusive( &SRWLock );
        Result = FALSE;
    }

    if (! TryAcquireSRWLockShared( &SRWLock )) {
        fprintf(stderr, "[Failed] TryAcquireSRWLockShared recursive shared\n");
        Result = FALSE;
    } else {
        ReleaseSRWLockShared( &SRWLock );
    }
    ReleaseSRWLockShared( &SRWLock );

    if (! TryAcquireSRWLockExclusive( &SRWLock )) {
        fprintf(stderr, "[Failed] TryAcquireSRWLockExclusive free lock\n");
        Result = FALSE;
        goto Cleanup;
    }

    if (TryAcquireSRWLockShared( &SRWLock )) {
        fprintf(stderr, "[Failed] TryAcquireSRWLockShared while exclusive\n");
        ReleaseSRWLockShared( &SRWLock );
        Result = FALSE;
    }
    ReleaseSRWLockExclusive( &SRWLock );

    RtlZeroMemory( &Param,
                   sizeof(Param) );
    Param.SRWLock = &SRWLock;

    for (Index = 0; Index < SYNC_COUNTER_THREAD_COUNT; Index++) {
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       SRWLockCounterThreadProc,
                                       &Param,
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] SRWLock CreateThread[%ld] ErrorCode = %lu\n",
                    Index,
                    GetLastError());
            Result = FALSE;
            goto Cleanup;
        }
        CreatedCount++;
    }

    WaitResult = WaitForMultipleObjects( SYNC_COUNTER_THREAD_COUNT,
                                         Threads,
                                         TRUE,
                                         5000 );
    if (WaitResult != WAIT_OBJECT_0 ||
        Param.Counter != SYNC_COUNTER_THREAD_COUNT * SYNC_COUNTER_ITERATIONS) {
        fprintf(stderr,
                "[Failed] SRWLock counter Wait = 0x%08x Ready = %ld Counter = %ld Expected = %ld\n",
                WaitResult,
                Param.ReadyCount,
                Param.Counter,
                (LONG)(SYNC_COUNTER_THREAD_COUNT * SYNC_COUNTER_ITERATIONS));
        Result = FALSE;
    }

Cleanup:
    if (CreatedCount) {
        SyncWaitForThreads( Threads,
                            CreatedCount );
    }
    for (Index = 0; Index < SYNC_COUNTER_THREAD_COUNT; Index++) {
        if (Threads[Index]) {
            CloseHandle( Threads[Index] );
        }
    }
    return Result;
}

BOOLEAN
SRWLockMixedStressTest (
    VOID
    )
{
    SRW_MIXED_STRESS_PARAM Param;
    SRW_MIXED_THREAD_PARAM ThreadParams[SRW_MIXED_THREAD_COUNT];
    HANDLE Threads[SRW_MIXED_THREAD_COUNT] = { NULL, };
    DWORD WaitResult;
    LONG CreatedCount = 0;
    LONG ExpectedWrites = 0;
    LONG Index;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    RtlZeroMemory( &Param,
                   sizeof(Param) );
    InitializeSRWLock( &Param.SRWLock );

    for (Index = 0; Index < SRW_MIXED_THREAD_COUNT; Index++) {
        ThreadParams[Index].Context = &Param;
        ThreadParams[Index].Index = Index;
        ThreadParams[Index].Writer = (BOOLEAN)((Index % 3) == 0);
        if (ThreadParams[Index].Writer) {
            ExpectedWrites += SRW_MIXED_ITERATIONS;
        }

        Threads[Index] = CreateThread( NULL,
                                       0,
                                       SRWLockMixedStressThreadProc,
                                       &ThreadParams[Index],
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] SRWLock mixed CreateThread[%ld] ErrorCode = %lu\n",
                    Index,
                    GetLastError());
            Result = FALSE;
            goto Cleanup;
        }
        CreatedCount++;
    }

    if (! SyncWaitForCounter( &Param.ReadyCount,
                              CreatedCount,
                              5000 )) {
        fprintf(stderr,
                "[Failed] SRWLock mixed ready Ready = %ld Expected = %ld\n",
                Param.ReadyCount,
                CreatedCount);
        Result = FALSE;
        goto Cleanup;
    }

    InterlockedExchange( (PLONG)&Param.Start,
                         1 );

    WaitResult = WaitForMultipleObjects( CreatedCount,
                                         Threads,
                                         TRUE,
                                         SYNC_STRESS_TIMEOUT );
    if (WaitResult != WAIT_OBJECT_0 ||
        Param.Writes != ExpectedWrites ||
        Param.Value != ExpectedWrites ||
        Param.ActiveReaders != 0 ||
        Param.ActiveWriters != 0 ||
        Param.FailureLine != 0) {
        fprintf(stderr,
                "[Failed] SRWLock mixed Wait = 0x%08x Ready = %ld Writes = %ld Value = %ld Expected = %ld Readers = %ld Writers = %ld ReadChecks = %ld FailureLine = %ld\n",
                WaitResult,
                Param.ReadyCount,
                Param.Writes,
                Param.Value,
                ExpectedWrites,
                Param.ActiveReaders,
                Param.ActiveWriters,
                Param.ReadChecks,
                Param.FailureLine);
        Result = FALSE;
    } else {
        printf("[Success] SRWLock mixed stress\n\n");
    }

Cleanup:
    InterlockedExchange( (PLONG)&Param.Start,
                         1 );
    if (CreatedCount) {
        SyncWaitForThreads( Threads,
                            CreatedCount );
    }
    for (Index = 0; Index < SRW_MIXED_THREAD_COUNT; Index++) {
        if (Threads[Index]) {
            CloseHandle( Threads[Index] );
        }
    }
    return Result;
}

BOOLEAN
ConditionVariableCSProducerConsumerStressTest (
    VOID
    )
{
    COND_CS_STRESS_PARAM Param;
    HANDLE Threads[COND_STRESS_CONSUMER_COUNT] = { NULL, };
    DWORD WaitResult;
    LONG CreatedCount = 0;
    LONG Index;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    RtlZeroMemory( &Param,
                   sizeof(Param) );
    InitializeCriticalSection( &Param.CriticalSection );
    InitializeConditionVariable( &Param.ConditionVariable );

    for (Index = 0; Index < COND_STRESS_CONSUMER_COUNT; Index++) {
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       ConditionVariableCSConsumerThreadProc,
                                       &Param,
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] CondCS stress CreateThread[%ld] ErrorCode = %lu\n",
                    Index,
                    GetLastError());
            Result = FALSE;
            goto Cleanup;
        }
        CreatedCount++;
    }

    if (! SyncWaitForCounter( &Param.ReadyCount,
                              CreatedCount,
                              5000 )) {
        fprintf(stderr,
                "[Failed] CondCS stress ready Ready = %ld Expected = %ld\n",
                Param.ReadyCount,
                CreatedCount);
        Result = FALSE;
        goto Cleanup;
    }

    for (Index = 0; Index < COND_STRESS_WORK_ITEMS; Index++) {
        EnterCriticalSection( &Param.CriticalSection );
        Param.WorkItems++;
        Param.Produced++;
        LeaveCriticalSection( &Param.CriticalSection );

        WakeConditionVariable( &Param.ConditionVariable );

        if ((Index & 7) == 0) {
            Sleep( 0 );
        }
    }

    if (! SyncWaitForCounter( (volatile LONG*)&Param.Consumed,
                              COND_STRESS_WORK_ITEMS,
                              SYNC_STRESS_TIMEOUT )) {
        fprintf(stderr,
                "[Failed] CondCS stress consume Produced = %ld Consumed = %ld Work = %ld FailureLine = %ld\n",
                Param.Produced,
                Param.Consumed,
                Param.WorkItems,
                Param.FailureLine);
        Result = FALSE;
    }

Cleanup:
    EnterCriticalSection( &Param.CriticalSection );
    Param.Done = TRUE;
    LeaveCriticalSection( &Param.CriticalSection );
    WakeAllConditionVariable( &Param.ConditionVariable );

    if (CreatedCount) {
        WaitResult = WaitForMultipleObjects( CreatedCount,
                                             Threads,
                                             TRUE,
                                             SYNC_STRESS_TIMEOUT );
        if (WaitResult != WAIT_OBJECT_0) {
            fprintf(stderr,
                    "[Failed] CondCS stress cleanup wait Wait = 0x%08x Ready = %ld Produced = %ld Consumed = %ld Work = %ld FailureLine = %ld\n",
                    WaitResult,
                    Param.ReadyCount,
                    Param.Produced,
                    Param.Consumed,
                    Param.WorkItems,
                    Param.FailureLine);
            Result = FALSE;
        }
        SyncWaitForThreads( Threads,
                            CreatedCount );
    }

    if (Param.Produced != Param.Consumed ||
        Param.WorkItems != 0 ||
        Param.FailureLine != 0) {
        fprintf(stderr,
                "[Failed] CondCS stress final Produced = %ld Consumed = %ld Work = %ld FailureLine = %ld\n",
                Param.Produced,
                Param.Consumed,
                Param.WorkItems,
                Param.FailureLine);
        Result = FALSE;
    } else if (Result) {
        printf("[Success] ConditionVariable CS producer-consumer stress\n\n");
    }

    for (Index = 0; Index < COND_STRESS_CONSUMER_COUNT; Index++) {
        if (Threads[Index]) {
            CloseHandle( Threads[Index] );
        }
    }
    DeleteCriticalSection( &Param.CriticalSection );
    return Result;
}

BOOLEAN
ConditionVariableSRWRegressionTest (
    VOID
    )
{
    COND_SRW_TEST_PARAM Param;
    HANDLE Threads[COND_SRW_TEST_THREAD_COUNT] = { NULL, };
    DWORD WaitResult;
    LONG CreatedCount = 0;
    LONG Index;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    RtlZeroMemory( &Param,
                   sizeof(Param) );
    InitializeSRWLock( &Param.SRWLock );
    InitializeConditionVariable( &Param.ConditionVariable );

    for (Index = 0; Index < COND_SRW_TEST_THREAD_COUNT; Index++) {
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       CondSRWTestThreadProc,
                                       &Param,
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] CondSRW CreateThread[%ld] ErrorCode = %lu\n",
                    Index,
                    GetLastError());
            Result = FALSE;
            goto Cleanup;
        }
        CreatedCount++;
    }

    while (Param.ReadyCount < COND_SRW_TEST_THREAD_COUNT) {
        Sleep( 1 );
    }

    AcquireSRWLockExclusive( &Param.SRWLock );
    Param.Ready = TRUE;
    ReleaseSRWLockExclusive( &Param.SRWLock );
    WakeAllConditionVariable( &Param.ConditionVariable );

    WaitResult = WaitForMultipleObjects( COND_SRW_TEST_THREAD_COUNT,
                                         Threads,
                                         TRUE,
                                         5000 );
    if (WaitResult != WAIT_OBJECT_0 ||
        Param.Processed != COND_SRW_TEST_THREAD_COUNT) {
        fprintf(stderr,
                "[Failed] CondSRW Wait = 0x%08x Ready = %ld Processed = %ld Expected = %ld\n",
                WaitResult,
                Param.ReadyCount,
                Param.Processed,
                (LONG)COND_SRW_TEST_THREAD_COUNT);
        Result = FALSE;
    }

Cleanup:
    AcquireSRWLockExclusive( &Param.SRWLock );
    Param.Ready = TRUE;
    ReleaseSRWLockExclusive( &Param.SRWLock );
    WakeAllConditionVariable( &Param.ConditionVariable );

    if (CreatedCount) {
        SyncWaitForThreads( Threads,
                            CreatedCount );
    }

    for (Index = 0; Index < COND_SRW_TEST_THREAD_COUNT; Index++) {
        if (Threads[Index]) {
            CloseHandle( Threads[Index] );
        }
    }
    return Result;
}

BOOLEAN
ConditionVariableSRWExclusiveStressTest (
    VOID
    )
{
    COND_SRW_EXCLUSIVE_STRESS_PARAM Param;
    HANDLE Threads[COND_STRESS_CONSUMER_COUNT] = { NULL, };
    DWORD WaitResult;
    LONG CreatedCount = 0;
    LONG Index;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    RtlZeroMemory( &Param,
                   sizeof(Param) );
    InitializeSRWLock( &Param.SRWLock );
    InitializeConditionVariable( &Param.ConditionVariable );

    for (Index = 0; Index < COND_STRESS_CONSUMER_COUNT; Index++) {
        Threads[Index] = CreateThread( NULL,
                                       0,
                                       ConditionVariableSRWExclusiveConsumerThreadProc,
                                       &Param,
                                       0,
                                       NULL );
        if (! Threads[Index]) {
            fprintf(stderr,
                    "[Failed] CondSRW exclusive CreateThread[%ld] ErrorCode = %lu\n",
                    Index,
                    GetLastError());
            Result = FALSE;
            goto Cleanup;
        }
        CreatedCount++;
    }

    if (! SyncWaitForCounter( &Param.ReadyCount,
                              CreatedCount,
                              5000 )) {
        fprintf(stderr,
                "[Failed] CondSRW exclusive ready Ready = %ld Expected = %ld\n",
                Param.ReadyCount,
                CreatedCount);
        Result = FALSE;
        goto Cleanup;
    }

    for (Index = 0; Index < COND_STRESS_WORK_ITEMS; Index++) {
        AcquireSRWLockExclusive( &Param.SRWLock );
        Param.WorkItems++;
        Param.Produced++;
        ReleaseSRWLockExclusive( &Param.SRWLock );

        WakeConditionVariable( &Param.ConditionVariable );

        if ((Index & 7) == 0) {
            Sleep( 0 );
        }
    }

    if (! SyncWaitForCounter( (volatile LONG*)&Param.Consumed,
                              COND_STRESS_WORK_ITEMS,
                              SYNC_STRESS_TIMEOUT )) {
        fprintf(stderr,
                "[Failed] CondSRW exclusive consume Produced = %ld Consumed = %ld Work = %ld FailureLine = %ld\n",
                Param.Produced,
                Param.Consumed,
                Param.WorkItems,
                Param.FailureLine);
        Result = FALSE;
    }

Cleanup:
    AcquireSRWLockExclusive( &Param.SRWLock );
    Param.Done = TRUE;
    ReleaseSRWLockExclusive( &Param.SRWLock );
    WakeAllConditionVariable( &Param.ConditionVariable );

    if (CreatedCount) {
        WaitResult = WaitForMultipleObjects( CreatedCount,
                                             Threads,
                                             TRUE,
                                             SYNC_STRESS_TIMEOUT );
        if (WaitResult != WAIT_OBJECT_0) {
            fprintf(stderr,
                    "[Failed] CondSRW exclusive cleanup wait Wait = 0x%08x Ready = %ld Produced = %ld Consumed = %ld Work = %ld FailureLine = %ld\n",
                    WaitResult,
                    Param.ReadyCount,
                    Param.Produced,
                    Param.Consumed,
                    Param.WorkItems,
                    Param.FailureLine);
            Result = FALSE;
        }
        SyncWaitForThreads( Threads,
                            CreatedCount );
    }

    if (Param.Produced != Param.Consumed ||
        Param.WorkItems != 0 ||
        Param.FailureLine != 0) {
        fprintf(stderr,
                "[Failed] CondSRW exclusive final Produced = %ld Consumed = %ld Work = %ld FailureLine = %ld\n",
                Param.Produced,
                Param.Consumed,
                Param.WorkItems,
                Param.FailureLine);
        Result = FALSE;
    } else if (Result) {
        printf("[Success] ConditionVariable SRW exclusive stress\n\n");
    }

    for (Index = 0; Index < COND_STRESS_CONSUMER_COUNT; Index++) {
        if (Threads[Index]) {
            CloseHandle( Threads[Index] );
        }
    }
    return Result;
}

BOOLEAN
ConditionVariableTest (
    VOID
    )
{
    PAGED_CODE();

    if (! CriticalSectionRegressionTest()) {
        return FALSE;
    }

    if (! CriticalSectionContentionStressTest()) {
        return FALSE;
    }

    if (! SRWLockRegressionTest()) {
        return FALSE;
    }

    if (! SRWLockMixedStressTest()) {
        return FALSE;
    }

    if (! ConditionVariableCSProducerConsumerStressTest()) {
        return FALSE;
    }

    if (! ConditionVariableSRWRegressionTest()) {
        return FALSE;
    }

    if (! ConditionVariableSRWExclusiveStressTest()) {
        return FALSE;
    }

    COND_TEST_THREAD_PARAM param;
    param.name = "Test 1";
    param.ready = FALSE;
    param.processed = 0;
    InitializeCriticalSection(&param.cs);
    InitializeConditionVariable(&param.cv);
    HANDLE threads[COND_TEST_THREAD_COUNT];
    for (int i = 0; i < COND_TEST_THREAD_COUNT; ++i) {
        threads[i] = CreateThread(NULL, 0, CondTestThreadProc, &param, 0, NULL);
    }

    COND_TEST_THREAD_PARAM param2;
    param2.name = "Test 2";
    param2.ready = FALSE;
    param2.processed = 0;
    InitializeCriticalSection(&param2.cs);
    InitializeConditionVariable(&param2.cv);
    HANDLE threads2[COND_TEST_THREAD_COUNT];
    for (int i = 0; i < COND_TEST_THREAD_COUNT; ++i) {
        threads2[i] = CreateThread(NULL, 0, CondTestThreadProc, &param2, 0, NULL);
    }

    EnterCriticalSection(&param.cs);
    param.ready = TRUE;
    LeaveCriticalSection(&param.cs);
    WakeAllConditionVariable(&param.cv);

    EnterCriticalSection(&param.cs);
    while (param.processed < COND_TEST_THREAD_COUNT) {
        SleepConditionVariableCS(&param.cv, &param.cs, INFINITE);
    }
    WakeAllConditionVariable(&param.cv);
    LeaveCriticalSection(&param.cs);

    WaitForMultipleObjects(COND_TEST_THREAD_COUNT, threads, TRUE, INFINITE);
    DeleteCriticalSection(&param.cs);



    EnterCriticalSection(&param2.cs);
    param2.ready = TRUE;
    LeaveCriticalSection(&param2.cs);
    WakeAllConditionVariable(&param2.cv);

    EnterCriticalSection(&param2.cs);
    while (param2.processed < COND_TEST_THREAD_COUNT) {
        SleepConditionVariableCS(&param2.cv, &param2.cs, INFINITE);
    }
    WakeAllConditionVariable(&param2.cv);
    LeaveCriticalSection(&param2.cs);

    WaitForMultipleObjects(COND_TEST_THREAD_COUNT, threads2, TRUE, INFINITE);
    DeleteCriticalSection(&param2.cs);



    for (int i = 0; i < COND_TEST_THREAD_COUNT; ++i) {
        if (threads[i]) {
            CloseHandle(threads[i]);
        }
    }

    for (int i = 0; i < COND_TEST_THREAD_COUNT; ++i) {
        if (threads2[i]) {
            CloseHandle(threads2[i]);
        }
    }

    return TRUE;
}
