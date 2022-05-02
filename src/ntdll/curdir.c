
#include "ntdll.h"

#include "../ldk.h"

//
// Current Directory Stuff
//

typedef struct _RTLP_CURDIR_REF *PRTLP_CURDIR_REF;

typedef struct _RTL_RELATIVE_NAME_U {
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

typedef enum _RTL_PATH_TYPE {
    RtlPathTypeUnknown,         // 0
    RtlPathTypeUncAbsolute,     // 1
    RtlPathTypeDriveAbsolute,   // 2
    RtlPathTypeDriveRelative,   // 3
    RtlPathTypeRooted,          // 4
    RtlPathTypeRelative,        // 5
    RtlPathTypeLocalDevice,     // 6
    RtlPathTypeRootLocalDevice  // 7
} RTL_PATH_TYPE;

#define IS_PATH_SEPARATOR_U(ch) ((ch == L'\\') || (ch == L'/'))

const UNICODE_STRING RtlpDosLPTDevice = RTL_CONSTANT_STRING(L"LPT");
const UNICODE_STRING RtlpDosCOMDevice = RTL_CONSTANT_STRING(L"COM");
const UNICODE_STRING RtlpDosPRNDevice = RTL_CONSTANT_STRING(L"PRN");
const UNICODE_STRING RtlpDosAUXDevice = RTL_CONSTANT_STRING(L"AUX");
const UNICODE_STRING RtlpDosNULDevice = RTL_CONSTANT_STRING(L"NUL");
const UNICODE_STRING RtlpDosCONDevice = RTL_CONSTANT_STRING(L"CON");

const UNICODE_STRING RtlpDosSlashCONDevice = RTL_CONSTANT_STRING(L"\\\\.\\CON");

RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U (
    _In_ PCWSTR DosFileName
    )
{
	RTL_PATH_TYPE ReturnValue;

	if (IS_PATH_SEPARATOR_U(*DosFileName)) {
		if (IS_PATH_SEPARATOR_U(*(DosFileName+1))) {
			if (DosFileName[2] == L'.') {
				if (IS_PATH_SEPARATOR_U(*(DosFileName+3)) ){
					ReturnValue = RtlPathTypeLocalDevice;
				}else if ( (*(DosFileName+3)) == UNICODE_NULL ){
					ReturnValue = RtlPathTypeRootLocalDevice;
				} else {
					ReturnValue = RtlPathTypeUncAbsolute;
				}
			} else {
				ReturnValue = RtlPathTypeUncAbsolute;
			}
		} else {
			ReturnValue = RtlPathTypeRooted;
		}
	} else if (*DosFileName && *(DosFileName+1) == L':') {
		if (IS_PATH_SEPARATOR_U(*(DosFileName+2))) {
			ReturnValue = RtlPathTypeDriveAbsolute;
		} else  {
			ReturnValue = RtlPathTypeDriveRelative;
		}
	} else {
		ReturnValue = RtlPathTypeRelative;
	}

	return ReturnValue;
}

ULONG
NTAPI
RtlIsDosDeviceName_Ustr (
	_In_ PUNICODE_STRING DosFileName
	)
{
	UNICODE_STRING UnicodeString;
	USHORT NumberOfCharacters;
	ULONG ReturnLength;
	ULONG ReturnOffset;
	LPWSTR p;
	USHORT ColonBias = 0;
	WCHAR wch;
	RTL_PATH_TYPE PathType = RtlDetermineDosPathNameType_U(DosFileName->Buffer);

	switch ( PathType ) {
	case RtlPathTypeLocalDevice:
		if (RtlEqualUnicodeString( DosFileName, &RtlpDosSlashCONDevice, TRUE )) {
			return 0x00080006;
		}
	case RtlPathTypeUncAbsolute:
	case RtlPathTypeUnknown:
		return 0;
	}

	UnicodeString = *DosFileName;
	NumberOfCharacters = DosFileName->Length >> 1;

	if (NumberOfCharacters && DosFileName->Buffer[NumberOfCharacters-1] == L':') {
		UnicodeString.Length -= sizeof(WCHAR);
		NumberOfCharacters--;
		ColonBias = 1;
	}

	if (NumberOfCharacters == 0) {
		return 0;
	}

	wch = UnicodeString.Buffer[NumberOfCharacters-1];

	while (NumberOfCharacters && (wch == L'.' || wch == L' ')) {
		UnicodeString.Length -= sizeof(WCHAR);
		NumberOfCharacters--;
		ColonBias++;
		wch = UnicodeString.Buffer[NumberOfCharacters-1];
	}

	ReturnLength = NumberOfCharacters << 1;
	ReturnOffset = 0;

	if (NumberOfCharacters) {

		p = UnicodeString.Buffer + NumberOfCharacters-1;
		
		while ( p >= UnicodeString.Buffer ) {
			if (*p == L'\\' ||
				*p == L'/' ||
				(*p == L':' && p == UnicodeString.Buffer + 1)) {
				
				p++;
				wch = (*p) | 0x20;

				if (! (wch == L'l' || wch == L'c' || wch == L'p' || wch == L'a' || wch == L'n')) {
					return 0;
				}

				ReturnOffset = (ULONG)((PSZ)p - (PSZ)UnicodeString.Buffer);
				RtlInitUnicodeString(&UnicodeString,p);
				NumberOfCharacters = UnicodeString.Length >> 1;
				NumberOfCharacters -= ColonBias;
				ReturnLength = NumberOfCharacters << 1;
				UnicodeString.Length -= ColonBias*sizeof(WCHAR);
				break;
			}
			p--;
		}

		wch = UnicodeString.Buffer[0] | 0x20;

		if ( !( wch == L'l' || wch == L'c' || wch == L'p' || wch == L'a' || wch == L'n' ) ) {
			return 0;
		}
	}

	p = UnicodeString.Buffer;
	while (p < UnicodeString.Buffer + NumberOfCharacters && *p != L'.' && *p != L':') {
		p++;
	}

	while (p > UnicodeString.Buffer && p[-1] == L' ') {
		p--;
	}

	NumberOfCharacters = (USHORT)(p - UnicodeString.Buffer);
	UnicodeString.Length = NumberOfCharacters * sizeof( WCHAR );
	
	if (NumberOfCharacters == 4 && iswdigit(UnicodeString.Buffer[3])) {
		if ((WCHAR)UnicodeString.Buffer[3] == L'0') {
			return 0;
		} else {
			UnicodeString.Length -= sizeof(WCHAR);
			if (RtlEqualUnicodeString(&UnicodeString,&RtlpDosLPTDevice,TRUE) ||
				RtlEqualUnicodeString(&UnicodeString,&RtlpDosCOMDevice,TRUE) ) {
				
				ReturnLength = NumberOfCharacters << 1;
			} else {
				return 0;
			}
		}
	} else if (NumberOfCharacters != 3) {
		return 0;
	} else if (RtlEqualUnicodeString(&UnicodeString,&RtlpDosPRNDevice, TRUE)) {
		ReturnLength = NumberOfCharacters << 1;
	} else if (RtlEqualUnicodeString(&UnicodeString,&RtlpDosAUXDevice, TRUE)) {
		ReturnLength = NumberOfCharacters << 1;
	} else if (RtlEqualUnicodeString(&UnicodeString,&RtlpDosNULDevice, TRUE)) {
		ReturnLength = NumberOfCharacters << 1;
	} else if (RtlEqualUnicodeString(&UnicodeString,&RtlpDosCONDevice, TRUE)) {
		ReturnLength = NumberOfCharacters << 1;
	} else {
		return 0;
	}

	return ReturnLength | (ReturnOffset << 16);
}

ULONG
NTAPI
RtlIsDosDeviceName_U(
    _In_ PCWSTR DosFileName
    )
{
    UNICODE_STRING UnicodeString;
    RtlInitUnicodeString(&UnicodeString,DosFileName);
    return RtlIsDosDeviceName_Ustr(&UnicodeString);
}