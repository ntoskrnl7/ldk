#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
PipeTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PipeTest)
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
PipeTest (
    VOID
    )
{
    HANDLE hReadPipe = NULL;
    HANDLE hWritePipe = NULL;
    DWORD length;
    CHAR WriteBuffer[] = "pipe";
    CHAR ReadBuffer[4];
    BOOL rv = TRUE;

    PAGED_CODE();

    printf("Pipe Test\n");

    printf("Test CreatePipe\n");
    if (!CreatePipe( &hReadPipe, &hWritePipe, NULL, 0 ) ||
        hReadPipe == NULL || hReadPipe == INVALID_HANDLE_VALUE ||
        hWritePipe == NULL || hWritePipe == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreatePipe\n\n");
        return FALSE;
    }
    printf("[Success] Test CreatePipe\n\n");

    printf("Test WriteFile(hWritePipe, pipe, 4)\n");
    rv = WriteFile( hWritePipe, WriteBuffer, 4, &length, NULL );
    if (!rv || length != 4) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test WriteFile(hWritePipe, pipe, 4)\n\n");
        CloseHandle( hReadPipe );
        CloseHandle( hWritePipe );
        return FALSE;
    }
    printf("[Success] Test WriteFile(hWritePipe, pipe, 4)\n\n");

    printf("Test ReadFile(hReadPipe)\n");
    rv = ReadFile( hReadPipe, ReadBuffer, 4, &length, NULL );
    if (!rv || length != 4 || memcmp(ReadBuffer, WriteBuffer, 4)) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test ReadFile(hReadPipe)\n\n");
        CloseHandle( hReadPipe );
        CloseHandle( hWritePipe );
        return FALSE;
    }
    printf("[Success] Test ReadFile(hReadPipe)\n\n");

    CloseHandle( hReadPipe );
    CloseHandle( hWritePipe );
    return TRUE;
}
