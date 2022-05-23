#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
FibersTest (
    VOID
    );

DWORD
WINAPI
FibersTestThreadProc(
    LPVOID lpThreadParameter
    );

DWORD
WINAPI
FibersTestThreadProc2 (
    LPVOID lpThreadParameter
    );

VOID
NTAPI
FlsCallback (
	_In_ PVOID lpFlsData
	);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FibersTest)
#pragma alloc_text(PAGE, FibersTestThreadProc)
#pragma alloc_text(PAGE, FibersTestThreadProc2)
#pragma alloc_text(PAGE, FlsCallback)
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

DWORD FlsIndex = FLS_OUT_OF_INDEXES;
CONDITION_VARIABLE cv;
CRITICAL_SECTION cs;
HANDLE hEvent;

DWORD
WINAPI
FibersTestThreadProc2 (
    LPVOID lpThreadParameter
    )
{
    PAGED_CODE();
    if (lpThreadParameter == NULL) {
        return 0;
    }
    if (FlsSetValue( FlsIndex, lpThreadParameter )) {
        InterlockedIncrement( (PLONG)lpThreadParameter );
    }
    return 0;
}

DWORD
WINAPI
FibersTestThreadProc (
    LPVOID lpThreadParameter
    )
{
    PAGED_CODE();
    if (lpThreadParameter == NULL) {
        return 0;
    }
    if (! FlsSetValue( FlsIndex, lpThreadParameter )) {
        return 0;
    }

    EnterCriticalSection( &cs );

    PLONG count = (PLONG)lpThreadParameter;
    *count = *count + 1;
    LeaveCriticalSection( &cs );

    WakeConditionVariable( &cv );

    WaitForSingleObject( hEvent, INFINITE );
    return 0;
}

VOID
NTAPI
FlsCallback (
	_In_ PVOID lpFlsData
	)
{
    PAGED_CODE();
    InterlockedDecrement( (PLONG)lpFlsData );
}

BOOLEAN
FibersTest (
    VOID
    )
{
    PAGED_CODE();

    KdBreakPoint();

    printf("Fibers Test\n");

    hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
    if (hEvent == NULL) {
        return TRUE;
    }
    InitializeConditionVariable( &cv );
    InitializeCriticalSection( &cs );
    LONG count = 0;
    FlsIndex = FlsAlloc( FlsCallback );
    if (FlsIndex == FLS_OUT_OF_INDEXES) {
        CloseHandle( hEvent );
        return TRUE;
    }

    HANDLE threads[5];
    for (int i = 0; i < 5; ++i) {
        threads[i] = CreateThread( NULL, 0, FibersTestThreadProc, &count, 0, NULL );
    }

    EnterCriticalSection( &cs );
    while (count != 5) {
        SleepConditionVariableCS( &cv, &cs, INFINITE );
    }
    LeaveCriticalSection( &cs );

    LONG count2 = 0;
    HANDLE threads2[5];
    for (int i = 0; i < 5; ++i) {
        threads2[i] = CreateThread( NULL, 0, FibersTestThreadProc2, &count2, 0, NULL );
    }
    WaitForMultipleObjects( 5, threads2, TRUE, INFINITE );
    if (count2 == 0) {
        printf("[Success] Fibers Test\n\n");
    } else {
        printf("[Failed] Fibers Test\n\n");
    }

    FlsFree( FlsIndex );
    SetEvent( hEvent );    
    WaitForMultipleObjects( 5, threads, TRUE, INFINITE );
    for (int i = 0; i < 5; ++i) {
        if (threads[i]) {
            CloseHandle(threads[i]);
        }
    }
    CloseHandle( hEvent );

    if (count == 0) {
        printf("[Success] Fibers Test\n\n");
    } else {
        printf("[Failed] Fibers Test\n\n");
    }

    return TRUE;
}