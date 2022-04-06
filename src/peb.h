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

	ERESOURCE ModuleListResource;
	LIST_ENTRY ModuleListHead;
	NPAGED_LOOKASIDE_LIST ModuleListLookaside;

} LDK_PEB, *PLDK_PEB;

//ERESOURCE LdkpModuleListResource;
//LIST_ENTRY LdkpModuleListHead = { NULL };
//NPAGED_LOOKASIDE_LIST LdkpModuleListLookaside;




PLDK_PEB
LdkCurrentPeb (
	VOID
	);

NTSTATUS
LdkInitializePeb (
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
	);

VOID
LdkTerminatePeb (
	VOID
	);
