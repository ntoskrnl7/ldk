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

WINBASEAPI
BOOL
WINAPI
RegisterWaitForSingleObject(
    _Outptr_ PHANDLE phNewWaitObject,
    _In_ HANDLE hObject,
    _In_ WAITORTIMERCALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_ ULONG dwMilliseconds,
    _In_ ULONG dwFlags
    );

WINBASEAPI
BOOL
WINAPI
UnregisterWait(
    _In_ HANDLE WaitHandle
    );

WINBASEAPI
BOOL
WINAPI
UnregisterWaitEx(
    _In_ HANDLE WaitHandle,
    _In_opt_ HANDLE CompletionEvent
    );

WINBASEAPI
BOOL
WINAPI
CreateTimerQueueTimer(
    _Outptr_ PHANDLE phNewTimer,
    _In_opt_ HANDLE TimerQueue,
    _In_ WAITORTIMERCALLBACK Callback,
    _In_opt_ PVOID Parameter,
    _In_ DWORD DueTime,
    _In_ DWORD Period,
    _In_ ULONG Flags
    );

WINBASEAPI
BOOL
WINAPI
DeleteTimerQueueTimer(
    _In_opt_ HANDLE TimerQueue,
    _In_ HANDLE Timer,
    _In_opt_ HANDLE CompletionEvent
    );

EXTERN_C_END

#endif // _THREADPOOLLEGACYAPISET_H_
