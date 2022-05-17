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

BOOLEAN
EnvironmentVariableTest (
    VOID
    )
{
    PAGED_CODE();

    printf("EnvironmentVariable Test\n");

    CHAR buffer[128];    
    DWORD length;
    
    printf("Test GetEnvironmentVariableA(windir)\n");
    length = GetEnvironmentVariableA( "windir", buffer, sizeof(buffer) );
    if (length == 0) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] GetEnvironmentVariableA(windir)\n\n");
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
    } else {
        printf("%ws\n", wbuffer);
        printf("[Success] GetEnvironmentVariableW(SystemRoot)\n\n");
    }

    printf("Test SetEnvironmentVariableA(TestVar, Test)\n");
    if (SetEnvironmentVariableA( "TestVar", "Test" )) {
        printf("[Success] SetEnvironmentVariableA(TestVar, Test)\n\n");
    } else {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] SetEnvironmentVariableA(TestVar, Test)\n\n");
    }

    printf("Test GetEnvironmentVariableA(TestVar)\n");
    length = GetEnvironmentVariableA( "TestVar", buffer, sizeof(buffer) );
    if (length == 0) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] GetEnvironmentVariableA(TestVar)\n\n");
    } else {
        printf("%s\n", buffer);
        printf("[Success] GetEnvironmentVariableA(TestVar)\n\n");
    }

   printf("Test SetEnvironmentVariableW(TestVar, NULL)\n");
    if (SetEnvironmentVariableW( L"TestVar", NULL )) {
        printf("[Success] SetEnvironmentVariableW(TestVar, NULL)\n\n");
    } else {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] SetEnvironmentVariableW(TestVar, NULL)\n\n");
    }

    printf("Test GetEnvironmentVariableA(TestVar)\n");
    length = GetEnvironmentVariableA( "TestVar", buffer, sizeof(buffer) );
    if (length == 0) {
        fprintf(stderr, "[Success] ErrorCode = %d\n", GetLastError());
        printf("[Success] GetEnvironmentVariableA(TestVar)\n\n");
    } else {
        printf("[Failed] %s\n", buffer);
        printf("[Failed] GetEnvironmentVariableA(TestVar)\n\n");
    }

    return TRUE;
}