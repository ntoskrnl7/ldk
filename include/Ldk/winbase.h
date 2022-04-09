#pragma once

#ifndef _WINBASE_
#define _WINBASE_

#include "minwinbase.h"
#include "debugapi.h"
#include "errhandlingapi.h"
#include "handleapi.h"
#include "processthreadsapi.h"
#include "profileapi.h"
#include "synchapi.h"
#include "sysinfoapi.h"
#include  "libloaderapi.h"
#include  "utilapiset.h"
#include "threadpoolapiset.h"
#include "threadpoollegacyapiset.h"
#include "stringapiset.h"

EXTERN_C_START

#define CREATE_SUSPENDED				0x00000004

#define INFINITE						0xFFFFFFFF  // Infinite timeout



#define WAIT_FAILED             ((DWORD)0xFFFFFFFF)
#define WAIT_OBJECT_0           ((STATUS_WAIT_0 ) + 0 )

#define WAIT_ABANDONED          ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0        ((STATUS_ABANDONED_WAIT_0 ) + 0 )

#define WAIT_IO_COMPLETION      STATUS_USER_APC



WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryA(
    _In_ LPCSTR lpLibFileName
    );

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryW(
    _In_ LPCWSTR lpLibFileName
    );



#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100

WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageA(
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) != 0, _At_((LPSTR*)lpBuffer, _Outptr_result_z_))
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) == 0, _Out_writes_z_(nSize))
             LPSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    );
WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageW(
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) != 0, _At_((LPWSTR*)lpBuffer, _Outptr_result_z_))
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) == 0, _Out_writes_z_(nSize))
             LPWSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    );

#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_FROM_STRING     0x00000400
#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0x00002000
#define FORMAT_MESSAGE_MAX_WIDTH_MASK  0x000000FF



WINBASEAPI
_Success_(return==0)
_Ret_maybenull_
HLOCAL
WINAPI
LocalFree(
    _Frees_ptr_opt_ HLOCAL hMem
    );

EXTERN_C_END

#endif // _WINBASE_
