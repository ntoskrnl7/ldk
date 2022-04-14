#include "winbase.h"
#include "../ldk.h"
#include "../nt/zwapi.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, PulseEvent)
#endif

ULONG
BaseSetLastNTError (
	_In_ NTSTATUS Status
	)
{
	ULONG dwErrorCode = RtlNtStatusToDosError( Status );
	SetLastError( dwErrorCode );
	return dwErrorCode;
}

PLARGE_INTEGER
BaseFormatTimeout(
	_Out_ PLARGE_INTEGER Timeout,
	_In_ DWORD Milliseconds
	)
{
	if (Timeout == NULL) {
		return NULL;
	}

	if (Milliseconds == INFINITE) {
		return NULL;
	}

	Timeout->QuadPart = UInt32x32To64(Milliseconds, 10000);
	Timeout->QuadPart *= -1;
	return Timeout;
}



WINBASEAPI
BOOL
WINAPI
PulseEvent(
    _In_ HANDLE hEvent
    )
{
	PAGED_CODE();

    NTSTATUS Status = ZwPulseEvent( hEvent, NULL );
    if ( NT_SUCCESS(Status) ) {
        return TRUE;
    }
    BaseSetLastNTError(Status);
    return FALSE;
}



WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryA(
    _In_ LPCSTR lpLibFileName
    )
{
	HMODULE hModule;
	ANSI_STRING FileName;
	UNICODE_STRING FileNameW;

	RtlInitString(&FileName, lpLibFileName);
	NTSTATUS status = LdkAnsiStringToUnicodeString(&FileNameW, &FileName, TRUE);
	if (!NT_SUCCESS(status)) {
		return NULL;
	}
	hModule = LoadLibraryExW(FileNameW.Buffer, NULL, 0);

	LdkFreeUnicodeString(&FileNameW);

	return hModule;
}

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryW(
    _In_ LPCWSTR lpLibFileName
    )
{
	return LoadLibraryExW(lpLibFileName, NULL, 0);
}

WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageA(
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) != 0, _At_((LPSTR*)lpBuffer, _Outptr_result_z_))
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) == 0, _Out_writes_z_(nSize))
             LPSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    )
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(lpSource);
	UNREFERENCED_PARAMETER(dwMessageId);
	UNREFERENCED_PARAMETER(dwLanguageId);
	UNREFERENCED_PARAMETER(lpBuffer);
	UNREFERENCED_PARAMETER(nSize);
	UNREFERENCED_PARAMETER(Arguments);

	// TODO

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
	return 0;
}

WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageW(
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) != 0, _At_((LPWSTR*)lpBuffer, _Outptr_result_z_))
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) == 0, _Out_writes_z_(nSize))
             LPWSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    )
{
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(lpSource);
	UNREFERENCED_PARAMETER(dwMessageId);
	UNREFERENCED_PARAMETER(dwLanguageId);
	UNREFERENCED_PARAMETER(lpBuffer);
	UNREFERENCED_PARAMETER(nSize);
	UNREFERENCED_PARAMETER(Arguments);

	// TODO

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
	return 0;
}



WINBASEAPI
_Success_(return==0)
_Ret_maybenull_
HLOCAL
WINAPI
LocalFree(
    _Frees_ptr_opt_ HLOCAL hMem
    )
{
	UNREFERENCED_PARAMETER(hMem);

	// TODO

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
	return hMem;
}
