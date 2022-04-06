#pragma once

#include <ntifs.h>
#include <wdm.h>
#include <Ldk/ldk.h>
#include "internal.h"
#include "teb.h"
#include "peb.h"
#include "nt/ntldr.h"
#include "nt/zwapi.h"



#define LdkpDefaultPoolType			    NonPagedPool

#define TAG_UNICODE_POOL				'bUdL'
#define TAG_ANSI_POOL					'bAdL'
#define TAG_TMP_POOL                    'bTdL'
#define TAG_DLL_POOL                    'lDdL'
#define TAG_HEAP_POOL                   'pHdL'



typedef struct _LDK_FUNCTION_REGISTRATION {
	PSTR Name;
	PVOID Address;
} LDK_FUNCTION_REGISTRATION, * PLDK_FUNCTION_REGISTRATION;

#define LDK_MODULE_HAS_UNREGISTRABLE(m)		(! (m)->FunctionTable)

typedef struct _LDK_MODULE {
	ANSI_STRING ModuleName;
	ANSI_STRING FullPathName;
	PLDK_FUNCTION_REGISTRATION FunctionTable;
	PVOID Base;
	ULONG Size;
	LIST_ENTRY ActiveLinks;
} LDK_MODULE, * PLDK_MODULE;



_Must_inspect_result_
NTSTATUS
NTAPI
LdkAnsiStringToUnicodeString (
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_) PUNICODE_STRING DestinationString,
    _In_ PCANSI_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

_When_(AllocateDestinationString,
       _At_(DestinationString->MaximumLength,
            _Out_range_(<=, (SourceString->MaximumLength / sizeof(WCHAR)))))
_When_(!AllocateDestinationString,
       _At_(DestinationString->Buffer, _Const_)
       _At_(DestinationString->MaximumLength, _Const_))
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSTATUS
NTAPI
LdkUnicodeStringToAnsiString (
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_) PANSI_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

_Must_inspect_result_
_At_(String->Length, _Out_range_(==, 0))
_At_(String->MaximumLength, _In_) 
_At_(String->Buffer, _Pre_maybenull_ _Post_notnull_ _Post_writable_byte_size_(String->MaximumLength))
NTSTATUS
LdkAllocateUnicodeString (
    _Inout_ _At_(String->Buffer, __drv_allocatesMem(Mem))
		PUNICODE_STRING String
    );

_Must_inspect_result_
NTSTATUS
LdkDuplicateUnicodeString (
    _In_ PCUNICODE_STRING StringIn,
    _Out_ _At_(StringOut->Buffer, __drv_allocatesMem(Mem))
		PUNICODE_STRING StringOut
    );

VOID
NTAPI
LdkFreeUnicodeString (
    _Inout_ _At_(UnicodeString->Buffer, _Frees_ptr_opt_) PUNICODE_STRING UnicodeString
    );

_Must_inspect_result_
_At_(String->Length, _Out_range_(==, 0))
_At_(String->MaximumLength, _In_) 
_At_(String->Buffer, _Pre_maybenull_ _Post_notnull_ _Post_writable_byte_size_(String->MaximumLength))
NTSTATUS
LdkAllocateAnsiString (
    _Inout_ _At_(String->Buffer, __drv_allocatesMem(Mem))
		PANSI_STRING String
    );

_Must_inspect_result_
NTSTATUS
LdkDuplicateAnsiString (
    _In_ PCANSI_STRING StringIn,
    _Out_ _At_(StringOut->Buffer, __drv_allocatesMem(Mem))
		PANSI_STRING StringOut
    );

VOID
NTAPI
LdkFreeAnsiString (
    _Inout_ _At_(AnsiString->Buffer, _Frees_ptr_opt_) PANSI_STRING AnsiString
    );


NTSTATUS
LdkGetDllHandle (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

NTSTATUS
LdkLoadDll (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
	);

NTSTATUS
LdkUnloadDll (
	_In_ PVOID DllHandle
	);

NTSTATUS
LdkGetProcedureAddress (
    _In_ PVOID DllHandle,
    _In_opt_ PANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress
    );




NTSTATUS
LdkGetModuleByBase (
	_In_ PVOID Base,
	_Out_ PLDK_MODULE *Module
	);

NTSTATUS
LdkGetModuleByAddress (
	_In_ PVOID Address,
	_Out_ PLDK_MODULE *Module
	);

NTSTATUS
LdkGetModuleByName (
	_In_ PCSZ ModuleName,
	_Out_ PLDK_MODULE *Module
	);



PVOID
LdkGetRoutineAddress (
	_In_ PLDK_MODULE Module,
	_In_ PCSZ FunctionName,
    _In_opt_ ULONG FunctionNUmber
	);



NTSTATUS
LdkQueryModuleInformationFromAddress (
	_In_ PVOID Address,
	_Out_ PRTL_PROCESS_MODULE_INFORMATION ModuleInformation
	);