#pragma once

#include <Ldk/windows.h>

ULONG
LdkSetLastNTError (
	_In_ NTSTATUS Status
	);

PLARGE_INTEGER
LdkFormatTimeout (
	_Out_ PLARGE_INTEGER Timeout,
	_In_ DWORD Milliseconds
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