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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ConditionVariableTest)
#pragma alloc_text(PAGE, CondTestThreadProc)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <stdio.h>

#define PAGED_CODE()
#endif

typedef struct _COND_TEST_THREAD_PARAM {
    PCSTR name;
    BOOL ready;
    LONG processed;
    CONDITION_VARIABLE cv;
    CRITICAL_SECTION cs;
} COND_TEST_THREAD_PARAM, *PCOND_TEST_THREAD_PARAM;

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

BOOLEAN
ConditionVariableTest (
    VOID
    )
{
    PAGED_CODE();

    COND_TEST_THREAD_PARAM param;
    param.name = "Test 1";
    param.ready = FALSE;
    param.processed = 0;
    InitializeCriticalSection(&param.cs);
    InitializeConditionVariable(&param.cv);
    HANDLE threads[5];
    for (int i = 0; i < 5; ++i) {
        threads[i] = CreateThread(NULL, 0, CondTestThreadProc, &param, 0, NULL);
    }

    COND_TEST_THREAD_PARAM param2;
    param2.name = "Test 2";
    param2.ready = FALSE;
    param2.processed = 0;
    InitializeCriticalSection(&param2.cs);
    InitializeConditionVariable(&param2.cv);
    HANDLE threads2[5];
    for (int i = 0; i < 5; ++i) {
        threads2[i] = CreateThread(NULL, 0, CondTestThreadProc, &param2, 0, NULL);
    }

    EnterCriticalSection(&param.cs);
    param.ready = TRUE;
    LeaveCriticalSection(&param.cs);
    WakeAllConditionVariable(&param.cv);

    EnterCriticalSection(&param.cs);
    while (param.processed < 5) {
        SleepConditionVariableCS(&param.cv, &param.cs, INFINITE);
    }
    WakeAllConditionVariable(&param.cv);
    LeaveCriticalSection(&param.cs);

    WaitForMultipleObjects(5, threads, TRUE, INFINITE);
    DeleteCriticalSection(&param.cs);



    EnterCriticalSection(&param2.cs);
    param2.ready = TRUE;
    LeaveCriticalSection(&param2.cs);
    WakeAllConditionVariable(&param2.cv);

    EnterCriticalSection(&param2.cs);
    while (param2.processed < 5) {
        SleepConditionVariableCS(&param2.cv, &param2.cs, INFINITE);
    }
    WakeAllConditionVariable(&param2.cv);
    LeaveCriticalSection(&param2.cs);

    WaitForMultipleObjects(5, threads2, TRUE, INFINITE);
    DeleteCriticalSection(&param2.cs);



    for (int i = 0; i < 5; ++i) {
        if (threads[i]) {
            CloseHandle(threads[i]);
        }
    }

    for (int i = 0; i < 5; ++i) {
        if (threads2[i]) {
            CloseHandle(threads2[i]);
        }
    }

    return TRUE;
}