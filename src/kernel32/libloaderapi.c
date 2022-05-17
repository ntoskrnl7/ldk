#include "winbase.h"
#include "../ntdll/ntdll.h"
#include <ntimage.h>



LPWSTR
LdkpComputeDllPath (
	VOID
	);



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdkpComputeDllPath)
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
		if ((ULONG_PTR)hModule & 0x00000001) {
			return (bResourcesOnly) ? (PVOID)hModule : NULL;
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
		if (NT_SUCCESS(LdkGetModuleByBase( (PVOID)hModule,
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

	if (! ARGUMENT_PRESENT(phModule)) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	*phModule = NULL;

	if (ARGUMENT_PRESENT(lpModuleName)) {

		if (FlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)) {

			NTSTATUS Status;
			PLDK_MODULE Module;
			Status = LdkGetModuleByAddress( (PVOID)lpModuleName,
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
			Status = LdkGetModuleByName( lpModuleName,
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
    STRING ProcedureName;

	PAGED_CODE();

    if ((ULONG_PTR)lpProcName > 0xffff) {
        RtlInitString(&ProcedureName, lpProcName);
        Status = LdrGetProcedureAddress( LdkpMapModuleHandle( hModule,
															  FALSE ),
										 &ProcedureName,
										 0L,
										 &ProcedureAddress);
    } else {
        Status = LdrGetProcedureAddress( LdkpMapModuleHandle( hModule,
															  FALSE),
										 NULL,
										 PtrToUlong( (PVOID)lpProcName ),
										 &ProcedureAddress );
    }
    if (NT_SUCCESS(Status)) {
        if (ProcedureAddress == LdkpMapModuleHandle( hModule,
													 FALSE )) {
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

LPWSTR
LdkpComputeDllPath (
	VOID
	)
{
	PAGED_CODE();

	UNICODE_STRING ImagePath = NtCurrentPeb()->ProcessParameters->ImagePathName;
	PWSTR p = (PWSTR)Add2Ptr(ImagePath.Buffer, ImagePath.Length);
	while (p > ImagePath.Buffer) {
		if (*(--p) == OBJ_NAME_PATH_SEPARATOR) {
			ImagePath.Length = (USHORT)((p - ImagePath.Buffer) * sizeof(WCHAR));
			break;
		}
	}

	UNICODE_STRING Value;
	Value.MaximumLength = (4096 * sizeof(WCHAR)) + sizeof(WCHAR) + ImagePath.Length + sizeof(WCHAR);
	Value.Buffer = RtlAllocateHeap( RtlProcessHeap(),
									HEAP_ZERO_MEMORY,
									Value.MaximumLength );
retry:
	if (! Value.Buffer) {
		return NULL;
	}
	Value.Length = (USHORT)GetEnvironmentVariableW( L"PATH",
													Value.Buffer,
													Value.MaximumLength / sizeof(WCHAR) ) * sizeof(WCHAR);
	if (Value.Length == 0) {
		RtlCopyMemory( Value.Buffer,
					   ImagePath.Buffer,
					   ImagePath.Length );
		return Value.Buffer;
	}
	if (Value.MaximumLength - Value.Length < ImagePath.Length) {
		Value.MaximumLength = Value.Length + sizeof(WCHAR) + ImagePath.Length;
		PVOID Buffer = RtlReAllocateHeap( RtlProcessHeap(),
										  HEAP_ZERO_MEMORY,
										  Value.Buffer,
										  Value.MaximumLength );
		if (Buffer) {
			Value.Buffer = Buffer;
			goto retry;
		}
		RtlCopyMemory( Value.Buffer,
					   ImagePath.Buffer,
					   ImagePath.Length );
		return Value.Buffer;
	}
	RtlAppendUnicodeToString( &Value,
							  L";" );
	RtlAppendUnicodeStringToString( &Value,
									&ImagePath );
	return Value.Buffer;
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
	ULONG DllCharacteristics = 0;
	
	PAGED_CODE();
	UNREFERENCED_PARAMETER(hFile);

	hModule = GetModuleHandleW( lpLibFileName );
	if (hModule) {
		return hModule;
	}

    if (dwFlags & DONT_RESOLVE_DLL_REFERENCES) {
		DllCharacteristics |= IMAGE_FILE_EXECUTABLE_IMAGE;
	}

	RtlInitUnicodeString( &DllName,
						  lpLibFileName );

	PPEB Peb = NtCurrentPeb();
	if (! (dwFlags & LOAD_LIBRARY_AS_DATAFILE) && (DllName.Length == Peb->ProcessParameters->ImagePathName.Length)) {
		if (RtlEqualUnicodeString( &DllName,
								   &Peb->ProcessParameters->ImagePathName,
								   TRUE )) {
			return (HMODULE)Peb->ImageBaseAddress;
		}
	}

	PWSTR DllPath = LdkpComputeDllPath();
	Status = LdrLoadDll( DllPath,
						 &DllCharacteristics,
						 &DllName,
						 (PVOID *)&hModule );
	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 DllPath );
	if (! NT_SUCCESS(Status) ) {
		LdkSetLastNTError( Status );
		return NULL;
	} else {
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

	PAGED_CODE()

	Status = LdrUnloadDll( hLibModule );
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