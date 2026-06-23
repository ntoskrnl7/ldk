#include "winbase.h"
#include "../ntdll/ntdll.h"
#include <stdlib.h>


#define MB_UTF8_VALID_FLAGS        MB_ERR_INVALID_CHARS
#define WC_UTF8_VALID_FLAGS        (WC_COMPOSITECHECK | \
									WC_DISCARDNS | \
									WC_SEPCHARS | \
									WC_DEFAULTCHAR | \
									WC_ERR_INVALID_CHARS | \
									WC_NO_BEST_FIT_CHARS)


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

static
BOOLEAN
LdkpIsHangul (
	_In_ WCHAR Ch
	);

static
BOOLEAN
LdkpIsCjkIdeograph (
	_In_ WCHAR Ch
	);

static
BOOLEAN
LdkpIsCyrillic (
	_In_ WCHAR Ch
	);

static
BOOLEAN
LdkpIsHiragana (
	_In_ WCHAR Ch
	);

static
BOOLEAN
LdkpIsKatakana (
	_In_ WCHAR Ch
	);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdkpOemToUnicodeSize)
#pragma alloc_text(PAGE, LdkpUnicodeToOemSize)
#pragma alloc_text(PAGE, LdkpCustomCPToUnicodeSize)
#pragma alloc_text(PAGE, LdkpUnicodeToCustomCPSize)
#pragma alloc_text(PAGE, LdkpIsHangul)
#pragma alloc_text(PAGE, LdkpIsCjkIdeograph)
#pragma alloc_text(PAGE, LdkpIsCyrillic)
#pragma alloc_text(PAGE, LdkpIsHiragana)
#pragma alloc_text(PAGE, LdkpIsKatakana)
#pragma alloc_text(PAGE, MultiByteToWideChar)
#pragma alloc_text(PAGE, WideCharToMultiByte)
#pragma alloc_text(PAGE, GetStringTypeW)
#endif

static
BOOLEAN
LdkpIsHangul (
	_In_ WCHAR Ch
	)
{
	return (Ch >= 0x1100 && Ch <= 0x11ff) ||
		   (Ch >= 0x3130 && Ch <= 0x318f) ||
		   (Ch >= 0xac00 && Ch <= 0xd7af);
}

static
BOOLEAN
LdkpIsCjkIdeograph (
	_In_ WCHAR Ch
	)
{
	return (Ch >= 0x3400 && Ch <= 0x4dbf) ||
		   (Ch >= 0x4e00 && Ch <= 0x9fff) ||
		   (Ch >= 0xf900 && Ch <= 0xfaff);
}

static
BOOLEAN
LdkpIsCyrillic (
	_In_ WCHAR Ch
	)
{
	return Ch >= 0x0400 && Ch <= 0x052f;
}

static
BOOLEAN
LdkpIsHiragana (
	_In_ WCHAR Ch
	)
{
	return Ch >= 0x3040 && Ch <= 0x309f;
}

static
BOOLEAN
LdkpIsKatakana (
	_In_ WCHAR Ch
	)
{
	return (Ch >= 0x30a0 && Ch <= 0x30ff) ||
		   (Ch >= 0xff66 && Ch <= 0xff9f);
}

static
WORD
LdkpGetStringType1 (
	_In_ WCHAR Ch
	)
{
	WORD Type = 0;

	if (Ch <= 0x7f) {
		if (Ch >= L'A' && Ch <= L'Z') {
			Type |= C1_UPPER | C1_ALPHA;
		} else if (Ch >= L'a' && Ch <= L'z') {
			Type |= C1_LOWER | C1_ALPHA;
		} else if (Ch >= L'0' && Ch <= L'9') {
			Type |= C1_DIGIT;
		}

		if ((Ch >= L'0' && Ch <= L'9') ||
			(Ch >= L'A' && Ch <= L'F') ||
			(Ch >= L'a' && Ch <= L'f')) {
			Type |= C1_XDIGIT;
		}

		if (Ch == L' ' || Ch == L'\f' || Ch == L'\n' ||
			Ch == L'\r' || Ch == L'\t' || Ch == L'\v') {
			Type |= C1_SPACE;
		}

		if (Ch == L' ' || Ch == L'\t') {
			Type |= C1_BLANK;
		}

		if (Ch < L' ' || Ch == 0x7f) {
			Type |= C1_CNTRL;
		} else {
			Type |= C1_DEFINED;
			if ((Type & (C1_ALPHA | C1_DIGIT | C1_SPACE)) == 0) {
				Type |= C1_PUNCT;
			}
		}
	} else {
		Type |= C1_DEFINED | C1_ALPHA;
	}

	return Type;
}

static
WORD
LdkpGetStringType2 (
	_In_ WCHAR Ch
	)
{
	if (Ch == L' ' || Ch == L'\f' || Ch == L'\n' ||
		Ch == L'\r' || Ch == L'\t' || Ch == L'\v') {
		return C2_WHITESPACE;
	}

	if (Ch >= L'0' && Ch <= L'9') {
		return C2_EUROPENUMBER;
	}

	if ((Ch >= L'A' && Ch <= L'Z') || (Ch >= L'a' && Ch <= L'z')) {
		return C2_LEFTTORIGHT;
	}

	if (LdkpIsHangul( Ch ) ||
		LdkpIsCjkIdeograph( Ch ) ||
		LdkpIsCyrillic( Ch ) ||
		LdkpIsHiragana( Ch ) ||
		LdkpIsKatakana( Ch )) {
		return C2_LEFTTORIGHT;
	}

	return C2_OTHERNEUTRAL;
}

static
WORD
LdkpGetStringType3 (
	_In_ WCHAR Ch
	)
{
	if ((Ch >= L'A' && Ch <= L'Z') || (Ch >= L'a' && Ch <= L'z')) {
		return C3_ALPHA;
	}

	if ((Ch >= 0x21 && Ch <= 0x2f) || (Ch >= 0x3a && Ch <= 0x40) ||
		(Ch >= 0x5b && Ch <= 0x60) || (Ch >= 0x7b && Ch <= 0x7e)) {
		return C3_SYMBOL;
	}

	if (LdkpIsHiragana( Ch )) {
		return C3_HIRAGANA;
	}

	if (LdkpIsKatakana( Ch )) {
		return (Ch >= 0xff66 && Ch <= 0xff9f) ?
			   (C3_KATAKANA | C3_HALFWIDTH) :
			   C3_KATAKANA;
	}

	if (LdkpIsCjkIdeograph( Ch )) {
		return C3_IDEOGRAPH;
	}

	if (LdkpIsHangul( Ch ) ||
		LdkpIsCyrillic( Ch )) {
		return C3_ALPHA;
	}

	return C3_NOTAPPLICABLE;
}

WINBASEAPI
BOOL
WINAPI
GetStringTypeW (
    _In_ DWORD dwInfoType,
    _In_NLS_string_(cchSrc) LPCWCH lpSrcStr,
    _In_ int cchSrc,
    _Out_ LPWORD lpCharType
    )
{
	PAGED_CODE();

	if (lpSrcStr == NULL || lpCharType == NULL || cchSrc < -1) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (cchSrc == -1) {
		cchSrc = (int)wcslen(lpSrcStr) + 1;
	}

	for (int Index = 0; Index < cchSrc; ++Index) {
		switch (dwInfoType) {
		case CT_CTYPE1:
			lpCharType[Index] = LdkpGetStringType1( lpSrcStr[Index] );
			break;
		case CT_CTYPE2:
			lpCharType[Index] = LdkpGetStringType2( lpSrcStr[Index] );
			break;
		case CT_CTYPE3:
			lpCharType[Index] = LdkpGetStringType3( lpSrcStr[Index] );
			break;
		default:
			SetLastError( ERROR_INVALID_PARAMETER );
			return FALSE;
		}
	}

	return TRUE;
}



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
		if (dwFlags & ~MB_UTF8_VALID_FLAGS) {
			SetLastError( ERROR_INVALID_FLAGS );
			return 0;
		}
		Status = RtlUTF8ToUnicodeN( NULL,
									0,
									&BytesNeeded,
									lpMultiByteStr,
									(ULONG)cbMultiByte );
		if (Status == STATUS_SOME_NOT_MAPPED &&
			FlagOn(dwFlags, MB_ERR_INVALID_CHARS)) {
			SetLastError( ERROR_NO_UNICODE_TRANSLATION );
			return 0;
		}
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
		if (Status == STATUS_SOME_NOT_MAPPED &&
			FlagOn(dwFlags, MB_ERR_INVALID_CHARS)) {
			SetLastError( ERROR_NO_UNICODE_TRANSLATION );
			return 0;
		}
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
		if (dwFlags & ~WC_UTF8_VALID_FLAGS) {
			SetLastError( ERROR_INVALID_FLAGS );
			return 0;
		}
		Status = RtlUnicodeToUTF8N( NULL,
									0,
									&BytesNeeded,
									lpWideCharStr,
									SourceBytes );
		if (Status == STATUS_SOME_NOT_MAPPED &&
			FlagOn(dwFlags, WC_ERR_INVALID_CHARS)) {
			SetLastError( ERROR_NO_UNICODE_TRANSLATION );
			return 0;
		}
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
		if (Status == STATUS_SOME_NOT_MAPPED &&
			FlagOn(dwFlags, WC_ERR_INVALID_CHARS)) {
			SetLastError( ERROR_NO_UNICODE_TRANSLATION );
			return 0;
		}
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
