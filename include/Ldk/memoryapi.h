#pragma once

#ifndef _MEMORYAPI_H_
#define _MEMORYAPI_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"

EXTERN_C_START

WINBASEAPI
SIZE_T
WINAPI
VirtualQuery(
    _In_opt_ LPCVOID lpAddress,
    _Out_writes_bytes_to_(dwLength, return) PMEMORY_BASIC_INFORMATION lpBuffer,
    _In_ SIZE_T dwLength
    );

WINBASEAPI
BOOL
WINAPI
VirtualProtect(
    _In_ LPVOID lpAddress,
    _In_ SIZE_T dwSize,
    _In_ DWORD flNewProtect,
    _Out_ PDWORD lpflOldProtect
    );

WINBASEAPI
BOOL
WINAPI
VirtualFree(
    _Pre_notnull_ _When_(dwFreeType == MEM_DECOMMIT, _Post_invalid_) _When_(dwFreeType == MEM_RELEASE, _Post_ptr_invalid_) LPVOID lpAddress,
    _In_ SIZE_T dwSize,
    _In_ DWORD dwFreeType
    );

EXTERN_C_END

#endif // _MEMORYAPI_H_
