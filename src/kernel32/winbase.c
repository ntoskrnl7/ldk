#include "winbase.h"
#include "../ldk.h"

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