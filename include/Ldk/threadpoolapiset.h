#pragma once

#ifndef _THREADPOOLAPISET_H_
#define _THREADPOOLAPISET_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "minwinbase.h"

EXTERN_C_START

WINBASEAPI
PTP_POOL
WINAPI
CreateThreadpool(
    _Reserved_ PVOID reserved
    );

WINBASEAPI
VOID
WINAPI
SetThreadpoolThreadMaximum(
    _Inout_ PTP_POOL ptpp,
    _In_ DWORD cthrdMost
    );

WINBASEAPI
BOOL
WINAPI
SetThreadpoolThreadMinimum(
    _Inout_ PTP_POOL ptpp,
    _In_ DWORD cthrdMic
    );

WINBASEAPI
BOOL
WINAPI
QueryThreadpoolStackInformation(
    _In_ PTP_POOL ptpp,
    _Out_ PTP_POOL_STACK_INFORMATION ptpsi
    );

WINBASEAPI
BOOL
WINAPI
SetThreadpoolStackInformation(
    _Inout_ PTP_POOL ptpp,
    _In_ PTP_POOL_STACK_INFORMATION ptpsi
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpool(
    _Inout_ PTP_POOL ptpp
    );

WINBASEAPI
PTP_CLEANUP_GROUP
WINAPI
CreateThreadpoolCleanupGroup(
    VOID
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpoolCleanupGroup(
    _Inout_ PTP_CLEANUP_GROUP ptpcg
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpoolCleanupGroupMembers(
    _Inout_ PTP_CLEANUP_GROUP ptpcg,
    _In_ BOOL fCancelPendingCallbacks,
    _Inout_opt_ PVOID pvCleanupContext
    );

WINBASEAPI
_Must_inspect_result_
PTP_WORK
WINAPI
CreateThreadpoolWork(
    _In_ PTP_WORK_CALLBACK pfnwk,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe
    );

WINBASEAPI
VOID
WINAPI
SubmitThreadpoolWork(
    _Inout_ PTP_WORK pwk
    );

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolWorkCallbacks(
    _Inout_ PTP_WORK pwk,
    _In_ BOOL fCancelPendingCallbacks
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWork(
    _Inout_ PTP_WORK pwk
    );

WINBASEAPI
_Must_inspect_result_
PTP_TIMER
WINAPI
CreateThreadpoolTimer(
    _In_ PTP_TIMER_CALLBACK pfnti,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe
    );

WINBASEAPI
VOID
WINAPI
SetThreadpoolTimer(
    _Inout_ PTP_TIMER pti,
    _In_opt_ PFILETIME pftDueTime,
    _In_ DWORD msPeriod,
    _In_opt_ DWORD msWindowLength
    );

WINBASEAPI
BOOL
WINAPI
IsThreadpoolTimerSet(
    _Inout_ PTP_TIMER pti
    );

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolTimerCallbacks(
    _Inout_ PTP_TIMER pti,
    _In_ BOOL fCancelPendingCallbacks
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpoolTimer(
    _Inout_ PTP_TIMER pti
    );

WINBASEAPI
_Must_inspect_result_
PTP_WAIT
WINAPI
CreateThreadpoolWait(
    _In_ PTP_WAIT_CALLBACK pfnwa,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe
    );

WINBASEAPI
VOID
WINAPI
SetThreadpoolWait(
    _Inout_ PTP_WAIT pwa,
    _In_opt_ HANDLE h,
    _In_opt_ PFILETIME pftTimeout
    );

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolWaitCallbacks(
    _Inout_ PTP_WAIT pwa,
    _In_ BOOL fCancelPendingCallbacks
    );

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWait(
    _Inout_ PTP_WAIT pwa
    );

WINBASEAPI
VOID
WINAPI
FreeLibraryWhenCallbackReturns(
    _Inout_ PTP_CALLBACK_INSTANCE pci,
    _In_ HMODULE mod
    );

EXTERN_C_END

#endif _THREADPOOLAPISET_H_
