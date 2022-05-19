#include "winbase.h"
#include <stdlib.h>



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, MultiByteToWideChar)
#pragma alloc_text(PAGE, WideCharToMultiByte)
#endif



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

	UNICODE_STRING Dst;
	Dst.Buffer = cchWideChar == 0 ? NULL : lpWideCharStr;
	Dst.Length = 0;
	Dst.MaximumLength = lpWideCharStr ? (USHORT)(cchWideChar * sizeof(WCHAR)) : 0;

	if (lpMultiByteStr == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (cbMultiByte == -1) {
		cbMultiByte = (int)strlen(lpMultiByteStr);
	}

	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	switch (CodePage) {
	case CP_THREAD_ACP:
	case CP_ACP: {
		CANSI_STRING Src;
		Src.Buffer = (PCHAR)lpMultiByteStr;
		Src.MaximumLength = Src.Length = (USHORT)cbMultiByte;
		if (Dst.Buffer == NULL || Dst.MaximumLength == 0) {
			return (int)RtlAnsiStringToUnicodeSize( &Src ) / sizeof(WCHAR);
		}
		Status = RtlAnsiStringToUnicodeString( &Dst,
											   &Src,
											   FALSE );
		if (NT_SUCCESS(Status)) {
			return (int)RtlAnsiStringToUnicodeSize( &Src ) / sizeof(WCHAR);
		}
		break;
	}
	case CP_OEMCP: {
		OEM_STRING Src;
		Src.Buffer = (PCHAR)lpMultiByteStr;
		Src.MaximumLength = Src.Length = (USHORT)cbMultiByte;
		if (Dst.Buffer == NULL || Dst.MaximumLength == 0) {
			return (int)RtlOemStringToUnicodeSize( &Src ) / sizeof(WCHAR);
		}
		Status = RtlOemStringToUnicodeString( &Dst,
											  &Src,
											  FALSE );
		if (NT_SUCCESS(Status)) {
			return (int)RtlOemStringToUnicodeSize(&Src) / sizeof(WCHAR);
		}
		break;
	}
	case CP_UTF8: {
		ULONG ActualByteCount = 0;
		if (Dst.Buffer == NULL || Dst.MaximumLength == 0) {
			Status = RtlUTF8ToUnicodeN( NULL,
										0,
										&ActualByteCount,
										lpMultiByteStr,
										cbMultiByte );
		
			return (int)ActualByteCount / sizeof(WCHAR);
		}
		Status = RtlUTF8ToUnicodeN( Dst.Buffer,
									Dst.MaximumLength,
									&ActualByteCount,
									lpMultiByteStr,
									cbMultiByte );
		return (int)ActualByteCount / sizeof(WCHAR);
	}
	default:
		SetLastError( ERROR_INVALID_PARAMETER );
		return 0;
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
	UNREFERENCED_PARAMETER( lpUsedDefaultChar );

	PAGED_CODE();

	if (lpWideCharStr == NULL) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return 0;
	}

	if (cchWideChar == -1) {
		cchWideChar = (int)wcslen(lpWideCharStr);
	}

	UNICODE_STRING Src;
	Src.Buffer = (PWCH)lpWideCharStr;
	Src.MaximumLength = Src.Length = (USHORT)cchWideChar;

	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	switch (CodePage) {
	case CP_THREAD_ACP:
	case CP_ACP: {
		ANSI_STRING Dst;
		Dst.Buffer = cbMultiByte == 0 ? NULL : lpMultiByteStr;
		Dst.Length = 0;
		Dst.MaximumLength = lpMultiByteStr ? (USHORT)(cbMultiByte * sizeof(CHAR)) : 0;
		if (Dst.Buffer == NULL || Dst.MaximumLength == 0) {
			return (int)RtlUnicodeStringToAnsiSize( &Src ) / sizeof(CHAR);
		}
		Status = RtlUnicodeStringToAnsiString( &Dst,
											   &Src,
											   FALSE );
		if (NT_SUCCESS(Status)) {
			return (int)RtlUnicodeStringToAnsiSize( &Src ) / sizeof(CHAR);
		}
		break;
	}
	case CP_OEMCP: {
		OEM_STRING Dst;
		Dst.Buffer = cbMultiByte == 0 ? NULL : lpMultiByteStr;
		Dst.Length = 0;
		Dst.MaximumLength = lpMultiByteStr ? (USHORT)(cbMultiByte * sizeof(CHAR)) : 0;
		if (Dst.Buffer == NULL || Dst.MaximumLength == 0) {
			return (int)RtlUnicodeStringToOemSize( &Src ) / sizeof(CHAR);
		}
		Status = RtlUnicodeStringToOemString( &Dst,
											  &Src,
											  FALSE );
		if (NT_SUCCESS(Status)) {
			return (int)RtlUnicodeStringToOemSize( &Src ) / sizeof(CHAR);
		}
		break;
	}
	case CP_UTF8: {
		ULONG ActualByteCount = 0;
		if (lpMultiByteStr == NULL || cbMultiByte == 0) {
			Status = RtlUnicodeToUTF8N( NULL,
										0,
										&ActualByteCount,
										lpWideCharStr,
										cchWideChar * sizeof(WCHAR) );
		
			return (int)ActualByteCount / sizeof(WCHAR);
		}
		Status = RtlUnicodeToUTF8N( lpMultiByteStr,
									cbMultiByte,
									&ActualByteCount,
									lpWideCharStr,
									cchWideChar * sizeof(WCHAR) );
		return (int)ActualByteCount / sizeof(WCHAR);
	}
	default:
		SetLastError( ERROR_INVALID_PARAMETER );
		return 0;
	}
	
	LdkSetLastNTError( Status );
	return 0;
}