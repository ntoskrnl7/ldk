#include "winbase.h"
#include "../ldk.h"

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpAnsiStringToUnicodeSize (
    _In_ PANSI_STRING AnsiString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpUnicodeStringToAnsiSize (
    _In_ PUNICODE_STRING UnicodeString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpOemStringToUnicodeSize(
    _In_ PANSI_STRING OemString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpUnicodeStringToOemSize (
	_In_ PUNICODE_STRING UnicodeString
	);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ldk8BitStringToStaticUnicodeString)
#pragma alloc_text(PAGE, Ldk8BitStringToDynamicUnicodeString)
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpAnsiStringToUnicodeSize (
    _In_ PANSI_STRING AnsiString
    )
{
	PAGED_CODE();
    return RtlAnsiStringToUnicodeSize( AnsiString );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpUnicodeStringToAnsiSize (
    _In_ PUNICODE_STRING UnicodeString
    )
{
	PAGED_CODE();
    return RtlUnicodeStringToAnsiSize( UnicodeString );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpOemStringToUnicodeSize(
    _In_ PANSI_STRING OemString
    )
{
	PAGED_CODE();
    return RtlOemStringToUnicodeSize( OemString );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpUnicodeStringToOemSize (
	_In_ PUNICODE_STRING UnicodeString
	)
{
	PAGED_CODE();
    return RtlUnicodeStringToOemSize( UnicodeString );
}



ULONG
LdkSetLastNTError (
	_In_ NTSTATUS Status
	)
{
	ULONG dwErrorCode = RtlNtStatusToDosError( Status );
	SetLastError( dwErrorCode );
	return dwErrorCode;
}

PLARGE_INTEGER
LdkFormatTimeout (
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



_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PUNICODE_STRING
LdkAnsiStringToStaticUnicodeString (
    _In_ LPCSTR SourceString
    )
{
    ANSI_STRING Ansi;

	PAGED_CODE();

    RtlInitAnsiString( &Ansi,
                       SourceString );
	PUNICODE_STRING Unicode = &NtCurrentTeb()->StaticUnicodeString;
    NTSTATUS Status = LdkAnsiStringToUnicodeString( Unicode,
                                                    &Ansi,
                                                    FALSE );
    if (NT_SUCCESS(Status)) {
		return Unicode;
	}
	if (Status == STATUS_BUFFER_OVERFLOW) {
		SetLastError( ERROR_FILENAME_EXCED_RANGE );
	} else {
		LdkSetLastNTError( Status );
	}
	return NULL;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PUNICODE_STRING
Ldk8BitStringToStaticUnicodeString (
    _In_ PCSTR String
    )
{
    ANSI_STRING Ansi;

	PAGED_CODE();

    RtlInitAnsiString( &Ansi,
                       String );
    PUNICODE_STRING Unicode = &NtCurrentTeb()->StaticUnicodeString;
	NTSTATUS Status = Ldk8BitStringToUnicodeString( Unicode,
													&Ansi,
													FALSE );
    if (NT_SUCCESS(Status)) {
		return Unicode;
	}
	if (Status == STATUS_BUFFER_OVERFLOW) {
		SetLastError( ERROR_FILENAME_EXCED_RANGE );
	} else {
		LdkSetLastNTError( Status );
	}
	return NULL;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOL
NTAPI
Ldk8BitStringToDynamicUnicodeString (
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_) PUNICODE_STRING DestinationString,
    _In_ PCSTR SourceString
    )
{
	STRING String;

	PAGED_CODE();

	RtlInitString( &String,
				   SourceString );
	NTSTATUS Status = Ldk8BitStringToUnicodeString( DestinationString,
													&String,
													TRUE );
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}
	if (Status == STATUS_BUFFER_OVERFLOW) {
		SetLastError( ERROR_FILENAME_EXCED_RANGE );
	} else {
		LdkSetLastNTError( Status );
	}
	return FALSE;
}



WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageA (
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

	LdkSetLastNTError( STATUS_NOT_IMPLEMENTED );
	return 0;
}

WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageW (
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

	LdkSetLastNTError( STATUS_NOT_IMPLEMENTED );
	return 0;
}



WINBASEAPI
_Success_(return != NULL)
_Post_writable_byte_size_(uBytes)
DECLSPEC_ALLOCATOR
HLOCAL
WINAPI
LocalAlloc (
    _In_ UINT uFlags,
    _In_ SIZE_T uBytes
    )
{
	DWORD dwFlags;
	if (FlagOn(uFlags, LMEM_ZEROINIT)) {
		SetFlag(dwFlags, HEAP_ZERO_MEMORY);
	}
	return (HLOCAL)HeapAlloc( GetProcessHeap(),
							  uFlags,
							  (DWORD)uBytes );
}

WINBASEAPI
_Ret_reallocated_bytes_(hMem, uBytes)
DECLSPEC_ALLOCATOR
HLOCAL
WINAPI
LocalReAlloc (
    _Frees_ptr_opt_ HLOCAL hMem,
    _In_ SIZE_T uBytes,
    _In_ UINT uFlags
    )
{
	DWORD dwFlags;
	if (FlagOn(uFlags, LMEM_ZEROINIT)) {
		SetFlag(dwFlags, HEAP_ZERO_MEMORY);
	}
	return (HLOCAL)HeapReAlloc( GetProcessHeap(),
								dwFlags,
								(PVOID)hMem,
								(DWORD)uBytes );
}

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
LocalLock (
    _In_ HLOCAL hMem
    )
{
	return (LPVOID)hMem;
}

WINBASEAPI
_Ret_maybenull_
HLOCAL
WINAPI
LocalHandle (
    _In_ LPCVOID pMem
    )
{
	return (HLOCAL)pMem;
}

WINBASEAPI
BOOL
WINAPI
LocalUnlock (
    _In_ HLOCAL hMem
    )
{
	return hMem != NULL;
}

WINBASEAPI
SIZE_T
WINAPI
LocalSize (
    _In_ HLOCAL hMem
    )
{
	return HeapSize( GetProcessHeap(),
					 0,
					 (LPCVOID)hMem );
}

WINBASEAPI
_Success_(return==0)
_Ret_maybenull_
HLOCAL
WINAPI
LocalFree (
    _Frees_ptr_opt_ HLOCAL hMem
    )
{
	UNREFERENCED_PARAMETER(hMem);
	if (HeapFree( GetProcessHeap(),
				  0,
				  (PVOID)hMem)) {
		return NULL;
	}
	return hMem;
}