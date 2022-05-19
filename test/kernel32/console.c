#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
ConsoleTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ConsoleTest)
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

BOOLEAN
ConsoleTest (
    VOID
    )
{
    PAGED_CODE();

    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    printf("Test WriteConsoleA(hStdOut, test, 4)\n");
    if (WriteConsoleA( hStdOut, "test", 4, NULL, NULL )) {
        printf("[Success] Test WriteConsoleA(hStdOut, test, 4)\n\n");
    } else {
        printf("[Failed] Test WriteConsoleA(hStdOut, test, 4)\n\n");
    }
    printf("Test WriteConsoleW(hStdOut, test, 4)\n");
    if (WriteConsoleW( hStdOut, L"test", 4, NULL, NULL )) {
        printf("[Success] Test WriteConsoleW(hStdOut, test, 4)\n\n");
    } else {
        printf("[Failed] Test WriteConsoleW(hStdOut, test, 4)\n\n");
    }
    printf("Test WriteConsoleA(hStdOut, test, 1)\n");
    if (WriteConsoleA( hStdOut, "test", 1, NULL, NULL )) {
        printf("[Success] Test WriteConsoleA(hStdOut, test, 1)\n\n");
    } else {
        printf("[Failed] Test WriteConsoleA(hStdOut, test, 1)\n\n");
    }
    printf("Test WriteConsoleW(hStdOut, test, 1)\n");
    if (WriteConsoleW( hStdOut, L"test", 1, NULL, NULL )) {
        printf("[Success] Test WriteConsoleW(hStdOut, test, 1)\n\n");
    } else {
        printf("[Failed] Test WriteConsoleW(hStdOut, test, 1)\n\n");
    }

    HANDLE hStdErr = GetStdHandle(STD_OUTPUT_HANDLE);
    printf("Test WriteConsoleA(hStdErr, test, 4)\n");
    if (WriteConsoleA( hStdErr, "test", 4, NULL, NULL )) {
        printf("[Success] Test WriteConsoleA(hStdErr, test, 4)\n\n");
    } else {
        printf("[Failed] Test WriteConsoleA(hStdErr, test, 4)\n\n");
    }
    printf("Test WriteConsoleW(hStdErr, test, 4)\n");
    if (WriteConsoleW( hStdErr, L"test", 4, NULL, NULL )) {
        printf("[Success] Test WriteConsoleW(hStdErr, test, 4)\n\n");
    } else {
        printf("[Failed] Test WriteConsoleW(hStdErr, test, 4)\n\n");
    }
    printf("Test WriteConsoleA(hStdErr, test, 1)\n");
    if (WriteConsoleA( hStdErr, "test", 1, NULL, NULL )) {
        printf("[Success] Test WriteConsoleA(hStdErr, test, 1)\n\n");
    } else {
        printf("[Failed] Test WriteConsoleA(hStdErr, test, 1)\n\n");
    }
    printf("Test WriteConsoleW(hStdErr, test, 1)\n");
    if (WriteConsoleW( hStdErr, L"test", 1, NULL, NULL )) {
        printf("[Success] Test WriteConsoleW(hStdErr, test, 1)\n\n");
    } else {
        printf("[Failed] Test WriteConsoleW(hStdErr, test, 1)\n\n");
    }
    return TRUE;
}