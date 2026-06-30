#pragma once

#ifndef _PROCESSTHREADSAPI_H_
#define _PROCESSTHREADSAPI_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "minwinbase.h"

EXTERN_C_START

WINBASEAPI
HANDLE
WINAPI
GetCurrentProcess(
    VOID
    );

WINBASEAPI
DWORD
WINAPI
GetCurrentProcessId(
    VOID
    );

WINBASEAPI
DWORD
WINAPI
GetProcessId(
    _In_ HANDLE Process
    );

WINBASEAPI
BOOL
WINAPI
GetProcessAffinityMask(
    _In_ HANDLE hProcess,
    _Out_ PDWORD_PTR lpProcessAffinityMask,
    _Out_ PDWORD_PTR lpSystemAffinityMask
    );

WINBASEAPI
BOOL
WINAPI
SetProcessAffinityMask(
    _In_ HANDLE hProcess,
    _In_ DWORD_PTR dwProcessAffinityMask
    );

#ifndef PROCESS_NAME_NATIVE
#define PROCESS_NAME_NATIVE 0x00000001
#endif

WINBASEAPI
BOOL
WINAPI
QueryFullProcessImageNameA(
    _In_ HANDLE hProcess,
    _In_ DWORD dwFlags,
    _Out_writes_to_(*lpdwSize, *lpdwSize) LPSTR lpExeName,
    _Inout_ PDWORD lpdwSize
    );

WINBASEAPI
BOOL
WINAPI
QueryFullProcessImageNameW(
    _In_ HANDLE hProcess,
    _In_ DWORD dwFlags,
    _Out_writes_to_(*lpdwSize, *lpdwSize) LPWSTR lpExeName,
    _Inout_ PDWORD lpdwSize
    );

WINBASEAPI
HANDLE
WINAPI
OpenProcess(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwProcessId
    );

WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
ExitProcess(
    _In_ UINT uExitCode
    );

WINBASEAPI
BOOL
WINAPI
TerminateProcess(
    _In_ HANDLE hProcess,
    _In_ UINT uExitCode
    );

WINBASEAPI
BOOL
WINAPI
GetExitCodeProcess(
    _In_ HANDLE hProcess,
    _Out_ LPDWORD lpExitCode
    );

WINBASEAPI
BOOL
WINAPI
SwitchToThread(
    VOID
    );

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateThread(
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ SIZE_T dwStackSize,
    _In_ LPTHREAD_START_ROUTINE lpStartAddress,
    _In_opt_ __drv_aliasesMem LPVOID lpParameter,
    _In_ DWORD dwCreationFlags,
    _Out_opt_ LPDWORD lpThreadId
    );



WINBASEAPI
HANDLE
WINAPI
GetCurrentThread(
    VOID
    );

WINBASEAPI
DWORD
WINAPI
GetCurrentThreadId(
    VOID
    );

WINBASEAPI
DWORD
WINAPI
GetThreadId(
    _In_ HANDLE Thread
    );

WINBASEAPI
BOOL
WINAPI
GetThreadGroupAffinity(
    _In_ HANDLE hThread,
    _Out_ PGROUP_AFFINITY GroupAffinity
    );

WINBASEAPI
BOOL
WINAPI
SetThreadGroupAffinity(
    _In_ HANDLE hThread,
    _In_ CONST GROUP_AFFINITY *GroupAffinity,
    _Out_opt_ PGROUP_AFFINITY PreviousGroupAffinity
    );

WINBASEAPI
DWORD
WINAPI
GetCurrentProcessorNumber(
    VOID
    );

WINBASEAPI
VOID
WINAPI
GetCurrentProcessorNumberEx(
    _Out_ PPROCESSOR_NUMBER ProcNumber
    );

WINBASEAPI
HRESULT
WINAPI
SetThreadDescription(
    _In_ HANDLE hThread,
    _In_ PCWSTR lpThreadDescription
    );

WINBASEAPI
HRESULT
WINAPI
GetThreadDescription(
    _In_ HANDLE hThread,
    _Outptr_result_z_ PWSTR *ppszThreadDescription
    );

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenThread(
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwThreadId
    );

WINBASEAPI
DWORD
WINAPI
SuspendThread(
    _In_ HANDLE hThread
    );

WINBASEAPI
DWORD
WINAPI
ResumeThread(
    _In_ HANDLE hThread
    );

WINBASEAPI
int
WINAPI
GetThreadPriority(
    _In_ HANDLE hThread
    );

WINBASEAPI
BOOL
WINAPI
SetThreadPriority(
    _In_ HANDLE hThread,
    _In_ int nPriority
    );

WINBASEAPI
BOOL
WINAPI
GetThreadPriorityBoost(
    _In_ HANDLE hThread,
    _Out_ PBOOL pDisablePriorityBoost
    );

WINBASEAPI
BOOL
WINAPI
SetThreadPriorityBoost(
    _In_ HANDLE hThread,
    _In_ BOOL bDisablePriorityBoost
    );

WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
ExitThread(
    _In_ DWORD dwExitCode
    );


WINBASEAPI
_Success_(return != 0)
BOOL
WINAPI
GetExitCodeThread(
    _In_ HANDLE hThread,
    _Out_ LPDWORD lpExitCode
    );

WINBASEAPI
BOOL
WINAPI
GetThreadTimes(
    _In_ HANDLE hThread,
    _Out_ LPFILETIME lpCreationTime,
    _Out_ LPFILETIME lpExitTime,
    _Out_ LPFILETIME lpKernelTime,
    _Out_ LPFILETIME lpUserTime
    );

WINBASEAPI
BOOL
WINAPI
GetProcessTimes(
    _In_ HANDLE hProcess,
    _Out_ LPFILETIME lpCreationTime,
    _Out_ LPFILETIME lpExitTime,
    _Out_ LPFILETIME lpKernelTime,
    _Out_ LPFILETIME lpUserTime
    );

WINBASEAPI
VOID
WINAPI
GetCurrentThreadStackLimits(
    _Out_ PULONG_PTR LowLimit,
    _Out_ PULONG_PTR HighLimit
    );


#ifndef TLS_OUT_OF_INDEXES
#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#endif

WINBASEAPI
DWORD
WINAPI
TlsAlloc(
    VOID
    );

WINBASEAPI
LPVOID
WINAPI
TlsGetValue(
    _In_ DWORD dwTlsIndex
    );

WINBASEAPI
BOOL
WINAPI
TlsSetValue(
    _In_ DWORD dwTlsIndex,
    _In_opt_ LPVOID lpTlsValue
    );

WINBASEAPI
BOOL
WINAPI
TlsFree(
    _In_ DWORD dwTlsIndex
    );




WINBASEAPI
BOOL
WINAPI
IsProcessorFeaturePresent(
	_In_ DWORD ProcessorFeature
	);



typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

typedef struct _STARTUPINFOA {
    DWORD   cb;
    LPSTR   lpReserved;
    LPSTR   lpDesktop;
    LPSTR   lpTitle;
    DWORD   dwX;
    DWORD   dwY;
    DWORD   dwXSize;
    DWORD   dwYSize;
    DWORD   dwXCountChars;
    DWORD   dwYCountChars;
    DWORD   dwFillAttribute;
    DWORD   dwFlags;
    WORD    wShowWindow;
    WORD    cbReserved2;
    LPBYTE  lpReserved2;
    HANDLE  hStdInput;
    HANDLE  hStdOutput;
    HANDLE  hStdError;
} STARTUPINFOA, *LPSTARTUPINFOA;
typedef struct _STARTUPINFOW {
    DWORD   cb;
    LPWSTR  lpReserved;
    LPWSTR  lpDesktop;
    LPWSTR  lpTitle;
    DWORD   dwX;
    DWORD   dwY;
    DWORD   dwXSize;
    DWORD   dwYSize;
    DWORD   dwXCountChars;
    DWORD   dwYCountChars;
    DWORD   dwFillAttribute;
    DWORD   dwFlags;
    WORD    wShowWindow;
    WORD    cbReserved2;
    LPBYTE  lpReserved2;
    HANDLE  hStdInput;
    HANDLE  hStdOutput;
    HANDLE  hStdError;
} STARTUPINFOW, *LPSTARTUPINFOW;

WINBASEAPI
VOID
WINAPI
GetStartupInfoA (
    _Out_ LPSTARTUPINFOA lpStartupInfo
    );

WINBASEAPI
VOID
WINAPI
GetStartupInfoW (
    _Out_ LPSTARTUPINFOW lpStartupInfo
    );

EXTERN_C_END

#endif // _PROCESSTHREADSAPI_H_
