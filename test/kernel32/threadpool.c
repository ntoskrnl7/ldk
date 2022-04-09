#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
LegacyThreadPoolTest (
    VOID
    );

DWORD
WINAPI
LegacyThreadWorkFunction (
    PLONG Count
    );

BOOLEAN
ThreadPoolTest (
    VOID
    );

VOID
NTAPI
ThreadPoolTestWorkCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LegacyThreadPoolTest)
#pragma alloc_text(PAGE, LegacyThreadWorkFunction)
#pragma alloc_text(PAGE, ThreadPoolTest)
#pragma alloc_text(PAGE, ThreadPoolTestWorkCallback)
#endif
#else
#include <windows.h>
#include <stdio.h>

#define DbgPrint        printf
#define PAGED_CODE()
#endif

DWORD
WINAPI
LegacyThreadWorkFunction (
    PLONG Count
    )
{
    PAGED_CODE();

    InterlockedIncrement( Count );

    return 0;
}

BOOLEAN
LegacyThreadPoolTest (
    VOID
    )
{
    LONG count = 10;
    LONG processed = 0;

    PAGED_CODE();

    for (count = 0; count < 10; count++) {
        if (! QueueUserWorkItem(LegacyThreadWorkFunction, &processed, WT_EXECUTEDEFAULT)) {
            break;
        }
    }

    while (InterlockedCompareExchange( &processed, 0, count ) != count) {
        YieldProcessor();
    }
    return TRUE;
}



VOID
NTAPI
ThreadPoolTestWorkCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    )
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    PAGED_CODE();

    InterlockedDecrement((PLONG)Context);
}

BOOLEAN
ThreadPoolTest (
    VOID
    )
{
    PTP_WORK works[10] = { NULL, };
    LONG count = ARRAYSIZE(works);

    PAGED_CODE();

    for (int i = 0; i < ARRAYSIZE(works); ++i) {
        works[i] = CreateThreadpoolWork(ThreadPoolTestWorkCallback, &count, NULL);
        if (works[i] == NULL) {
            goto Cleanup;
        }
    }

    for (int i = 0; i < ARRAYSIZE(works); ++i) {
        SubmitThreadpoolWork(works[i]);
    }

Cleanup:
    for (int i = 0; i < ARRAYSIZE(works); ++i) {
        if (works[i]) {
            WaitForThreadpoolWorkCallbacks(works[i], FALSE);
            CloseThreadpoolWork(works[i]);
        }
    }

    return count == 0;
}