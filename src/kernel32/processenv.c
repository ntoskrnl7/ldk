#include "winbase.h"
#include "../peb.h"
#include "../ntdll/ntdll.h"


BOOL
LdkpCheckForSameCurdir (
    _In_ PUNICODE_STRING PathName
    );

NTSTATUS
LdkpDuplicateDllDirectoryString (
    _In_opt_ PCUNICODE_STRING PathName,
    _Out_ PUNICODE_STRING Directory
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GetCurrentDirectoryA)
#pragma alloc_text(PAGE, GetCurrentDirectoryW)
#pragma alloc_text(PAGE, GetDllDirectoryA)
#pragma alloc_text(PAGE, GetDllDirectoryW)
#pragma alloc_text(PAGE, LdkpCheckForSameCurdir)
#pragma alloc_text(PAGE, LdkpDuplicateDllDirectoryString)
#pragma alloc_text(PAGE, SetCurrentDirectoryA)
#pragma alloc_text(PAGE, SetCurrentDirectoryW)
#pragma alloc_text(PAGE, AddDllDirectory)
#pragma alloc_text(PAGE, RemoveDllDirectory)
#pragma alloc_text(PAGE, SetDefaultDllDirectories)
#pragma alloc_text(PAGE, SetDllDirectoryA)
#pragma alloc_text(PAGE, SetDllDirectoryW)
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

BOOLEAN
LdkpIsDirectorySeparator (
    _In_ WCHAR Character
    )
{
    return Character == L'\\' || Character == L'/';
}

BOOLEAN
LdkpIsFullyQualifiedDllDirectory (
    _In_ PCWSTR Path
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

    if (LdkpIsDirectorySeparator(Path[0]) &&
        Path[1] &&
        LdkpIsDirectorySeparator(Path[1])) {
        return TRUE;
    }

    return Path[0] &&
           Path[1] &&
           Path[2] &&
           Path[1] == L':' &&
           LdkpIsDirectorySeparator(Path[2]);
}

NTSTATUS
LdkpDuplicateDllDirectoryString (
    _In_opt_ PCUNICODE_STRING PathName,
    _Out_ PUNICODE_STRING Directory
    )
{
    NTSTATUS Status;

    Directory->Buffer = NULL;
    Directory->Length = 0;
    Directory->MaximumLength = 0;

    if (! ARGUMENT_PRESENT(PathName) ||
        ! PathName->Buffer ||
        ! PathName->Length) {
        return STATUS_SUCCESS;
    }

    if (PathName->Length > MAXUSHORT - sizeof(UNICODE_NULL)) {
        return STATUS_NAME_TOO_LONG;
    }

    Directory->MaximumLength = PathName->Length + sizeof(UNICODE_NULL);
    Status = LdkAllocateUnicodeString( Directory );
    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    RtlCopyMemory( Directory->Buffer,
                   PathName->Buffer,
                   PathName->Length );
    Directory->Length = PathName->Length;
    Directory->Buffer[Directory->Length / sizeof(WCHAR)] = UNICODE_NULL;
    return STATUS_SUCCESS;
}

BOOL
LdkpSetDllDirectory (
    _In_opt_ PCUNICODE_STRING PathName
    )
{
    UNICODE_STRING NewDirectory;
    UNICODE_STRING OldDirectory;
    NTSTATUS Status;

    PAGED_CODE();

    Status = LdkpDuplicateDllDirectoryString( PathName,
                                              &NewDirectory );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    RtlAcquirePebLock();
    OldDirectory = NtCurrentPeb()->DllDirectory;
    NtCurrentPeb()->DllDirectory = NewDirectory;
    RtlReleasePebLock();

    LdkFreeUnicodeString( &OldDirectory );
    return TRUE;
}

WINBASEAPI
DWORD
WINAPI
GetDllDirectoryW (
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPWSTR lpBuffer
    )
{
    UNICODE_STRING Directory;
    DWORD Length;
    DWORD Required;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Directory.Buffer = NULL;
    Directory.Length = 0;
    Directory.MaximumLength = 0;

    RtlAcquirePebLock();
    if (NtCurrentPeb()->DllDirectory.Length) {
        Status = LdkDuplicateUnicodeString( &NtCurrentPeb()->DllDirectory,
                                            &Directory );
    }
    RtlReleasePebLock();

    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return 0;
    }

    Length = Directory.Length / sizeof(WCHAR);
    Required = Length + 1;

    if (Length == 0) {
        if (nBufferLength == 0) {
            return Required;
        }
        if (! lpBuffer) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        lpBuffer[0] = UNICODE_NULL;
        return 0;
    }

    if (nBufferLength == 0) {
        LdkFreeUnicodeString( &Directory );
        return Required;
    }

    if (! lpBuffer) {
        LdkFreeUnicodeString( &Directory );
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (nBufferLength <= Length) {
        lpBuffer[0] = UNICODE_NULL;
        LdkFreeUnicodeString( &Directory );
        return Required;
    }

    RtlCopyMemory( lpBuffer,
                   Directory.Buffer,
                   Directory.Length );
    lpBuffer[Length] = UNICODE_NULL;
    LdkFreeUnicodeString( &Directory );
    return Length;
}

WINBASEAPI
DWORD
WINAPI
GetDllDirectoryA (
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPSTR lpBuffer
    )
{
    UNICODE_STRING Directory;
    ULONG ByteCount;
    ULONG BytesWritten;
    DWORD Required;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    Directory.Buffer = NULL;
    Directory.Length = 0;
    Directory.MaximumLength = 0;

    RtlAcquirePebLock();
    if (NtCurrentPeb()->DllDirectory.Length) {
        Status = LdkDuplicateUnicodeString( &NtCurrentPeb()->DllDirectory,
                                            &Directory );
    }
    RtlReleasePebLock();

    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return 0;
    }

    if (Directory.Length == 0) {
        if (nBufferLength == 0) {
            return 1;
        }
        if (! lpBuffer) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }
        lpBuffer[0] = ANSI_NULL;
        return 0;
    }

    Status = RtlUnicodeToMultiByteSize( &ByteCount,
                                        Directory.Buffer,
                                        Directory.Length );
    if (! NT_SUCCESS(Status)) {
        LdkFreeUnicodeString( &Directory );
        LdkSetLastNTError( Status );
        return 0;
    }

    Required = ByteCount + 1;
    if (nBufferLength == 0) {
        LdkFreeUnicodeString( &Directory );
        return Required;
    }

    if (! lpBuffer) {
        LdkFreeUnicodeString( &Directory );
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (nBufferLength <= ByteCount) {
        lpBuffer[0] = ANSI_NULL;
        LdkFreeUnicodeString( &Directory );
        return Required;
    }

    Status = RtlUnicodeToMultiByteN( lpBuffer,
                                     nBufferLength - 1,
                                     &BytesWritten,
                                     Directory.Buffer,
                                     Directory.Length );
    if (! NT_SUCCESS(Status)) {
        LdkFreeUnicodeString( &Directory );
        LdkSetLastNTError( Status );
        return 0;
    }

    lpBuffer[BytesWritten] = ANSI_NULL;
    LdkFreeUnicodeString( &Directory );
    return BytesWritten;
}

WINBASEAPI
DLL_DIRECTORY_COOKIE
WINAPI
AddDllDirectory (
    _In_ PCWSTR NewDirectory
    )
{
    UNICODE_STRING PathName;
    PLDK_DLL_DIRECTORY Directory;
    NTSTATUS Status;

    PAGED_CODE();

    if (! LdkpIsFullyQualifiedDllDirectory( NewDirectory )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }

    RtlInitUnicodeString( &PathName,
                          NewDirectory );

    Directory = RtlAllocateHeap( RtlProcessHeap(),
                                 HEAP_ZERO_MEMORY,
                                 sizeof(*Directory) );
    if (! Directory) {
        LdkSetLastNTError( STATUS_NO_MEMORY );
        return NULL;
    }

    Status = LdkpDuplicateDllDirectoryString( &PathName,
                                              &Directory->Path );
    if (! NT_SUCCESS(Status)) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     Directory );
        LdkSetLastNTError( Status );
        return NULL;
    }

    RtlAcquirePebLock();
    InsertTailList( &NtCurrentPeb()->DllDirectoryListHead,
                    &Directory->Links );
    RtlReleasePebLock();

    return (DLL_DIRECTORY_COOKIE)Directory;
}

WINBASEAPI
BOOL
WINAPI
RemoveDllDirectory (
    _In_ DLL_DIRECTORY_COOKIE Cookie
    )
{
    PLDK_DLL_DIRECTORY Directory;
    PLIST_ENTRY Entry;
    BOOLEAN Found = FALSE;

    PAGED_CODE();

    if (! Cookie) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    Directory = (PLDK_DLL_DIRECTORY)Cookie;

    RtlAcquirePebLock();
    for (Entry = NtCurrentPeb()->DllDirectoryListHead.Flink;
         Entry != &NtCurrentPeb()->DllDirectoryListHead;
         Entry = Entry->Flink) {
        if (CONTAINING_RECORD( Entry,
                               LDK_DLL_DIRECTORY,
                               Links ) == Directory) {
            RemoveEntryList( &Directory->Links );
            Found = TRUE;
            break;
        }
    }
    RtlReleasePebLock();

    if (! Found) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    LdkFreeUnicodeString( &Directory->Path );
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 Directory );
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
SetDefaultDllDirectories (
    _In_ DWORD DirectoryFlags
    )
{
    const DWORD ValidFlags = LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                             LOAD_LIBRARY_SEARCH_USER_DIRS |
                             LOAD_LIBRARY_SEARCH_SYSTEM32 |
                             LOAD_LIBRARY_SEARCH_DEFAULT_DIRS;

    PAGED_CODE();

    if (DirectoryFlags == 0 ||
        FlagOn(DirectoryFlags, ~ValidFlags)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    RtlAcquirePebLock();
    NtCurrentPeb()->DefaultDllDirectories = DirectoryFlags;
    RtlReleasePebLock();
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
SetDllDirectoryA (
    _In_opt_ LPCSTR lpPathName
    )
{
    PAGED_CODE();

    if (! ARGUMENT_PRESENT(lpPathName)) {
        return LdkpSetDllDirectory( NULL );
    }

    PUNICODE_STRING Unicode = Ldk8BitStringToStaticUnicodeString( lpPathName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return LdkpSetDllDirectory( Unicode );
}

WINBASEAPI
BOOL
WINAPI
SetDllDirectoryW (
    _In_opt_ LPCWSTR lpPathName
    )
{
    UNICODE_STRING PathName;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(lpPathName)) {
        return LdkpSetDllDirectory( NULL );
    }

    RtlInitUnicodeString( &PathName,
                          lpPathName );
    return LdkpSetDllDirectory( &PathName );
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
