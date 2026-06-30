#pragma once

#ifndef _APISETSTRING_
#define _APISETSTRING_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "winnls.h"

EXTERN_C_START

WINBASEAPI
int
WINAPI
CompareStringEx(
    _In_opt_ LPCWSTR lpLocaleName,
    _In_ DWORD dwCmpFlags,
    _In_NLS_string_(cchCount1) LPCWCH lpString1,
    _In_ int cchCount1,
    _In_NLS_string_(cchCount2) LPCWCH lpString2,
    _In_ int cchCount2,
    _Reserved_ LPNLSVERSIONINFO lpVersionInformation,
    _Reserved_ LPVOID lpReserved,
    _Reserved_ LPARAM lParam
    );

WINBASEAPI
int
WINAPI
CompareStringOrdinal(
    _In_NLS_string_(cchCount1) LPCWCH lpString1,
    _In_ int cchCount1,
    _In_NLS_string_(cchCount2) LPCWCH lpString2,
    _In_ int cchCount2,
    _In_ BOOL bIgnoreCase
    );

WINBASEAPI
int
WINAPI
CompareStringW(
    _In_ LCID Locale,
    _In_ DWORD dwCmpFlags,
    _In_NLS_string_(cchCount1) LPCWCH lpString1,
    _In_ int cchCount1,
    _In_NLS_string_(cchCount2) LPCWCH lpString2,
    _In_ int cchCount2
    );

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
    );

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
    );

WINBASEAPI
BOOL
WINAPI
GetStringTypeExA(
    _In_ LCID Locale,
    _In_ DWORD dwInfoType,
    _In_NLS_string_(cchSrc) LPCSTR lpSrcStr,
    _In_ int cchSrc,
    _Out_ LPWORD lpCharType
    );

WINBASEAPI
BOOL
WINAPI
GetStringTypeExW(
    _In_ LCID Locale,
    _In_ DWORD dwInfoType,
    _In_NLS_string_(cchSrc) LPCWCH lpSrcStr,
    _In_ int cchSrc,
    _Out_ LPWORD lpCharType
    );

WINBASEAPI
BOOL
WINAPI
GetStringTypeA(
    _In_ LCID Locale,
    _In_ DWORD dwInfoType,
    _In_NLS_string_(cchSrc) LPCSTR lpSrcStr,
    _In_ int cchSrc,
    _Out_ LPWORD lpCharType
    );

WINBASEAPI
BOOL
WINAPI
GetStringTypeW(
    _In_ DWORD dwInfoType,
    _In_NLS_string_(cchSrc) LPCWCH lpSrcStr,
    _In_ int cchSrc,
    _Out_ LPWORD lpCharType
    );

EXTERN_C_END

#endif // _APISETSTRING_
