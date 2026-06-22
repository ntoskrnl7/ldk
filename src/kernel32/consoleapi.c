#include "winbase.h"
#include "../peb.h"

BOOL
LdkpReadConsole (
    _In_ HANDLE hConsoleInput,
    _Out_writes_bytes_(nNumberOfBytesToRead) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead,
    _Out_ LPDWORD lpNumberOfBytesRead
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdkpReadConsole)
#pragma alloc_text(PAGE, ReadConsoleA)
#pragma alloc_text(PAGE, ReadConsoleW)
#pragma alloc_text(PAGE, WriteConsoleA)
#pragma alloc_text(PAGE, WriteConsoleW)
#endif

UINT LdkpConsoleCp = 0;
UINT LdkpConsoleOutputCP = 0;

extern HANDLE LdkpStdInHandle;
DWORD LdkpStdInConsoleMode = 0;

extern HANDLE LdkpStdOutHandle;
DWORD LdkpStdOutConsoleMode = 0;
ULONG LdkpStdOutComponentId = DPFLTR_IHVDRIVER_ID;
ULONG LdkpStdOutLevel = DPFLTR_ERROR_LEVEL;

extern HANDLE LdkpStdErrHandle;
DWORD LdkpStdErrConsoleMode = 0;
ULONG LdkpStdErrComponentId = DPFLTR_IHVDRIVER_ID;
ULONG LdkpStdErrLevel = DPFLTR_ERROR_LEVEL;



BOOLEAN
LdkGetConsoleHandle (
	_In_ HANDLE Handle,
	_Out_ PHANDLE RealHandle
	)
{
    switch (HandleToUlong(Handle)) {
	case STD_INPUT_HANDLE:
		if (ARGUMENT_PRESENT(RealHandle)) {
			*RealHandle = NtCurrentPeb()->ProcessParameters->StandardInput;
		}
		break;
	case STD_OUTPUT_HANDLE:
		if (ARGUMENT_PRESENT(RealHandle)) {
			*RealHandle = NtCurrentPeb()->ProcessParameters->StandardOutput;
		}
		break;
	case STD_ERROR_HANDLE:
		if (ARGUMENT_PRESENT(RealHandle)) {
			*RealHandle = NtCurrentPeb()->ProcessParameters->StandardError;
		}
		break;
	default:
		return LdkIsConsoleHandle( Handle );
	}
	return TRUE;
}



WINBASEAPI
UINT
WINAPI
GetConsoleCP (
    VOID
    )
{
    return LdkpConsoleCp;
}

WINBASEAPI
UINT
WINAPI
GetConsoleOutputCP (
    VOID
    )
{
    return LdkpConsoleOutputCP;
}



WINBASEAPI
BOOL
WINAPI
GetConsoleMode (
    _In_ HANDLE hConsoleHandle,
    _Out_ LPDWORD lpMode
    )
{
    if (! ARGUMENT_PRESENT(lpMode)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (! LdkGetConsoleHandle( hConsoleHandle,
                               &hConsoleHandle )) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (hConsoleHandle == LdkpStdInHandle) {
        *lpMode = LdkpStdInConsoleMode;
    } else if (hConsoleHandle == LdkpStdOutHandle) {
        *lpMode = LdkpStdOutConsoleMode;
    } else if (hConsoleHandle == LdkpStdErrHandle) {
        *lpMode = LdkpStdErrConsoleMode;
    } else {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
SetConsoleMode (
    _In_ HANDLE hConsoleHandle,
    _In_ DWORD dwMode
    )
{
    if (! LdkGetConsoleHandle( hConsoleHandle,
                               &hConsoleHandle )) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (hConsoleHandle == LdkpStdInHandle) {
        LdkpStdInConsoleMode = dwMode;
    } else if (hConsoleHandle == LdkpStdOutHandle) {
        LdkpStdOutConsoleMode = dwMode;
    } else if (hConsoleHandle == LdkpStdErrHandle) {
        LdkpStdErrConsoleMode = dwMode;
    } else {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }
    return TRUE;
}


BOOL
LdkpReadConsole (
    _In_ HANDLE hConsoleInput,
    _Out_writes_bytes_(nNumberOfBytesToRead) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead,
    _Out_ LPDWORD lpNumberOfBytesRead
    )
{
    PAGED_CODE();

    if (! LdkGetConsoleHandle( hConsoleInput,
                               &hConsoleInput )
        ||
        (hConsoleInput != LdkpStdInHandle)) {

        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    NTSTATUS Status;
    IO_STATUS_BLOCK ioStatus;

    Status = ZwReadFile( hConsoleInput,
                         NULL,
                         NULL,
                         NULL,
                         &ioStatus,
                         lpBuffer,
                         nNumberOfBytesToRead,
                         NULL,
                         NULL );

    if (Status == STATUS_PENDING) {
        Status = ZwWaitForSingleObject( hConsoleInput,
                                        FALSE,
                                        NULL );
        if (NT_SUCCESS(Status)) {
            Status = ioStatus.Status;
        }
    }
    if (NT_SUCCESS(Status)) {
        *lpNumberOfBytesRead = (DWORD)ioStatus.Information;
        return TRUE;
    } else if (Status == STATUS_END_OF_FILE) {
        *lpNumberOfBytesRead = 0;
        return TRUE;
    } else {
        if (NT_WARNING(Status)) {
            *lpNumberOfBytesRead = (DWORD)ioStatus.Information;
        }
    }
    LdkSetLastNTError( Status );
    return FALSE;
}


WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleA (
    _In_ HANDLE hConsoleInput,
    _Out_writes_bytes_to_(nNumberOfCharsToRead * sizeof(CHAR),*lpNumberOfCharsRead * sizeof(CHAR)) LPVOID lpBuffer,
    _In_ DWORD nNumberOfCharsToRead,
    _Out_ _Deref_out_range_(<=,nNumberOfCharsToRead) LPDWORD lpNumberOfCharsRead,
    _In_opt_ PCONSOLE_READCONSOLE_CONTROL pInputControl
    )
{
    UNREFERENCED_PARAMETER( pInputControl );

    PAGED_CODE();

    DWORD NumberOfBytesRead = 0;
    BOOL Result = LdkpReadConsole( hConsoleInput,
                                   lpBuffer,
                                   nNumberOfCharsToRead * sizeof(CHAR),
                                   &NumberOfBytesRead );
    *lpNumberOfCharsRead = NumberOfBytesRead / sizeof(CHAR);
    return Result;
}

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
ReadConsoleW (
    _In_ HANDLE hConsoleInput,
    _Out_writes_bytes_to_(nNumberOfCharsToRead * sizeof(WCHAR),*lpNumberOfCharsRead * sizeof(WCHAR)) LPVOID lpBuffer,
    _In_ DWORD nNumberOfCharsToRead,
    _Out_ _Deref_out_range_(<=,nNumberOfCharsToRead) LPDWORD lpNumberOfCharsRead,
    _In_opt_ PCONSOLE_READCONSOLE_CONTROL pInputControl
    )
{
    UNREFERENCED_PARAMETER( pInputControl );

    PAGED_CODE();

    if (nNumberOfCharsToRead > ((DWORD)-1) / sizeof(WCHAR)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    DWORD NumberOfBytesRead = 0;
    BOOL Result = LdkpReadConsole( hConsoleInput,
                                   lpBuffer,
                                   nNumberOfCharsToRead * sizeof(WCHAR),
                                   &NumberOfBytesRead );
    *lpNumberOfCharsRead = NumberOfBytesRead / sizeof(WCHAR);
    return Result;
}

WINBASEAPI
BOOL
WINAPI
WriteConsoleA (
    _In_ HANDLE hConsoleOutput,
    _In_reads_(nNumberOfCharsToWrite) CONST VOID* lpBuffer,
    _In_ DWORD nNumberOfCharsToWrite,
    _Out_opt_ LPDWORD lpNumberOfCharsWritten,
    _Reserved_ LPVOID lpReserved
    )
{
    UNREFERENCED_PARAMETER( lpReserved );

    if (! LdkGetConsoleHandle( hConsoleOutput,
                               &hConsoleOutput )) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ULONG componentId = DPFLTR_IHVDRIVER_ID;
    ULONG level = DPFLTR_ERROR_LEVEL;

    if (hConsoleOutput == LdkpStdOutHandle) {
        componentId = LdkpStdOutComponentId;
        level = LdkpStdOutLevel;
    } else if (hConsoleOutput == LdkpStdErrHandle) {
        componentId = LdkpStdErrComponentId;
        level = LdkpStdErrLevel;
    } else {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (ARGUMENT_PRESENT(lpNumberOfCharsWritten)) {
        *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
    }

    return NT_SUCCESS(DbgPrintEx( componentId,
                                  level,
                                  "%.*s",
                                  nNumberOfCharsToWrite,
                                  lpBuffer ));
}

WINBASEAPI
BOOL
WINAPI
WriteConsoleW (
    _In_ HANDLE hConsoleOutput,
    _In_reads_(nNumberOfCharsToWrite) CONST VOID* lpBuffer,
    _In_ DWORD nNumberOfCharsToWrite,
    _Out_opt_ LPDWORD lpNumberOfCharsWritten,
    _Reserved_ LPVOID lpReserved
    )
{
    UNREFERENCED_PARAMETER( lpReserved );

    if (! LdkGetConsoleHandle( hConsoleOutput,
                               &hConsoleOutput )) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    ULONG componentId = DPFLTR_IHVDRIVER_ID;
    ULONG level = DPFLTR_ERROR_LEVEL;

    if (hConsoleOutput == LdkpStdOutHandle) {
        componentId = LdkpStdOutComponentId;
        level = LdkpStdOutLevel;
    } else if (hConsoleOutput == LdkpStdErrHandle) {
        componentId = LdkpStdErrComponentId;
        level = LdkpStdErrLevel;
    } else {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    if (ARGUMENT_PRESENT(lpNumberOfCharsWritten)) {
        *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
    }
    return NT_SUCCESS(DbgPrintEx( componentId,
                                  level,
                                  "%.*ws",
                                  nNumberOfCharsToWrite,
                                  lpBuffer ));
}

WINBASEAPI
BOOL
WINAPI
SetConsoleCtrlHandler (
    _In_opt_ PHANDLER_ROUTINE HandlerRoutine,
    _In_ BOOL Add
    )
{
    UNREFERENCED_PARAMETER(HandlerRoutine);
    UNREFERENCED_PARAMETER(Add);
    return FALSE;
}
