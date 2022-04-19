#pragma once

#include "ntdll/ntdll.h"
#include "nt/ntpsapi.h"

typedef struct _LDK_PEB {

	PVOID ImageBaseAddress;
	SIZE_T ImageBaseSize;
	UNICODE_STRING ImagePathName;
	ANSI_STRING FullPathName;

	PVOID ProcessHeap;
	ULONG NumberOfProcessors;

	LARGE_INTEGER CriticalSectionTimeout;

	PVOID Environment;
	
	PDRIVER_OBJECT DriverObject;
	UNICODE_STRING RegistryPath;

	RTL_BITMAP TlsBitmap;
	ULONG TlsBitmapBits[32];

	RTL_BITMAP FlsBitmap;
	ULONG FlsBitmapBits[32];

	ERESOURCE ModuleListResource;
	LIST_ENTRY ModuleListHead;
	NPAGED_LOOKASIDE_LIST ModuleListLookaside;

} LDK_PEB, *PLDK_PEB;



PLDK_PEB
LdkCurrentPeb (
	VOID
	);

NTSTATUS
LdkpInitializePeb (
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
	);

VOID
LdkpTerminatePeb (
	VOID
	);