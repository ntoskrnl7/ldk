#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
AdvapiCompatibilityTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, AdvapiCompatibilityTest)
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

typedef
BOOLEAN
(WINAPI *PLDK_SYSTEM_FUNCTION_036) (
    _Out_writes_bytes_(RandomBufferLength) PVOID RandomBuffer,
    _In_ ULONG RandomBufferLength
    );

static
BOOLEAN
LdkpBufferEqualsByte (
    _In_reads_bytes_(Length) const UCHAR *Buffer,
    _In_ ULONG Length,
    _In_ UCHAR Value
    )
{
    for (ULONG Index = 0; Index < Length; Index++) {
        if (Buffer[Index] != Value) {
            return FALSE;
        }
    }

    return TRUE;
}

static
BOOLEAN
LdkpBuffersEqual (
    _In_reads_bytes_(Length) const UCHAR *Left,
    _In_reads_bytes_(Length) const UCHAR *Right,
    _In_ ULONG Length
    )
{
    for (ULONG Index = 0; Index < Length; Index++) {
        if (Left[Index] != Right[Index]) {
            return FALSE;
        }
    }

    return TRUE;
}

static
BOOLEAN
LdkpVerifyRandomRoutine (
    _In_z_ PCSTR Name,
    _In_ PLDK_SYSTEM_FUNCTION_036 RandomRoutine
    )
{
    UCHAR First[32];
    UCHAR Second[32];

    RtlFillMemory( First,
                   sizeof(First),
                   0xa5 );
    RtlFillMemory( Second,
                   sizeof(Second),
                   0xa5 );

    if (!RandomRoutine( First,
                        sizeof(First) )) {
        fprintf(stderr,
                "[Failed] %s first call failed\n",
                Name );
        return FALSE;
    }

    if (!RandomRoutine( Second,
                        sizeof(Second) )) {
        fprintf(stderr,
                "[Failed] %s second call failed\n",
                Name );
        return FALSE;
    }

    if (LdkpBufferEqualsByte( First,
                              sizeof(First),
                              0xa5 ) ||
        LdkpBufferEqualsByte( Second,
                              sizeof(Second),
                              0xa5 )) {
        fprintf(stderr,
                "[Failed] %s did not modify the output buffer\n",
                Name );
        return FALSE;
    }

    if (LdkpBuffersEqual( First,
                          Second,
                          sizeof(First) )) {
        fprintf(stderr,
                "[Failed] %s returned identical buffers on repeated calls\n",
                Name );
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
AdvapiCompatibilityTest (
    VOID
    )
{
    HMODULE Advapi;
    PLDK_SYSTEM_FUNCTION_036 SystemFunction036Routine;
    PLDK_SYSTEM_FUNCTION_036 RtlGenRandomRoutine;

    PAGED_CODE();

    printf("ADVAPI compatibility test\n");

    Advapi = LoadLibraryW( L"advapi32.dll" );
    if (Advapi == NULL) {
        fprintf(stderr,
                "[Failed] LoadLibraryW(advapi32.dll) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    SystemFunction036Routine = (PLDK_SYSTEM_FUNCTION_036)GetProcAddress( Advapi,
                                                                         "SystemFunction036" );
    if (SystemFunction036Routine == NULL) {
        fprintf(stderr,
                "[Failed] GetProcAddress(advapi32.dll, SystemFunction036) ErrorCode = %lu\n",
                GetLastError() );
        FreeLibrary( Advapi );
        return FALSE;
    }

    if (!LdkpVerifyRandomRoutine( "SystemFunction036",
                                  SystemFunction036Routine )) {
        FreeLibrary( Advapi );
        return FALSE;
    }

    RtlGenRandomRoutine = (PLDK_SYSTEM_FUNCTION_036)GetProcAddress( Advapi,
                                                                    "RtlGenRandom" );
    if (RtlGenRandomRoutine == NULL) {
        printf("[Skipped] advapi32.dll RtlGenRandom export alias is unavailable\n");
    } else if (!LdkpVerifyRandomRoutine( "RtlGenRandom",
                                         RtlGenRandomRoutine )) {
        FreeLibrary( Advapi );
        return FALSE;
    }

    if (!FreeLibrary( Advapi )) {
        fprintf(stderr,
                "[Failed] FreeLibrary(advapi32.dll) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    printf("[Success] ADVAPI compatibility test\n\n");
    return TRUE;
}
