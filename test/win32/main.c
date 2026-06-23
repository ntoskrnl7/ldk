#include <windows.h>
#include <stdio.h>
#include <string.h>

typedef BOOLEAN (*LDK_WIN32_TEST_ROUTINE)(VOID);

typedef struct _LDK_WIN32_TEST_ENTRY {
    PCSTR Name;
    LDK_WIN32_TEST_ROUTINE Routine;
} LDK_WIN32_TEST_ENTRY, *PLDK_WIN32_TEST_ENTRY;

static WCHAR LdkpWin32WorkDirectory[MAX_PATH];

BOOLEAN
NtdllCurrentDirectoryTest (
    VOID
    );

BOOLEAN
NtdllEnvironmentVariableTest (
    VOID
    );

BOOLEAN
KeyedEventTest (
    VOID
    );

BOOLEAN
LdrTest (
    VOID
    );

BOOLEAN
FibersTest (
    VOID
    );

BOOLEAN
FileTest (
    VOID
    );

BOOLEAN
HeapApiTest (
    VOID
    );

BOOLEAN
FormatMessageTest (
    VOID
    );

BOOLEAN
NlsTest (
    VOID
    );

BOOLEAN
CurrentDirectoryTest (
    VOID
    );

BOOLEAN
EnvironmentVariableTest (
    VOID
    );

BOOLEAN
LegacyThreadPoolTest (
    VOID
    );

BOOLEAN
ThreadPoolTest (
    VOID
    );

BOOLEAN
ConditionVariableTest (
    VOID
    );

BOOLEAN
WaitOnAddressTest (
    VOID
    );

BOOLEAN
LibraryTest (
    VOID
    );

static
BOOL
LdkpConfigureTestDirectories (
    VOID
    )
{
    WCHAR ModulePath[MAX_PATH];
    WCHAR TempPath[MAX_PATH];
    WCHAR WorkDirectory[MAX_PATH];
    DWORD Length;
    DWORD TempLength;

    Length = GetModuleFileNameW( NULL,
                                 ModulePath,
                                 RTL_NUMBER_OF(ModulePath) );
    if (Length == 0 ||
        Length >= RTL_NUMBER_OF(ModulePath)) {
        fprintf(stderr,
                "[LdkWin32ApiTest] GetModuleFileNameW failed: %lu\n",
                GetLastError() );
        return FALSE;
    }

    for (DWORD Index = Length; Index != 0; Index--) {
        if (ModulePath[Index - 1] == L'\\' ||
            ModulePath[Index - 1] == L'/') {
            ModulePath[Index - 1] = UNICODE_NULL;
            break;
        }
    }

    if (! SetDllDirectoryW( ModulePath )) {
        fprintf(stderr,
                "[LdkWin32ApiTest] Failed to add executable directory to DLL search path: %lu\n",
                GetLastError() );
        return FALSE;
    }

    TempLength = GetTempPathW( RTL_NUMBER_OF(TempPath),
                               TempPath );
    if (TempLength == 0 ||
        TempLength >= RTL_NUMBER_OF(TempPath)) {
        fprintf(stderr,
                "[LdkWin32ApiTest] GetTempPathW failed: %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (swprintf_s( WorkDirectory,
                    RTL_NUMBER_OF(WorkDirectory),
                    L"%sLdkWin32ApiTest-%lu",
                    TempPath,
                    GetCurrentProcessId() ) < 0) {
        fprintf(stderr,
                "[LdkWin32ApiTest] Work directory path is too long\n" );
        return FALSE;
    }

    if (! CreateDirectoryW( WorkDirectory,
                            NULL ) &&
        GetLastError() != ERROR_ALREADY_EXISTS) {
        fprintf(stderr,
                "[LdkWin32ApiTest] CreateDirectoryW(%ws) failed: %lu\n",
                WorkDirectory,
                GetLastError() );
        return FALSE;
    }

    if (! SetCurrentDirectoryW( WorkDirectory )) {
        fprintf(stderr,
                "[LdkWin32ApiTest] Failed to enter work directory %ws: %lu\n",
                WorkDirectory,
                GetLastError() );
        return FALSE;
    }

    wcscpy_s( LdkpWin32WorkDirectory,
              RTL_NUMBER_OF(LdkpWin32WorkDirectory),
              WorkDirectory );

    return TRUE;
}

int
main (
    _In_ int argc,
    _In_reads_(argc) char** argv
    )
{
    PCSTR Filter = argc > 1 ? argv[1] : NULL;
    LDK_WIN32_TEST_ENTRY Tests[] = {
        { "NtdllCurrentDirectoryTest", NtdllCurrentDirectoryTest },
        { "NtdllEnvironmentVariableTest", NtdllEnvironmentVariableTest },
        { "KeyedEventTest", KeyedEventTest },
        { "LdrTest", LdrTest },
        { "FibersTest", FibersTest },
        { "FileTest", FileTest },
        { "HeapApiTest", HeapApiTest },
        { "FormatMessageTest", FormatMessageTest },
        { "NlsTest", NlsTest },
        { "CurrentDirectoryTest", CurrentDirectoryTest },
        { "EnvironmentVariableTest", EnvironmentVariableTest },
        { "LegacyThreadPoolTest", LegacyThreadPoolTest },
        { "ThreadPoolTest", ThreadPoolTest },
        { "ConditionVariableTest", ConditionVariableTest },
        { "WaitOnAddressTest", WaitOnAddressTest },
        { "LibraryTest", LibraryTest },
        { NULL, NULL }
    };

    if (! LdkpConfigureTestDirectories()) {
        return 2;
    }

    for (PLDK_WIN32_TEST_ENTRY Test = Tests; Test->Routine != NULL; Test++) {
        if (Filter != NULL &&
            strstr( Test->Name,
                    Filter ) == NULL) {
            continue;
        }

        if (! SetCurrentDirectoryW( LdkpWin32WorkDirectory )) {
            fprintf(stderr,
                    "[LdkWin32ApiTest] Failed to restore work directory before %s: %lu\n",
                    Test->Name,
                    GetLastError() );
            return 2;
        }

        printf("[LdkWin32ApiTest] RUN  %s\n",
               Test->Name );

        if (! Test->Routine()) {
            printf("[LdkWin32ApiTest] FAIL %s\n",
                   Test->Name );
            return 1;
        }

        printf("[LdkWin32ApiTest] PASS %s\n",
               Test->Name );
    }

    printf("[LdkWin32ApiTest] PASS all tests\n");
    return 0;
}
