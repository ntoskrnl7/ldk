#include "winbase.h"
#include <stdlib.h>

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
