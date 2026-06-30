#pragma once

#include <Ldk/windows.h>
#include "../internal.h"

ULONG
LdkSetLastNTError (
	_In_ NTSTATUS Status
	);

PLARGE_INTEGER
LdkFormatTimeout (
	_Out_ PLARGE_INTEGER Timeout,
	_In_ DWORD Milliseconds
	);

NTSTATUS
LdkpInitializeProcessHandles (
	VOID
	);

VOID
LdkpTerminateProcessHandles (
	VOID
	);

HANDLE
LdkCreateCurrentProcessHandle (
	_In_ DWORD DesiredAccess,
	_In_ BOOL InheritHandle
	);

BOOL
LdkCloseProcessHandle (
	_In_ HANDLE Handle,
	_Out_ PBOOL Handled
	);

BOOL
LdkDuplicateProcessHandle (
	_In_ HANDLE SourceHandle,
	_Outptr_ LPHANDLE TargetHandle,
	_In_ DWORD DesiredAccess,
	_In_ BOOL InheritHandle,
	_In_ DWORD Options,
	_Out_ PBOOL Handled
	);

BOOL
LdkResolveProcessHandleForNativeDuplicate (
	_In_ HANDLE ProcessHandle,
	_Out_ PHANDLE NativeProcessHandle
	);

BOOL
LdkResolveProcessHandleForNativeQuery (
	_In_ HANDLE ProcessHandle,
	_Out_ PHANDLE NativeProcessHandle
	);

BOOL
LdkGetProcessIdHandle (
	_In_ HANDLE ProcessHandle,
	_Out_ LPDWORD ProcessId,
	_Out_ PBOOL Handled
	);

BOOL
LdkGetProcessImageNameHandle (
	_In_ HANDLE ProcessHandle,
	_In_ DWORD Flags,
	_Out_ PCUNICODE_STRING *ImageName,
	_Out_ PANSI_STRING NativeImageName,
	_Out_ PBOOL Handled
	);

BOOL
LdkGetExitCodeProcessHandle (
	_In_ HANDLE ProcessHandle,
	_Out_ LPDWORD ExitCode,
	_Out_ PBOOL Handled
	);

BOOL
LdkValidateProcessHandleForSetInformation (
	_In_ HANDLE ProcessHandle,
	_Out_ PBOOL Handled
	);

BOOL
LdkTerminateProcessHandle (
	_In_ HANDLE ProcessHandle,
	_In_ UINT ExitCode,
	_Out_ PBOOL Handled
	);

BOOLEAN
LdkIsProcessHandleCandidate (
	_In_ HANDLE Handle
	);

NTSTATUS
LdkReferenceProcessWaitObject (
	_In_ HANDLE Handle,
	_Outptr_ PVOID *Object,
	_Out_ PBOOLEAN IsLdkObject
	);

VOID
LdkDereferenceProcessWaitObject (
	_In_ PVOID Object,
	_In_ BOOLEAN IsLdkObject
	);

VOID
LdkpInitializeWinBaseMessageResources (
	VOID
	);

VOID
LdkpTerminateWinBaseMessageResources (
	VOID
	);

NTSTATUS
LdkpGetCodePageTable (
	_In_ UINT CodePage,
	_Outptr_ PCPTABLEINFO *CodePageTable
	);

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PUNICODE_STRING
LdkAnsiStringToStaticUnicodeString (
    _In_ LPCSTR SourceString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PUNICODE_STRING
Ldk8BitStringToStaticUnicodeString (
    _In_ PCSTR String
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOL
NTAPI
Ldk8BitStringToDynamicUnicodeString (
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_) PUNICODE_STRING DestinationString,
    _In_ PCSTR SourceString
    );

extern
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
(*Ldk8BitStringToUnicodeString)(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
		PUNICODE_STRING DestinationString,
    _In_ PSTRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

extern
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
(*Ldk8BitStringToUnicodeSize)(
    _In_ PSTRING AnsiString
    );

extern
_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
(*LdkUnicodeStringTo8BitString)(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
    	PSTRING DestinationString,
    _In_ PUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

extern
_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
(*LdkUnicodeStringTo8BitSize)(
    _In_ PUNICODE_STRING UnicodeString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
(*LdkFree8BitString)(
	_Inout_ _At_(String->Buffer, _Frees_ptr_opt_)
    _In_ PSTRING String
    );
