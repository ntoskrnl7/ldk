﻿#pragma once

#ifndef _FIBERS_H_
#define _FIBERS_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"

EXTERN_C_START

#if (_WIN32_WINNT >= 0x0600)

#ifndef FLS_OUT_OF_INDEXES
#define FLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#endif

WINBASEAPI
DWORD
WINAPI
FlsAlloc(
    _In_opt_ PFLS_CALLBACK_FUNCTION lpCallback
    );


WINBASEAPI
PVOID
WINAPI
FlsGetValue(
    _In_ DWORD dwFlsIndex
    );


WINBASEAPI
BOOL
WINAPI
FlsSetValue(
    _In_ DWORD dwFlsIndex,
    _In_opt_ PVOID lpFlsData
    );


WINBASEAPI
BOOL
WINAPI
FlsFree(
    _In_ DWORD dwFlsIndex
    );

#endif // (_WIN32_WINNT >= 0x0600)

EXTERN_C_END

#endif // _FIBERS_H_