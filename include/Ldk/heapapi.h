#pragma once

#ifndef _HEAPAPI_H_
#define _HEAPAPI_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "minwinbase.h"

#if _MSC_VER < 1900
#define DECLSPEC_ALLOCATOR
#else
#define DECLSPEC_ALLOCATOR __declspec(allocator)
#endif

EXTERN_C_START

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
HeapCreate(
    _In_ DWORD flOptions,
    _In_ SIZE_T dwInitialSize,
    _In_ SIZE_T dwMaximumSize
    );

WINBASEAPI
BOOL
WINAPI
HeapDestroy(
    _In_ HANDLE hHeap
    );

WINBASEAPI
_Ret_maybenull_
_Post_writable_byte_size_(dwBytes)
LPVOID
WINAPI
HeapAlloc(
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ SIZE_T dwBytes
    );

WINBASEAPI
_Success_(return!=0)
_Ret_maybenull_
_Post_writable_byte_size_(dwBytes)
LPVOID
WINAPI
HeapReAlloc(
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _Frees_ptr_opt_ LPVOID lpMem,
    _In_ SIZE_T dwBytes
    );

WINBASEAPI
BOOL
WINAPI
HeapFree(
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    __drv_freesMem(Mem) _Frees_ptr_opt_ LPVOID lpMem
    );

WINBASEAPI
SIZE_T
WINAPI
HeapSize(
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ LPCVOID lpMem
    );

WINBASEAPI
BOOL
WINAPI
HeapValidate(
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_opt_ LPCVOID lpMem
    );

WINBASEAPI
HANDLE
WINAPI
GetProcessHeap(
    VOID
    );

WINBASEAPI
SIZE_T
WINAPI
HeapCompact(
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags
    );

WINBASEAPI
BOOL
WINAPI
HeapWalk(
    _In_ HANDLE hHeap,
    _Inout_ LPPROCESS_HEAP_ENTRY lpEntry
    );

WINBASEAPI
BOOL
WINAPI
HeapQueryInformation(
    _In_opt_ HANDLE HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _Out_writes_bytes_to_opt_(HeapInformationLength,*ReturnLength) PVOID HeapInformation,
    _In_ SIZE_T HeapInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    );

EXTERN_C_END

#endif // _HEAPAPI_H_