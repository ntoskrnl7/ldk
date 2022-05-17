#if _KERNEL_MODE
#include <Ldk/windows.h>

BOOLEAN
CurrentDirectoryTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CurrentDirectoryTest)
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
CurrentDirectoryTest (
    VOID
    )
{
    PAGED_CODE();

    printf("CurrentDirectory Test\n");

    CHAR buffer[128];    
    DWORD length;
    
    printf("Test GetCurrentDirectoryA\n");
    length = GetCurrentDirectoryA( sizeof(buffer), buffer );
    if (length == 0) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] GetCurrentDirectoryA\n\n");
    } else {
        printf("%s\n", buffer);
        printf("[Success] GetCurrentDirectoryA\n\n");
    }

    printf("Test SetCurrentDirectoryA(C:\\)\n");
    if (SetCurrentDirectoryA( "C:\\" )) {
        printf("[Success] SetCurrentDirectoryA(C:\\)\n\n");
    } else {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] SetCurrentDirectoryA(C:\\)\n\n");
    }

    printf("Test GetCurrentDirectoryW\n");
    WCHAR wbuffer[128];    
    length = GetCurrentDirectoryW( sizeof(wbuffer) / sizeof(WCHAR), wbuffer );
    if (length == 0) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] GetCurrentDirectoryW\n\n");
    } else {
        if (wcscmp(L"C:\\", wbuffer)) {
            printf("[Failed] Expect = C:\\, Actual = %ws\n", wbuffer);
            printf("[Failed] GetCurrentDirectoryW\n\n");
        } else {
            printf("%s\n", buffer);
            printf("[Success] GetCurrentDirectoryW\n\n");
        }
    }

    printf("Test SetCurrentDirectoryW(C:\\Windows\\System32)\n");
    if (SetCurrentDirectoryW( L"C:\\Windows\\System32" )) {
        printf("[Success] SetCurrentDirectoryA(C:\\Windows\\System32)\n\n");
    } else {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] SetCurrentDirectoryA(C:\\Windows\\System32)\n\n");
    }

    printf("Test GetCurrentDirectoryA\n");
    length = GetCurrentDirectoryA( sizeof(buffer), buffer );
    if (length == 0) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] GetCurrentDirectoryA\n\n");
    } else {
        if (strcmp("C:\\Windows\\System32", buffer)) {
            printf("[Failed] Expect = C:\\Windows\\System32, Actual = %s\n", buffer);
            printf("[Failed] GetCurrentDirectoryA\n\n");
        } else {
            printf("%s\n", buffer);
            printf("[Success] GetCurrentDirectoryA\n\n");
        }
    }
    return TRUE;
}