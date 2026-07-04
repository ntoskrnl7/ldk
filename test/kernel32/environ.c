#if _KERNEL_MODE
#include <Ldk/windows.h>

BOOLEAN
EnvironmentVariableTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, EnvironmentVariableTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <Windows.h>
#define PAGED_CODE()
#endif

#ifndef ERROR_ENVVAR_NOT_FOUND
#define ERROR_ENVVAR_NOT_FOUND 203L
#endif

static
BOOLEAN
LdkTestExpectEnvironmentA (
    _In_ LPCSTR Name,
    _In_ LPCSTR Expected
    )
{
    CHAR Buffer[256];
    DWORD Length;

    Length = GetEnvironmentVariableA( Name,
                                      Buffer,
                                      sizeof(Buffer) );
    return Length != 0 &&
           Length < sizeof(Buffer) &&
           strcmp( Buffer,
                   Expected ) == 0;
}

static
BOOLEAN
LdkTestExpectEnvironmentW (
    _In_ LPCWSTR Name,
    _In_ LPCWSTR Expected
    )
{
    WCHAR Buffer[256];
    DWORD Length;

    Length = GetEnvironmentVariableW( Name,
                                      Buffer,
                                      RTL_NUMBER_OF(Buffer) );
    return Length != 0 &&
           Length < RTL_NUMBER_OF(Buffer) &&
           wcscmp( Buffer,
                   Expected ) == 0;
}

static
BOOLEAN
LdkTestExpectEnvironmentMissingA (
    _In_ LPCSTR Name
    )
{
    CHAR Buffer[16];
    DWORD Length;

    SetLastError( ERROR_SUCCESS );
    Length = GetEnvironmentVariableA( Name,
                                      Buffer,
                                      sizeof(Buffer) );
    return Length == 0 &&
           GetLastError() == ERROR_ENVVAR_NOT_FOUND;
}

static
BOOLEAN
LdkTestExpectEnvironmentMissingW (
    _In_ LPCWSTR Name
    )
{
    WCHAR Buffer[16];
    DWORD Length;

    SetLastError( ERROR_SUCCESS );
    Length = GetEnvironmentVariableW( Name,
                                      Buffer,
                                      RTL_NUMBER_OF(Buffer) );
    return Length == 0 &&
           GetLastError() == ERROR_ENVVAR_NOT_FOUND;
}

BOOLEAN
EnvironmentVariableTest (
    VOID
    )
{
    BOOLEAN Result;
    PAGED_CODE();

    Result = TRUE;
    printf("EnvironmentVariable Test\n");

    CHAR buffer[128];    
    DWORD length;
    
    printf("Test GetEnvironmentVariableA(windir)\n");
    length = GetEnvironmentVariableA( "windir", buffer, sizeof(buffer) );
    if (length == 0) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] GetEnvironmentVariableA(windir)\n\n");
        Result = FALSE;
    } else {
        printf("%s\n", buffer);
        printf("[Success] GetEnvironmentVariableA(windir)\n\n");
    }

    printf("Test GetEnvironmentVariableW(SystemRoot)\n");
    WCHAR wbuffer[128];    
    length = GetEnvironmentVariableW( L"SystemRoot", wbuffer, sizeof(wbuffer) / sizeof(WCHAR) );
    if (length == 0) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] GetEnvironmentVariableW(SystemRoot)\n\n");
        Result = FALSE;
    } else {
        printf("%ws\n", wbuffer);
        printf("[Success] GetEnvironmentVariableW(SystemRoot)\n\n");
    }

    printf("Test SetEnvironmentVariableA add/update/delete\n");
    if (SetEnvironmentVariableA( "LDK_ENV_TEST_A_MISSING_DELETE",
                                 NULL ) &&
        LdkTestExpectEnvironmentMissingA( "LDK_ENV_TEST_A_MISSING_DELETE" ) &&
        SetEnvironmentVariableA( "LDK_ENV_TEST_A",
                                 "alpha" ) &&
        LdkTestExpectEnvironmentA( "LDK_ENV_TEST_A",
                                   "alpha" ) &&
        SetEnvironmentVariableA( "LDK_ENV_TEST_A",
                                 "beta" ) &&
        LdkTestExpectEnvironmentA( "LDK_ENV_TEST_A",
                                   "beta" ) &&
        SetEnvironmentVariableA( "LDK_ENV_TEST_A",
                                 "alpha-beta-gamma-long-value" ) &&
        LdkTestExpectEnvironmentA( "LDK_ENV_TEST_A",
                                   "alpha-beta-gamma-long-value" ) &&
        SetEnvironmentVariableA( "LDK_ENV_TEST_A",
                                 NULL ) &&
        LdkTestExpectEnvironmentMissingA( "LDK_ENV_TEST_A" )) {
        printf("[Success] SetEnvironmentVariableA add/update/delete\n\n");
    } else {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] SetEnvironmentVariableA add/update/delete\n\n");
        Result = FALSE;
    }

    printf("Test SetEnvironmentVariableW add/update/delete\n");
    if (SetEnvironmentVariableW( L"LDK_ENV_TEST_W_MISSING_DELETE",
                                 NULL ) &&
        LdkTestExpectEnvironmentMissingW( L"LDK_ENV_TEST_W_MISSING_DELETE" ) &&
        SetEnvironmentVariableW( L"LDK_ENV_TEST_W",
                                 L"wide" ) &&
        LdkTestExpectEnvironmentW( L"LDK_ENV_TEST_W",
                                   L"wide" ) &&
        SetEnvironmentVariableW( L"LDK_ENV_TEST_W",
                                 L"narrower" ) &&
        LdkTestExpectEnvironmentW( L"LDK_ENV_TEST_W",
                                   L"narrower" ) &&
        SetEnvironmentVariableW( L"LDK_ENV_TEST_W",
                                 L"wide-alpha-beta-gamma-long-value" ) &&
        LdkTestExpectEnvironmentW( L"LDK_ENV_TEST_W",
                                   L"wide-alpha-beta-gamma-long-value" ) &&
        SetEnvironmentVariableW( L"LDK_ENV_TEST_W",
                                 NULL ) &&
        LdkTestExpectEnvironmentMissingW( L"LDK_ENV_TEST_W" )) {
        printf("[Success] SetEnvironmentVariableW add/update/delete\n\n");
    } else {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] SetEnvironmentVariableW add/update/delete\n\n");
        Result = FALSE;
    }

    printf("Test GetEnvironmentVariableA(LDK_ENV_TEST_A)\n");
    length = GetEnvironmentVariableA( "LDK_ENV_TEST_A", buffer, sizeof(buffer) );
    if (length == 0) {
        DWORD ErrorCode;

        ErrorCode = GetLastError();
        if (ErrorCode == ERROR_ENVVAR_NOT_FOUND) {
            printf("[Success] GetEnvironmentVariableA(LDK_ENV_TEST_A) not found ErrorCode = %d\n\n",
                   ErrorCode);
        } else {
            fprintf(stderr, "[Failed] ErrorCode = %d\n", ErrorCode);
            printf("[Failed] GetEnvironmentVariableA(LDK_ENV_TEST_A)\n\n");
            Result = FALSE;
        }
    } else {
        printf("[Failed] %s\n", buffer);
        printf("[Failed] GetEnvironmentVariableA(LDK_ENV_TEST_A)\n\n");
        Result = FALSE;
    }

    return Result;
}
