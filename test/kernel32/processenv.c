#if _KERNEL_MODE
#include <Ldk/Windows.h>
#else
#include <windows.h>
#include <stdio.h>
#define PAGED_CODE()
#define DbgPrintEx(_id_, _level_, ...) printf(__VA_ARGS__)
#define DPFLTR_IHVDRIVER_ID 0
#define DPFLTR_INFO_LEVEL 0
#define DPFLTR_ERROR_LEVEL 1
#endif

BOOLEAN
ProcessEnvironmentTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ProcessEnvironmentTest)
#endif

#define printf(...)         DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)
#define fprintf(_f_, ...)   DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__)
#define stderr              DPFLTR_ERROR_LEVEL

static
BOOLEAN
ProcessEnvironmentExpect (
    _In_ BOOL Condition,
    _In_ PCSTR Message
    )
{
    if (! Condition) {
        fprintf(stderr, "[Failed] %s (LastError = %lu)\n", Message, GetLastError());
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
ProcessEnvironmentStringEqualsA (
    _In_z_ PCSTR Left,
    _In_z_ PCSTR Right
    )
{
    while (*Left || *Right) {
        if (*Left != *Right) {
            return FALSE;
        }

        Left++;
        Right++;
    }

    return TRUE;
}

static
BOOLEAN
ProcessEnvironmentStringEqualsW (
    _In_z_ PCWSTR Left,
    _In_z_ PCWSTR Right
    )
{
    while (*Left || *Right) {
        if (*Left != *Right) {
            return FALSE;
        }

        Left++;
        Right++;
    }

    return TRUE;
}

static
SIZE_T
ProcessEnvironmentStringLengthA (
    _In_z_ PCSTR String
    )
{
    SIZE_T Length = 0;

    while (String[Length] != '\0') {
        Length++;
    }

    return Length;
}

static
SIZE_T
ProcessEnvironmentStringLengthW (
    _In_z_ PCWSTR String
    )
{
    SIZE_T Length = 0;

    while (String[Length] != UNICODE_NULL) {
        Length++;
    }

    return Length;
}

static
BOOLEAN
ProcessEnvironmentBlockContainsA (
    _In_ PCCH Environment,
    _In_z_ PCSTR Entry
    )
{
    SIZE_T EntryLength = ProcessEnvironmentStringLengthA( Entry );
    PCCH Cursor = Environment;

    while (*Cursor != '\0') {
        if (ProcessEnvironmentStringEqualsA( Cursor,
                                             Entry )) {
            return TRUE;
        }

        Cursor += ProcessEnvironmentStringLengthA( Cursor ) + 1;
    }

    UNREFERENCED_PARAMETER(EntryLength);
    return FALSE;
}

static
BOOLEAN
ProcessEnvironmentBlockContainsW (
    _In_ PCWCH Environment,
    _In_z_ PCWSTR Entry
    )
{
    PCWCH Cursor = Environment;

    while (*Cursor != UNICODE_NULL) {
        if (ProcessEnvironmentStringEqualsW( Cursor,
                                             Entry )) {
            return TRUE;
        }

        Cursor += ProcessEnvironmentStringLengthW( Cursor ) + 1;
    }

    return FALSE;
}

static
BOOLEAN
ProcessEnvironmentStdHandleTest (
    VOID
    )
{
    HANDLE OriginalOutput;
    HANDLE Previous;
    HANDLE RestoredPrevious;
    HANDLE TestHandle = (HANDLE)(ULONG_PTR)0x12345678;
    BOOL Result = TRUE;

    PAGED_CODE();

    OriginalOutput = GetStdHandle( STD_OUTPUT_HANDLE );

    Result &= ProcessEnvironmentExpect( SetStdHandleEx( STD_OUTPUT_HANDLE,
                                                        TestHandle,
                                                        &Previous ),
                                        "SetStdHandleEx(STD_OUTPUT_HANDLE)" );
    Result &= ProcessEnvironmentExpect( Previous == OriginalOutput &&
                                        GetStdHandle( STD_OUTPUT_HANDLE ) == TestHandle,
                                        "GetStdHandle returns updated STD_OUTPUT_HANDLE" );

    Result &= ProcessEnvironmentExpect( SetStdHandleEx( STD_OUTPUT_HANDLE,
                                                        OriginalOutput,
                                                        &RestoredPrevious ),
                                        "SetStdHandleEx restores STD_OUTPUT_HANDLE" );
    Result &= ProcessEnvironmentExpect( RestoredPrevious == TestHandle &&
                                        GetStdHandle( STD_OUTPUT_HANDLE ) == OriginalOutput,
                                        "SetStdHandleEx returns previous handle while restoring" );

    Result &= ProcessEnvironmentExpect( ! SetStdHandleEx( 0x1234,
                                                          TestHandle,
                                                          &Previous ),
                                        "SetStdHandleEx rejects invalid standard handle" );

    return Result ? TRUE : FALSE;
}

static
BOOLEAN
ProcessEnvironmentVariableTest (
    VOID
    )
{
    static const WCHAR NameW[] = L"LDK_PROCESSENV_TEST";
    static const WCHAR ValueW[] = L"Value42";
    static const WCHAR EntryW[] = L"LDK_PROCESSENV_TEST=Value42";
    static const CHAR NameA[] = "LDK_PROCESSENV_TEST";
    static const CHAR ValueA[] = "Value42";
    static const CHAR EntryA[] = "LDK_PROCESSENV_TEST=Value42";
    WCHAR WideBuffer[32];
    CHAR AnsiBuffer[32];
    WCHAR SmallWideBuffer[4];
    CHAR SmallAnsiBuffer[4];
    LPWCH EnvironmentW;
    LPCH EnvironmentA;
    DWORD Length;
    BOOL Result = TRUE;

    PAGED_CODE();

    SetEnvironmentVariableW( NameW,
                             NULL );

    Result &= ProcessEnvironmentExpect( SetEnvironmentVariableW( NameW,
                                                                 ValueW ),
                                        "SetEnvironmentVariableW(test variable)" );

    Length = GetEnvironmentVariableW( NameW,
                                      NULL,
                                      0 );
    Result &= ProcessEnvironmentExpect( Length == RTL_NUMBER_OF(ValueW),
                                        "GetEnvironmentVariableW size query includes terminator" );

    Length = GetEnvironmentVariableW( NameW,
                                      SmallWideBuffer,
                                      RTL_NUMBER_OF(SmallWideBuffer) );
    Result &= ProcessEnvironmentExpect( Length == RTL_NUMBER_OF(ValueW),
                                        "GetEnvironmentVariableW reports required length for small buffer" );

    Length = GetEnvironmentVariableW( NameW,
                                      WideBuffer,
                                      RTL_NUMBER_OF(WideBuffer) );
    Result &= ProcessEnvironmentExpect( Length == RTL_NUMBER_OF(ValueW) - 1 &&
                                        ProcessEnvironmentStringEqualsW( WideBuffer,
                                                                         ValueW ),
                                        "GetEnvironmentVariableW reads value" );

    Length = GetEnvironmentVariableA( NameA,
                                      SmallAnsiBuffer,
                                      sizeof(SmallAnsiBuffer) );
    Result &= ProcessEnvironmentExpect( Length == sizeof(ValueA),
                                        "GetEnvironmentVariableA reports required length for small buffer" );

    Length = GetEnvironmentVariableA( NameA,
                                      AnsiBuffer,
                                      sizeof(AnsiBuffer) );
    Result &= ProcessEnvironmentExpect( Length == sizeof(ValueA) - 1 &&
                                        ProcessEnvironmentStringEqualsA( AnsiBuffer,
                                                                         ValueA ),
                                        "GetEnvironmentVariableA reads value" );

    EnvironmentW = GetEnvironmentStringsW();
    Result &= ProcessEnvironmentExpect( EnvironmentW != NULL,
                                        "GetEnvironmentStringsW" );
    if (EnvironmentW != NULL) {
        Result &= ProcessEnvironmentExpect( ProcessEnvironmentBlockContainsW( EnvironmentW,
                                                                              EntryW ),
                                            "GetEnvironmentStringsW contains test variable" );
        Result &= ProcessEnvironmentExpect( FreeEnvironmentStringsW( EnvironmentW ),
                                            "FreeEnvironmentStringsW" );
    }

    EnvironmentA = GetEnvironmentStrings();
    Result &= ProcessEnvironmentExpect( EnvironmentA != NULL,
                                        "GetEnvironmentStringsA" );
    if (EnvironmentA != NULL) {
        Result &= ProcessEnvironmentExpect( ProcessEnvironmentBlockContainsA( EnvironmentA,
                                                                              EntryA ),
                                            "GetEnvironmentStringsA contains test variable" );
        Result &= ProcessEnvironmentExpect( FreeEnvironmentStringsA( EnvironmentA ),
                                            "FreeEnvironmentStringsA" );
    }

    Result &= ProcessEnvironmentExpect( SetEnvironmentVariableW( NameW,
                                                                 NULL ),
                                        "SetEnvironmentVariableW deletes test variable" );

    SetLastError( ERROR_SUCCESS );
    Length = GetEnvironmentVariableW( NameW,
                                      WideBuffer,
                                      RTL_NUMBER_OF(WideBuffer) );
    Result &= ProcessEnvironmentExpect( Length == 0,
                                        "GetEnvironmentVariableW no longer finds deleted variable" );

    return Result ? TRUE : FALSE;
}

static
BOOLEAN
ProcessEnvironmentCommandLineTest (
    VOID
    )
{
    LPWSTR CommandLineW;

    PAGED_CODE();

    CommandLineW = GetCommandLineW();
    return ProcessEnvironmentExpect( CommandLineW != NULL &&
                                     CommandLineW[0] != UNICODE_NULL,
                                     "GetCommandLineW returns non-empty command line" );
}

BOOLEAN
ProcessEnvironmentTest (
    VOID
    )
{
    BOOL Result = TRUE;

    PAGED_CODE();

    printf("Process Environment Test\n");

    Result &= ProcessEnvironmentStdHandleTest();
    Result &= ProcessEnvironmentVariableTest();
    Result &= ProcessEnvironmentCommandLineTest();

    if (Result) {
        printf("[Success] Process Environment Test\n\n");
    } else {
        printf("[Failed] Process Environment Test\n\n");
    }

    return Result ? TRUE : FALSE;
}
