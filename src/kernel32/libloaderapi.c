#include "winbase.h"
#include "../ntdll/ntdll.h"

#include "../nt/ntldr.h"

#include <ntimage.h>



PVOID
BasepMapModuleHandle(
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
GetModuleFileNameA(
    _In_opt_ HMODULE hModule,
    _Out_writes_to_(nSize, ((return < nSize) ? (return + 1) : nSize)) LPSTR lpFilename,
    _In_ DWORD nSize
    )
{
	PSTR FileName = NULL;
	DWORD FileNameLength = 0;

	if (ARGUMENT_PRESENT(hModule)) {
		PLDK_MODULE Module;
		if (NT_SUCCESS(LdkGetModuleByBase((PVOID)hModule, &Module))) {
			FileName = Module->FullPathName.Buffer;
			FileNameLength = Module->FullPathName.Length;
		}
	} else {
		FileName = (PSTR)NtCurrentPeb()->FullPathName.Buffer;
		FileNameLength = (DWORD)NtCurrentPeb()->FullPathName.Length;
	}

	if (FileNameLength == 0) {
		SetLastError(ERROR_NOT_FOUND);
		return 0;
	}

	if (FileNameLength > nSize) {
		RtlCopyMemory(lpFilename, FileName, nSize);
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return nSize;
	} else {
		RtlCopyMemory(lpFilename, FileName, FileNameLength);
		lpFilename[FileNameLength] = ANSI_NULL;
		return FileNameLength;
	}
}

WINBASEAPI
_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
GetModuleFileNameW(
    _In_opt_ HMODULE hModule,
    _Out_writes_to_(nSize, ((return < nSize) ? (return + 1) : nSize)) LPWSTR lpFilename,
    _In_ DWORD nSize
    )
{
	NTSTATUS status;

	ANSI_STRING FileName;
	FileName.MaximumLength = (USHORT)nSize;
	status = LdkAllocateAnsiString(&FileName);
	if (!NT_SUCCESS(status)) {
		BaseSetLastNTError(status);
		return 0;
	}

	DWORD length = GetModuleFileNameA(hModule, FileName.Buffer, FileName.MaximumLength);
	if (length) {
		FileName.Length = (USHORT)length;

		UNICODE_STRING FileNameW;
		FileNameW.Buffer = lpFilename;
		FileNameW.Length = 0;
		FileNameW.MaximumLength = (USHORT)nSize;
		status = LdkAnsiStringToUnicodeString(&FileNameW, &FileName, FALSE);

		if (!NT_SUCCESS(status)) {
			LdkFreeAnsiString(&FileName);
			BaseSetLastNTError(status);
			return 0;
		}
		length = FileNameW.Length / sizeof(WCHAR);
	}

	LdkFreeAnsiString(&FileName);
	return length;
}

WINBASEAPI
_When_(lpModuleName == NULL, _Ret_notnull_)
_When_(lpModuleName != NULL, _Ret_maybenull_)
HMODULE
WINAPI
GetModuleHandleA(
    _In_opt_ LPCSTR lpModuleName
    )
{
	HMODULE hModule;
	
	if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpModuleName, &hModule)) {
		return NULL;
	}

	return hModule;
}

WINBASEAPI
_When_(lpModuleName == NULL, _Ret_notnull_)
_When_(lpModuleName != NULL, _Ret_maybenull_)
HMODULE
WINAPI
GetModuleHandleW(
    _In_opt_ LPCWSTR lpModuleName
    )
{
	HMODULE hModule;

	if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, lpModuleName, &hModule)) {
		return NULL;
	}

	return hModule;
}

WINBASEAPI
BOOL
WINAPI
GetModuleHandleExA(
    _In_ DWORD dwFlags,
    _In_opt_ LPCSTR lpModuleName,
    _Out_ HMODULE * phModule
    )
{
	if (! ARGUMENT_PRESENT(phModule)) {
		SetLastError(ERROR_INVALID_PARAMETER);
		return FALSE;
	}

	*phModule = NULL;

	if (ARGUMENT_PRESENT(lpModuleName)) {

		if (FlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS)) {

			NTSTATUS Status;
			PLDK_MODULE Module;
			Status = LdkGetModuleByAddress((PVOID)lpModuleName, &Module);

			if (!NT_SUCCESS(Status)) {
				BaseSetLastNTError(Status);
				return FALSE;
			}

			if (Module->Base) {
				*phModule = (HMODULE)Module->Base;
				return TRUE;
			}

		} else {

			NTSTATUS Status;
			PLDK_MODULE Module;
			Status = LdkGetModuleByName(lpModuleName, &Module);

			if (!NT_SUCCESS(Status)) {
				BaseSetLastNTError(Status);
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
			SetLastError(ERROR_INVALID_PARAMETER);
			return FALSE;
		}

		*phModule = (HMODULE)NtCurrentPeb()->ImageBaseAddress;
		return TRUE;

	}

	SetLastError(ERROR_MOD_NOT_FOUND);
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetModuleHandleExW(
    _In_ DWORD dwFlags,
    _In_opt_ LPCWSTR lpModuleName,
    _Out_ HMODULE * phModule
    )
{
	if (ARGUMENT_PRESENT(lpModuleName) &&
		(! FlagOn(dwFlags, GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS))) {

		BOOL bRet;
		NTSTATUS Status;
		UNICODE_STRING Unicode;
		ANSI_STRING Ansi;
		
		RtlInitUnicodeString(&Unicode, lpModuleName);
		Status = LdkUnicodeStringToAnsiString(&Ansi, &Unicode, TRUE);
		if (!NT_SUCCESS(Status)) {
			BaseSetLastNTError(Status);
			return FALSE;
		}

		bRet = GetModuleHandleExA(dwFlags, Ansi.Buffer, phModule);
		LdkFreeAnsiString(&Ansi);
		return bRet;
	}

	return GetModuleHandleExA(dwFlags, (PCSTR)lpModuleName, phModule);;
}



WINBASEAPI
FARPROC
WINAPI
GetProcAddress(
   _In_ HMODULE hModule,
   _In_ LPCSTR lpProcName
   )
{
    NTSTATUS status;
	PVOID ProcedureAddress = NULL;
    STRING ProcedureName;

    if ((ULONG_PTR)lpProcName > 0xffff) {
        RtlInitString(&ProcedureName, lpProcName);
        status = LdrGetProcedureAddress(BasepMapModuleHandle(hModule, FALSE),&ProcedureName,0L,&ProcedureAddress);
    } else {
        status = LdrGetProcedureAddress(BasepMapModuleHandle(hModule, FALSE),NULL,PtrToUlong((PVOID)lpProcName),&ProcedureAddress);
    }
    if (!NT_SUCCESS(status)) {
		BaseSetLastNTError(status);
		return NULL;
    } else {
        if (ProcedureAddress == BasepMapModuleHandle(hModule, FALSE)) {
            if ((ULONG_PTR)lpProcName > 0xffff) {
                status = STATUS_ENTRYPOINT_NOT_FOUND;
			} else {
                status = STATUS_ORDINAL_NOT_FOUND;
            }
            BaseSetLastNTError(status);
            return NULL;
        } else {
            return (FARPROC)ProcedureAddress;
        }
    }
}

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryExA(
    _In_ LPCSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    )
{
	HMODULE hModule;
	ANSI_STRING FileName;
	UNICODE_STRING FileNameW;

	RtlInitString(&FileName, lpLibFileName);
	NTSTATUS status = LdkAnsiStringToUnicodeString(&FileNameW, &FileName, TRUE);
	if (!NT_SUCCESS(status)) {
		return NULL;
	}
	hModule = LoadLibraryExW(FileNameW.Buffer, hFile, dwFlags);

	LdkFreeUnicodeString(&FileNameW);

	return hModule;
}

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryExW(
    _In_ LPCWSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    )
{
	HMODULE hModule;
	NTSTATUS status;
	UNICODE_STRING DllName;
	ULONG DllCharacteristics = 0;
	
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

	PLDK_PEB peb = NtCurrentPeb();

	if (! (dwFlags & LOAD_LIBRARY_AS_DATAFILE) && (DllName.Length == peb->ProcessParameters->ImagePathName.Length)) {
		if (RtlEqualUnicodeString( &DllName,
								   &peb->ProcessParameters->ImagePathName,
								   TRUE )) {
			return (HMODULE)peb->ImageBaseAddress;
		}
	}

	status = LdrLoadDll( NULL,
						 &DllCharacteristics,
						 &DllName,
						 (PVOID *)&hModule );
	if (! NT_SUCCESS(status) ) {
		BaseSetLastNTError(status);
		return NULL;
	} else {
		return hModule;
	}
}

WINBASEAPI
BOOL
WINAPI
FreeLibrary(
    _In_ HMODULE hLibModule
    )
{
	NTSTATUS status;
	status = LdrUnloadDll(hLibModule);
	if (NT_SUCCESS(status)) {
		return TRUE;
	}
	BaseSetLastNTError(status);
	return FALSE;
}