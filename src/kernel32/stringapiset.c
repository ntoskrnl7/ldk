#include "winbase.h"
#include <stdlib.h>

WINBASEAPI
_Success_(return != 0)
         _When_((cbMultiByte == -1) && (cchWideChar != 0), _Post_equal_to_(_String_length_(lpWideCharStr)+1))
int
WINAPI
MultiByteToWideChar(
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _In_NLS_string_(cbMultiByte) LPCCH lpMultiByteStr,
    _In_ int cbMultiByte,
    _Out_writes_to_opt_(cchWideChar,return) LPWSTR lpWideCharStr,
    _In_ int cchWideChar
    )
{
	UNREFERENCED_PARAMETER(CodePage);
	UNREFERENCED_PARAMETER(dwFlags);

	if (!cchWideChar) {
		return (int)strlen(lpMultiByteStr) + ((cbMultiByte <= -1) ? 1 : 0);
	}

	if (cbMultiByte == -1) {
		cbMultiByte = (int)strlen(lpMultiByteStr) + 1;
	}
	
	return (int)mbstowcs(lpWideCharStr, lpMultiByteStr, cbMultiByte);
}

WINBASEAPI
_Success_(return != 0)
         _When_((cchWideChar == -1) && (cbMultiByte != 0), _Post_equal_to_(_String_length_(lpMultiByteStr)+1))
int
WINAPI
WideCharToMultiByte(
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
	UNREFERENCED_PARAMETER(CodePage);
	UNREFERENCED_PARAMETER(dwFlags);
	UNREFERENCED_PARAMETER(lpDefaultChar);
	UNREFERENCED_PARAMETER(lpUsedDefaultChar);
	
	if (!cbMultiByte) {
		return (int)wcslen(lpWideCharStr) + ((cchWideChar <= -1) ? 1 : 0);
	}

	if (cchWideChar == -1) {
		cchWideChar = (int)wcslen(lpWideCharStr) + 1;
	}

	return (int)wcstombs(lpMultiByteStr, lpWideCharStr, cchWideChar);
}
