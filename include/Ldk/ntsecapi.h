#pragma once

#ifndef _NTSECAPI_
#define _NTSECAPI_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI
BOOLEAN
WINAPI
SystemFunction036(
    _Out_writes_bytes_(RandomBufferLength) PVOID RandomBuffer,
    _In_ ULONG RandomBufferLength
    );

#ifndef RtlGenRandom
#define RtlGenRandom SystemFunction036
#endif

#ifdef __cplusplus
}
#endif

#endif // _NTSECAPI_
