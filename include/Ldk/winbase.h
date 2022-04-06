#pragma once

#include "minwinbase.h"
#include "debugapi.h"
#include "errhandlingapi.h"
#include "handleapi.h"
#include "processthreadsapi.h"
#include "profileapi.h"
#include "synchapi.h"
#include "sysinfoapi.h"
#include  "libloaderapi.h"



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

EXTERN_C_END