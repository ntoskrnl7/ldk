#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"
#include <ntimage.h>



#define LDK_LOAD_LIBRARY_RESOURCE_FLAGS \
	(LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | LOAD_LIBRARY_AS_IMAGE_RESOURCE)

#define LDK_LOAD_LIBRARY_SEARCH_FLAGS \
	(LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_APPLICATION_DIR | \
	 LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 | \
	 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER)

#define LDK_LOAD_LIBRARY_SUPPORTED_FLAGS \
	(DONT_RESOLVE_DLL_REFERENCES | LOAD_LIBRARY_AS_DATAFILE | \
	 LOAD_WITH_ALTERED_SEARCH_PATH | LOAD_IGNORE_CODE_AUTHZ_LEVEL | \
	 LOAD_LIBRARY_AS_IMAGE_RESOURCE | LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE | \
	 LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_APPLICATION_DIR | \
	 LOAD_LIBRARY_SEARCH_USER_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32 | \
	 LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER)

#define LDK_RESOURCE_MODULE_HANDLE_TAG ((ULONG_PTR)1)

NTSTATUS
LdkpComputeDllPath (
	_In_ LPCWSTR lpLibFileName,
	_In_ DWORD dwFlags,
	_Outptr_ LPWSTR *DllPath
	);

NTSTATUS
LdkpDuplicateDllDirectory (
	_Out_ PUNICODE_STRING DllDirectory
	);



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdkpComputeDllPath)
#pragma alloc_text(PAGE, LdkpDuplicateDllDirectory)
#pragma alloc_text(PAGE, GetModuleFileNameA)
#pragma alloc_text(PAGE, GetModuleFileNameW)
#pragma alloc_text(PAGE, GetModuleHandleA)
#pragma alloc_text(PAGE, GetModuleHandleW)
#pragma alloc_text(PAGE, GetModuleHandleExA)
#pragma alloc_text(PAGE, GetModuleHandleExW)
#pragma alloc_text(PAGE, GetProcAddress)
#pragma alloc_text(PAGE, LoadLibraryA)
#pragma alloc_text(PAGE, LoadLibraryW)
#pragma alloc_text(PAGE, LoadLibraryExA)
#pragma alloc_text(PAGE, LoadLibraryExW)
#pragma alloc_text(PAGE, FreeLibrary)
#pragma alloc_text(PAGE, FreeLibraryAndExitThread)
#endif



PVOID
LdkpMapModuleHandle (
    _In_opt_ HMODULE hModule,
    _In_ BOOLEAN bResourcesOnly
    )
{
	if (ARGUMENT_PRESENT(hModule)) {
		if ((ULONG_PTR)hModule & LDK_RESOURCE_MODULE_HANDLE_TAG) {
			return (bResourcesOnly) ? (PVOID)((ULONG_PTR)hModule & ~LDK_RESOURCE_MODULE_HANDLE_TAG) : NULL;
		} else {
			return (PVOID)hModule;
		}
	} else {
		return (PVOID)NtCurrentPeb()->ImageBaseAddress;
	}
}



WINBASEAPI
_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
GetModuleFileNameA (
    _In_opt_ HMODULE hModule,
    _Out_writes_to_(nSize, ((return < nSize) ? (return + 1) : nSize)) LPSTR lpFilename,
    _In_ DWORD nSize
    )
{
	PSTR FileName = NULL;
	DWORD FileNameLength = 0;

	PAGED_CODE();

	if (ARGUMENT_PRESENT(hModule)) {
		PLDK_MODULE Module;
		PVOID ModuleBase = LdkpMapModuleHandle( hModule,
												TRUE );
		if (ModuleBase &&
			NT_SUCCESS(LdkGetModuleByBase( ModuleBase,
										   &Module ))) {
			FileName = Module->FullPathName.Buffer;
			FileNameLength = Module->FullPathName.Length;
		}
	} else {
		FileName = (PSTR)NtCurrentPeb()->FullPathName.Buffer;
		FileNameLength = (DWORD)NtCurrentPeb()->FullPathName.Length;
	}

	if (FileNameLength == 0) {
		SetLastError( ERROR_NOT_FOUND );
		return 0;
	}

	if (FileNameLength > nSize) {
		RtlCopyMemory( lpFilename,
					   FileName,
					   nSize );
		SetLastError( ERROR_INSUFFICIENT_BUFFER );
		return nSize;
	} else {
		RtlCopyMemory( lpFilename,
					   FileName,
					   FileNameLength );
		lpFilename[FileNameLength] = ANSI_NULL;
		return FileNameLength;
	}
}

WINBASEAPI
_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
GetModuleFileNameW (
    _In_opt_ HMODULE hModule,
    _Out_writes_to_(nSize, ((return < nSize) ? (return + 1) : nSize)) LPWSTR lpFilename,
    _In_ DWORD nSize
    )
{
	NTSTATUS Status;
	ANSI_STRING FileName;

	PAGED_CODE();

	FileName.MaximumLength = (USHORT)nSize;
	Status = LdkAllocateAnsiString( &FileName );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return 0;
	}

	DWORD length = GetModuleFileNameA( hModule,
									   FileName.Buffer,
									   FileName.MaximumLength );
	if (length) {
		FileName.Length = (USHORT)length;

		UNICODE_STRING FileNameW;
		FileNameW.Buffer = lpFilename;
		FileNameW.Length = 0;
		FileNameW.MaximumLength = (USHORT)nSize;
		Status = LdkAnsiStringToUnicodeString( &FileNameW,
											   &FileName,
											   FALSE );

		if (! NT_SUCCESS(Status)) {
			LdkFreeAnsiString( &FileName );
			LdkSetLastNTError( Status );
			return 0;
		}
		length = FileNameW.Length / sizeof(WCHAR);
	}

	LdkFreeAnsiString( &FileName );
	return length;
}

WINBASEAPI
_When_(lpModuleName == NULL, _Ret_notnull_)
_When_(lpModuleName != NULL, _Ret_maybenull_)
HMODULE
WINAPI
GetModuleHandleA (
    _In_opt_ LPCSTR lpModuleName
    )
{
	HMODULE hModule;
	
	PAGED_CODE();

	if (! GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
							  lpModuleName,
							  &hModule )) {
		return NULL;
	}

	return hModule;
}

WINBASEAPI
_When_(lpModuleName == NULL, _Ret_notnull_)
_When_(lpModuleName != NULL, _Ret_maybenull_)
HMODULE
WINAPI
GetModuleHandleW (
    _In_opt_ LPCWSTR lpModuleName
    )
{
	HMODULE hModule;

	PAGED_CODE();

	if (! GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
							  lpModuleName,
							  &hModule )) {
		return NULL;
	}

	return hModule;
}

WINBASEAPI
BOOL
WINAPI
GetModuleHandleExA (
    _In_ DWORD dwFlags,
    _In_opt_ LPCSTR lpModuleName,
    _Out_ HMODULE * phModule
    )
{
	PAGED_CODE();

	if (FlagOn(dwFlags, ~(GET_MODULE_HANDLE_EX_FLAG_PIN |
						  GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT |
						  GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)) ||
		(FlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_PIN) &&
		 FlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT))) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (! ARGUMENT_PRESENT(phModule)) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	*phModule = NULL;

	if (ARGUMENT_PRESENT(lpModuleName)) {

		if (FlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)) {

			NTSTATUS Status;
			PLDK_MODULE Module;
			Status = LdkReferenceModuleByAddress( (PVOID)lpModuleName,
												  BooleanFlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_PIN),
												  ! BooleanFlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT),
												  &Module );

			if (! NT_SUCCESS(Status)) {
				LdkSetLastNTError( Status );
				return FALSE;
			}

			if (Module->Base) {
				*phModule = (HMODULE)Module->Base;
				return TRUE;
			}

		} else {

			NTSTATUS Status;
			PLDK_MODULE Module;
			Status = LdkReferenceModuleByName( lpModuleName,
											   BooleanFlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_PIN),
											   ! BooleanFlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT),
											   &Module );
			if (! NT_SUCCESS(Status)) {
				LdkSetLastNTError( Status );
				return FALSE;
			}

			if (Module->Base) {
				*phModule = (HMODULE)Module->Base;
				return TRUE;
			}

			*phModule = (HMODULE)Module;
			return TRUE;
		}

	} else {

		if (FlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)) {
			*phModule = NULL;
			SetLastError( ERROR_INVALID_PARAMETER );
			return FALSE;
		}

		*phModule = (HMODULE)NtCurrentPeb()->ImageBaseAddress;
		return TRUE;

	}

	SetLastError( ERROR_MOD_NOT_FOUND );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetModuleHandleExW (
    _In_ DWORD dwFlags,
    _In_opt_ LPCWSTR lpModuleName,
    _Out_ HMODULE * phModule
    )
{
	PAGED_CODE();

	if (ARGUMENT_PRESENT(lpModuleName) &&
		(! FlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))) {

		BOOL bRet;
		NTSTATUS Status;
		UNICODE_STRING Unicode;
		ANSI_STRING Ansi;
		
		RtlInitUnicodeString( &Unicode,
							  lpModuleName );
		Status = LdkUnicodeStringToAnsiString( &Ansi,
											   &Unicode,
											   TRUE );
		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError(Status);
			return FALSE;
		}

		bRet = GetModuleHandleExA( dwFlags,
								   Ansi.Buffer,
								   phModule );
		LdkFreeAnsiString(&Ansi);
		return bRet;
	}

	return GetModuleHandleExA( dwFlags,
							   (PCSTR)lpModuleName,
							   phModule );
}



WINBASEAPI
FARPROC
WINAPI
GetProcAddress (
   _In_ HMODULE hModule,
   _In_ LPCSTR lpProcName
   )
{
    NTSTATUS Status;
	PVOID ProcedureAddress = NULL;
	PVOID DllHandle;
    STRING ProcedureName;

	PAGED_CODE();

	DllHandle = LdkpMapModuleHandle( hModule,
									 FALSE );
	if (ARGUMENT_PRESENT(hModule) && !ARGUMENT_PRESENT(DllHandle)) {
		LdkSetLastNTError( STATUS_PROCEDURE_NOT_FOUND );
		return NULL;
	}

    if ((ULONG_PTR)lpProcName > 0xffff) {
        RtlInitString(&ProcedureName, lpProcName);
        Status = LdrGetProcedureAddress( DllHandle,
										 &ProcedureName,
										 0L,
										 &ProcedureAddress);
    } else {
        Status = LdrGetProcedureAddress( DllHandle,
										 NULL,
										 PtrToUlong( (PVOID)lpProcName ),
										 &ProcedureAddress );
    }
    if (NT_SUCCESS(Status)) {
        if (ProcedureAddress == DllHandle) {
            Status = ((ULONG_PTR)lpProcName > 0xffff) ? STATUS_ENTRYPOINT_NOT_FOUND : STATUS_ORDINAL_NOT_FOUND;
        } else {
            return (FARPROC)ProcedureAddress;
        }
    }

	LdkSetLastNTError( Status );
	return NULL;
}

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryA (
    _In_ LPCSTR lpLibFileName
    )
{
	PUNICODE_STRING Unicode;
	
	PAGED_CODE();

	Unicode = Ldk8BitStringToStaticUnicodeString( lpLibFileName );
	if (Unicode == NULL) {
		return NULL;
	}
	return LoadLibraryExW( Unicode->Buffer,
						   NULL,
						   0 );
}

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryW (
    _In_ LPCWSTR lpLibFileName
    )
{
	PAGED_CODE();

	return LoadLibraryExW( lpLibFileName,
						   NULL,
						   0 );
}

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryExA (
    _In_ LPCSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    )
{
	PUNICODE_STRING Unicode;

	PAGED_CODE()

	Unicode = Ldk8BitStringToStaticUnicodeString( lpLibFileName );
	if (Unicode == NULL) {
		return NULL;
	}
	return LoadLibraryExW( Unicode->Buffer,
						   hFile,
						   dwFlags );
}

BOOLEAN
LdkpIsPathSeparator (
	_In_ WCHAR Character
	)
{
	return Character == L'\\' || Character == L'/';
}

BOOLEAN
LdkpIsFullyQualifiedDllPath (
	_In_ LPCWSTR Path
	)
{
	if (! Path || ! *Path) {
		return FALSE;
	}

	if ((Path[0] == L'\\') &&
		Path[1] &&
		Path[2] &&
		Path[3] &&
		(Path[1] == L'?') &&
		(Path[2] == L'?') &&
		(Path[3] == L'\\')) {
		Path += 4;
	}

	if (LdkpIsPathSeparator(Path[0]) &&
		Path[1] &&
		LdkpIsPathSeparator(Path[1])) {
		return TRUE;
	}

	if (LdkpIsPathSeparator(Path[0])) {
		return TRUE;
	}

	return Path[0] &&
		   Path[1] &&
		   Path[2] &&
		   Path[1] == L':' &&
		   LdkpIsPathSeparator(Path[2]);
}

NTSTATUS
LdkpDuplicateDirectoryFromPath (
	_In_ LPCWSTR Path,
	_Out_ PUNICODE_STRING Directory
	)
{
	PCWSTR LastSeparator = NULL;
	PCWSTR Current;
	USHORT Length;

	Directory->Length = 0;
	Directory->MaximumLength = 0;
	Directory->Buffer = NULL;

	if (! Path) {
		return STATUS_SUCCESS;
	}

	for (Current = Path; *Current; Current++) {
		if (LdkpIsPathSeparator(*Current)) {
			LastSeparator = Current;
		}
	}

	if (! LastSeparator) {
		return STATUS_SUCCESS;
	}

	Length = (USHORT)((LastSeparator - Path) * sizeof(WCHAR));
	if (LastSeparator == Path + 2 &&
		Path[1] == L':') {
		Length += sizeof(WCHAR);
	}

	if (! Length) {
		return STATUS_SUCCESS;
	}

	Directory->MaximumLength = Length + sizeof(UNICODE_NULL);
	Directory->Buffer = RtlAllocateHeap( RtlProcessHeap(),
										 HEAP_ZERO_MEMORY,
										 Directory->MaximumLength );
	if (! Directory->Buffer) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlCopyMemory( Directory->Buffer,
				   Path,
				   Length );
	Directory->Length = Length;
	return STATUS_SUCCESS;
}

NTSTATUS
LdkpDuplicateImageDirectory (
	_Out_ PUNICODE_STRING Directory
	)
{
	UNICODE_STRING ImagePath = NtCurrentPeb()->ProcessParameters->ImagePathName;
	PWSTR End = (PWSTR)Add2Ptr(ImagePath.Buffer, ImagePath.Length);

	Directory->Length = 0;
	Directory->MaximumLength = 0;
	Directory->Buffer = NULL;

	while (End > ImagePath.Buffer) {
		End--;
		if (*End == OBJ_NAME_PATH_SEPARATOR) {
			ImagePath.Length = (USHORT)((End - ImagePath.Buffer) * sizeof(WCHAR));
			break;
		}
	}

	if (! ImagePath.Length) {
		return STATUS_SUCCESS;
	}

	Directory->MaximumLength = ImagePath.Length + sizeof(UNICODE_NULL);
	Directory->Buffer = RtlAllocateHeap( RtlProcessHeap(),
										 HEAP_ZERO_MEMORY,
										 Directory->MaximumLength );
	if (! Directory->Buffer) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	RtlCopyMemory( Directory->Buffer,
				   ImagePath.Buffer,
				   ImagePath.Length );
	Directory->Length = ImagePath.Length;
	return STATUS_SUCCESS;
}

NTSTATUS
LdkpDuplicateEnvironmentPath (
	_Out_ PUNICODE_STRING EnvironmentPath
	)
{
	EnvironmentPath->Length = 0;
	EnvironmentPath->MaximumLength = 4096 * sizeof(WCHAR);
	EnvironmentPath->Buffer = RtlAllocateHeap( RtlProcessHeap(),
											   HEAP_ZERO_MEMORY,
											   EnvironmentPath->MaximumLength );
	if (! EnvironmentPath->Buffer) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	for (;;) {
		DWORD EnvironmentPathLength = GetEnvironmentVariableW( L"PATH",
															   EnvironmentPath->Buffer,
															   EnvironmentPath->MaximumLength / sizeof(WCHAR) );
		if (EnvironmentPathLength < EnvironmentPath->MaximumLength / sizeof(WCHAR)) {
			EnvironmentPath->Length = (USHORT)(EnvironmentPathLength * sizeof(WCHAR));
			return STATUS_SUCCESS;
		}

		EnvironmentPath->MaximumLength = (USHORT)((EnvironmentPathLength + 1) * sizeof(WCHAR));
		PVOID Buffer = RtlReAllocateHeap( RtlProcessHeap(),
										  HEAP_ZERO_MEMORY,
										  EnvironmentPath->Buffer,
										  EnvironmentPath->MaximumLength );
		if (! Buffer) {
			RtlFreeHeap( RtlProcessHeap(),
						 0,
						 EnvironmentPath->Buffer );
			EnvironmentPath->Buffer = NULL;
			EnvironmentPath->MaximumLength = 0;
			return STATUS_INSUFFICIENT_RESOURCES;
		}
		EnvironmentPath->Buffer = Buffer;
	}
}

NTSTATUS
LdkpDuplicateDllDirectory (
	_Out_ PUNICODE_STRING DllDirectory
	)
{
	PAGED_CODE();

	DllDirectory->Length = 0;
	DllDirectory->MaximumLength = 0;
	DllDirectory->Buffer = NULL;

	RtlAcquirePebLock();
	if (NtCurrentPeb()->DllDirectory.Length) {
		NTSTATUS Status = LdkDuplicateUnicodeString( &NtCurrentPeb()->DllDirectory,
													 DllDirectory );
		RtlReleasePebLock();
		return Status;
	}
	RtlReleasePebLock();
	return STATUS_SUCCESS;
}

VOID
LdkpFreeHeapUnicodeString (
	_Inout_ PUNICODE_STRING String
	)
{
	if (String->Buffer) {
		RtlFreeHeap( RtlProcessHeap(),
					 0,
					 String->Buffer );
	}
	String->Buffer = NULL;
	String->Length = 0;
	String->MaximumLength = 0;
}

NTSTATUS
LdkpAppendDllPathElement (
	_Inout_ PUNICODE_STRING SearchPath,
	_In_ PCUNICODE_STRING Element
	)
{
	NTSTATUS Status;

	if (! Element->Length) {
		return STATUS_SUCCESS;
	}

	if (SearchPath->Length) {
		Status = RtlAppendUnicodeToString( SearchPath,
										   L";" );
		if (! NT_SUCCESS(Status)) {
			return Status;
		}
	}

	return RtlAppendUnicodeStringToString( SearchPath,
										   Element );
}

NTSTATUS
LdkpComputeDllPath (
	_In_ LPCWSTR lpLibFileName,
	_In_ DWORD dwFlags,
	_Outptr_ LPWSTR *DllPath
	)
{
	NTSTATUS Status;
	UNICODE_STRING LoadDirectory;
	UNICODE_STRING ImageDirectory;
	UNICODE_STRING SystemDirectory;
	UNICODE_STRING EnvironmentPath;
	UNICODE_STRING UserDllDirectories;
	UNICODE_STRING SearchPath;
	DWORD SearchFlags;

	PAGED_CODE();

	*DllPath = NULL;

	LoadDirectory.Buffer = NULL;
	ImageDirectory.Buffer = NULL;
	SystemDirectory.Buffer = NULL;
	EnvironmentPath.Buffer = NULL;
	UserDllDirectories.Buffer = NULL;

	Status = LdkpDuplicateDirectoryFromPath( lpLibFileName,
											 &LoadDirectory );
	if (! NT_SUCCESS(Status)) {
		goto Cleanup;
	}

	Status = LdkpDuplicateImageDirectory( &ImageDirectory );
	if (! NT_SUCCESS(Status)) {
		goto Cleanup;
	}

	SystemDirectory = NtCurrentPeb()->ProcessParameters->DllPath;

	Status = LdkpDuplicateEnvironmentPath( &EnvironmentPath );
	if (! NT_SUCCESS(Status)) {
		goto Cleanup;
	}

	Status = LdkpDuplicateDllDirectory( &UserDllDirectories );
	if (! NT_SUCCESS(Status)) {
		goto Cleanup;
	}

	SearchPath.Length = 0;
	SearchPath.MaximumLength = LoadDirectory.Length +
							   ImageDirectory.Length +
							   SystemDirectory.Length +
							   EnvironmentPath.Length +
							   UserDllDirectories.Length +
							   (5 * sizeof(WCHAR)) +
							   sizeof(UNICODE_NULL);
	SearchPath.Buffer = RtlAllocateHeap( RtlProcessHeap(),
										 HEAP_ZERO_MEMORY,
										 SearchPath.MaximumLength );
	if (! SearchPath.Buffer) {
		Status = STATUS_INSUFFICIENT_RESOURCES;
		goto Cleanup;
	}

	SearchFlags = dwFlags & LDK_LOAD_LIBRARY_SEARCH_FLAGS;
	if (dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH) {
		Status = LdkpAppendDllPathElement( &SearchPath,
										   &LoadDirectory );
		if (! NT_SUCCESS(Status)) {
			goto CleanupSearchPath;
		}
		Status = LdkpAppendDllPathElement( &SearchPath,
										   &ImageDirectory );
		if (! NT_SUCCESS(Status)) {
			goto CleanupSearchPath;
		}
		Status = LdkpAppendDllPathElement( &SearchPath,
										   &EnvironmentPath );
		if (! NT_SUCCESS(Status)) {
			goto CleanupSearchPath;
		}
	} else if (SearchFlags) {
		if (SearchFlags & LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR) {
			Status = LdkpAppendDllPathElement( &SearchPath,
											   &LoadDirectory );
			if (! NT_SUCCESS(Status)) {
				goto CleanupSearchPath;
			}
		}

		if (SearchFlags & LOAD_LIBRARY_SEARCH_DEFAULT_DIRS) {
			Status = LdkpAppendDllPathElement( &SearchPath,
											   &ImageDirectory );
			if (! NT_SUCCESS(Status)) {
				goto CleanupSearchPath;
			}
			Status = LdkpAppendDllPathElement( &SearchPath,
											   &UserDllDirectories );
			if (! NT_SUCCESS(Status)) {
				goto CleanupSearchPath;
			}
			Status = LdkpAppendDllPathElement( &SearchPath,
											   &SystemDirectory );
			if (! NT_SUCCESS(Status)) {
				goto CleanupSearchPath;
			}
		} else {
			if (SearchFlags & LOAD_LIBRARY_SEARCH_APPLICATION_DIR) {
				Status = LdkpAppendDllPathElement( &SearchPath,
												   &ImageDirectory );
				if (! NT_SUCCESS(Status)) {
					goto CleanupSearchPath;
				}
			}
			if (SearchFlags & (LOAD_LIBRARY_SEARCH_SYSTEM32 | LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER)) {
				Status = LdkpAppendDllPathElement( &SearchPath,
												   &SystemDirectory );
				if (! NT_SUCCESS(Status)) {
					goto CleanupSearchPath;
				}
			}
			if (SearchFlags & LOAD_LIBRARY_SEARCH_USER_DIRS) {
				Status = LdkpAppendDllPathElement( &SearchPath,
												   &UserDllDirectories );
				if (! NT_SUCCESS(Status)) {
					goto CleanupSearchPath;
				}
			}
		}
	} else {
		Status = LdkpAppendDllPathElement( &SearchPath,
										   &ImageDirectory );
		if (! NT_SUCCESS(Status)) {
			goto CleanupSearchPath;
		}
		Status = LdkpAppendDllPathElement( &SearchPath,
										   &EnvironmentPath );
		if (! NT_SUCCESS(Status)) {
			goto CleanupSearchPath;
		}
	}

	*DllPath = SearchPath.Buffer;
	SearchPath.Buffer = NULL;
	Status = STATUS_SUCCESS;

CleanupSearchPath:
	if (SearchPath.Buffer) {
		RtlFreeHeap( RtlProcessHeap(),
					 0,
					 SearchPath.Buffer );
	}

Cleanup:
	LdkpFreeHeapUnicodeString( &LoadDirectory );
	LdkpFreeHeapUnicodeString( &ImageDirectory );
	LdkpFreeHeapUnicodeString( &EnvironmentPath );
	LdkFreeUnicodeString( &UserDllDirectories );
	return Status;
}

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryExW (
    _In_ LPCWSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    )
{
	NTSTATUS Status;
	HMODULE hModule;
	UNICODE_STRING DllName;
	ULONG LoadFlags = 0;
	BOOL ResourceOnly;
	DWORD SearchFlags;
	
	PAGED_CODE();

	if (! lpLibFileName ||
		hFile ||
		FlagOn(dwFlags, ~LDK_LOAD_LIBRARY_SUPPORTED_FLAGS)) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	SearchFlags = dwFlags & LDK_LOAD_LIBRARY_SEARCH_FLAGS;
	if ((dwFlags & LOAD_WITH_ALTERED_SEARCH_PATH) &&
		SearchFlags) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	if ((dwFlags & LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR) &&
		(! LdkpIsFullyQualifiedDllPath(lpLibFileName))) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	ResourceOnly = BooleanFlagOn(dwFlags, LDK_LOAD_LIBRARY_RESOURCE_FLAGS);
	if (dwFlags & DONT_RESOLVE_DLL_REFERENCES) {
		LoadFlags |= LDK_LOAD_DLL_DONT_RESOLVE_IMPORTS;
	}
	if (ResourceOnly) {
		LoadFlags |= LDK_LOAD_DLL_RESOURCE_ONLY;
	}

	RtlInitUnicodeString( &DllName,
						  lpLibFileName );

	PPEB Peb = NtCurrentPeb();
	if ((! ResourceOnly) &&
		(DllName.Length == Peb->ProcessParameters->ImagePathName.Length)) {
		if (RtlEqualUnicodeString( &DllName,
								   &Peb->ProcessParameters->ImagePathName,
								   TRUE )) {
			return (HMODULE)Peb->ImageBaseAddress;
		}
	}

	PWSTR DllPath;
	Status = LdkpComputeDllPath( lpLibFileName,
								 dwFlags,
								 &DllPath );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return NULL;
	}

	Status = LdrLoadDll( DllPath,
						 &LoadFlags,
						 &DllName,
						 (PVOID *)&hModule );
	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 DllPath );
	if (! NT_SUCCESS(Status) ) {
		LdkSetLastNTError( Status );
		return NULL;
	} else {
		if (ResourceOnly) {
			hModule = (HMODULE)((ULONG_PTR)hModule | LDK_RESOURCE_MODULE_HANDLE_TAG);
		}
		return hModule;
	}
}

WINBASEAPI
BOOL
WINAPI
FreeLibrary (
    _In_ HMODULE hLibModule
    )
{
	NTSTATUS Status;
	PVOID ModuleHandle;

	PAGED_CODE()

	ModuleHandle = LdkpMapModuleHandle( hLibModule,
										 TRUE );
	if (! ModuleHandle) {
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	Status = LdrUnloadDll( ModuleHandle );
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}
	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
FreeLibraryAndExitThread (
    _In_ HMODULE hLibModule,
    _In_ DWORD dwExitCode
    )
{
	PAGED_CODE();

	FreeLibrary( hLibModule );
	ExitThread( dwExitCode );
}
