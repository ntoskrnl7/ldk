#pragma once

#ifndef _APISETHANDLE_
#define _APISETHANDLE_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"

EXTERN_C_START

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)

WINBASEAPI
BOOL
WINAPI
CloseHandle(
    _In_ HANDLE hObject
    );

WINBASEAPI
BOOL
WINAPI
DuplicateHandle(
    _In_ HANDLE hSourceProcessHandle,
    _In_ HANDLE hSourceHandle,
    _In_ HANDLE hTargetProcessHandle,
    _Outptr_ LPHANDLE lpTargetHandle,
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwOptions
    );

EXTERN_C_END

#endif // _APISETHANDLE_