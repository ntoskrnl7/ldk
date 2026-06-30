#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
FibersTest (
    VOID
    );

BOOLEAN
TlsApiTest (
    VOID
    );

BOOLEAN
FlsGetValue2Test (
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
#pragma alloc_text(PAGE, TlsApiTest)
#pragma alloc_text(PAGE, FlsGetValue2Test)
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

typedef PVOID (WINAPI *PFN_FLS_GET_VALUE2)(
    _In_ DWORD dwFlsIndex
    );

BOOLEAN
TlsApiTest (
    VOID
    )
{
    DWORD Index;
    DWORD Value = 0x12345678;
    PVOID Result;

    PAGED_CODE();

    Index = TlsAlloc();
    if (Index == TLS_OUT_OF_INDEXES) {
        fprintf(stderr,
                "[Failed] TlsAlloc ErrorCode = %lu\n",
                GetLastError());
        return FALSE;
    }

    SetLastError( ERROR_ACCESS_DENIED );
    Result = TlsGetValue( Index );
    if (Result != NULL ||
        GetLastError() != ERROR_SUCCESS) {
        fprintf(stderr,
                "[Failed] TlsGetValue initial Result = %p ErrorCode = %lu\n",
                Result,
                GetLastError());
        TlsFree( Index );
        return FALSE;
    }

    if (! TlsSetValue( Index,
                       &Value )) {
        fprintf(stderr,
                "[Failed] TlsSetValue ErrorCode = %lu\n",
                GetLastError());
        TlsFree( Index );
        return FALSE;
    }

    SetLastError( ERROR_ACCESS_DENIED );
    Result = TlsGetValue( Index );
    if (Result != &Value ||
        GetLastError() != ERROR_SUCCESS) {
        fprintf(stderr,
                "[Failed] TlsGetValue Result = %p Expected = %p ErrorCode = %lu\n",
                Result,
                &Value,
                GetLastError());
        TlsFree( Index );
        return FALSE;
    }

    if (! TlsFree( Index )) {
        fprintf(stderr,
                "[Failed] TlsFree ErrorCode = %lu\n",
                GetLastError());
        return FALSE;
    }

    if (TlsFree( Index ) ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
        fprintf(stderr,
                "[Failed] TlsFree freed index ErrorCode = %lu\n",
                GetLastError());
        return FALSE;
    }

    printf("[Success] TLS API test\n\n");
    return TRUE;
}

BOOLEAN
FlsGetValue2Test (
    VOID
    )
{
    DWORD Index;
    DWORD Value = 0x87654321;
    PVOID Result;
    HMODULE Kernel32;
    PFN_FLS_GET_VALUE2 pFlsGetValue2;

    PAGED_CODE();

    Kernel32 = GetModuleHandleW( L"kernel32.dll" );
    pFlsGetValue2 = Kernel32 ?
        (PFN_FLS_GET_VALUE2)GetProcAddress( Kernel32,
                                            "FlsGetValue2" ) :
        NULL;
    if (! pFlsGetValue2) {
        printf("[Skipped] FlsGetValue2 unavailable\n\n");
        return TRUE;
    }

    Index = FlsAlloc( NULL );
    if (Index == FLS_OUT_OF_INDEXES) {
        fprintf(stderr,
                "[Failed] FlsAlloc ErrorCode = %lu\n",
                GetLastError());
        return FALSE;
    }

    if (! FlsSetValue( Index,
                       &Value )) {
        fprintf(stderr,
                "[Failed] FlsSetValue ErrorCode = %lu\n",
                GetLastError());
        FlsFree( Index );
        return FALSE;
    }

    SetLastError( ERROR_ACCESS_DENIED );
    Result = pFlsGetValue2( Index );
    if (Result != &Value ||
        GetLastError() != ERROR_ACCESS_DENIED) {
        fprintf(stderr,
                "[Failed] FlsGetValue2 Result = %p Expected = %p ErrorCode = %lu\n",
                Result,
                &Value,
                GetLastError());
        FlsFree( Index );
        return FALSE;
    }

    FlsFree( Index );

    printf("[Success] FlsGetValue2 test\n\n");
    return TRUE;
}

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
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    printf("Fibers Test\n");

    if (! TlsApiTest()) {
        return FALSE;
    }
    if (! FlsGetValue2Test()) {
        return FALSE;
    }

    hEvent = CreateEventA( NULL, TRUE, FALSE, NULL );
    if (hEvent == NULL) {
        return FALSE;
    }
    InitializeConditionVariable( &cv );
    InitializeCriticalSection( &cs );
    LONG count = 0;
    FlsIndex = FlsAlloc( FlsCallback );
    if (FlsIndex == FLS_OUT_OF_INDEXES) {
        CloseHandle( hEvent );
        return FALSE;
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
        Result = FALSE;
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
        Result = FALSE;
    }

    return Result;
}
