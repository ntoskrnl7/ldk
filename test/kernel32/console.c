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
    BOOLEAN rv = TRUE;
    DWORD mode;
    DWORD written;

    PAGED_CODE();

    HANDLE hStdIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

    printf("Test SetConsoleMode/GetConsoleMode(hStdIn)\n");
    if (! SetConsoleMode( hStdIn,
                          ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT ) ||
        ! GetConsoleMode( hStdIn,
                          &mode ) ||
        mode != (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT)) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetConsoleMode/GetConsoleMode(hStdIn)\n\n");
        rv = FALSE;
    } else {
        printf("[Success] Test SetConsoleMode/GetConsoleMode(hStdIn)\n\n");
    }

    printf("Test SetConsoleMode/GetConsoleMode(hStdOut)\n");
    if (! SetConsoleMode( hStdOut,
                          ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT ) ||
        ! GetConsoleMode( hStdOut,
                          &mode ) ||
        mode != (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT)) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetConsoleMode/GetConsoleMode(hStdOut)\n\n");
        rv = FALSE;
    } else {
        printf("[Success] Test SetConsoleMode/GetConsoleMode(hStdOut)\n\n");
    }

    printf("Test WriteConsoleA(hStdOut, test, 4)\n");
    if (WriteConsoleA( hStdOut, "test", 4, &written, NULL ) &&
        written == 4) {
        printf("[Success] Test WriteConsoleA(hStdOut, test, 4)\n\n");
    } else {
        rv = FALSE;
        printf("[Failed] Test WriteConsoleA(hStdOut, test, 4)\n\n");
    }
    printf("Test WriteConsoleW(hStdOut, test, 4)\n");
    if (WriteConsoleW( hStdOut, L"test", 4, &written, NULL ) &&
        written == 4) {
        printf("[Success] Test WriteConsoleW(hStdOut, test, 4)\n\n");
    } else {
        rv = FALSE;
        printf("[Failed] Test WriteConsoleW(hStdOut, test, 4)\n\n");
    }
    printf("Test WriteConsoleA(hStdOut, test, 1)\n");
    if (WriteConsoleA( hStdOut, "test", 1, &written, NULL ) &&
        written == 1) {
        printf("[Success] Test WriteConsoleA(hStdOut, test, 1)\n\n");
    } else {
        rv = FALSE;
        printf("[Failed] Test WriteConsoleA(hStdOut, test, 1)\n\n");
    }
    printf("Test WriteConsoleW(hStdOut, test, 1)\n");
    if (WriteConsoleW( hStdOut, L"test", 1, &written, NULL ) &&
        written == 1) {
        printf("[Success] Test WriteConsoleW(hStdOut, test, 1)\n\n");
    } else {
        rv = FALSE;
        printf("[Failed] Test WriteConsoleW(hStdOut, test, 1)\n\n");
    }

    HANDLE hStdErr = GetStdHandle(STD_ERROR_HANDLE);

    printf("Test SetConsoleMode/GetConsoleMode(hStdErr)\n");
    if (! SetConsoleMode( hStdErr,
                          ENABLE_PROCESSED_OUTPUT ) ||
        ! GetConsoleMode( hStdErr,
                          &mode ) ||
        mode != ENABLE_PROCESSED_OUTPUT) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetConsoleMode/GetConsoleMode(hStdErr)\n\n");
        rv = FALSE;
    } else {
        printf("[Success] Test SetConsoleMode/GetConsoleMode(hStdErr)\n\n");
    }

    printf("Test WriteConsoleA(hStdErr, test, 4)\n");
    if (WriteConsoleA( hStdErr, "test", 4, &written, NULL ) &&
        written == 4) {
        printf("[Success] Test WriteConsoleA(hStdErr, test, 4)\n\n");
    } else {
        rv = FALSE;
        printf("[Failed] Test WriteConsoleA(hStdErr, test, 4)\n\n");
    }
    printf("Test WriteConsoleW(hStdErr, test, 4)\n");
    if (WriteConsoleW( hStdErr, L"test", 4, &written, NULL ) &&
        written == 4) {
        printf("[Success] Test WriteConsoleW(hStdErr, test, 4)\n\n");
    } else {
        rv = FALSE;
        printf("[Failed] Test WriteConsoleW(hStdErr, test, 4)\n\n");
    }
    printf("Test WriteConsoleA(hStdErr, test, 1)\n");
    if (WriteConsoleA( hStdErr, "test", 1, &written, NULL ) &&
        written == 1) {
        printf("[Success] Test WriteConsoleA(hStdErr, test, 1)\n\n");
    } else {
        rv = FALSE;
        printf("[Failed] Test WriteConsoleA(hStdErr, test, 1)\n\n");
    }
    printf("Test WriteConsoleW(hStdErr, test, 1)\n");
    if (WriteConsoleW( hStdErr, L"test", 1, &written, NULL ) &&
        written == 1) {
        printf("[Success] Test WriteConsoleW(hStdErr, test, 1)\n\n");
    } else {
        rv = FALSE;
        printf("[Failed] Test WriteConsoleW(hStdErr, test, 1)\n\n");
    }
    return rv;
}
