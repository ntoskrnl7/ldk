#include "winbase.h"
#include "../peb.h"
#include "../ntdll/ntdll.h"


BOOL
LdkpCheckForSameCurdir (
    _In_ PUNICODE_STRING PathName
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GetCurrentDirectoryA)
#pragma alloc_text(PAGE, GetCurrentDirectoryW)
#pragma alloc_text(PAGE, LdkpCheckForSameCurdir)
#pragma alloc_text(PAGE, SetCurrentDirectoryA)
#pragma alloc_text(PAGE, SetCurrentDirectoryW)
#pragma alloc_text(PAGE, GetEnvironmentStrings)
#pragma alloc_text(PAGE, GetEnvironmentStringsW)
#pragma alloc_text(PAGE, FreeEnvironmentStringsA)
#pragma alloc_text(PAGE, FreeEnvironmentStringsW)
#pragma alloc_text(PAGE, GetEnvironmentVariableA)
#pragma alloc_text(PAGE, GetEnvironmentVariableW)
#pragma alloc_text(PAGE, SetEnvironmentVariableA)
#pragma alloc_text(PAGE, SetEnvironmentVariableW)
#endif



WINBASEAPI
HANDLE
WINAPI
GetStdHandle (
    _In_ DWORD nStdHandle
    )
{ 
    switch (nStdHandle) {
    case STD_INPUT_HANDLE:
        return NtCurrentPeb()->ProcessParameters->StandardInput;
    case STD_OUTPUT_HANDLE:
        return NtCurrentPeb()->ProcessParameters->StandardOutput;
    case STD_ERROR_HANDLE:
        return NtCurrentPeb()->ProcessParameters->StandardError;
    }
    return INVALID_HANDLE_VALUE;
}

WINBASEAPI
BOOL
WINAPI
SetStdHandle (
    _In_ DWORD nStdHandle,
    _In_ HANDLE hHandle
    )
{
    return SetStdHandleEx( nStdHandle,
                           hHandle,
                           NULL );
}

WINBASEAPI
BOOL
WINAPI
SetStdHandleEx (
    _In_ DWORD nStdHandle,
    _In_ HANDLE hHandle,
    _Out_opt_ PHANDLE phPrevValue
    )
{
    HANDLE hPrevValue = NULL;
    switch (nStdHandle) {
    case STD_INPUT_HANDLE:
        hPrevValue = InterlockedExchangePointer( &NtCurrentPeb()->ProcessParameters->StandardInput,
                                                 hHandle );
        break;
    case STD_OUTPUT_HANDLE:
        hPrevValue = InterlockedExchangePointer( &NtCurrentPeb()->ProcessParameters->StandardOutput,
                                                 hHandle );
        break;
    case STD_ERROR_HANDLE:
        hPrevValue = InterlockedExchangePointer( &NtCurrentPeb()->ProcessParameters->StandardError,
                                                 hHandle );
        break;
    default:
        return FALSE;
    }

    if (ARGUMENT_PRESENT(phPrevValue)) {
        *phPrevValue = hPrevValue;
    }

    return TRUE;
}



WINBASEAPI
LPSTR
WINAPI
GetCommandLineA (
    VOID
    )
{
    extern UNICODE_STRING RtlpDosDevicesPrefix;
	return NtCurrentPeb()->FullPathName.Buffer + (RtlpDosDevicesPrefix.Length / sizeof(WCHAR));
}

WINBASEAPI
LPWSTR
WINAPI
GetCommandLineW (
    VOID
    )
{
    return NtCurrentPeb()->ProcessParameters->CommandLine.Buffer;
}



WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetCurrentDirectoryA (
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPSTR lpBuffer
    )
{
    PUNICODE_STRING Unicode;
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    DWORD ReturnValue;
    ULONG cbAnsiString;

    PAGED_CODE();

    if (nBufferLength > MAXUSHORT) {
        nBufferLength = MAXUSHORT - 2;
    }

    Unicode = &NtCurrentTeb()->StaticUnicodeString;

    Unicode->Length = (USHORT)RtlGetCurrentDirectory_U( Unicode->MaximumLength,
                                                        Unicode->Buffer );

    Status = RtlUnicodeToMultiByteSize( &cbAnsiString,
                                        Unicode->Buffer,
                                        Unicode->Length );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        ReturnValue = 0;
    } else {
        if (nBufferLength > (DWORD)(cbAnsiString)) {
            AnsiString.Buffer = lpBuffer;
            AnsiString.MaximumLength = (USHORT)(nBufferLength + 1);
            Status = LdkUnicodeStringTo8BitString( &AnsiString,
                                                   Unicode,
                                                   FALSE );
            if (! NT_SUCCESS(Status)) {
                LdkSetLastNTError( Status );
                ReturnValue = 0;
            } else {
                ReturnValue = AnsiString.Length;
            }
        } else {
            ReturnValue = cbAnsiString + 1;
        }
    }
    return ReturnValue;
}

WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetCurrentDirectoryW (
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPWSTR lpBuffer
    )
{
    PAGED_CODE();

    return (DWORD)RtlGetCurrentDirectory_U( nBufferLength * sizeof(WCHAR),
                                            lpBuffer ) / sizeof(WCHAR);
}



BOOL
LdkpCheckForSameCurdir (
    _In_ PUNICODE_STRING PathName
    )
{
    PAGED_CODE();

    PCURDIR CurDir = &(NtCurrentPeb()->ProcessParameters->CurrentDirectory);

    if (CurDir->DosPath.Length > 6) {
        if ((CurDir->DosPath.Length-2) != PathName->Length) {
            return FALSE;
        }
    } else {
        if (CurDir->DosPath.Length != PathName->Length) {
            return FALSE;
        }
    }

    RtlAcquirePebLock();
    BOOL rv = FALSE;
    UNICODE_STRING CurrentDir = CurDir->DosPath;
    if (CurrentDir.Length > 6) {
        CurrentDir.Length -= 2;
    }
    rv = RtlEqualUnicodeString( &CurrentDir,
                                PathName,
                                TRUE );
    RtlReleasePebLock();
    return rv;
}

WINBASEAPI
BOOL
WINAPI
SetCurrentDirectoryA (
    _In_ LPCSTR lpPathName
    )
{
    PAGED_CODE();

    PUNICODE_STRING Unicode = Ldk8BitStringToStaticUnicodeString( lpPathName );
    if (Unicode == NULL) {
        return FALSE;
    }

    if (LdkpCheckForSameCurdir( Unicode )) {
        return TRUE;
    }

    NTSTATUS Status = RtlSetCurrentDirectory_U( Unicode );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

    if (Unicode->Buffer[0] == L'"' && Unicode->Length > 2) {
        Unicode = Ldk8BitStringToStaticUnicodeString( lpPathName + 1 );
        if (Unicode == NULL) {
            return FALSE;
        }
        Status = RtlSetCurrentDirectory_U( Unicode );
        if (NT_SUCCESS(Status)) {
            return TRUE;
        }
    }
    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
SetCurrentDirectoryW (
    _In_ LPCWSTR lpPathName
    )
{
    UNICODE_STRING PathName;

    PAGED_CODE();
    
    RtlInitUnicodeString( &PathName,
                          lpPathName );

    if (LdkpCheckForSameCurdir( &PathName )) {
        return TRUE;
    }
    NTSTATUS Status = RtlSetCurrentDirectory_U( &PathName );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}



WINBASEAPI
_NullNull_terminated_
LPCH
WINAPI
GetEnvironmentStrings (
    VOID
    )
{
	ULONG Length, Size;
	NTSTATUS Status;
	PWCHAR Environment, p;
	PCHAR Buffer = NULL;

    PAGED_CODE();

	RtlAcquirePebLock();

	p = Environment = NtCurrentPeb()->ProcessParameters->Environment;

	do {
		p += wcslen(p) + 1;
	} while (*p);

	Length = (ULONG)(p - Environment + 1);

	Status = RtlUnicodeToMultiByteSize( &Size,
                                        Environment,
                                        Length * sizeof(WCHAR) );
	if (NT_SUCCESS(Status)) {
		Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                  0,
                                  Size );
		if (Buffer) {
			Status = RtlUnicodeToOemN( Buffer,
                                       Size,
                                       0,
                                       Environment,
                                       Length * sizeof(WCHAR) );
			if (! NT_SUCCESS(Status)) {
				RtlFreeHeap( RtlProcessHeap(),
                             0,
                             Buffer );
				Buffer = NULL;
				LdkSetLastNTError( Status );
			}
		} else {
			LdkSetLastNTError( STATUS_NO_MEMORY );
		}
	} else {
		LdkSetLastNTError( Status );
	}

    RtlReleasePebLock();
    return Buffer;
}

WINBASEAPI
_NullNull_terminated_
LPWCH
WINAPI
GetEnvironmentStringsW (
    VOID
    )
{
	PWCH Environment, p;
	SIZE_T Length;

    PAGED_CODE();

	RtlAcquirePebLock();
	p = Environment = NtCurrentPeb()->ProcessParameters->Environment;

	do{
		p += wcslen(p) + 1;
	} while (*p);

	Length = p - Environment + 1;

	p = RtlAllocateHeap( RtlProcessHeap(),
                         0,
                         Length * sizeof(WCHAR) );
	if (p) {
		RtlCopyMemory( p,
                       Environment,
                       Length * sizeof(WCHAR) );
	} else {
		LdkSetLastNTError( STATUS_NO_MEMORY );
	}

	RtlReleasePebLock();
	return p;
}

WINBASEAPI
BOOL
WINAPI
FreeEnvironmentStringsA (
    _In_ _Pre_ _NullNull_terminated_ LPCH penv
    )
{
	return RtlFreeHeap( RtlProcessHeap(),
                        0,
                        penv );
}

WINBASEAPI
BOOL
WINAPI
FreeEnvironmentStringsW (
    _In_ _Pre_ _NullNull_terminated_ LPWCH penv
    )
{
	return RtlFreeHeap( RtlProcessHeap(),
                        0,
                        penv );
}



WINBASEAPI
_Success_(return != 0 && return < nSize)
DWORD
WINAPI
GetEnvironmentVariableA (
    _In_opt_ LPCSTR lpName,
    _Out_writes_to_opt_(nSize, return + 1) LPSTR lpBuffer,
    _In_ DWORD nSize
    )
{
    PAGED_CODE();

	NTSTATUS Status;
	UNICODE_STRING ValueW;
	DWORD iSize;

    PUNICODE_STRING NameW = LdkAnsiStringToStaticUnicodeString( lpName );
	if (NameW == NULL) {
		return 0;
	}

	if (nSize > (MAXUSHORT >> 1) - 2) {
		iSize = (MAXUSHORT >> 1) - 2;
    } else {
	    iSize = nSize;
    }
	
    ValueW.MaximumLength = (USHORT)(iSize ? iSize - 1 : iSize) * sizeof(WCHAR);
	ValueW.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                     0,
                                     ValueW.MaximumLength );
    if (! ValueW.Buffer) {
		LdkSetLastNTError( STATUS_NO_MEMORY );
		return 0;
	}

	Status = RtlQueryEnvironmentVariable_U( NULL,
                                            NameW,
                                            &ValueW );
	if (NT_SUCCESS(Status) && (nSize == 0)) {
		Status = STATUS_BUFFER_OVERFLOW;
	}
    if (NT_SUCCESS(Status)) {
		if (nSize > MAXUSHORT-2) {
			iSize = MAXUSHORT-2;
		} else {
			iSize = nSize;
		}
	    
        ANSI_STRING Value;
        Value.Buffer = lpBuffer;
        Value.MaximumLength = (USHORT)iSize;
        Status = LdkUnicodeStringToAnsiString( &Value,
                                               &ValueW,
                                               FALSE );
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     ValueW.Buffer );
        if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return 0;
		}

        lpBuffer[Value.Length] = ANSI_NULL;
        return Value.Length;

	} else if (Status == STATUS_BUFFER_TOO_SMALL) {

        DWORD dwAnsiStringSize = 0;
        ValueW.MaximumLength = ValueW.Length + sizeof(WCHAR);
        PVOID Buffer = RtlReAllocateHeap( RtlProcessHeap(),
                                          0,
                                          ValueW.Buffer,
                                          ValueW.MaximumLength );
        if (! Buffer) {
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         ValueW.Buffer );
            LdkSetLastNTError( STATUS_NO_MEMORY );
            return 0;
        }
        ValueW.Buffer = Buffer;

        Status = RtlQueryEnvironmentVariable_U( NULL,
                                                NameW,
                                                &ValueW );
        if (NT_SUCCESS(Status)) {
            dwAnsiStringSize = RtlUnicodeStringToAnsiSize( &ValueW );
        }
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     ValueW.Buffer );
        return dwAnsiStringSize;
	}

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 ValueW.Buffer );
    LdkSetLastNTError( Status );
    return 0;
}

WINBASEAPI
_Success_(return != 0 && return < nSize)
DWORD
WINAPI
GetEnvironmentVariableW (
    _In_opt_ LPCWSTR lpName,
    _Out_writes_to_opt_(nSize, return + 1) LPWSTR lpBuffer,
    _In_ DWORD nSize
    )
{
	NTSTATUS Status;
	UNICODE_STRING Name, Value;
	DWORD iSize;

    PAGED_CODE();

	if (nSize > (MAXUSHORT >> 1) - 2) {
		iSize = (MAXUSHORT >> 1) - 2;
	} else {
		iSize = nSize;
	}

	RtlInitUnicodeString( &Name,
                          lpName );

	Value.Buffer = lpBuffer;
	Value.Length = 0;
	Value.MaximumLength = (USHORT)(iSize ? iSize - 1 : iSize) * sizeof(WCHAR);

	Status = RtlQueryEnvironmentVariable_U( NULL,
                                            &Name,
                                            &Value );
	if (NT_SUCCESS(Status) && (nSize == 0)) {
		Status = STATUS_BUFFER_OVERFLOW;
	}
	if (NT_SUCCESS(Status)) {
		lpBuffer[Value.Length / sizeof(WCHAR)] = UNICODE_NULL;
		return Value.Length / sizeof(WCHAR);
	} else if (Status == STATUS_BUFFER_TOO_SMALL) {
        return Value.Length / sizeof(WCHAR)+1;
	}
    LdkSetLastNTError( Status );
    return 0;
}

WINBASEAPI
BOOL
WINAPI
SetEnvironmentVariableA (
    _In_ LPCSTR lpName,
    _In_opt_ LPCSTR lpValue
    )
{
    PAGED_CODE();

	NTSTATUS Status;
	ANSI_STRING Value;
	UNICODE_STRING ValueW;

	PUNICODE_STRING NameW = LdkAnsiStringToStaticUnicodeString( lpName );
	if (NameW == NULL) {
		LdkSetLastNTError( STATUS_NO_MEMORY );
		return 0;
	}

	if (ARGUMENT_PRESENT(lpValue)) {
		RtlInitAnsiString( &Value,
                           lpValue );
		Status = LdkAnsiStringToUnicodeString( &ValueW,
                                               &Value,
                                               TRUE );
		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return 0;
		}
		Status = RtlSetEnvironmentVariable( NULL,
                                            NameW,
                                            &ValueW );
		LdkFreeUnicodeString( &ValueW );
	} else {
		Status = RtlSetEnvironmentVariable( NULL,
                                            NameW,
                                            NULL );
	}
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}

    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
SetEnvironmentVariableW(
    _In_ LPCWSTR lpName,
    _In_opt_ LPCWSTR lpValue
    )
{
	NTSTATUS Status;
	UNICODE_STRING Name;
	UNICODE_STRING Value;
	RtlInitUnicodeString(&Name, lpName);
	RtlInitUnicodeString(&Value, lpValue);

    PAGED_CODE();

    Status = RtlSetEnvironmentVariable( NULL,
                                        &Name,
                                        ARGUMENT_PRESENT(lpValue) ? &Value : NULL );
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}

    LdkSetLastNTError(Status);
    return FALSE;
}