#pragma once

#ifndef _THREADPOOLLEGACYAPISET_H_
#define _THREADPOOLLEGACYAPISET_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "minwinbase.h"

EXTERN_C_START

WINBASEAPI
BOOL
WINAPI
QueueUserWorkItem(
    _In_ LPTHREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_ ULONG Flags
    );

EXTERN_C_END

#endif // _THREADPOOLLEGACYAPISET_H_