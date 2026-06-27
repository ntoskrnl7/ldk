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
#include <string.h>
#define PAGED_CODE()
#endif

BOOLEAN
PipeTest (
    VOID
    )
{
    BOOLEAN Result = TRUE;
    HANDLE ReadPipe = NULL;
    HANDLE WritePipe = NULL;
    DWORD BytesWritten = 0;
    DWORD BytesRead = 0;
    DWORD BytesAvailable = 0;
    CHAR Buffer[8];
    static const CHAR Message[] = "pipe";

    PAGED_CODE();

    printf("Pipe Test\n");

    printf("Test CreatePipe / PeekNamedPipe / ReadFile / WriteFile\n");
    if (! CreatePipe( &ReadPipe,
                      &WritePipe,
                      NULL,
                      512 )) {
        fprintf(stderr,
                "[Failed] CreatePipe ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (! WriteFile( WritePipe,
                     Message,
                     sizeof(Message) - 1,
                     &BytesWritten,
                     NULL ) ||
        BytesWritten != sizeof(Message) - 1) {
        fprintf(stderr,
                "[Failed] WriteFile(pipe) ErrorCode = %lu BytesWritten = %lu\n",
                GetLastError(),
                BytesWritten );
        Result = FALSE;
        goto Exit;
    }

    if (! PeekNamedPipe( ReadPipe,
                         NULL,
                         0,
                         NULL,
                         &BytesAvailable,
                         NULL ) ||
        BytesAvailable < sizeof(Message) - 1) {
        fprintf(stderr,
                "[Failed] PeekNamedPipe ErrorCode = %lu BytesAvailable = %lu\n",
                GetLastError(),
                BytesAvailable );
        Result = FALSE;
        goto Exit;
    }

    RtlZeroMemory( Buffer,
                   sizeof(Buffer) );
    if (! ReadFile( ReadPipe,
                    Buffer,
                    sizeof(Message) - 1,
                    &BytesRead,
                    NULL ) ||
        BytesRead != sizeof(Message) - 1 ||
        memcmp( Buffer,
                Message,
                sizeof(Message) - 1 ) != 0) {
        fprintf(stderr,
                "[Failed] ReadFile(pipe) ErrorCode = %lu BytesRead = %lu Buffer = %.*s\n",
                GetLastError(),
                BytesRead,
                (int)BytesRead,
                Buffer );
        Result = FALSE;
        goto Exit;
    }
    printf("[Success] Test CreatePipe / PeekNamedPipe / ReadFile / WriteFile\n\n");

Exit:
    if (ReadPipe != NULL) {
        CloseHandle( ReadPipe );
        ReadPipe = NULL;
    }
    if (WritePipe != NULL) {
        CloseHandle( WritePipe );
        WritePipe = NULL;
    }

    printf("Test repeated CreatePipe name generation\n");
    for (ULONG Index = 0; Index < 64; Index++) {
        if (! CreatePipe( &ReadPipe,
                          &WritePipe,
                          NULL,
                          0 )) {
            fprintf(stderr,
                    "[Failed] Repeated CreatePipe Index = %lu ErrorCode = %lu\n",
                    Index,
                    GetLastError() );
            Result = FALSE;
            break;
        }
        CloseHandle( ReadPipe );
        CloseHandle( WritePipe );
        ReadPipe = NULL;
        WritePipe = NULL;
    }

    if (ReadPipe != NULL) {
        CloseHandle( ReadPipe );
    }
    if (WritePipe != NULL) {
        CloseHandle( WritePipe );
    }

    if (Result) {
        printf("[Success] Test repeated CreatePipe name generation\n\n");
    } else {
        printf("[Failed] Pipe Test\n\n");
    }

    return Result;
}
