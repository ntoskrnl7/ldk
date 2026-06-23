#pragma once

#ifndef _WINREG_
#define _WINREG_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#ifdef WINADVAPI
#undef WINADVAPI
#endif
#define WINADVAPI

#include <minwindef.h>
#include "winnt.h"

EXTERN_C_START

typedef ACCESS_MASK REGSAM;
typedef LONG LSTATUS;

#define HKEY_CLASSES_ROOT     ((HKEY)(ULONG_PTR)((LONG)0x80000000))
#define HKEY_CURRENT_USER     ((HKEY)(ULONG_PTR)((LONG)0x80000001))
#define HKEY_LOCAL_MACHINE    ((HKEY)(ULONG_PTR)((LONG)0x80000002))
#define HKEY_USERS            ((HKEY)(ULONG_PTR)((LONG)0x80000003))
#define HKEY_PERFORMANCE_DATA ((HKEY)(ULONG_PTR)((LONG)0x80000004))
#define HKEY_CURRENT_CONFIG   ((HKEY)(ULONG_PTR)((LONG)0x80000005))

WINADVAPI
LSTATUS
APIENTRY
RegOpenKeyExW(
    _In_ HKEY hKey,
    _In_opt_ LPCWSTR lpSubKey,
    _Reserved_ DWORD ulOptions,
    _In_ REGSAM samDesired,
    _Out_ PHKEY phkResult
    );

WINADVAPI
LSTATUS
APIENTRY
RegQueryValueExW(
    _In_ HKEY hKey,
    _In_opt_ LPCWSTR lpValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) LPBYTE lpData,
    _Inout_opt_ LPDWORD lpcbData
    );

WINADVAPI
LSTATUS
APIENTRY
RegCloseKey(
    _In_ HKEY hKey
    );

EXTERN_C_END

#endif // _WINREG_
