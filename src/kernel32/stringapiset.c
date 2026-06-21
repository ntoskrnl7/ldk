#include "winbase.h"
#include "../ntdll/ntdll.h"
#include <stdlib.h>



static
NTSTATUS
LdkpOemToUnicodeSize (
	_Out_ PULONG BytesNeeded,
	_In_reads_bytes_(BytesInOemString) PCCH OemString,
	_In_ ULONG BytesInOemString
	);

static
NTSTATUS
LdkpUnicodeToOemSize (
	_Out_ PULONG BytesNeeded,
	_In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
	_In_ ULONG BytesInUnicodeString
	);

static
NTSTATUS
LdkpCustomCPToUnicodeSize (
	_In_ PCPTABLEINFO CodePageTable,
	_Out_ PULONG BytesNeeded,
	_In_reads_bytes_(BytesInCustomCPString) PCCH CustomCPString,
	_In_ ULONG BytesInCustomCPString
	);

static
NTSTATUS
LdkpUnicodeToCustomCPSize (
	_In_ PCPTABLEINFO CodePageTable,
	_Out_ PULONG BytesNeeded,
	_In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
	_In_ ULONG BytesInUnicodeString
	);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdkpOemToUnicodeSize)
#pragma alloc_text(PAGE, LdkpUnicodeToOemSize)
#pragma alloc_text(PAGE, LdkpCustomCPToUnicodeSize)
#pragma alloc_text(PAGE, LdkpUnicodeToCustomCPSize)
#pragma alloc_text(PAGE, MultiByteToWideChar)
#pragma alloc_text(PAGE, WideCharToMultiByte)
#endif



static
NTSTATUS
LdkpOemToUnicodeSize (
	_Out_ PULONG BytesNeeded,
	_In_reads_bytes_(BytesInOemString) PCCH OemString,
	_In_ ULONG BytesInOemString
	)
{
	PWCH Buffer;
	ULONG MaximumBytesInUnicodeString;
	NTSTATUS Status;

	PAGED_CODE();

	if (BytesInOemString > (ULONG_MAX / sizeof(WCHAR))) {
		return STATUS_INVALID_PARAMETER;
	}

	MaximumBytesInUnicodeString = BytesInOemString * sizeof(WCHAR);
	Buffer = RtlAllocateHeap( RtlProcessHeap(),
							  0,
							  MaximumBytesInUnicodeString );
	if (Buffer == NULL) {
		return STATUS_NO_MEMORY;
	}

	Status = RtlOemToUnicodeN( Buffer,
							   MaximumBytesInUnicodeString,
							   BytesNeeded,
							   OemString,
							   BytesInOemString );

	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 Buffer );

	return Status;
}

static
NTSTATUS
LdkpUnicodeToOemSize (
	_Out_ PULONG BytesNeeded,
	_In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
	_In_ ULONG BytesInUnicodeString
	)
{
	PCHAR Buffer;
	NTSTATUS Status;

	PAGED_CODE();

	Buffer = RtlAllocateHeap( RtlProcessHeap(),
							  0,
							  BytesInUnicodeString );
	if (Buffer == NULL) {
		return STATUS_NO_MEMORY;
	}

	Status = RtlUnicodeToOemN( Buffer,
							   BytesInUnicodeString,
							   BytesNeeded,
							   UnicodeString,
							   BytesInUnicodeString );

	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 Buffer );

	return Status;
}

static
NTSTATUS
LdkpCustomCPToUnicodeSize (
	_In_ PCPTABLEINFO CodePageTable,
	_Out_ PULONG BytesNeeded,
	_In_reads_bytes_(BytesInCustomCPString) PCCH CustomCPString,
	_In_ ULONG BytesInCustomCPString
	)
{
	PWCH Buffer;
	ULONG MaximumBytesInUnicodeString;
	NTSTATUS Status;

	PAGED_CODE();

	if (BytesInCustomCPString > (ULONG_MAX / sizeof(WCHAR))) {
		return STATUS_INVALID_PARAMETER;
	}

	MaximumBytesInUnicodeString = BytesInCustomCPString * sizeof(WCHAR);
	Buffer = RtlAllocateHeap( RtlProcessHeap(),
							  0,
							  MaximumBytesInUnicodeString );
	if (Buffer == NULL) {
		return STATUS_NO_MEMORY;
	}

	Status = RtlCustomCPToUnicodeN( CodePageTable,
									Buffer,
									MaximumBytesInUnicodeString,
									BytesNeeded,
									(PCH)CustomCPString,
									BytesInCustomCPString );

	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 Buffer );

	return Status;
}

static
NTSTATUS
LdkpUnicodeToCustomCPSize (
	_In_ PCPTABLEINFO CodePageTable,
	_Out_ PULONG BytesNeeded,
	_In_reads_bytes_(BytesInUnicodeString) PCWCH UnicodeString,
	_In_ ULONG BytesInUnicodeString
	)
{
	PCHAR Buffer;
	ULONG Characters;
	ULONG MaximumBytesInCustomCPString;
	NTSTATUS Status;

	PAGED_CODE();

	if (CodePageTable->MaximumCharacterSize == 0) {
		return STATUS_INVALID_PARAMETER;
	}

	Characters = BytesInUnicodeString / sizeof(WCHAR);
	if (Characters > (ULONG_MAX / CodePageTable->MaximumCharacterSize)) {
		return STATUS_INVALID_PARAMETER;
	}

	MaximumBytesInCustomCPString = Characters * CodePageTable->MaximumCharacterSize;
	Buffer = RtlAllocateHeap( RtlProcessHeap(),
							  0,
							  MaximumBytesInCustomCPString );
	if (Buffer == NULL) {
		return STATUS_NO_MEMORY;
	}

	Status = RtlUnicodeToCustomCPN( CodePageTable,
									Buffer,
									MaximumBytesInCustomCPString,
									BytesNeeded,
									(PWCH)UnicodeString,
									BytesInUnicodeString );

	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 Buffer );

	return Status;
}



WINBASEAPI
_Success_(return != 0)
         _When_((cbMultiByte == -1) && (cchWideChar != 0), _Post_equal_to_(_String_length_(lpWideCharStr)+1))
int
WINAPI
MultiByteToWideChar (
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cbMultiByte) LPCCH lpMultiByteStr,
    _In_ int cbMultiByte,
    _Out_writes_to_opt_(cchWideChar,return) LPWSTR lpWideCharStr,
    _In_ int cchWideChar
    )
{
	UNREFERENCED_PARAMETER( dwFlags );

	PAGED_CODE();

	if (lpMultiByteStr == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (cbMultiByte == -1) {
		cbMultiByte = (int)strlen(lpMultiByteStr) + 1;
	} else if (cbMultiByte <= 0) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (cchWideChar < 0 || (! lpWideCharStr && cchWideChar != 0)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ULONG BytesNeeded = 0;
	ULONG BytesWritten = 0;
	ULONG DestinationBytes = (ULONG)cchWideChar * sizeof(WCHAR);

	switch (CodePage) {
	case CP_THREAD_ACP:
	case CP_ACP:
		Status = RtlMultiByteToUnicodeSize( &BytesNeeded,
											lpMultiByteStr,
											(ULONG)cbMultiByte );
		if (! NT_SUCCESS(Status)) {
			break;
		}
		if (lpWideCharStr == NULL || cchWideChar == 0) {
			return (int)(BytesNeeded / sizeof(WCHAR));
		}
		if (DestinationBytes < BytesNeeded) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		Status = RtlMultiByteToUnicodeN( lpWideCharStr,
										 DestinationBytes,
										 &BytesWritten,
										 lpMultiByteStr,
										 (ULONG)cbMultiByte );
		if (NT_SUCCESS(Status)) {
			return (int)(BytesWritten / sizeof(WCHAR));
		}
		break;

	case CP_OEMCP:
		Status = LdkpOemToUnicodeSize( &BytesNeeded,
										lpMultiByteStr,
										(ULONG)cbMultiByte );
		if (! NT_SUCCESS(Status)) {
			break;
		}
		if (lpWideCharStr == NULL || cchWideChar == 0) {
			return (int)(BytesNeeded / sizeof(WCHAR));
		}
		if (DestinationBytes < BytesNeeded) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		Status = RtlOemToUnicodeN( lpWideCharStr,
								   DestinationBytes,
								   &BytesWritten,
								   lpMultiByteStr,
								   (ULONG)cbMultiByte );
		if (NT_SUCCESS(Status)) {
			return (int)(BytesWritten / sizeof(WCHAR));
		}
		break;

	case CP_UTF8:
		Status = RtlUTF8ToUnicodeN( NULL,
									0,
									&BytesNeeded,
									lpMultiByteStr,
									(ULONG)cbMultiByte );
		if (! NT_SUCCESS(Status)) {
			break;
		}
		if (lpWideCharStr == NULL || cchWideChar == 0) {
			return (int)(BytesNeeded / sizeof(WCHAR));
		}
		if (DestinationBytes < BytesNeeded) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		Status = RtlUTF8ToUnicodeN( lpWideCharStr,
									DestinationBytes,
									&BytesWritten,
									lpMultiByteStr,
									(ULONG)cbMultiByte );
		if (NT_SUCCESS(Status)) {
			return (int)(BytesWritten / sizeof(WCHAR));
		}
		break;

	default: {
		PCPTABLEINFO CodePageTable;

		Status = LdkpGetCodePageTable( CodePage,
										&CodePageTable );
		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return 0;
		}

		Status = LdkpCustomCPToUnicodeSize( CodePageTable,
											&BytesNeeded,
											lpMultiByteStr,
											(ULONG)cbMultiByte );
		if (! NT_SUCCESS(Status)) {
			break;
		}
		if (lpWideCharStr == NULL || cchWideChar == 0) {
			return (int)(BytesNeeded / sizeof(WCHAR));
		}
		if (DestinationBytes < BytesNeeded) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		Status = RtlCustomCPToUnicodeN( CodePageTable,
										lpWideCharStr,
										DestinationBytes,
										&BytesWritten,
										(PCH)lpMultiByteStr,
										(ULONG)cbMultiByte );
		if (NT_SUCCESS(Status)) {
			return (int)(BytesWritten / sizeof(WCHAR));
		}
		break;
	}
	}
	
	LdkSetLastNTError( Status );
	return 0;
}

WINBASEAPI
_Success_(return != 0)
         _When_((cchWideChar == -1) && (cbMultiByte != 0), _Post_equal_to_(_String_length_(lpMultiByteStr)+1))
int
WINAPI
WideCharToMultiByte (
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cchWideChar) LPCWCH lpWideCharStr,
    _In_ int cchWideChar,
    _Out_writes_bytes_to_opt_(cbMultiByte,return) LPSTR lpMultiByteStr,
    _In_ int cbMultiByte,
    _In_opt_ LPCCH lpDefaultChar,
    _Out_opt_ LPBOOL lpUsedDefaultChar
    )
{
	UNREFERENCED_PARAMETER( dwFlags );
	UNREFERENCED_PARAMETER( lpDefaultChar );

	PAGED_CODE();

	if (lpWideCharStr == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (cchWideChar == -1) {
		cchWideChar = (int)wcslen(lpWideCharStr) + 1;
	} else if (cchWideChar <= 0) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (cbMultiByte < 0 || (! lpMultiByteStr && cbMultiByte != 0)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (lpUsedDefaultChar) {
		*lpUsedDefaultChar = FALSE;
	}

	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ULONG BytesNeeded = 0;
	ULONG BytesWritten = 0;
	ULONG SourceBytes = (ULONG)cchWideChar * sizeof(WCHAR);

	switch (CodePage) {
	case CP_THREAD_ACP:
	case CP_ACP:
		Status = RtlUnicodeToMultiByteSize( &BytesNeeded,
											lpWideCharStr,
											SourceBytes );
		if (! NT_SUCCESS(Status)) {
			break;
		}
		if (lpMultiByteStr == NULL || cbMultiByte == 0) {
			return (int)BytesNeeded;
		}
		if ((ULONG)cbMultiByte < BytesNeeded) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		Status = RtlUnicodeToMultiByteN( lpMultiByteStr,
										 (ULONG)cbMultiByte,
										 &BytesWritten,
										 lpWideCharStr,
										 SourceBytes );
		if (NT_SUCCESS(Status)) {
			return (int)BytesWritten;
		}
		break;

	case CP_OEMCP:
		Status = LdkpUnicodeToOemSize( &BytesNeeded,
										lpWideCharStr,
										SourceBytes );
		if (! NT_SUCCESS(Status)) {
			break;
		}
		if (lpMultiByteStr == NULL || cbMultiByte == 0) {
			return (int)BytesNeeded;
		}
		if ((ULONG)cbMultiByte < BytesNeeded) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		Status = RtlUnicodeToOemN( lpMultiByteStr,
								   (ULONG)cbMultiByte,
								   &BytesWritten,
								   lpWideCharStr,
								   SourceBytes );
		if (NT_SUCCESS(Status)) {
			return (int)BytesWritten;
		}
		break;

	case CP_UTF8:
		if (lpDefaultChar || lpUsedDefaultChar) {
			SetLastError( ERROR_INVALID_PARAMETER );
			return 0;
		}
		Status = RtlUnicodeToUTF8N( NULL,
									0,
									&BytesNeeded,
									lpWideCharStr,
									SourceBytes );
		if (! NT_SUCCESS(Status)) {
			break;
		}
		if (lpMultiByteStr == NULL || cbMultiByte == 0) {
			return (int)BytesNeeded;
		}
		if ((ULONG)cbMultiByte < BytesNeeded) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		Status = RtlUnicodeToUTF8N( lpMultiByteStr,
									(ULONG)cbMultiByte,
									&BytesWritten,
									lpWideCharStr,
									SourceBytes );
		if (NT_SUCCESS(Status)) {
			return (int)BytesWritten;
		}
		break;

	default: {
		PCPTABLEINFO CodePageTable;

		Status = LdkpGetCodePageTable( CodePage,
										&CodePageTable );
		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return 0;
		}

		Status = LdkpUnicodeToCustomCPSize( CodePageTable,
											&BytesNeeded,
											lpWideCharStr,
											SourceBytes );
		if (! NT_SUCCESS(Status)) {
			break;
		}
		if (lpMultiByteStr == NULL || cbMultiByte == 0) {
			return (int)BytesNeeded;
		}
		if ((ULONG)cbMultiByte < BytesNeeded) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		Status = RtlUnicodeToCustomCPN( CodePageTable,
										lpMultiByteStr,
										(ULONG)cbMultiByte,
										&BytesWritten,
										(PWCH)lpWideCharStr,
										SourceBytes );
		if (NT_SUCCESS(Status)) {
			return (int)BytesWritten;
		}
		break;
	}
	}
	
	LdkSetLastNTError( Status );
	return 0;
}