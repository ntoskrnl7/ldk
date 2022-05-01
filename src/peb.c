#include "ldk.h"
#include "peb.h"
#include "nt/ntexapi.h"
#include "nt/ntldr.h"

BOOLEAN LdkPebInitialized = FALSE;

LDK_PEB CurrentPeb;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializePeb)
#pragma alloc_text(PAGE, LdkpTerminatePeb)
#endif



PLDK_PEB
LdkCurrentPeb (
	VOID
	)
{
	return &CurrentPeb;
}

NTSTATUS
LdkpInitializePeb (
	_In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
	)
{
	NTSTATUS status;

	PAGED_CODE();

	CurrentPeb.DriverObject = DriverObject;
	status = LdkDuplicateUnicodeString(RegistryPath, &CurrentPeb.RegistryPath);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	RTL_PROCESS_MODULE_INFORMATION moduleInfo;
	status = LdkQueryModuleInformationFromAddress(&CurrentPeb, &moduleInfo);
	if (!NT_SUCCESS(status)) {
		LdkFreeUnicodeString(&CurrentPeb.RegistryPath);
		return status;
	}
	CurrentPeb.ImageBaseAddress = moduleInfo.ImageBase;
	CurrentPeb.ImageBaseSize = moduleInfo.ImageSize;

	ANSI_STRING ImagePathNameAnsi;
	RtlInitAnsiString(&ImagePathNameAnsi, (PSZ)moduleInfo.FullPathName);
	status = LdkAnsiStringToUnicodeString(&CurrentPeb.ImagePathName, &ImagePathNameAnsi, TRUE);
	if (!NT_SUCCESS(status)) {
		LdkFreeUnicodeString(&CurrentPeb.RegistryPath);
		return status;
	}

	status = LdkDuplicateAnsiString(&ImagePathNameAnsi, &CurrentPeb.FullPathName);
	if (!NT_SUCCESS(status)) {
		LdkFreeUnicodeString(&CurrentPeb.ImagePathName);
		LdkFreeUnicodeString(&CurrentPeb.RegistryPath);
		return status;
	}

	InitializeListHead(&CurrentPeb.ModuleListHead);

	extern LDK_MODULE LdkpKernel32Module;
	InsertTailList(&CurrentPeb.ModuleListHead, &LdkpKernel32Module.ActiveLinks);

	extern LDK_MODULE LdkpNtdllModule;
	InsertTailList(&CurrentPeb.ModuleListHead, &LdkpNtdllModule.ActiveLinks);

	status = ExInitializeResourceLite(&CurrentPeb.ModuleListResource);
	if (!NT_SUCCESS(status)) {
		LdkFreeAnsiString(&CurrentPeb.FullPathName);
		LdkFreeUnicodeString(&CurrentPeb.ImagePathName);
		LdkFreeUnicodeString(&CurrentPeb.RegistryPath);
		return status;
	}

	ExInitializeNPagedLookasideList(&CurrentPeb.ModuleListLookaside, NULL, NULL, POOL_NX_ALLOCATION, sizeof(LDK_MODULE), '1111', 0);

	CurrentPeb.NumberOfProcessors = KeNumberProcessors;

	CurrentPeb.ProcessHeap = NULL;
	
	CurrentPeb.CriticalSectionTimeout.QuadPart = Int32x32To64(2592000, -10000000);

	RtlInitializeBitMap(&CurrentPeb.TlsBitmap, &CurrentPeb.TlsBitmapBits[0], LDK_TLS_SLOTS_SIZE);
	RtlInitializeBitMap(&CurrentPeb.FlsBitmap, &CurrentPeb.FlsBitmapBits[0], LDK_FLS_SLOTS_SIZE);

	LdkPebInitialized = NT_SUCCESS(status);
	return status;
}

VOID
LdkpTerminatePeb (
	VOID
	)
{
	PAGED_CODE();

	if (!LdkPebInitialized) return;

	LdkFreeAnsiString(&CurrentPeb.FullPathName);
	LdkFreeUnicodeString(&CurrentPeb.ImagePathName);
	LdkFreeUnicodeString(&CurrentPeb.RegistryPath);

	ExDeleteResourceLite(&CurrentPeb.ModuleListResource);

	ExDeleteNPagedLookasideList(&CurrentPeb.ModuleListLookaside);

	LdkPebInitialized = FALSE;
}