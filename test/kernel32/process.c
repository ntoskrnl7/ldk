#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
ProcessApiTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ProcessApiTest)
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
ProcessApiTest (
    VOID
    )
{
    BOOLEAN Result = TRUE;
    DWORD CurrentProcessId;
    DWORD ExitCode;
    HANDLE ProcessHandle;
    HANDLE DuplicatedHandle;
    HANDLE WaitHandles[2];
    DWORD WaitResult;

    PAGED_CODE();

    printf("Process API Test\n");

    CurrentProcessId = GetCurrentProcessId();
    if (CurrentProcessId == 0) {
        fprintf(stderr,
                "[Failed] GetCurrentProcessId returned zero\n" );
        return FALSE;
    }

#if _KERNEL_MODE
    if (CurrentProcessId == HandleToUlong( PsGetCurrentProcessId() )) {
        fprintf(stderr,
                "[Failed] LDK process id matched real EPROCESS id: %lu\n",
                CurrentProcessId );
        return FALSE;
    }
#endif

    if (! GetExitCodeProcess( GetCurrentProcess(),
                              &ExitCode ) ||
        ExitCode != STILL_ACTIVE) {
        fprintf(stderr,
                "[Failed] GetExitCodeProcess(current) Result = %lu ExitCode = 0x%08lx ErrorCode = %lu\n",
                ExitCode == STILL_ACTIVE,
                ExitCode,
                GetLastError() );
        return FALSE;
    }

    WaitResult = WaitForSingleObject( GetCurrentProcess(),
                                      1 );
    if (WaitResult != WAIT_TIMEOUT) {
        fprintf(stderr,
                "[Failed] WaitForSingleObject(current) = 0x%08lx ErrorCode = %lu\n",
                WaitResult,
                GetLastError() );
        return FALSE;
    }

    SetLastError( ERROR_SUCCESS );
    ProcessHandle = OpenProcess( PROCESS_QUERY_INFORMATION | SYNCHRONIZE | PROCESS_DUP_HANDLE,
                                 FALSE,
                                 CurrentProcessId );
    if (ProcessHandle == NULL) {
        fprintf(stderr,
                "[Failed] OpenProcess(current pid) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    DuplicatedHandle = NULL;
    if (! DuplicateHandle( GetCurrentProcess(),
                           ProcessHandle,
                           GetCurrentProcess(),
                           &DuplicatedHandle,
                           0,
                           FALSE,
                           DUPLICATE_SAME_ACCESS )) {
        fprintf(stderr,
                "[Failed] DuplicateHandle(process) ErrorCode = %lu\n",
                GetLastError() );
        Result = FALSE;
        goto CloseProcessHandle;
    }

    if (! GetExitCodeProcess( DuplicatedHandle,
                              &ExitCode ) ||
        ExitCode != STILL_ACTIVE) {
        fprintf(stderr,
                "[Failed] GetExitCodeProcess(duplicated) ExitCode = 0x%08lx ErrorCode = %lu\n",
                ExitCode,
                GetLastError() );
        Result = FALSE;
        goto CloseDuplicatedHandle;
    }

    WaitHandles[0] = ProcessHandle;
    WaitHandles[1] = DuplicatedHandle;
    WaitResult = WaitForMultipleObjects( RTL_NUMBER_OF(WaitHandles),
                                         WaitHandles,
                                         FALSE,
                                         1 );
    if (WaitResult != WAIT_TIMEOUT) {
        fprintf(stderr,
                "[Failed] WaitForMultipleObjects(process handles) = 0x%08lx ErrorCode = %lu\n",
                WaitResult,
                GetLastError() );
        Result = FALSE;
        goto CloseDuplicatedHandle;
    }

CloseDuplicatedHandle:
    if (DuplicatedHandle != NULL) {
        CloseHandle( DuplicatedHandle );
    }

CloseProcessHandle:
    CloseHandle( ProcessHandle );

    SetLastError( ERROR_SUCCESS );
    ProcessHandle = OpenProcess( PROCESS_QUERY_INFORMATION,
                                 FALSE,
                                 0 );
    if (ProcessHandle != NULL ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
        fprintf(stderr,
                "[Failed] OpenProcess(0) Handle = %p ErrorCode = %lu\n",
                ProcessHandle,
                GetLastError() );
        if (ProcessHandle != NULL) {
            CloseHandle( ProcessHandle );
        }
        return FALSE;
    }

#if _KERNEL_MODE
    if (Result) {
        const DWORD TestExitCode = 0x12345678;

        ProcessHandle = OpenProcess( PROCESS_TERMINATE | PROCESS_QUERY_INFORMATION | SYNCHRONIZE,
                                     FALSE,
                                     CurrentProcessId );
        if (ProcessHandle == NULL) {
            fprintf(stderr,
                    "[Failed] OpenProcess(current pid, terminate) ErrorCode = %lu\n",
                    GetLastError() );
            return FALSE;
        }

        if (! TerminateProcess( ProcessHandle,
                                TestExitCode )) {
            fprintf(stderr,
                    "[Failed] TerminateProcess(ldk process) ErrorCode = %lu\n",
                    GetLastError() );
            CloseHandle( ProcessHandle );
            return FALSE;
        }

        WaitResult = WaitForSingleObject( ProcessHandle,
                                          0 );
        if (WaitResult != WAIT_OBJECT_0) {
            fprintf(stderr,
                    "[Failed] WaitForSingleObject(terminated ldk process) = 0x%08lx ErrorCode = %lu\n",
                    WaitResult,
                    GetLastError() );
            CloseHandle( ProcessHandle );
            return FALSE;
        }

        if (! GetExitCodeProcess( ProcessHandle,
                                  &ExitCode ) ||
            ExitCode != TestExitCode) {
            fprintf(stderr,
                    "[Failed] GetExitCodeProcess(terminated ldk process) ExitCode = 0x%08lx ErrorCode = %lu\n",
                    ExitCode,
                    GetLastError() );
            CloseHandle( ProcessHandle );
            return FALSE;
        }

        CloseHandle( ProcessHandle );
    }
#endif

    if (Result) {
        printf("[Success] Process API Test\n\n");
    } else {
        printf("[Failed] Process API Test\n\n");
    }

    return Result;
}
