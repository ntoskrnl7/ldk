#pragma once

#ifndef _APISETUTIL_
#define _APISETUTIL_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"

EXTERN_C_START

WINBASEAPI
_Ret_maybenull_
PVOID
WINAPI
EncodePointer(
    _In_opt_ PVOID Ptr
    );


WINBASEAPI
_Ret_maybenull_
PVOID
WINAPI
DecodePointer(
    _In_opt_ PVOID Ptr
    );

EXTERN_C_END

#endif // _APISETUTIL_
