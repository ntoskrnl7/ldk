#include "ldk.h"
#include "peb.h"
#include "teb.h"



//
// ntdll/ldr.c
//

extern BOOLEAN LdrpInLdrInit;
extern BOOLEAN LdrpShutdownInProgress;
extern HANDLE LdrpShutdownThreadId;



ULONG LdkGlobalFlags = 0;



NTSTATUS
NtdllInitialize (
    VOID
    );

VOID
NtdllTerminate (
    VOID
    );

NTSTATUS
Kernel32Initialize (
    VOID
    );

VOID
Kernel32Terminate (
    VOID
    );



NTSTATUS
LdkInitialize (
	_In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath,
	_In_ ULONG Flags
	)
{
	LdkLockGlobalFlags();

	if (FlagOn(Flags, LDK_FLAG_SAFE_MODE)) {
		SetFlag(LdkGlobalFlags, LDK_FLAG_SAFE_MODE);
	}

	if (LDK_IS_INITIALIZED) {
		LdkUnlockGlobalFlags();
		return STATUS_ALREADY_COMPLETE;
	}

	LdrpInLdrInit = TRUE;

	NTSTATUS status = LdkInitializePeb(DriverObject, RegistryPath);
	if (!NT_SUCCESS(status)) {
		goto Cleanup;
	}

	status = LdkInitializeTebMap();
	if (!NT_SUCCESS(status)) {
		LdkTerminatePeb();
		goto Cleanup;
	}

	status = NtdllInitialize();
	if (!NT_SUCCESS(status)) {
		LdkTerminateTebMap();
		LdkTerminatePeb();
		goto Cleanup;
	}

	status = Kernel32Initialize();
	if (!NT_SUCCESS(status)) {
		NtdllTerminate();
		LdkTerminateTebMap();
		LdkTerminatePeb();
		goto Cleanup;
	}

	if (NT_SUCCESS(status)) {
		SetFlag(LdkGlobalFlags, LDK_FLAG_INITIALIZED);
	}

Cleanup:
	LdrpInLdrInit = FALSE;
	LdkUnlockGlobalFlags();
	return status;
}

VOID
LdkTerminate (
	VOID
	)
{
	LdkLockGlobalFlags();

	if (! LDK_IS_INITIALIZED) {
		LdkUnlockGlobalFlags();
		return;
	}

	LdrpShutdownInProgress = TRUE;
	LdrpShutdownThreadId = PsGetCurrentThreadId();

	Kernel32Terminate();
	NtdllTerminate();

	LdkTerminatePeb();
	LdkTerminateTebMap();

	ClearFlag(LdkGlobalFlags, LDK_FLAG_INITIALIZED);

	LdkUnlockGlobalFlags();
}



FORCEINLINE
VOID
LdkpAcquireModuleListExclusive (
	VOID
	)
{
	KeEnterCriticalRegion();
	ExAcquireResourceExclusiveLite(&LdkCurrentPeb()->ModuleListResource, TRUE);
}

FORCEINLINE
VOID
LdkpAcquireModuleListShared (
	VOID
	)
{
	KeEnterCriticalRegion();
	ExAcquireResourceSharedLite(&LdkCurrentPeb()->ModuleListResource, TRUE);
}

FORCEINLINE
VOID
LdkpReleaseModuleList (
	VOID
	)
{
	ExReleaseResourceLite(&LdkCurrentPeb()->ModuleListResource);
	KeLeaveCriticalRegion();
}

NTSTATUS
LdkRegisterModule (
	_In_ PUNICODE_STRING ModuleName,
	_In_ PUNICODE_STRING FullPathName,
	_In_ PVOID ImageBase,
	_In_ ULONG ImageSize,
	_Out_opt_ PLDK_MODULE *Module
	)
{
	NTSTATUS Status;
	PLDK_MODULE module;

	module = ExAllocateFromNPagedLookasideList(&LdkCurrentPeb()->ModuleListLookaside);

	if (!module) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	
	Status = LdkUnicodeStringToAnsiString(&module->ModuleName, ModuleName, TRUE);
	if (!NT_SUCCESS(Status)) {
		ExFreeToNPagedLookasideList(&LdkCurrentPeb()->ModuleListLookaside, module);
		return Status;
	}

	Status = LdkUnicodeStringToAnsiString(&module->FullPathName, FullPathName, TRUE);
	if (!NT_SUCCESS(Status)) {
		LdkFreeAnsiString(&module->ModuleName);
		ExFreeToNPagedLookasideList(&LdkCurrentPeb()->ModuleListLookaside, module);
		return Status;
	}

	module->Base = ImageBase;
	module->Size = ImageSize;
	module->FunctionTable = NULL;
	
	LdkpAcquireModuleListExclusive();
	InsertTailList(&LdkCurrentPeb()->ModuleListHead, &module->ActiveLinks);
	LdkpReleaseModuleList();

	if (ARGUMENT_PRESENT(Module)) {
		*Module = module;
	}

	return STATUS_SUCCESS;
}

VOID
LdkpUnregisterModule (
	_In_ PLDK_MODULE Module
	)
{
	if (LDK_MODULE_HAS_UNREGISTRABLE(Module)) {
		LdkFreeAnsiString(&Module->ModuleName);
		LdkFreeAnsiString(&Module->FullPathName);
		RemoveEntryList(&Module->ActiveLinks);
	}
}



#include "kernel32/winbase.h"

#include <ntimage.h>
#include "nt/ntrtl.h"

NTSTATUS
LdkpGetModuleByBase (
	_In_ PVOID Base,
	_Out_ PLDK_MODULE *Module
	)
{
	NTSTATUS Status;
	PLDK_MODULE module = NULL;
	PLIST_ENTRY NextEntry;

	if (! ARGUMENT_PRESENT(Base)) {
		return STATUS_INVALID_PARAMETER;
	}

	Status = STATUS_NOT_FOUND;
	
	for (NextEntry = LdkCurrentPeb()->ModuleListHead.Flink;
		 NextEntry != &LdkCurrentPeb()->ModuleListHead;
		 NextEntry = NextEntry->Flink) {
		
		module = CONTAINING_RECORD(NextEntry, LDK_MODULE, ActiveLinks);
		
		if (module->Base == Base) {
			*Module = module;
			Status = STATUS_SUCCESS;
			break;
		}

	}

	return Status;
}

NTSTATUS
LdkpGetModuleByName (
	_In_ PCSZ ModuleName,
	_Out_ PLDK_MODULE *Module
	)
{
	NTSTATUS Status;
	PLDK_MODULE module = NULL;
	PLIST_ENTRY NextEntry;

	Status = STATUS_NOT_FOUND;
	
	for (NextEntry = LdkCurrentPeb()->ModuleListHead.Flink;
		 NextEntry != &LdkCurrentPeb()->ModuleListHead;
		 NextEntry = NextEntry->Flink) {
		
		module = CONTAINING_RECORD(NextEntry, LDK_MODULE, ActiveLinks);
		
		if (_stricmp(module->ModuleName.Buffer, ModuleName) == 0) {
			*Module = module;
			Status = STATUS_SUCCESS;
			break;
		}

	}

	return Status;
}

BOOLEAN
LdkpIsValidDll (
	_In_ PVOID DllBase
	)
{
	return RtlImageNtHeader(DllBase) != NULL;
}



USHORT
LdkpNameToOrdinal(
    _In_ PSZ NameOfEntryPoint,
    _In_ PVOID DllBase,
    _In_ ULONG NumberOfNames,
    _In_ PULONG NameTableBase,
    _In_ PUSHORT NameOrdinalTableBase
    )
{
    ULONG SplitIndex;
    LONG CompareResult;

    if (NumberOfNames == 0) {
        return (USHORT)-1;
    }

    SplitIndex = NumberOfNames >> 1;

    CompareResult = strcmp(NameOfEntryPoint, (PSZ)(Add2Ptr(DllBase, NameTableBase[SplitIndex])));
    if (CompareResult == 0) {
        return NameOrdinalTableBase[SplitIndex];
    }

    if (NumberOfNames == 1) {
        return (USHORT)-1;
    }

    if (CompareResult < 0) {
        NumberOfNames = SplitIndex;
    } else {
        NameTableBase = &NameTableBase[SplitIndex + 1];
        NameOrdinalTableBase = &NameOrdinalTableBase[SplitIndex + 1];
        NumberOfNames = NumberOfNames - SplitIndex - 1;
    }

    return LdkpNameToOrdinal(NameOfEntryPoint, DllBase, NumberOfNames, NameTableBase, NameOrdinalTableBase);
}



NTSTATUS
LdkpBuildImportDescriptor(
	_In_ PVOID ImageBase
	)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	ULONG ImportSize;
	PIMAGE_IMPORT_DESCRIPTOR ImportDescriptor = RtlImageDirectoryEntryToData(ImageBase, TRUE, IMAGE_DIRECTORY_ENTRY_IMPORT, &ImportSize);
	PIMAGE_THUNK_DATA OriginalFirstThunk;
	PIMAGE_THUNK_DATA FirstThunk;
	PIMAGE_IMPORT_BY_NAME ImportByName;
	PLDK_MODULE Module;

	if (!ImportDescriptor) {
		return STATUS_SUCCESS;
	}

	for (; ImportDescriptor->Characteristics; ImportDescriptor++) {

		Status = LdkGetModuleByName((LPSTR)Add2Ptr(ImageBase, ImportDescriptor->Name), &Module);
		if (!NT_SUCCESS(Status)) {
			HMODULE hModule = LoadLibraryA((LPSTR)Add2Ptr(ImageBase, ImportDescriptor->Name));
			if (hModule) {
				Status = LdkGetModuleByName((LPSTR)Add2Ptr(ImageBase, ImportDescriptor->Name), &Module);
			}
		}

		if (!NT_SUCCESS(Status)) {
			KdBreakPoint();
			return Status;
		}

		OriginalFirstThunk = (PIMAGE_THUNK_DATA)Add2Ptr(ImageBase, ImportDescriptor->OriginalFirstThunk);
		FirstThunk = (PIMAGE_THUNK_DATA)Add2Ptr(ImageBase, ImportDescriptor->FirstThunk);

		for (; OriginalFirstThunk->u1.AddressOfData; ++OriginalFirstThunk, ++FirstThunk) {

			if (IMAGE_SNAP_BY_ORDINAL(OriginalFirstThunk->u1.Ordinal)) {

				KdBreakPoint();
				return STATUS_NOT_SUPPORTED;

			} else {
				ImportByName = (PIMAGE_IMPORT_BY_NAME)Add2Ptr(ImageBase, OriginalFirstThunk->u1.AddressOfData);
#if _WIN64
				FirstThunk->u1.Function = (ULONGLONG)LdkGetRoutineAddress(Module, ImportByName->Name, 0);
#else
				FirstThunk->u1.Function = (ULONG)LdkGetRoutineAddress(Module, ImportByName->Name, 0);
#endif
				if (!FirstThunk->u1.Function) {
					KdBreakPoint();
					return STATUS_PROCEDURE_NOT_FOUND;
				}
			}
		}
	}
	return STATUS_SUCCESS;
}

NTSTATUS
LdkpRelocateImage (
	__in PVOID ImageBase
	)
{
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_BASE_RELOCATION NextBlock;
	ULONG_PTR Diff;
	ULONG TotalCount;
	ULONG_PTR *FixupVA;
	PUSHORT NextOffset;
	USHORT Offset;

	NtHeader = RtlImageNtHeader( ImageBase );

	NextBlock = (PIMAGE_BASE_RELOCATION)(Add2Ptr(ImageBase, NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress));
		
	Diff = (ULONG_PTR)((LPBYTE)ImageBase - NtHeader->OptionalHeader.ImageBase);

	while (NextBlock->VirtualAddress) {

		if (NextBlock->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION)) {

			TotalCount = (NextBlock->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(USHORT);
			NextOffset = (PUSHORT)(NextBlock + 1);

			for (ULONG i = 0; i<TotalCount; i++) {
				if (NextOffset[i]) {
					Offset = *NextOffset & (USHORT)0xfff;
					FixupVA = (ULONG_PTR *)(Add2Ptr(ImageBase, (NextBlock->VirtualAddress + (NextOffset[i] & 0xFFF))));
					*FixupVA += Diff;
				}
			}
		}

		NextBlock = (PIMAGE_BASE_RELOCATION)(Add2Ptr(NextBlock, NextBlock->SizeOfBlock));
	}

	return STATUS_SUCCESS;
}

typedef BOOL(WINAPI* PDLL_MAIN)(HINSTANCE, DWORD, PVOID);

typedef struct _LDK_DLL_ENTRY_POINIT_CONTEXT {
	PDLL_MAIN EntryPoint;
	HINSTANCE Base;
	DWORD Reason;
	PVOID Reserved;
	BOOL Result;
} LDK_DLL_ENTRY_POINIT_CONTEXT, *PLDK_DLL_ENTRY_POINIT_CONTEXT;

_IRQL_requires_same_
_Function_class_(EXPAND_STACK_CALLOUT)
VOID
NTAPI
LdkpCallEntryPointExpandStackCallout (
    _In_ PLDK_DLL_ENTRY_POINIT_CONTEXT Context
    )
{
	 Context->Result = Context->EntryPoint( Context->Base,
	 										Context->Reason,
											Context->Reserved);
}

BOOL
LdkpCallEntryPoint (
	_In_ HINSTANCE Base,
	_In_ DWORD Reason,
	_In_ PVOID Reserved
	)
{
	PIMAGE_NT_HEADERS NtHeader = RtlImageNtHeader(Base);
	if (! NtHeader->OptionalHeader.AddressOfEntryPoint) {
		return FALSE;
	}
	LDK_DLL_ENTRY_POINIT_CONTEXT Context;
	Context.EntryPoint = (PDLL_MAIN)((ULONG_PTR)Add2Ptr(Base, NtHeader->OptionalHeader.AddressOfEntryPoint));
	Context.Base = Base;
	Context.Reason = Reason;
	Context.Reserved = Reserved;
	Context.Result = FALSE;
	return NT_SUCCESS(KeExpandKernelStackAndCalloutEx( LdkpCallEntryPointExpandStackCallout,
													   &Context,
													   MAXIMUM_EXPANSION_SIZE,
													   TRUE,
													   NULL )) ?
			Context.Result :
			Context.EntryPoint(Base, Reason, Reserved);
}

NTSTATUS
LdkpLoadDll (
	_In_ PUNICODE_STRING FileName,
	_Outptr_ PVOID *ImageBase,
	_Out_opt_ PULONG ImageSize
	)
{
	NTSTATUS Status = STATUS_UNSUCCESSFUL;
	HANDLE FileHandle = NULL;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatus;
	
	FILE_STANDARD_INFORMATION StandardInfo;
	LARGE_INTEGER ByteOffset;

	PVOID Buffer = NULL;
	PVOID Base = NULL;

	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;

	PAGED_CODE();

	InitializeObjectAttributes( &ObjectAttributes, FileName, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL );
	
	Status = ZwOpenFile( &FileHandle,
						 FILE_GENERIC_READ,
						 &ObjectAttributes,
						 &IoStatus,
						 FILE_SHARE_READ | FILE_SHARE_WRITE,
						 FILE_NON_DIRECTORY_FILE );
	if (!NT_SUCCESS(Status)) {
		return Status;
	}

	try {
		
		Status = ZwQueryInformationFile( FileHandle,
										 &IoStatus,
										 &StandardInfo,
										 sizeof(StandardInfo),
										 FileStandardInformation );
		if (!NT_SUCCESS(Status)) {
			leave;
		}

		Buffer = ExAllocatePoolWithTag( PagedPool,
										StandardInfo.EndOfFile.LowPart,
										TAG_DLL_POOL );
		if (!Buffer) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}

		ByteOffset.LowPart = ByteOffset.HighPart = 0;
		Status = ZwReadFile( FileHandle,
							 NULL,
							 NULL,
							 NULL,
							 &IoStatus,
							 Buffer,
							 StandardInfo.EndOfFile.LowPart,
							 &ByteOffset,
							 NULL );
		if (Status == STATUS_PENDING) {
			Status = ZwWaitForSingleObject( FileHandle,
											FALSE,
											NULL );
			if (NT_SUCCESS(Status)) {
				Status = IoStatus.Status;
			}
		}
		if (!NT_SUCCESS(Status)) {
			leave;
		}

		DosHeader = (PIMAGE_DOS_HEADER)Buffer;
		if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
			Status = STATUS_INVALID_IMAGE_NOT_MZ;
			leave;
		}

		NtHeader = (PIMAGE_NT_HEADERS)(Add2Ptr(Buffer, DosHeader->e_lfanew));
		if (NtHeader->Signature != IMAGE_NT_SIGNATURE) {
			Status = STATUS_INVALID_IMAGE_FORMAT;
			leave;
		}

#if !defined(_X86_)
		if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) {
			Status = STATUS_INVALID_IMAGE_WIN_32;
			leave;
		}
#elif !defined(_AMD64_)
		if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) {
			Status = STATUS_INVALID_IMAGE_WIN_64;
			leave;
		}
#endif

#if defined(_X86_)
		if (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) {
			Status = STATUS_INVALID_IMAGE_FORMAT;
			leave;
		}
#elif defined(_AMD64_)
		if (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64) {
			Status = STATUS_INVALID_IMAGE_FORMAT;
			leave;
		}
#elif defined(_ARM_)
		if (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_ARM) {
			Status = STATUS_INVALID_IMAGE_FORMAT;
			leave;
		}
#elif defined(_ARM64_)
		if (NtHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_ARM64) {
			Status = STATUS_INVALID_IMAGE_FORMAT;
			leave;
		}
#else
#error "Not Supported Target Architecture"
#endif
		if (!(NtHeader->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
			Status = STATUS_INVALID_IMAGE_FORMAT;
			leave;
		}

		Base = ExAllocatePoolWithTag( NonPagedPool,
									  NtHeader->OptionalHeader.SizeOfImage,
									  TAG_DLL_POOL );
		if (! Base) {
			Status = STATUS_INSUFFICIENT_RESOURCES;
			leave;
		}
		
		RtlZeroMemory(Base, NtHeader->OptionalHeader.SizeOfImage);
		RtlCopyMemory(Base, Buffer, NtHeader->OptionalHeader.SizeOfHeaders);
		
		SectionHeader = IMAGE_FIRST_SECTION(NtHeader);

		for (ULONG i = 0; i < NtHeader->FileHeader.NumberOfSections; i++) {
			RtlCopyMemory( Add2Ptr(Base, SectionHeader[i].VirtualAddress),
						   Add2Ptr(Buffer, SectionHeader[i].PointerToRawData),
						   SectionHeader[i].SizeOfRawData );
		}

		Status = LdkpRelocateImage(Base);
		if (!NT_SUCCESS(Status)) {
			leave;
		}

		Status = LdkpBuildImportDescriptor(Base);
		if (!NT_SUCCESS(Status)) {
			leave;
		}

		*ImageBase = Base;
		if (ARGUMENT_PRESENT(ImageSize)) {
			*ImageSize = NtHeader->OptionalHeader.SizeOfImage;
		}
		Status = STATUS_SUCCESS;

	} finally {
		
		if (FileHandle) {
			ZwClose( FileHandle );
		}
		if (Buffer) {
			ExFreePoolWithTag( Buffer,
							   TAG_DLL_POOL );
		}

		if (! NT_SUCCESS(Status)) {
			if (Base) {
				ExFreePoolWithTag( Base,
								   TAG_DLL_POOL );
			}
		}
	}

	return Status;
}

VOID
LdkpUnloadDll (
	_In_ PVOID ImageBase
	)
{
	ExFreePoolWithTag( ImageBase,
					   TAG_DLL_POOL );
}



NTSTATUS
LdkGetModuleByBase (
	_In_ PVOID Base,
	_Out_ PLDK_MODULE *Module
	)
{
	NTSTATUS Status;

	LdkpAcquireModuleListShared();
	Status = LdkpGetModuleByBase(Base, Module);
	LdkpReleaseModuleList();
	return Status;
}

NTSTATUS
LdkGetModuleByAddress (
	_In_ PVOID Address,
	_Out_ PLDK_MODULE *Module
	)
{
	NTSTATUS Status;
	PLDK_MODULE module = NULL;
	PLIST_ENTRY NextEntry;
	PVOID ImageBase;
	ULONG ImageSize;

	LdkpAcquireModuleListShared();
	
	Status = STATUS_NOT_FOUND;
	
	for (NextEntry = LdkCurrentPeb()->ModuleListHead.Flink;
		 NextEntry != &LdkCurrentPeb()->ModuleListHead;
		 NextEntry = NextEntry->Flink) {
		
		module = CONTAINING_RECORD(NextEntry, LDK_MODULE, ActiveLinks);
		ImageBase = module->Base;
		
		if (ImageBase) {

			ImageSize = module->Size;

			if (((ULONG_PTR)ImageBase < (ULONG_PTR)Address &&
				((ULONG_PTR)(Add2Ptr(ImageBase, ImageSize)) >= (ULONG_PTR)Address))) {

				*Module = module;
				Status = STATUS_SUCCESS;
				break;
			}
		}
	}

	LdkpReleaseModuleList();

	return Status;
}

NTSTATUS
LdkGetModuleByName (
	_In_ PCSZ ModuleName,
	_Out_ PLDK_MODULE *Module
	)
{
	NTSTATUS Status;

	LdkpAcquireModuleListShared();
	
	Status = LdkpGetModuleByName(ModuleName, Module);

	LdkpReleaseModuleList();

	return Status;
}



_Ret_maybenull_
PVOID
LdkGetRoutineAddress (
	_In_ PLDK_MODULE Module,
	_In_opt_ PCSZ ProcedureName,
	_In_opt_ ULONG ProcedureNumber
	)
{
	PVOID Address = NULL;
	ANSI_STRING procedureName;
	RtlInitAnsiString(&procedureName, ProcedureName);
	
	if (Module->Base) {
		LdkGetProcedureAddress(Module->Base, &procedureName, ProcedureNumber, &Address);
	} else {
		LdkpAcquireModuleListShared();
		for (PLDK_FUNCTION_REGISTRATION Function = Module->FunctionTable; Function->Name; Function++) {
			if (_stricmp(Function->Name, ProcedureName) == 0) {
				Address = Function->Address;
			}
		}
		LdkpReleaseModuleList();
	}

	return Address;
}



NTSTATUS
LdkGetDllHandle (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    )
{
	UNREFERENCED_PARAMETER(DllPath);
	UNREFERENCED_PARAMETER(DllCharacteristics);

	NTSTATUS status;
	ANSI_STRING dllName;

	status = LdkUnicodeStringToAnsiString(&dllName, DllName, TRUE);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	PLDK_MODULE module;
	status = LdkGetModuleByName(dllName.Buffer, &module);
	LdkFreeAnsiString(&dllName);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	if (module->Base) {
		*DllHandle = module->Base;
	} else {
		*DllHandle = (PVOID)module;
	}

	return STATUS_SUCCESS;
}

NTSTATUS
LdkLoadDll (
	_In_opt_ PWSTR DllPath,
	_In_opt_ PULONG DllCharacteristics,
	_In_ PUNICODE_STRING DllName,
	_Out_ PVOID* DllHandle
	)
{
	UNREFERENCED_PARAMETER(DllPath);
	UNREFERENCED_PARAMETER(DllCharacteristics);

	NTSTATUS status;
	ULONG imageSize;
	status = LdkpLoadDll(DllName, DllHandle, &imageSize);
	if (NT_SUCCESS(status)) {

		UNICODE_STRING moduleName;
		moduleName.Buffer = wcsrchr(DllName->Buffer, L'\\');
		++moduleName.Buffer;
		moduleName.MaximumLength = moduleName.Length = (USHORT)(wcslen(moduleName.Buffer) * sizeof(WCHAR));

		status = LdkRegisterModule(&moduleName, DllName, DllHandle, imageSize, NULL);
		if (!NT_SUCCESS(status)) {
			LdkpUnloadDll(*DllHandle);
			return status;
		}

		LdkpCallEntryPoint(*DllHandle, DLL_PROCESS_ATTACH, NULL);
	}
	return status;
}

NTSTATUS
LdkUnloadDll (
	_In_ PVOID DllHandle
	)
{
	LdkpCallEntryPoint((HINSTANCE)DllHandle, DLL_PROCESS_DETACH, NULL);

	PLDK_MODULE Module;
	LdkpAcquireModuleListExclusive();
	if (NT_SUCCESS(LdkpGetModuleByBase(DllHandle, &Module))) {
		LdkpUnregisterModule(Module);
	}
	LdkpReleaseModuleList();

	LdkpUnloadDll(DllHandle);

	return STATUS_SUCCESS;
}

NTSTATUS
LdkGetProcedureAddress(
    _In_ PVOID DllHandle,
    _In_opt_ PANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress
    )
{
	if (!ARGUMENT_PRESENT(DllHandle)) {
		KdBreakPoint();
		DllHandle = NtCurrentPeb()->ImageBaseAddress;
	}

	if (!LdkpIsValidDll(DllHandle)) {
		if (!ARGUMENT_PRESENT(ProcedureAddress)) {
			return STATUS_INVALID_PARAMETER;
		}
		LdkpAcquireModuleListShared();
		for (PLDK_FUNCTION_REGISTRATION Function = ((PLDK_MODULE)DllHandle)->FunctionTable; Function->Name; Function++) {
			if (_stricmp(Function->Name, ProcedureName->Buffer) == 0) {
				*ProcedureAddress = Function->Address;
				LdkpReleaseModuleList();
				return STATUS_SUCCESS;
			}
		}
		LdkpReleaseModuleList();
		return STATUS_PROCEDURE_NOT_FOUND;
	}

    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    ULONG ExportSize;
    USHORT Ordinal;
    PULONG Addr;
    CHAR NameBuffer[64];

    if (!MmIsAddressValid(DllHandle)) {
        return STATUS_INVALID_PARAMETER;
    }

    ExportDirectory = RtlImageDirectoryEntryToData(DllHandle, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ExportSize);

    if (!ExportDirectory) {
        return STATUS_INVALID_PARAMETER;
    }

    if (ARGUMENT_PRESENT(ProcedureNumber)) {

        ULONG OrdinalBase = ExportDirectory->Base;

        KdBreakPoint();

        Ordinal = IMAGE_ORDINAL((ULONG_PTR)ProcedureNumber);

        if (Ordinal < OrdinalBase || Ordinal >= OrdinalBase + ExportDirectory->NumberOfFunctions) {
            return STATUS_PROCEDURE_NOT_FOUND;
        }

        Ordinal -= (USHORT)OrdinalBase;

    } else if (ARGUMENT_PRESENT(ProcedureName)) {

        if (ProcedureName->Length > sizeof(NameBuffer) - 2) {
            return STATUS_INVALID_PARAMETER;
        }

        strcpy(NameBuffer, ProcedureName->Buffer);

        Ordinal = LdkpNameToOrdinal(
            NameBuffer,
			DllHandle,
            ExportDirectory->NumberOfNames,
            Add2Ptr(DllHandle, ExportDirectory->AddressOfNames),
            Add2Ptr(DllHandle, ExportDirectory->AddressOfNameOrdinals)
        );

        if ((ULONG)Ordinal >= ExportDirectory->NumberOfFunctions) {
            return STATUS_PROCEDURE_NOT_FOUND;
        }

    } else {

        return STATUS_INVALID_PARAMETER;
    }

    Addr = Add2Ptr(DllHandle, ExportDirectory->AddressOfFunctions);
    *ProcedureAddress = Add2Ptr(DllHandle, Addr[Ordinal]);
    return STATUS_SUCCESS;
}