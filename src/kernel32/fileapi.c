#include "winbase.h"
#include "../ldk.h"
#include "ioapiset.h"
#include "../ntdll/ntdll.h"

#ifdef DeleteFile
#undef DeleteFile
#endif



BOOL
IsThisARootDirectory (
    _In_ HANDLE RootHandle,
    _In_opt_ PUNICODE_STRING FileName
    );

BOOL
LdkpQueryFullFileAttributes (
    _In_ LPCWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    );

#define LDK_FIND_HANDLE_MAGIC 0x464B444Cu
#ifdef FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY
#define LDK_FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY
#else
#define LDK_FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY 0
#endif
#define LDK_FIND_FIRST_EX_SUPPORTED_FLAGS \
    (FIND_FIRST_EX_CASE_SENSITIVE | \
     FIND_FIRST_EX_LARGE_FETCH | \
     LDK_FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY)
#define LDK_FILE_RENAME_INFO_EX_SUPPORTED_FLAGS \
    (FILE_RENAME_FLAG_REPLACE_IF_EXISTS | \
     FILE_RENAME_FLAG_POSIX_SEMANTICS | \
     FILE_RENAME_FLAG_SUPPRESS_PIN_STATE_INHERITANCE)
#define LDK_FILE_RENAME_INFORMATION_EX_CLASS ((FILE_INFORMATION_CLASS)65)

typedef union _LDK_FIND_QUERY_BUFFER {
    FILE_BOTH_DIR_INFORMATION Alignment;
    UCHAR Buffer[sizeof(FILE_BOTH_DIR_INFORMATION) + MAX_PATH * sizeof(WCHAR)];
} LDK_FIND_QUERY_BUFFER, *PLDK_FIND_QUERY_BUFFER;

typedef struct _LDK_FIND_HANDLE {
    ULONG Magic;
    HANDLE DirectoryHandle;
    UNICODE_STRING Pattern;
    WCHAR PatternBuffer[MAX_PATH];
    BOOLEAN FirstQuery;
    LDK_FIND_QUERY_BUFFER QueryBuffer;
} LDK_FIND_HANDLE, *PLDK_FIND_HANDLE;

static volatile LONG LdkpTempFileNameCounter;

enum {
    LDK_FILE_RENAME_INFO = 3,
    LDK_FILE_STREAM_INFO = 7,
    LDK_FILE_COMPRESSION_INFO = 8,
    LDK_FILE_ID_BOTH_DIRECTORY_INFO = 10,
    LDK_FILE_ID_BOTH_DIRECTORY_RESTART_INFO = 11,
    LDK_FILE_IO_PRIORITY_HINT_INFO = 12,
    LDK_FILE_REMOTE_PROTOCOL_INFO = 13,
    LDK_FILE_FULL_DIRECTORY_INFO = 14,
    LDK_FILE_FULL_DIRECTORY_RESTART_INFO = 15,
    LDK_FILE_STORAGE_INFO = 16,
    LDK_FILE_ALIGNMENT_INFO = 17,
    LDK_FILE_ID_EXTD_DIRECTORY_INFO = 19,
    LDK_FILE_ID_EXTD_DIRECTORY_RESTART_INFO = 20,
    LDK_FILE_RENAME_INFO_EX = 22,
    LDK_FILE_CASE_SENSITIVE_INFO = 23,
    LDK_FILE_NORMALIZED_NAME_INFO = 24
};

static
BOOLEAN
LdkpIsGetFileInformationByHandleExClass (
    _In_ FILE_INFO_BY_HANDLE_CLASS FileInformationClass
    )
{
    switch ((ULONG)FileInformationClass) {
    case 0:
    case 1:
    case 2:
    case LDK_FILE_STREAM_INFO:
    case LDK_FILE_COMPRESSION_INFO:
    case 9:
    case LDK_FILE_ID_BOTH_DIRECTORY_INFO:
    case LDK_FILE_ID_BOTH_DIRECTORY_RESTART_INFO:
    case LDK_FILE_REMOTE_PROTOCOL_INFO:
    case LDK_FILE_FULL_DIRECTORY_INFO:
    case LDK_FILE_FULL_DIRECTORY_RESTART_INFO:
    case LDK_FILE_STORAGE_INFO:
    case LDK_FILE_ALIGNMENT_INFO:
    case 18:
    case LDK_FILE_ID_EXTD_DIRECTORY_INFO:
    case LDK_FILE_ID_EXTD_DIRECTORY_RESTART_INFO:
    case LDK_FILE_NORMALIZED_NAME_INFO:
        return TRUE;
    default:
        return FALSE;
    }
}

static
BOOLEAN
LdkpIsSetFileInformationByHandleClass (
    _In_ FILE_INFO_BY_HANDLE_CLASS FileInformationClass
    )
{
    switch ((ULONG)FileInformationClass) {
    case 0:
    case LDK_FILE_RENAME_INFO:
    case 4:
    case 5:
    case 6:
    case LDK_FILE_IO_PRIORITY_HINT_INFO:
    case 21:
    case LDK_FILE_RENAME_INFO_EX:
    case LDK_FILE_CASE_SENSITIVE_INFO:
        return TRUE;
    default:
        return FALSE;
    }
}

static
VOID
LdkpSetFileInformationClassError (
    _In_ BOOLEAN KnownButUnsupported
    )
{
    if (KnownButUnsupported) {
        LDK_DIAGNOSTIC_BREAK();
        LdkSetLastNTError( STATUS_NOT_IMPLEMENTED );
    } else {
        SetLastError( ERROR_INVALID_PARAMETER );
    }
}



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateDirectoryA)
#pragma alloc_text(PAGE, CreateDirectoryW)
#pragma alloc_text(PAGE, CreateFileA)
#pragma alloc_text(PAGE, CreateFileW)
#pragma alloc_text(PAGE, ReadFile)
#pragma alloc_text(PAGE, WriteFile)
#pragma alloc_text(PAGE, LockFile)
#pragma alloc_text(PAGE, LockFileEx)
#pragma alloc_text(PAGE, UnlockFile)
#pragma alloc_text(PAGE, UnlockFileEx)
#pragma alloc_text(PAGE, FlushFileBuffers)
#pragma alloc_text(PAGE, SetEndOfFile)
#pragma alloc_text(PAGE, SetFilePointer)
#pragma alloc_text(PAGE, SetFilePointerEx)
#pragma alloc_text(PAGE, GetFileAttributesA)
#pragma alloc_text(PAGE, GetFileAttributesW)
#pragma alloc_text(PAGE, GetFileAttributesExW)
#pragma alloc_text(PAGE, SetFileAttributesW)
#pragma alloc_text(PAGE, GetFileSize)
#pragma alloc_text(PAGE, GetFileSizeEx)
#pragma alloc_text(PAGE, GetFileType)
#pragma alloc_text(PAGE, SetFileTime)
#pragma alloc_text(PAGE, DeleteFileA)
#pragma alloc_text(PAGE, DeleteFileW)
#pragma alloc_text(PAGE, RemoveDirectoryA)
#pragma alloc_text(PAGE, RemoveDirectoryW)
#pragma alloc_text(PAGE, GetFileInformationByHandle)
#pragma alloc_text(PAGE, GetFileInformationByHandleEx)
#pragma alloc_text(PAGE, SetFileInformationByHandle)
#pragma alloc_text(PAGE, GetFinalPathNameByHandleW)
#pragma alloc_text(PAGE, FindClose)
#pragma alloc_text(PAGE, FindFirstFileW)
#pragma alloc_text(PAGE, FindFirstFileExW)
#pragma alloc_text(PAGE, FindNextFileW)
#pragma alloc_text(PAGE, GetFullPathNameA)
#pragma alloc_text(PAGE, GetFullPathNameW)
#pragma alloc_text(PAGE, GetDriveTypeA)
#pragma alloc_text(PAGE, GetDriveTypeW)
#pragma alloc_text(PAGE, GetDiskFreeSpaceExW)
#pragma alloc_text(PAGE, GetTempPathA)
#pragma alloc_text(PAGE, GetTempPathW)
#pragma alloc_text(PAGE, GetTempPath2W)
#pragma alloc_text(PAGE, GetTempFileNameA)
#pragma alloc_text(PAGE, GetTempFileNameW)
#pragma alloc_text(PAGE, CopyFileW)
#pragma alloc_text(PAGE, CreateDirectoryExW)
#pragma alloc_text(PAGE, CreateHardLinkW)
#pragma alloc_text(PAGE, MoveFileExW)
#pragma alloc_text(PAGE, CreateSymbolicLinkW)
#pragma alloc_text(PAGE, DeviceIoControl)
#pragma alloc_text(PAGE, IsThisARootDirectory)
#endif


BOOL
LdkpQueryFullFileAttributes (
    _In_ LPCWSTR FileName,
    _Out_ PFILE_NETWORK_OPEN_INFORMATION FileInformation
    )
{
    NTSTATUS Status;
    UNICODE_STRING NtFileName;
    RTL_RELATIVE_NAME_U RelativeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PVOID FreeBuffer = NULL;
    HANDLE RootDirectory = NULL;

	PAGED_CODE();

    if (FileName == NULL || FileInformation == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (RtlDosPathNameToNtPathName_U( FileName,
                                      &NtFileName,
                                      NULL,
                                      &RelativeName )) {
        FreeBuffer = NtFileName.Buffer;
        if (RelativeName.RelativeName.Length) {
            NtFileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
            RootDirectory = RelativeName.ContainingDirectory;
        }
    } else {
        RtlInitUnicodeString( &NtFileName,
                              FileName );
    }

    InitializeObjectAttributes( &ObjectAttributes,
                                &NtFileName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                RootDirectory,
                                NULL );

    Status = ZwQueryFullAttributesFile( &ObjectAttributes,
                                        FileInformation );

    if (FreeBuffer) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FreeBuffer );
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
CreateDirectoryA (
    _In_ LPCSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    PUNICODE_STRING Unicode;

	PAGED_CODE();

    Unicode = Ldk8BitStringToStaticUnicodeString( lpPathName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return CreateDirectoryW( (LPCWSTR)Unicode->Buffer,
							 lpSecurityAttributes );
}

WINBASEAPI
BOOL
WINAPI
CreateDirectoryW (
    _In_ LPCWSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
	NTSTATUS Status;
	HANDLE DirectoryHandle;
	UNICODE_STRING FileName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatus;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer = NULL;

	PAGED_CODE();

    if (lpPathName == NULL) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    if (RtlDosPathNameToNtPathName_U( lpPathName,
                                      &FileName,
                                      NULL,
                                      &RelativeName )) {
        FreeBuffer = FileName.Buffer;
        if (RelativeName.RelativeName.Length) {
            FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
        } else {
            RelativeName.ContainingDirectory = NULL;
        }
    } else {
        RtlInitUnicodeString( &FileName,
                              lpPathName );
        RelativeName.ContainingDirectory = NULL;
    }

	InitializeObjectAttributes( &ObjectAttributes,
								&FileName,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								RelativeName.ContainingDirectory,
								NULL );
	if (ARGUMENT_PRESENT(lpSecurityAttributes)) {
		ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
		if (lpSecurityAttributes->bInheritHandle) {
			SetFlag(ObjectAttributes.Attributes, OBJ_INHERIT);
		}
	}

	Status = ZwCreateFile( &DirectoryHandle,
						   FILE_LIST_DIRECTORY | SYNCHRONIZE,
						   &ObjectAttributes,
						   &IoStatus,
						   NULL,
						   FILE_ATTRIBUTE_NORMAL,
						   FILE_SHARE_READ | FILE_SHARE_WRITE,
						   FILE_CREATE,
						   FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
						   NULL,
						   0L );

    if (FreeBuffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FreeBuffer );
    }

	if (NT_SUCCESS(Status)) {
		ZwClose( DirectoryHandle );
		return TRUE;
	} else {
		if (RtlIsDosDeviceName_U( (LPWSTR)lpPathName) ) {
			Status = STATUS_NOT_A_DIRECTORY;
		}
		LdkSetLastNTError( Status );
		return FALSE;
	}
}



WINBASEAPI
HANDLE
WINAPI
CreateFileA (
    _In_ LPCSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
    )
{
    PUNICODE_STRING Unicode;

	PAGED_CODE();

    Unicode = Ldk8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return INVALID_HANDLE_VALUE;
    }

    return CreateFileW( Unicode->Buffer,
                        dwDesiredAccess,
                        dwShareMode,
                        lpSecurityAttributes,
                        dwCreationDisposition,
                        dwFlagsAndAttributes,
                        hTemplateFile );
}

WINBASEAPI
HANDLE
WINAPI
CreateFileW (
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwDesiredAccess,
    _In_ DWORD dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_ DWORD dwCreationDisposition,
    _In_ DWORD dwFlagsAndAttributes,
    _In_opt_ HANDLE hTemplateFile
    )
{
	NTSTATUS Status;

	HANDLE FileHandle;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatus;
	UNICODE_STRING FileName;
	ULONG CreateDisposition = 0L;
	ULONG CreateFlags = 0L;

	DWORD SQOSFlags;
	SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

	PFILE_FULL_EA_INFORMATION EaBuffer = NULL;
	ULONG EaSize = 0;

	PAGED_CODE();

	switch (dwCreationDisposition) {
	case CREATE_NEW:
		CreateDisposition = FILE_CREATE;
		break;

	case CREATE_ALWAYS:
		CreateDisposition = FILE_OVERWRITE_IF;
		break;

	case OPEN_EXISTING:
		CreateDisposition = FILE_OPEN;
		break;

	case OPEN_ALWAYS:
		CreateDisposition = FILE_OPEN_IF;
		break;

	case TRUNCATE_EXISTING:
		CreateDisposition = FILE_OPEN;
		if (! FlagOn(dwDesiredAccess,GENERIC_WRITE)) {
			LdkSetLastNTError( STATUS_INVALID_PARAMETER );
			return INVALID_HANDLE_VALUE;
		}
		break;

	default:
		LdkSetLastNTError( STATUS_INVALID_PARAMETER );
		return INVALID_HANDLE_VALUE;
	}
	
	PVOID FreeBuffer = NULL;
	RTL_RELATIVE_NAME_U RelativeName;
    if (! RtlDosPathNameToNtPathName_U( lpFileName,
										&FileName,
										NULL,
										&RelativeName )) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return INVALID_HANDLE_VALUE;
    }
	FreeBuffer = FileName.Buffer;

    if (RelativeName.RelativeName.Length ) {
    	FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    } else {
    	RelativeName.ContainingDirectory = NULL;
    }

    SQOSFlags = dwFlagsAndAttributes & SECURITY_VALID_SQOS_FLAGS;
	if (FlagOn(SQOSFlags, SECURITY_SQOS_PRESENT)) {
		SQOSFlags &= ~SECURITY_SQOS_PRESENT;
		if (FlagOn(SQOSFlags, SECURITY_CONTEXT_TRACKING)) {
			SecurityQualityOfService.ContextTrackingMode = TRUE;
			SQOSFlags &= ~SECURITY_CONTEXT_TRACKING;
		} else {
			SecurityQualityOfService.ContextTrackingMode = FALSE;
		}
		if (FlagOn(SQOSFlags, SECURITY_EFFECTIVE_ONLY)) {
			SecurityQualityOfService.EffectiveOnly = TRUE;
			SQOSFlags &= ~SECURITY_EFFECTIVE_ONLY;
		} else {
			SecurityQualityOfService.EffectiveOnly = FALSE;
		}
		SecurityQualityOfService.ImpersonationLevel = SQOSFlags >> 16;
	} else {
		SecurityQualityOfService.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
		SecurityQualityOfService.ImpersonationLevel = SecurityImpersonation;
		SecurityQualityOfService.EffectiveOnly = TRUE;
	}
    SecurityQualityOfService.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);

	InitializeObjectAttributes( &ObjectAttributes,
								&FileName,
								OBJ_KERNEL_HANDLE | (dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ? 0 : OBJ_CASE_INSENSITIVE),
								RelativeName.ContainingDirectory,
								NULL );

	ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

	if (ARGUMENT_PRESENT(lpSecurityAttributes)) {	
		ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
		if (lpSecurityAttributes->bInheritHandle) {
			ObjectAttributes.Attributes |= OBJ_INHERIT;
		}
	}

	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_BACKUP_SEMANTICS) ? FILE_OPEN_FOR_BACKUP_INTENT : 0 ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_SEQUENTIAL_SCAN) ? FILE_SEQUENTIAL_ONLY : 0 ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_RANDOM_ACCESS) ? FILE_RANDOM_ACCESS : 0 ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_NO_BUFFERING) ? FILE_NO_INTERMEDIATE_BUFFERING : 0 ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_OVERLAPPED) ? 0 : FILE_SYNCHRONOUS_IO_NONALERT ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_WRITE_THROUGH) ? FILE_WRITE_THROUGH : 0 ));
	if (! FlagOn(dwFlagsAndAttributes, FILE_FLAG_BACKUP_SEMANTICS)) {
		SetFlag(CreateFlags, FILE_NON_DIRECTORY_FILE);
	} else {
		if ((CreateDisposition == FILE_CREATE) &&
			FlagOn(dwFlagsAndAttributes, FILE_ATTRIBUTE_DIRECTORY) &&
			FlagOn(dwFlagsAndAttributes, FILE_FLAG_POSIX_SEMANTICS)
			) {
			SetFlag(CreateFlags, FILE_DIRECTORY_FILE);
		}
	}
	if (FlagOn(dwFlagsAndAttributes, FILE_FLAG_OPEN_NO_RECALL)) {
		SetFlag(CreateFlags, FILE_OPEN_NO_RECALL);
	}
	if (FlagOn(dwFlagsAndAttributes, FILE_FLAG_OPEN_REPARSE_POINT)) {
		SetFlag(CreateFlags, FILE_OPEN_REPARSE_POINT);
	}
	if (FlagOn(dwFlagsAndAttributes, FILE_FLAG_DELETE_ON_CLOSE)) {
		SetFlag(CreateFlags, FILE_DELETE_ON_CLOSE);
		SetFlag(dwDesiredAccess, DELETE);
	}

	if (ARGUMENT_PRESENT(hTemplateFile)) {

		FILE_EA_INFORMATION EaInfo;
		Status = ZwQueryInformationFile( hTemplateFile,
										 &IoStatus,
										 &EaInfo,
										 sizeof(EaInfo),
										 FileEaInformation );

		if (NT_SUCCESS(Status) && EaInfo.EaSize) {
			EaSize = EaInfo.EaSize;

			do {
				EaSize *= 2;
				EaBuffer = RtlAllocateHeap( RtlProcessHeap(),
											MAKE_TAG( TMP_TAG ),
							 				EaSize );
				if (! EaBuffer) {
					RtlFreeHeap( RtlProcessHeap(),
								 0,
								 FreeBuffer );
					LdkSetLastNTError( STATUS_NO_MEMORY );
					return INVALID_HANDLE_VALUE;
				}

				Status = ZwQueryEaFile( hTemplateFile,
										&IoStatus,
										EaBuffer,
										EaSize,
										FALSE,
										(PVOID)NULL,
										0,
										(PULONG)NULL,
										TRUE );

				if (! NT_SUCCESS(Status)) {
					RtlFreeHeap( RtlProcessHeap(),
								 0,
								 EaBuffer );
					EaBuffer = NULL;
					IoStatus.Information = 0;
				}
			} while ((Status == STATUS_BUFFER_OVERFLOW) || (Status == STATUS_BUFFER_TOO_SMALL));

			EaSize = (ULONG)IoStatus.Information;
		}
	}

	Status = ZwCreateFile( &FileHandle,
						   (ACCESS_MASK)dwDesiredAccess | SYNCHRONIZE | FILE_READ_ATTRIBUTES,
						   &ObjectAttributes,
						   &IoStatus,
						   NULL,
						   dwFlagsAndAttributes & (FILE_ATTRIBUTE_VALID_FLAGS & ~FILE_ATTRIBUTE_DIRECTORY),
						   dwShareMode,
						   CreateDisposition,
						   CreateFlags,
						   EaBuffer,
						   EaSize );

	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 FreeBuffer );

	if (EaBuffer) {
		RtlFreeHeap( RtlProcessHeap(),
					 0,
					EaBuffer );
	}

	if (! NT_SUCCESS( Status )) {
		if (Status == STATUS_OBJECT_NAME_COLLISION) {
			SetLastError( ERROR_FILE_EXISTS );
		} else if (Status == STATUS_FILE_IS_A_DIRECTORY) {
			SetLastError( ERROR_ACCESS_DENIED );
		} else {
			LdkSetLastNTError( Status );
		}
		return INVALID_HANDLE_VALUE;
	}

	if (((dwCreationDisposition == CREATE_ALWAYS) && (IoStatus.Information == FILE_OVERWRITTEN)) ||
		((dwCreationDisposition == OPEN_ALWAYS) && (IoStatus.Information == FILE_OPENED))) {
		SetLastError( ERROR_ALREADY_EXISTS );
	} else {		
		SetLastError( ERROR_SUCCESS );
	}

    if (dwCreationDisposition == TRUNCATE_EXISTING) {
		FILE_ALLOCATION_INFORMATION AllocationInfo;
		AllocationInfo.AllocationSize.QuadPart = 0;

		Status = ZwSetInformationFile( FileHandle,
									   &IoStatus,
									   &AllocationInfo,
									   sizeof(AllocationInfo),
									   FileAllocationInformation );

		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			ZwClose( FileHandle );
			FileHandle = INVALID_HANDLE_VALUE;
		}
	}
	return FileHandle;
}

WINBASEAPI
_Must_inspect_result_
BOOL
WINAPI
ReadFile (
    _In_ HANDLE hFile,
    _Out_writes_bytes_to_opt_(nNumberOfBytesToRead, *lpNumberOfBytesRead) __out_data_source(FILE) LPVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToRead,
    _Out_opt_ LPDWORD lpNumberOfBytesRead,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
    )
{
	NTSTATUS Status;

	PAGED_CODE();

	LdkGetConsoleHandle( hFile,
						 &hFile );

	if (LdkIsConsoleHandle( hFile )) {
		if (ReadConsoleA( hFile,
						  lpBuffer,
						  nNumberOfBytesToRead,
						  lpNumberOfBytesRead,
						  NULL )) {
 			DWORD InputMode;
			Status = STATUS_SUCCESS;
			if (! GetConsoleMode( hFile,
								  &InputMode )) {
				InputMode = 0;
			}
			if (InputMode & ENABLE_PROCESSED_INPUT) {
				try {
					if (*(PCHAR)lpBuffer == 0x1A) {
						*lpNumberOfBytesRead = 0;
					}
				} except (EXCEPTION_EXECUTE_HANDLER) {
					Status = GetExceptionCode();
				}
			}
			if (NT_SUCCESS(Status)) {
				return TRUE;
			} else {
				LdkSetLastNTError( Status );
				return FALSE;
			}
		}
	}

	if (ARGUMENT_PRESENT(lpNumberOfBytesRead)) {
		*lpNumberOfBytesRead = 0;
	}

	if (ARGUMENT_PRESENT(lpOverlapped)) {
		LARGE_INTEGER ByteOffset;
		ByteOffset.LowPart = lpOverlapped->Offset;
		ByteOffset.HighPart = lpOverlapped->OffsetHigh;

		lpOverlapped->Internal = (DWORD)STATUS_PENDING;

		Status = ZwReadFile( hFile,
							 lpOverlapped->hEvent,
							 NULL,
							 (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
							 (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
							 lpBuffer,
							 nNumberOfBytesToRead,
							 &ByteOffset,
							 NULL );

		if (NT_SUCCESS(Status) && Status != STATUS_PENDING) {
			if (ARGUMENT_PRESENT(lpNumberOfBytesRead)) {
				try {
					*lpNumberOfBytesRead = (DWORD)lpOverlapped->InternalHigh;
				} except(EXCEPTION_EXECUTE_HANDLER) {
					*lpNumberOfBytesRead = 0;
				}
			}
			return TRUE;
		} else if (Status == STATUS_END_OF_FILE) {
			if (ARGUMENT_PRESENT(lpNumberOfBytesRead)) {
				*lpNumberOfBytesRead = 0;
			}
			LdkSetLastNTError( Status );
			return FALSE;
		} else {
			LdkSetLastNTError( Status );
			return FALSE;
		}
	} else {
		IO_STATUS_BLOCK IoStatus;

		Status = ZwReadFile( hFile,
							 NULL,
							 NULL,
							 NULL,
							 &IoStatus,
							 lpBuffer,
							 nNumberOfBytesToRead,
							 NULL,
							 NULL );

		if (Status == STATUS_PENDING) {
			Status = ZwWaitForSingleObject( hFile,
											FALSE,
											NULL );

			if (NT_SUCCESS(Status)) {
				Status = IoStatus.Status;
			}
		}

		if (NT_SUCCESS(Status)) {
			*lpNumberOfBytesRead = (DWORD)IoStatus.Information;
			return TRUE;
		} else if (Status == STATUS_END_OF_FILE) {
			*lpNumberOfBytesRead = 0;
			return TRUE;
		} else {
			if (NT_WARNING(Status)) {
				*lpNumberOfBytesRead = (DWORD)IoStatus.Information;
			}
			LdkSetLastNTError( Status );
			return FALSE;
		}
	}
}

WINBASEAPI
BOOL
WINAPI
WriteFile (
    _In_ HANDLE hFile,
    _In_reads_bytes_opt_(nNumberOfBytesToWrite) LPCVOID lpBuffer,
    _In_ DWORD nNumberOfBytesToWrite,
    _Out_opt_ LPDWORD lpNumberOfBytesWritten,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
    )
{
	NTSTATUS Status;

	PAGED_CODE();

	LdkGetConsoleHandle( hFile,
						 &hFile );

	if (LdkIsConsoleHandle( hFile )) {
		return WriteConsoleA( hFile,
							  lpBuffer,
							  nNumberOfBytesToWrite,
							  lpNumberOfBytesWritten,
							  NULL );
	}

	if (ARGUMENT_PRESENT(lpNumberOfBytesWritten)) {
		*lpNumberOfBytesWritten = 0;
	}

	if (ARGUMENT_PRESENT(lpOverlapped)) {

		LARGE_INTEGER ByteOffset;
		ByteOffset.LowPart = lpOverlapped->Offset;
		ByteOffset.HighPart = lpOverlapped->OffsetHigh;

		lpOverlapped->Internal = (DWORD)STATUS_PENDING;

		Status = ZwWriteFile( hFile,
							  lpOverlapped->hEvent,
							  NULL,
							  (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
							  (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
							  (PVOID)lpBuffer,
							  nNumberOfBytesToWrite,
							  &ByteOffset,
							  NULL );

		if (NT_SUCCESS(Status) && Status != STATUS_PENDING) {
			if (ARGUMENT_PRESENT(lpNumberOfBytesWritten)) {
				try {
					*lpNumberOfBytesWritten = (DWORD)lpOverlapped->InternalHigh;
				} except(EXCEPTION_EXECUTE_HANDLER) {
					*lpNumberOfBytesWritten = 0;
				}
			}
			return TRUE;
		} else {
			LdkSetLastNTError( Status );
			return FALSE;
		}
	} else {
		IO_STATUS_BLOCK IoStatus;

		Status = ZwWriteFile( hFile,
							  NULL,
							  NULL,
							  NULL,
							  &IoStatus,
							  (PVOID)lpBuffer,
							  nNumberOfBytesToWrite,
							  NULL,
							  NULL);

		if (Status == STATUS_PENDING) {
			Status = ZwWaitForSingleObject( hFile,
											FALSE,
											NULL );

			if (NT_SUCCESS(Status)) {
				Status = IoStatus.Status;
			}
		}

		if (NT_SUCCESS(Status)) {
			*lpNumberOfBytesWritten = (DWORD)IoStatus.Information;
			return TRUE;
		} else {
			if (NT_WARNING(Status)) {
				*lpNumberOfBytesWritten = (DWORD)IoStatus.Information;
			}
			LdkSetLastNTError( Status );
			return FALSE;
		}
	}
}

WINBASEAPI
BOOL
WINAPI
FlushFileBuffers (
    _In_ HANDLE hFile
    )
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;

	PAGED_CODE();

	Status = ZwFlushBuffersFile( hFile,
								 &IoStatus );

	if (NT_SUCCESS(Status)) {
		return TRUE;
	}

	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
SetEndOfFile(
    _In_ HANDLE hFile
    )
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;
	FILE_POSITION_INFORMATION PositionInfo;
	FILE_END_OF_FILE_INFORMATION EndOfFile;
	FILE_ALLOCATION_INFORMATION AllocationInfo;
	
	PAGED_CODE();

	Status = ZwQueryInformationFile( hFile,
									 &IoStatus,
									 &PositionInfo,
									 sizeof(FILE_POSITION_INFORMATION),
									 FilePositionInformation );

	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FALSE;
	}

	EndOfFile.EndOfFile = PositionInfo.CurrentByteOffset;
	Status = ZwSetInformationFile( hFile,
								   &IoStatus,
								   &EndOfFile,
								   sizeof(FILE_END_OF_FILE_INFORMATION),
								   FileEndOfFileInformation );
	
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FALSE;
	}

	AllocationInfo.AllocationSize = PositionInfo.CurrentByteOffset;

	Status = ZwSetInformationFile( hFile,
								   &IoStatus,
								   &AllocationInfo,
								   sizeof(FILE_ALLOCATION_INFORMATION),
								   FileAllocationInformation );
	
	if (NT_SUCCESS(Status)) {
		return TRUE;
	} else {
		LdkSetLastNTError( Status );
		return FALSE;
	}
}

WINBASEAPI
DWORD
WINAPI
SetFilePointer (
    _In_ HANDLE hFile,
    _In_ LONG lDistanceToMove,
    _Inout_opt_ PLONG lpDistanceToMoveHigh,
    _In_ DWORD dwMoveMethod
    )
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;

	FILE_POSITION_INFORMATION PositionInfo;
	FILE_POSITION_INFORMATION CurrentPosition;
	FILE_STANDARD_INFORMATION StandardInfo;

	PAGED_CODE();

	if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)) {
		PositionInfo.CurrentByteOffset.HighPart = *lpDistanceToMoveHigh;
		PositionInfo.CurrentByteOffset.LowPart = lDistanceToMove;
	} else {
		PositionInfo.CurrentByteOffset.QuadPart = lDistanceToMove;
	}

	switch (dwMoveMethod) {
	case FILE_BEGIN:
		break;

	case FILE_CURRENT:
		Status = ZwQueryInformationFile( hFile,
										 &IoStatus,
										 &CurrentPosition,
										 sizeof(FILE_POSITION_INFORMATION),
										 FilePositionInformation );

		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return (DWORD)-1;
		}
		PositionInfo.CurrentByteOffset.QuadPart += CurrentPosition.CurrentByteOffset.QuadPart;
		break;

	case FILE_END:
		Status = ZwQueryInformationFile( hFile,
										 &IoStatus,
										 &StandardInfo,
										 sizeof(FILE_STANDARD_INFORMATION),
										 FileStandardInformation );

		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return (DWORD)-1;
		}
		PositionInfo.CurrentByteOffset.QuadPart += StandardInfo.EndOfFile.QuadPart;
		break;

	default:
		SetLastError( ERROR_INVALID_PARAMETER );
		return (DWORD)-1;
		break;
	}

	if (PositionInfo.CurrentByteOffset.QuadPart < 0) {
		SetLastError( ERROR_NEGATIVE_SEEK );
		return (DWORD)-1;
	}

	if (!ARGUMENT_PRESENT(lpDistanceToMoveHigh) &&
		(PositionInfo.CurrentByteOffset.HighPart & MAXLONG)) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return (DWORD)-1;
	}

	Status = ZwSetInformationFile( hFile,
								   &IoStatus,
								   &PositionInfo,
								   sizeof(FILE_POSITION_INFORMATION),
								   FilePositionInformation );

	if (NT_SUCCESS(Status)) {
		if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)) {
			*lpDistanceToMoveHigh = PositionInfo.CurrentByteOffset.HighPart;
		}

		if (PositionInfo.CurrentByteOffset.LowPart == -1) {
			SetLastError(0);
		}

		return PositionInfo.CurrentByteOffset.LowPart;
	} else {
		LdkSetLastNTError( Status );

		if (ARGUMENT_PRESENT(lpDistanceToMoveHigh)) {
			*lpDistanceToMoveHigh = -1;
		}

		return (DWORD)-1;
	}
}

WINBASEAPI
BOOL
WINAPI
SetFilePointerEx (
    _In_ HANDLE hFile,
    _In_ LARGE_INTEGER liDistanceToMove,
    _Out_opt_ PLARGE_INTEGER lpNewFilePointer,
    _In_ DWORD dwMoveMethod
    )
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;

	FILE_POSITION_INFORMATION PositionInfo;
	FILE_POSITION_INFORMATION CurrentPosition;
	FILE_STANDARD_INFORMATION StandardInfo;

	PAGED_CODE();

	PositionInfo.CurrentByteOffset.QuadPart = liDistanceToMove.QuadPart;

	switch (dwMoveMethod) {
	case FILE_BEGIN:
		break;

	case FILE_CURRENT:
		Status = ZwQueryInformationFile( hFile,
										 &IoStatus,
										 &CurrentPosition,
										 sizeof(FILE_POSITION_INFORMATION),
										 FilePositionInformation );

		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return (DWORD)-1;
		}

		PositionInfo.CurrentByteOffset.QuadPart += CurrentPosition.CurrentByteOffset.QuadPart;
		break;

	case FILE_END:
		Status = ZwQueryInformationFile( hFile,
										 &IoStatus,
										 &StandardInfo,
										 sizeof(FILE_STANDARD_INFORMATION),
										 FileStandardInformation );

		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return (DWORD)-1;
		}

		PositionInfo.CurrentByteOffset.QuadPart += StandardInfo.EndOfFile.QuadPart;
		break;

	default:
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
		break;
	}

	if (PositionInfo.CurrentByteOffset.QuadPart < 0) {
		SetLastError( ERROR_NEGATIVE_SEEK );
		return FALSE;
	}

	Status = ZwSetInformationFile( hFile,
								   &IoStatus,
								   &PositionInfo,
								   sizeof(FILE_POSITION_INFORMATION),
								   FilePositionInformation );

	if (NT_SUCCESS(Status)) {
		if (ARGUMENT_PRESENT(lpNewFilePointer)) {
			*lpNewFilePointer = PositionInfo.CurrentByteOffset;
		}

		if (PositionInfo.CurrentByteOffset.LowPart == -1) {
			SetLastError(0);
		}

		return TRUE;
	} else {
		LdkSetLastNTError( Status );
		return FALSE;
	}
}

WINBASEAPI
DWORD
WINAPI
GetFileAttributesA (
    _In_ LPCSTR lpFileName
    )
{
    PUNICODE_STRING Unicode;

	PAGED_CODE();

    Unicode = Ldk8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return (DWORD)-1;
    }

    return GetFileAttributesW( (LPCWSTR)Unicode->Buffer );
}

WINBASEAPI
DWORD
WINAPI
GetFileAttributesW (
    _In_ LPCWSTR lpFileName
    )
{
	FILE_NETWORK_OPEN_INFORMATION FileInformation;

	PAGED_CODE();

	if (LdkpQueryFullFileAttributes( lpFileName,
                                     &FileInformation )) {
		return FileInformation.FileAttributes;
	}

	return (DWORD)-1;
}

WINBASEAPI
BOOL
WINAPI
GetFileAttributesExW (
    _In_ LPCWSTR lpFileName,
    _In_ GET_FILEEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FILE_ATTRIBUTE_DATA)) LPVOID lpFileInformation
    )
{
    FILE_NETWORK_OPEN_INFORMATION FileInformation;
    LPWIN32_FILE_ATTRIBUTE_DATA AttributeData;

	PAGED_CODE();

    if (lpFileInformation == NULL || fInfoLevelId != GetFileExInfoStandard) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (! LdkpQueryFullFileAttributes( lpFileName,
                                       &FileInformation )) {
        return FALSE;
    }

    AttributeData = (LPWIN32_FILE_ATTRIBUTE_DATA)lpFileInformation;
    RtlZeroMemory( AttributeData,
                   sizeof(*AttributeData) );

    AttributeData->dwFileAttributes = FileInformation.FileAttributes;
    AttributeData->ftCreationTime = *(LPFILETIME)&FileInformation.CreationTime;
    AttributeData->ftLastAccessTime = *(LPFILETIME)&FileInformation.LastAccessTime;
    AttributeData->ftLastWriteTime = *(LPFILETIME)&FileInformation.LastWriteTime;
    AttributeData->nFileSizeHigh = FileInformation.EndOfFile.HighPart;
    AttributeData->nFileSizeLow = FileInformation.EndOfFile.LowPart;
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
SetFileAttributesW (
    _In_ LPCWSTR lpFileName,
    _In_ DWORD dwFileAttributes
    )
{
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatus;
    FILE_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;

	PAGED_CODE();

    Handle = CreateFileW( lpFileName,
                          FILE_WRITE_ATTRIBUTES,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                          NULL );
    if (Handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    RtlZeroMemory( &BasicInfo,
                   sizeof(BasicInfo) );
    BasicInfo.FileAttributes = dwFileAttributes;

    Status = ZwSetInformationFile( Handle,
                                   &IoStatus,
                                   &BasicInfo,
                                   sizeof(BasicInfo),
                                   FileBasicInformation );

    CloseHandle( Handle );

    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

    LdkSetLastNTError( Status );
    return FALSE;
}

#include <limits.h>

WINBASEAPI
DWORD
WINAPI
GetFileSize (
    _In_ HANDLE hFile,
    _Out_opt_ LPDWORD lpFileSizeHigh
    )
{
	LARGE_INTEGER fileSize;

	PAGED_CODE();

	if (GetFileSizeEx(hFile,&fileSize)) {
		if (ARGUMENT_PRESENT(lpFileSizeHigh)) {
			*lpFileSizeHigh = (DWORD)fileSize.HighPart;
		}
		if (fileSize.LowPart == ULONG_MAX) {
			SetLastError( ERROR_SUCCESS );
		}
	} else {
		fileSize.LowPart = ULONG_MAX;
	}
	return fileSize.LowPart;
}

WINBASEAPI
BOOL
WINAPI
GetFileSizeEx (
    _In_ HANDLE hFile,
    _Out_ PLARGE_INTEGER lpFileSize
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION StandardInfo;

	PAGED_CODE();

    Status = ZwQueryInformationFile( hFile,
									 &IoStatus,
									 &StandardInfo,
									 sizeof(StandardInfo),
									 FileStandardInformation );

    if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
        return FALSE;
	}
	*lpFileSize = StandardInfo.EndOfFile;
	return TRUE;
}

WINBASEAPI
DWORD
WINAPI
GetFileType (
    _In_ HANDLE hFile
    )
{
	NTSTATUS Status;
	FILE_FS_DEVICE_INFORMATION DeviceInformation;
	IO_STATUS_BLOCK IoStatus;

	PAGED_CODE();

	LdkGetConsoleHandle( hFile,
						 &hFile );

	if (LdkIsConsoleHandle( hFile )) {
		return FILE_TYPE_CHAR;
	}

	Status = ZwQueryVolumeInformationFile( hFile,
										   &IoStatus,
										   &DeviceInformation,
										   sizeof(FILE_FS_DEVICE_INFORMATION),
										   FileFsDeviceInformation );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FILE_TYPE_UNKNOWN;
	}

	switch (DeviceInformation.DeviceType) {
	case FILE_DEVICE_SCREEN:
	case FILE_DEVICE_KEYBOARD:
	case FILE_DEVICE_MOUSE:
	case FILE_DEVICE_PARALLEL_PORT:
	case FILE_DEVICE_PRINTER:
	case FILE_DEVICE_SERIAL_PORT:
	case FILE_DEVICE_MODEM:
	case FILE_DEVICE_SOUND:
	case FILE_DEVICE_NULL:
		return FILE_TYPE_CHAR;

	case FILE_DEVICE_CD_ROM:
	case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
	case FILE_DEVICE_CONTROLLER:
	case FILE_DEVICE_DATALINK:
	case FILE_DEVICE_DFS:
	case FILE_DEVICE_DISK:
	case FILE_DEVICE_DISK_FILE_SYSTEM:
	case FILE_DEVICE_VIRTUAL_DISK:
		return FILE_TYPE_DISK;

	case FILE_DEVICE_NAMED_PIPE:
		return FILE_TYPE_PIPE;

	case FILE_DEVICE_NETWORK:
	case FILE_DEVICE_NETWORK_FILE_SYSTEM:
	case FILE_DEVICE_PHYSICAL_NETCARD:
	case FILE_DEVICE_TAPE:
	case FILE_DEVICE_TAPE_FILE_SYSTEM:
	case FILE_DEVICE_TRANSPORT:
	case FILE_DEVICE_UNKNOWN:
	default:
		SetLastError( NO_ERROR );
		return FILE_TYPE_UNKNOWN;
	}
}

WINBASEAPI
BOOL
WINAPI
SetFileTime (
    _In_ HANDLE hFile,
    _In_opt_ CONST FILETIME * lpCreationTime,
    _In_opt_ CONST FILETIME * lpLastAccessTime,
    _In_opt_ CONST FILETIME * lpLastWriteTime
    )
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;
	FILE_BASIC_INFORMATION BasicInfo;

	PAGED_CODE();

	RtlZeroMemory( &BasicInfo,
				   sizeof(BasicInfo) );

	if (ARGUMENT_PRESENT( lpCreationTime )) {
		BasicInfo.CreationTime.LowPart = lpCreationTime->dwLowDateTime;
		BasicInfo.CreationTime.HighPart = lpCreationTime->dwHighDateTime;
	}

	if (ARGUMENT_PRESENT(lpLastAccessTime)) {
		BasicInfo.LastAccessTime.LowPart = lpLastAccessTime->dwLowDateTime;
		BasicInfo.LastAccessTime.HighPart = lpLastAccessTime->dwHighDateTime;
	}

	if (ARGUMENT_PRESENT(lpLastWriteTime)) {
		BasicInfo.LastWriteTime.LowPart = lpLastWriteTime->dwLowDateTime;
		BasicInfo.LastWriteTime.HighPart = lpLastWriteTime->dwHighDateTime;
	}

	Status = ZwSetInformationFile( hFile,
								   &IoStatus,
								   &BasicInfo,
								   sizeof(BasicInfo),
								   FileBasicInformation );

	if (NT_SUCCESS(Status)) {
		return TRUE;
	} else {
		LdkSetLastNTError( Status );
		return FALSE;
	}
}

WINBASEAPI
BOOL
WINAPI
LocalFileTimeToFileTime (
    _In_ CONST FILETIME * lpLocalFileTime,
    _Out_ LPFILETIME lpFileTime
    )
{
	LARGE_INTEGER FileTime;
	LARGE_INTEGER LocalFileTime;
	LARGE_INTEGER Bias;

	do {
		Bias.HighPart = SharedUserData->TimeZoneBias.High1Time;
		Bias.LowPart = SharedUserData->TimeZoneBias.LowPart;
	} while (Bias.HighPart != SharedUserData->TimeZoneBias.High2Time);
	
	LocalFileTime.LowPart = lpLocalFileTime->dwLowDateTime;
	LocalFileTime.HighPart = lpLocalFileTime->dwHighDateTime;
	
	FileTime.QuadPart = LocalFileTime.QuadPart + Bias.QuadPart;

	lpFileTime->dwLowDateTime = FileTime.LowPart;
	lpFileTime->dwHighDateTime = FileTime.HighPart;

	return TRUE;
}



WINBASEAPI
BOOL
WINAPI
LockFile (
    _In_ HANDLE hFile,
    _In_ DWORD dwFileOffsetLow,
    _In_ DWORD dwFileOffsetHigh,
    _In_ DWORD nNumberOfBytesToLockLow,
	_In_ DWORD nNumberOfBytesToLockHigh
    )
{
	PAGED_CODE();

	if (LdkIsConsoleHandle( hFile )) {
		LdkSetLastNTError( STATUS_INVALID_HANDLE );
		return FALSE;
	}

	LARGE_INTEGER ByteOffset;
	ByteOffset.LowPart = dwFileOffsetLow;
	ByteOffset.HighPart = dwFileOffsetHigh;

	LARGE_INTEGER Length;
	Length.LowPart = nNumberOfBytesToLockLow;
	Length.HighPart = nNumberOfBytesToLockHigh;

	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status = ZwLockFile( hFile,
								  NULL,
								  NULL,
								  NULL,
								  &IoStatus,
								  &ByteOffset,
								  &Length,
								  0,
								  TRUE,
								  TRUE );
	if (Status == STATUS_PENDING) {
		Status = ZwWaitForSingleObject( hFile,
										FALSE,
										NULL );
		if (NT_SUCCESS(Status)) {
			Status = IoStatus.Status;
		}
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
LockFileEx (
    _In_ HANDLE hFile,
    _In_ DWORD dwFlags,
    _Reserved_ DWORD dwReserved,
    _In_ DWORD nNumberOfBytesToLockLow,
    _In_ DWORD nNumberOfBytesToLockHigh,
    _Inout_ LPOVERLAPPED lpOverlapped
    )
{
	PAGED_CODE();

	if (LdkIsConsoleHandle( hFile )) {
		LdkSetLastNTError( STATUS_INVALID_HANDLE );
		return FALSE;
	}
	if (dwReserved) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	lpOverlapped->Internal = (DWORD)STATUS_PENDING;

	LARGE_INTEGER ByteOffset;
    ByteOffset.LowPart = lpOverlapped->Offset;
    ByteOffset.HighPart = lpOverlapped->OffsetHigh;

	LARGE_INTEGER Length;
	Length.LowPart = nNumberOfBytesToLockLow;
	Length.HighPart = nNumberOfBytesToLockHigh;

	NTSTATUS Status = ZwLockFile( hFile,
								  lpOverlapped->hEvent,
								  NULL,
								  (ULONG_PTR)lpOverlapped->hEvent & 1 ? NULL : lpOverlapped,
								  (PIO_STATUS_BLOCK)&lpOverlapped->Internal,
								  &ByteOffset,
								  &Length,
								  0,
								  BooleanFlagOn(dwFlags, LOCKFILE_FAIL_IMMEDIATELY),
								  BooleanFlagOn(dwFlags, LOCKFILE_EXCLUSIVE_LOCK) );
	if (NT_SUCCESS(Status) && Status != STATUS_PENDING) {
		return TRUE;
	}
	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
UnlockFile	(
    _In_ HANDLE hFile,
    _In_ DWORD dwFileOffsetLow,
    _In_ DWORD dwFileOffsetHigh,
    _In_ DWORD nNumberOfBytesToUnlockLow,
    _In_ DWORD nNumberOfBytesToUnlockHigh
    )
{
	PAGED_CODE();

	if (LdkIsConsoleHandle( hFile )) {
		LdkSetLastNTError( STATUS_INVALID_HANDLE );
		return FALSE;
	}

	LARGE_INTEGER ByteOffset;
	ByteOffset.LowPart = dwFileOffsetLow;
	ByteOffset.HighPart = dwFileOffsetHigh;

	LARGE_INTEGER Length;
	Length.LowPart = nNumberOfBytesToUnlockLow;
	Length.HighPart = nNumberOfBytesToUnlockHigh;

	IO_STATUS_BLOCK IoStatus;
	NTSTATUS Status = ZwUnlockFile( hFile,
								    &IoStatus,
									&ByteOffset,
									&Length,
									0 );
	if (Status == STATUS_PENDING) {
		Status = ZwWaitForSingleObject( hFile,
										FALSE,
										NULL );
		if (NT_SUCCESS(Status)) {
			Status = IoStatus.Status;
		}
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
UnlockFileEx(
    _In_ HANDLE hFile,
    _Reserved_ DWORD dwReserved,
    _In_ DWORD nNumberOfBytesToUnlockLow,
    _In_ DWORD nNumberOfBytesToUnlockHigh,
    _Inout_ LPOVERLAPPED lpOverlapped
    )
{
	PAGED_CODE();

	if (LdkIsConsoleHandle( hFile )) {
		LdkSetLastNTError( STATUS_INVALID_HANDLE );
		return FALSE;
	}

	if (dwReserved) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

    LARGE_INTEGER ByteOffset;
	ByteOffset.LowPart = lpOverlapped->Offset;
	ByteOffset.HighPart = lpOverlapped->OffsetHigh;

    LARGE_INTEGER Length;
	Length.LowPart = nNumberOfBytesToUnlockLow;
	Length.HighPart = nNumberOfBytesToUnlockHigh;

	NTSTATUS Status = ZwUnlockFile( hFile,
						   			(PIO_STATUS_BLOCK)&lpOverlapped->Internal,
						   			&ByteOffset,
						   			&Length,
						   			0 );
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}
	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
DeleteFileA (
    _In_ LPCSTR lpFileName
    )
{
    PUNICODE_STRING Unicode;

	PAGED_CODE();

    Unicode = Ldk8BitStringToStaticUnicodeString( lpFileName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return DeleteFileW( (LPCWSTR)Unicode->Buffer );
}

WINBASEAPI
BOOL
WINAPI
DeleteFileW (
    _In_ LPCWSTR lpFileName
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatus;
    FILE_DISPOSITION_INFORMATION Disposition;
    FILE_ATTRIBUTE_TAG_INFORMATION FileTagInformation;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer;
    BOOLEAN fIsSymbolicLink = FALSE;

	PAGED_CODE();

    TranslationStatus = RtlDosPathNameToNtPathName_U( lpFileName,
													  &FileName,
													  NULL,
													  &RelativeName );

    if (! TranslationStatus) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    FreeBuffer = FileName.Buffer;
    if (RelativeName.RelativeName.Length) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    } else {
		RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes( &ObjectAttributes,
								&FileName,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								RelativeName.ContainingDirectory,
								NULL );

    Status = ZwOpenFile( &Handle,
						 (ACCESS_MASK)DELETE | FILE_READ_ATTRIBUTES,
						 &ObjectAttributes,
						 &IoStatus,
						 FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
						 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT );
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_INVALID_PARAMETER) {
            Status = ZwOpenFile( &Handle,
								 (ACCESS_MASK)DELETE,
								 &ObjectAttributes,
								 &IoStatus,
								 FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
								 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT );
            if (! NT_SUCCESS(Status)) {
                RtlFreeHeap( RtlProcessHeap(),
							 0,
							 FreeBuffer );
                LdkSetLastNTError( Status );
                return FALSE;
            }
        } else {

            if (Status != STATUS_ACCESS_DENIED) {
                RtlFreeHeap( RtlProcessHeap(),
							 0,
							 FreeBuffer );
                LdkSetLastNTError( Status );
                return FALSE;
            }
            
            Status = ZwOpenFile( &Handle,
								 (ACCESS_MASK)DELETE,
								 &ObjectAttributes,
								 &IoStatus,
								 FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
								 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT );
            if (! NT_SUCCESS(Status)) {
                RtlFreeHeap( RtlProcessHeap(),
							 0,
							 FreeBuffer );
                LdkSetLastNTError( Status );
                return FALSE;
            }
        }
    } else {
        Status = ZwQueryInformationFile( Handle,
										 &IoStatus,
										 (PVOID)&FileTagInformation,
										 sizeof(FileTagInformation),
										 FileAttributeTagInformation );
        if (! NT_SUCCESS(Status)) {
            if ((Status != STATUS_NOT_IMPLEMENTED) &&
                (Status != STATUS_INVALID_PARAMETER)) {
                RtlFreeHeap( RtlProcessHeap(),
							 0,
							 FreeBuffer );
                ZwClose( Handle );
                LdkSetLastNTError( Status );
                return FALSE;
            }
        }
        if (NT_SUCCESS(Status) &&
            (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
            if ((FileTagInformation.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) ||
                (FileTagInformation.ReparseTag == IO_REPARSE_TAG_SYMLINK)) {
                fIsSymbolicLink = TRUE;
            }
        }
        if (NT_SUCCESS(Status) &&
            (FileTagInformation.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) &&
            !fIsSymbolicLink) {
            ZwClose(Handle);
            Status = ZwOpenFile( &Handle,
								 (ACCESS_MASK)DELETE,
                        		 &ObjectAttributes,
                        		 &IoStatus,
                        		 FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                        		 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT );
            if (! NT_SUCCESS(Status)) {
                if (Status == STATUS_IO_REPARSE_TAG_NOT_HANDLED) {
                    Status = NtOpenFile( &Handle,
                                		 (ACCESS_MASK)DELETE,
                                		 &ObjectAttributes,
                                		 &IoStatus,
                                		 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                		 FILE_NON_DIRECTORY_FILE | FILE_OPEN_FOR_BACKUP_INTENT | FILE_OPEN_REPARSE_POINT );
                }
                if (! NT_SUCCESS(Status)) {
                    RtlFreeHeap( RtlProcessHeap(),
								 0,
								 FreeBuffer );
                    LdkSetLastNTError( Status );
                    return FALSE;
				}
			}
		}
	}
	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 FreeBuffer );

    Disposition.DeleteFile = TRUE;

    Status = ZwSetInformationFile( Handle,
								   &IoStatus,
								   &Disposition,
                				   sizeof(Disposition),
								   FileDispositionInformation );
    ZwClose(Handle);
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
RemoveDirectoryA (
    _In_ LPCSTR lpPathName
    )
{
    PUNICODE_STRING Unicode;

	PAGED_CODE();

    Unicode = Ldk8BitStringToStaticUnicodeString( lpPathName );
    if (Unicode == NULL) {
        return FALSE;
    }

    return RemoveDirectoryW( (LPCWSTR)Unicode->Buffer );
}

WINBASEAPI
BOOL
WINAPI
RemoveDirectoryW (
    _In_ LPCWSTR lpPathName
    )
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatus;
    FILE_DISPOSITION_INFORMATION Disposition;
    BOOLEAN TranslationStatus;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer;

	PAGED_CODE();

    if (lpPathName == NULL) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    TranslationStatus = RtlDosPathNameToNtPathName_U( lpPathName,
                                                      &FileName,
                                                      NULL,
                                                      &RelativeName );
    if (! TranslationStatus) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    FreeBuffer = FileName.Buffer;
    if (RelativeName.RelativeName.Length) {
        FileName = *(PUNICODE_STRING)&RelativeName.RelativeName;
    } else {
        RelativeName.ContainingDirectory = NULL;
    }

    InitializeObjectAttributes( &ObjectAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                RelativeName.ContainingDirectory,
                                NULL );

    Status = ZwOpenFile( &Handle,
                         DELETE | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_DIRECTORY_FILE |
                             FILE_SYNCHRONOUS_IO_NONALERT |
                             FILE_OPEN_FOR_BACKUP_INTENT |
                             FILE_OPEN_REPARSE_POINT );

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 FreeBuffer );

    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    Disposition.DeleteFile = TRUE;

    Status = ZwSetInformationFile( Handle,
                                   &IoStatus,
                                   &Disposition,
                                   sizeof(Disposition),
                                   FileDispositionInformation );
    ZwClose( Handle );

    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

    LdkSetLastNTError( Status );
    return FALSE;
}

#define LDK_COPY_FILE_BUFFER_SIZE (64 * 1024)

WINBASEAPI
BOOL
WINAPI
CopyFileW (
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName,
    _In_ BOOL bFailIfExists
    )
{
    DWORD SourceAttributes;
    HANDLE SourceHandle;
    HANDLE DestinationHandle;
    PVOID Buffer;
    BOOL Result = FALSE;
    DWORD BytesRead;
    DWORD BytesWritten;

	PAGED_CODE();

    SourceAttributes = GetFileAttributesW( lpExistingFileName );
    if (SourceAttributes == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }

    if (FlagOn(SourceAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    SourceHandle = CreateFileW( lpExistingFileName,
                                GENERIC_READ,
                                FILE_SHARE_READ,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
    if (SourceHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    DestinationHandle = CreateFileW( lpNewFileName,
                                     GENERIC_WRITE,
                                     0,
                                     NULL,
                                     bFailIfExists ? CREATE_NEW : CREATE_ALWAYS,
                                     FILE_ATTRIBUTE_NORMAL,
                                     SourceHandle );
    if (DestinationHandle == INVALID_HANDLE_VALUE) {
        CloseHandle( SourceHandle );
        return FALSE;
    }

    Buffer = RtlAllocateHeap( RtlProcessHeap(),
                              MAKE_TAG( TMP_TAG ),
                              LDK_COPY_FILE_BUFFER_SIZE );
    if (Buffer == NULL) {
        LdkSetLastNTError( STATUS_NO_MEMORY );
        goto Exit;
    }

    for (;;) {
        if (! ReadFile( SourceHandle,
                        Buffer,
                        LDK_COPY_FILE_BUFFER_SIZE,
                        &BytesRead,
                        NULL )) {
            goto Exit;
        }

        if (BytesRead == 0) {
            Result = TRUE;
            goto Exit;
        }

        if (! WriteFile( DestinationHandle,
                         Buffer,
                         BytesRead,
                         &BytesWritten,
                         NULL )) {
            goto Exit;
        }

        if (BytesWritten != BytesRead) {
            SetLastError( ERROR_WRITE_FAULT );
            goto Exit;
        }
    }

Exit:
    if (Buffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     Buffer );
    }
    CloseHandle( DestinationHandle );
    CloseHandle( SourceHandle );
    return Result;
}

WINBASEAPI
BOOL
WINAPI
CreateDirectoryExW (
    _In_ LPCWSTR lpTemplateDirectory,
    _In_ LPCWSTR lpNewDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    DWORD Attributes;

	PAGED_CODE();

    if (! CreateDirectoryW( lpNewDirectory,
                            lpSecurityAttributes )) {
        return FALSE;
    }

    if (ARGUMENT_PRESENT(lpTemplateDirectory)) {
        Attributes = GetFileAttributesW( lpTemplateDirectory );
        if (Attributes != INVALID_FILE_ATTRIBUTES) {
            (VOID)SetFileAttributesW( lpNewDirectory,
                                      Attributes );
        }
    }

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
CreateHardLinkW (
    _In_ LPCWSTR lpFileName,
    _In_ LPCWSTR lpExistingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    NTSTATUS Status;
    DWORD ExistingAttributes;
    HANDLE ExistingHandle;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING NewFileName;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer;
    PUNICODE_STRING LinkFileName;
    HANDLE RootDirectory;
    PFILE_LINK_INFORMATION LinkInfo;
    ULONG LinkInfoSize;

	PAGED_CODE();

    UNREFERENCED_PARAMETER( lpSecurityAttributes );

    if (lpFileName == NULL || lpExistingFileName == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    ExistingAttributes = GetFileAttributesW( lpExistingFileName );
    if (ExistingAttributes == INVALID_FILE_ATTRIBUTES) {
        return FALSE;
    }

    if (FlagOn(ExistingAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        SetLastError( ERROR_ACCESS_DENIED );
        return FALSE;
    }

    if (! RtlDosPathNameToNtPathName_U( lpFileName,
                                        &NewFileName,
                                        NULL,
                                        &RelativeName )) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    FreeBuffer = NewFileName.Buffer;
    if (RelativeName.RelativeName.Length) {
        LinkFileName = (PUNICODE_STRING)&RelativeName.RelativeName;
        RootDirectory = RelativeName.ContainingDirectory;
    } else {
        LinkFileName = &NewFileName;
        RootDirectory = NULL;
    }

    ExistingHandle = CreateFileW( lpExistingFileName,
                                  DELETE | SYNCHRONIZE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );
    if (ExistingHandle == INVALID_HANDLE_VALUE) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FreeBuffer );
        return FALSE;
    }

    LinkInfoSize = FIELD_OFFSET(FILE_LINK_INFORMATION, FileName) +
                   LinkFileName->Length;
    LinkInfo = RtlAllocateHeap( RtlProcessHeap(),
                                MAKE_TAG( TMP_TAG ),
                                LinkInfoSize );
    if (LinkInfo == NULL) {
        CloseHandle( ExistingHandle );
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FreeBuffer );
        LdkSetLastNTError( STATUS_NO_MEMORY );
        return FALSE;
    }

    RtlZeroMemory( LinkInfo,
                   LinkInfoSize );
    LinkInfo->ReplaceIfExists = FALSE;
    LinkInfo->RootDirectory = RootDirectory;
    LinkInfo->FileNameLength = LinkFileName->Length;
    RtlCopyMemory( LinkInfo->FileName,
                   LinkFileName->Buffer,
                   LinkFileName->Length );

    Status = ZwSetInformationFile( ExistingHandle,
                                   &IoStatus,
                                   LinkInfo,
                                   LinkInfoSize,
                                   FileLinkInformation );

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 LinkInfo );
    CloseHandle( ExistingHandle );
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 FreeBuffer );

    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
MoveFileExW (
    _In_ LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_ DWORD dwFlags
    )
{
    NTSTATUS Status;
    HANDLE ExistingHandle;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING NewFileName;
    RTL_RELATIVE_NAME_U RelativeName;
    PVOID FreeBuffer;
    PUNICODE_STRING RenameFileName;
    HANDLE RootDirectory;
    PFILE_RENAME_INFORMATION RenameInfo;
    ULONG RenameInfoSize;
    DWORD UnsupportedFlags;

	PAGED_CODE();

    UnsupportedFlags = dwFlags & ~(MOVEFILE_REPLACE_EXISTING |
                                   MOVEFILE_COPY_ALLOWED |
                                   MOVEFILE_WRITE_THROUGH);
    if (UnsupportedFlags != 0) {
        LdkSetLastNTError( STATUS_NOT_SUPPORTED );
        return FALSE;
    }

    if (lpExistingFileName == NULL || lpNewFileName == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (! RtlDosPathNameToNtPathName_U( lpNewFileName,
                                        &NewFileName,
                                        NULL,
                                        &RelativeName )) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }
    FreeBuffer = NewFileName.Buffer;
    if (RelativeName.RelativeName.Length) {
        RenameFileName = (PUNICODE_STRING)&RelativeName.RelativeName;
        RootDirectory = RelativeName.ContainingDirectory;
    } else {
        RenameFileName = &NewFileName;
        RootDirectory = NULL;
    }

    ExistingHandle = CreateFileW( lpExistingFileName,
                                  DELETE | SYNCHRONIZE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT,
                                  NULL );
    if (ExistingHandle == INVALID_HANDLE_VALUE) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FreeBuffer );
        return FALSE;
    }

    RenameInfoSize = FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName) +
                     RenameFileName->Length;
    RenameInfo = RtlAllocateHeap( RtlProcessHeap(),
                                  MAKE_TAG( TMP_TAG ),
                                  RenameInfoSize );
    if (RenameInfo == NULL) {
        CloseHandle( ExistingHandle );
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FreeBuffer );
        LdkSetLastNTError( STATUS_NO_MEMORY );
        return FALSE;
    }

    RtlZeroMemory( RenameInfo,
                   RenameInfoSize );
    RenameInfo->ReplaceIfExists = FlagOn(dwFlags, MOVEFILE_REPLACE_EXISTING);
    RenameInfo->RootDirectory = RootDirectory;
    RenameInfo->FileNameLength = RenameFileName->Length;
    RtlCopyMemory( RenameInfo->FileName,
                   RenameFileName->Buffer,
                   RenameFileName->Length );

    Status = ZwSetInformationFile( ExistingHandle,
                                   &IoStatus,
                                   RenameInfo,
                                   RenameInfoSize,
                                   FileRenameInformation );

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 RenameInfo );
    CloseHandle( ExistingHandle );
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 FreeBuffer );

    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

    LdkSetLastNTError( Status );
    return FALSE;
}

WINBASEAPI
BOOLEAN
WINAPI
CreateSymbolicLinkW (
    _In_ LPCWSTR lpSymlinkFileName,
    _In_ LPCWSTR lpTargetFileName,
    _In_ DWORD dwFlags
    )
{
    HANDLE SymlinkHandle;
    DWORD FileAttributes;
    DWORD BytesReturned;
    ULONG SubstituteNameLength;
    ULONG PrintNameLength;
    ULONG SubstituteNameStoredLength;
    ULONG PrintNameStoredLength;
    ULONG ReparseDataLength;
    ULONG ReparseBufferLength;
    ULONG ReparsePathDataLength;
    ULONG CreateFlags;
    DWORD SavedLastError;
    PREPARSE_DATA_BUFFER ReparseBuffer;
    PWSTR PathBuffer;
    UNICODE_STRING SubstituteName;
    UNICODE_STRING NtTargetName;
    RTL_PATH_TYPE TargetPathType;
    PVOID FreeTargetBuffer = NULL;
    BOOLEAN IsDirectory;
    BOOLEAN IsRelative;
    BOOLEAN Created = FALSE;

	PAGED_CODE();

    if (lpSymlinkFileName == NULL || lpTargetFileName == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (FlagOn(dwFlags, ~(SYMBOLIC_LINK_FLAG_DIRECTORY |
                          SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE))) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    IsDirectory = FlagOn( dwFlags,
                          SYMBOLIC_LINK_FLAG_DIRECTORY );
    TargetPathType = RtlDetermineDosPathNameType_U( lpTargetFileName );
    IsRelative = (TargetPathType == RtlPathTypeRelative);

    if (IsRelative) {
        RtlInitUnicodeString( &SubstituteName,
                              lpTargetFileName );
    } else {
        if (! RtlDosPathNameToNtPathName_U( lpTargetFileName,
                                            &NtTargetName,
                                            NULL,
                                            NULL )) {
            SetLastError( ERROR_PATH_NOT_FOUND );
            return FALSE;
        }

        FreeTargetBuffer = NtTargetName.Buffer;
        SubstituteName = NtTargetName;
    }

    RtlInitUnicodeString( &NtTargetName,
                          lpTargetFileName );
    PrintNameLength = NtTargetName.Length;
    SubstituteNameLength = SubstituteName.Length;
    SubstituteNameStoredLength = SubstituteNameLength + sizeof(WCHAR);
    PrintNameStoredLength = PrintNameLength + sizeof(WCHAR);

    if ((SubstituteNameLength > MAXUSHORT) ||
        (PrintNameLength > MAXUSHORT) ||
        (SubstituteNameStoredLength > MAXUSHORT) ||
        (PrintNameStoredLength > MAXUSHORT)) {
        if (FreeTargetBuffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         FreeTargetBuffer );
        }
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (IsDirectory) {
        if (! CreateDirectoryW( lpSymlinkFileName,
                                NULL )) {
            if (FreeTargetBuffer != NULL) {
                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             FreeTargetBuffer );
            }
            return FALSE;
        }
    } else {
        SymlinkHandle = CreateFileW( lpSymlinkFileName,
                                     GENERIC_WRITE,
                                     0,
                                     NULL,
                                     CREATE_NEW,
                                     FILE_ATTRIBUTE_NORMAL,
                                     NULL );
        if (SymlinkHandle == INVALID_HANDLE_VALUE) {
            if (FreeTargetBuffer != NULL) {
                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             FreeTargetBuffer );
            }
            return FALSE;
        }
        CloseHandle( SymlinkHandle );
    }

    Created = TRUE;
    FileAttributes = IsDirectory ? FILE_FLAG_BACKUP_SEMANTICS : FILE_ATTRIBUTE_NORMAL;
    CreateFlags = FileAttributes | FILE_FLAG_OPEN_REPARSE_POINT;
    SymlinkHandle = CreateFileW( lpSymlinkFileName,
                                 GENERIC_WRITE,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                 NULL,
                                 OPEN_EXISTING,
                                 CreateFlags,
                                 NULL );
    if (SymlinkHandle == INVALID_HANDLE_VALUE) {
        goto Failure;
    }

    ReparsePathDataLength =
        FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) -
        REPARSE_DATA_BUFFER_HEADER_SIZE;
    ReparseDataLength =
        ReparsePathDataLength +
        SubstituteNameStoredLength +
        PrintNameStoredLength;
    ReparseBufferLength = REPARSE_DATA_BUFFER_HEADER_SIZE + ReparseDataLength;
    if (ReparseBufferLength > MAXIMUM_REPARSE_DATA_BUFFER_SIZE) {
        CloseHandle( SymlinkHandle );
        SetLastError( ERROR_INVALID_PARAMETER );
        goto Failure;
    }

    ReparseBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                     MAKE_TAG( TMP_TAG ),
                                     ReparseBufferLength );
    if (ReparseBuffer == NULL) {
        CloseHandle( SymlinkHandle );
        LdkSetLastNTError( STATUS_NO_MEMORY );
        goto Failure;
    }

    RtlZeroMemory( ReparseBuffer,
                   ReparseBufferLength );
    ReparseBuffer->ReparseTag = IO_REPARSE_TAG_SYMLINK;
    ReparseBuffer->ReparseDataLength = (USHORT)ReparseDataLength;
    ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameOffset = 0;
    ReparseBuffer->SymbolicLinkReparseBuffer.SubstituteNameLength = (USHORT)SubstituteNameLength;
    ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameOffset = (USHORT)SubstituteNameStoredLength;
    ReparseBuffer->SymbolicLinkReparseBuffer.PrintNameLength = (USHORT)PrintNameLength;
    ReparseBuffer->SymbolicLinkReparseBuffer.Flags = IsRelative ? SYMLINK_FLAG_RELATIVE : 0;

    PathBuffer = ReparseBuffer->SymbolicLinkReparseBuffer.PathBuffer;
    RtlCopyMemory( PathBuffer,
                   SubstituteName.Buffer,
                   SubstituteNameLength );
    PathBuffer[SubstituteNameLength / sizeof(WCHAR)] = UNICODE_NULL;
    RtlCopyMemory( (PUCHAR)PathBuffer + SubstituteNameStoredLength,
                   lpTargetFileName,
                   PrintNameLength );
    *(PWSTR)((PUCHAR)PathBuffer + SubstituteNameStoredLength + PrintNameLength) =
        UNICODE_NULL;

    if (! DeviceIoControl( SymlinkHandle,
                           FSCTL_SET_REPARSE_POINT,
                           ReparseBuffer,
                           ReparseBufferLength,
                           NULL,
                           0,
                           &BytesReturned,
                           NULL )) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     ReparseBuffer );
        CloseHandle( SymlinkHandle );
        goto Failure;
    }

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 ReparseBuffer );
    CloseHandle( SymlinkHandle );

    if (FreeTargetBuffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FreeTargetBuffer );
    }

    return TRUE;

Failure:
    SavedLastError = GetLastError();

    if (Created) {
        if (IsDirectory) {
            RemoveDirectoryW( lpSymlinkFileName );
        } else {
            DeleteFileW( lpSymlinkFileName );
        }
    }

    if (FreeTargetBuffer != NULL) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FreeTargetBuffer );
    }

    SetLastError( SavedLastError );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetFileInformationByHandle (
    _In_ HANDLE hFile,
    _Out_ LPBY_HANDLE_FILE_INFORMATION lpFileInformation
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    BY_HANDLE_FILE_INFORMATION LocalFileInformation;
    FILE_ALL_INFORMATION FileInformation;
    FILE_FS_VOLUME_INFORMATION VolumeInfo;

	PAGED_CODE();

	LdkGetConsoleHandle( hFile,
						 &hFile );

    if (LdkIsConsoleHandle( hFile )) {
        LdkSetLastNTError( STATUS_INVALID_HANDLE );
        return FALSE;
    }

    Status = ZwQueryVolumeInformationFile( hFile,
										   &IoStatus,
										   &VolumeInfo,
										   sizeof(VolumeInfo),
										   FileFsVolumeInformation );
    if (! NT_ERROR(Status)) {
        LocalFileInformation.dwVolumeSerialNumber = VolumeInfo.VolumeSerialNumber;
    } else {
        LdkSetLastNTError( Status );
        return FALSE;
    }
    Status = NtQueryInformationFile( hFile,
									 &IoStatus,
									 &FileInformation,
									 sizeof(FileInformation),
									 FileAllInformation );

    if (! NT_ERROR(Status)) {
        LocalFileInformation.dwFileAttributes = FileInformation.BasicInformation.FileAttributes;
        LocalFileInformation.ftCreationTime = *(LPFILETIME)&FileInformation.BasicInformation.CreationTime;
        LocalFileInformation.ftLastAccessTime = *(LPFILETIME)&FileInformation.BasicInformation.LastAccessTime;
        LocalFileInformation.ftLastWriteTime = *(LPFILETIME)&FileInformation.BasicInformation.LastWriteTime;
        LocalFileInformation.nFileSizeHigh = FileInformation.StandardInformation.EndOfFile.HighPart;
        LocalFileInformation.nFileSizeLow = FileInformation.StandardInformation.EndOfFile.LowPart;
        LocalFileInformation.nNumberOfLinks = FileInformation.StandardInformation.NumberOfLinks;
        LocalFileInformation.nFileIndexHigh = FileInformation.InternalInformation.IndexNumber.HighPart;
        LocalFileInformation.nFileIndexLow = FileInformation.InternalInformation.IndexNumber.LowPart;
    } else {
        LdkSetLastNTError( Status );
        return FALSE;
    }
    *lpFileInformation = LocalFileInformation;
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
GetFileInformationByHandleEx (
    _In_ HANDLE hFile,
    _In_ FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
    _Out_writes_bytes_(dwBufferSize) LPVOID lpFileInformation,
    _In_ DWORD dwBufferSize
    )
{
    BY_HANDLE_FILE_INFORMATION HandleInfo;

	PAGED_CODE();

    if (lpFileInformation == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (FileInformationClass == FileNameInfo) {
        IO_STATUS_BLOCK IoStatus;
        NTSTATUS Status;

        if (dwBufferSize < (DWORD)FIELD_OFFSET(FILE_NAME_INFO, FileName)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        Status = ZwQueryInformationFile( hFile,
                                         &IoStatus,
                                         lpFileInformation,
                                         dwBufferSize,
                                         FileNameInformation );
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            return FALSE;
        }

        return TRUE;
    }

    if (FileInformationClass == FileStreamInfo) {
        IO_STATUS_BLOCK IoStatus;
        NTSTATUS Status;

        if (dwBufferSize < (DWORD)FIELD_OFFSET(FILE_STREAM_INFO, StreamName)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        Status = ZwQueryInformationFile( hFile,
                                         &IoStatus,
                                         lpFileInformation,
                                         dwBufferSize,
                                         FileStreamInformation );
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            return FALSE;
        }

        return TRUE;
    }

    if (FileInformationClass == FileCompressionInfo) {
        PFILE_COMPRESSION_INFO CompressionInfo;
        FILE_COMPRESSION_INFORMATION NativeCompressionInfo;
        IO_STATUS_BLOCK IoStatus;
        NTSTATUS Status;

        if (dwBufferSize < sizeof(FILE_COMPRESSION_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        Status = ZwQueryInformationFile( hFile,
                                         &IoStatus,
                                         &NativeCompressionInfo,
                                         sizeof(NativeCompressionInfo),
                                         FileCompressionInformation );
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            return FALSE;
        }

        CompressionInfo = (PFILE_COMPRESSION_INFO)lpFileInformation;
        CompressionInfo->CompressedFileSize = NativeCompressionInfo.CompressedFileSize;
        CompressionInfo->CompressionFormat = NativeCompressionInfo.CompressionFormat;
        CompressionInfo->CompressionUnitShift = NativeCompressionInfo.CompressionUnitShift;
        CompressionInfo->ChunkShift = NativeCompressionInfo.ChunkShift;
        CompressionInfo->ClusterShift = NativeCompressionInfo.ClusterShift;
        RtlCopyMemory( CompressionInfo->Reserved,
                       NativeCompressionInfo.Reserved,
                       sizeof(CompressionInfo->Reserved) );
        return TRUE;
    }

    if (FileInformationClass == FileAlignmentInfo) {
        PFILE_ALIGNMENT_INFO AlignmentInfo;
        FILE_ALIGNMENT_INFORMATION NativeAlignmentInfo;
        IO_STATUS_BLOCK IoStatus;
        NTSTATUS Status;

        if (dwBufferSize < sizeof(FILE_ALIGNMENT_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        Status = ZwQueryInformationFile( hFile,
                                         &IoStatus,
                                         &NativeAlignmentInfo,
                                         sizeof(NativeAlignmentInfo),
                                         FileAlignmentInformation );
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            return FALSE;
        }

        AlignmentInfo = (PFILE_ALIGNMENT_INFO)lpFileInformation;
        AlignmentInfo->AlignmentRequirement = NativeAlignmentInfo.AlignmentRequirement;
        return TRUE;
    }

    if (! GetFileInformationByHandle( hFile,
                                      &HandleInfo )) {
        return FALSE;
    }

    if (FileInformationClass == FileBasicInfo) {
        PFILE_BASIC_INFO BasicInfo;

        if (dwBufferSize < sizeof(FILE_BASIC_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        BasicInfo = (PFILE_BASIC_INFO)lpFileInformation;
        BasicInfo->CreationTime.LowPart = HandleInfo.ftCreationTime.dwLowDateTime;
        BasicInfo->CreationTime.HighPart = HandleInfo.ftCreationTime.dwHighDateTime;
        BasicInfo->LastAccessTime.LowPart = HandleInfo.ftLastAccessTime.dwLowDateTime;
        BasicInfo->LastAccessTime.HighPart = HandleInfo.ftLastAccessTime.dwHighDateTime;
        BasicInfo->LastWriteTime.LowPart = HandleInfo.ftLastWriteTime.dwLowDateTime;
        BasicInfo->LastWriteTime.HighPart = HandleInfo.ftLastWriteTime.dwHighDateTime;
        BasicInfo->ChangeTime = BasicInfo->LastWriteTime;
        BasicInfo->FileAttributes = HandleInfo.dwFileAttributes;
        return TRUE;
    }

    if (FileInformationClass == FileStandardInfo) {
        PFILE_STANDARD_INFO StandardInfo;

        if (dwBufferSize < sizeof(FILE_STANDARD_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        StandardInfo = (PFILE_STANDARD_INFO)lpFileInformation;
        StandardInfo->EndOfFile.HighPart = HandleInfo.nFileSizeHigh;
        StandardInfo->EndOfFile.LowPart = HandleInfo.nFileSizeLow;
        StandardInfo->AllocationSize = StandardInfo->EndOfFile;
        StandardInfo->NumberOfLinks = HandleInfo.nNumberOfLinks;
        StandardInfo->DeletePending = FALSE;
        StandardInfo->Directory = (HandleInfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
        return TRUE;
    }

    if (FileInformationClass == FileAttributeTagInfo) {
        PFILE_ATTRIBUTE_TAG_INFO TagInfo;
        FILE_ATTRIBUTE_TAG_INFORMATION NativeTagInfo;
        IO_STATUS_BLOCK IoStatus;
        NTSTATUS Status;

        if (dwBufferSize < sizeof(FILE_ATTRIBUTE_TAG_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        Status = ZwQueryInformationFile( hFile,
                                         &IoStatus,
                                         &NativeTagInfo,
                                         sizeof(NativeTagInfo),
                                         FileAttributeTagInformation );
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            return FALSE;
        }

        TagInfo = (PFILE_ATTRIBUTE_TAG_INFO)lpFileInformation;
        TagInfo->FileAttributes = NativeTagInfo.FileAttributes;
        TagInfo->ReparseTag = NativeTagInfo.ReparseTag;
        return TRUE;
    }

    if (FileInformationClass == FileIdInfo) {
        PFILE_ID_INFO IdInfo;

        if (dwBufferSize < sizeof(FILE_ID_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        IdInfo = (PFILE_ID_INFO)lpFileInformation;
        RtlZeroMemory( IdInfo,
                       sizeof(*IdInfo) );
        IdInfo->VolumeSerialNumber = HandleInfo.dwVolumeSerialNumber;
        RtlCopyMemory( &IdInfo->FileId.Identifier[0],
                       &HandleInfo.nFileIndexLow,
                       sizeof(HandleInfo.nFileIndexLow) );
        RtlCopyMemory( &IdInfo->FileId.Identifier[sizeof(HandleInfo.nFileIndexLow)],
                       &HandleInfo.nFileIndexHigh,
                       sizeof(HandleInfo.nFileIndexHigh) );
        return TRUE;
    }

    LdkpSetFileInformationClassError(
        LdkpIsGetFileInformationByHandleExClass( FileInformationClass ) );
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
SetFileInformationByHandle (
    _In_ HANDLE hFile,
    _In_ FILE_INFO_BY_HANDLE_CLASS FileInformationClass,
    _In_reads_bytes_(dwBufferSize) LPVOID lpFileInformation,
    _In_ DWORD dwBufferSize
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

	PAGED_CODE();

    if (lpFileInformation == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (FileInformationClass == FileRenameInfo ||
        FileInformationClass == FileRenameInfoEx) {
        PFILE_RENAME_INFO RenameInfo;
        PFILE_RENAME_INFORMATION NativeRenameInfo;
        FILE_INFORMATION_CLASS NativeInformationClass;
        UNICODE_STRING RenameFileName;
        UNICODE_STRING NtRenameFileName;
        RTL_RELATIVE_NAME_U RelativeName;
        PUNICODE_STRING NativeFileName;
        HANDLE RootDirectory;
        PWSTR NullTerminatedName;
        PVOID FreeNtBuffer;
        ULONG MinimumSize;
        ULONG NativeRenameInfoSize;

        MinimumSize = FIELD_OFFSET(FILE_RENAME_INFO, FileName);
        if (dwBufferSize < MinimumSize) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        RenameInfo = (PFILE_RENAME_INFO)lpFileInformation;
        if (RenameInfo->FileNameLength == 0 ||
            RenameInfo->FileNameLength > MAXUSHORT ||
            (RenameInfo->FileNameLength & (sizeof(WCHAR) - 1)) != 0) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        if (RenameInfo->FileNameLength > dwBufferSize - MinimumSize) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        if (FileInformationClass == FileRenameInfoEx &&
            (RenameInfo->Flags & ~LDK_FILE_RENAME_INFO_EX_SUPPORTED_FLAGS) != 0) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return FALSE;
        }

        RenameFileName.Length = (USHORT)RenameInfo->FileNameLength;
        RenameFileName.MaximumLength = RenameFileName.Length;
        RenameFileName.Buffer = RenameInfo->FileName;
        NativeFileName = &RenameFileName;
        RootDirectory = RenameInfo->RootDirectory;
        FreeNtBuffer = NULL;

        if (RootDirectory == NULL) {
            NullTerminatedName = RtlAllocateHeap( RtlProcessHeap(),
                                                 MAKE_TAG( TMP_TAG ),
                                                 RenameInfo->FileNameLength + sizeof(UNICODE_NULL) );
            if (NullTerminatedName == NULL) {
                LdkSetLastNTError( STATUS_NO_MEMORY );
                return FALSE;
            }

            RtlCopyMemory( NullTerminatedName,
                           RenameInfo->FileName,
                           RenameInfo->FileNameLength );
            NullTerminatedName[RenameInfo->FileNameLength / sizeof(WCHAR)] = UNICODE_NULL;

            if (! RtlDosPathNameToNtPathName_U( NullTerminatedName,
                                                &NtRenameFileName,
                                                NULL,
                                                &RelativeName )) {
                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             NullTerminatedName );
                SetLastError( ERROR_PATH_NOT_FOUND );
                return FALSE;
            }

            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         NullTerminatedName );
            FreeNtBuffer = NtRenameFileName.Buffer;
            if (RelativeName.RelativeName.Length) {
                NativeFileName = (PUNICODE_STRING)&RelativeName.RelativeName;
                RootDirectory = RelativeName.ContainingDirectory;
            } else {
                NativeFileName = &NtRenameFileName;
            }
        }

        NativeRenameInfoSize = FIELD_OFFSET(FILE_RENAME_INFORMATION, FileName) +
                               NativeFileName->Length;
        NativeRenameInfo = RtlAllocateHeap( RtlProcessHeap(),
                                            MAKE_TAG( TMP_TAG ),
                                            NativeRenameInfoSize );
        if (NativeRenameInfo == NULL) {
            if (FreeNtBuffer != NULL) {
                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             FreeNtBuffer );
            }
            LdkSetLastNTError( STATUS_NO_MEMORY );
            return FALSE;
        }

        RtlZeroMemory( NativeRenameInfo,
                       NativeRenameInfoSize );
        if (FileInformationClass == FileRenameInfoEx) {
            RtlCopyMemory( NativeRenameInfo,
                           &RenameInfo->Flags,
                           sizeof(RenameInfo->Flags) );
            NativeInformationClass = LDK_FILE_RENAME_INFORMATION_EX_CLASS;
        } else {
            NativeRenameInfo->ReplaceIfExists = RenameInfo->ReplaceIfExists;
            NativeInformationClass = FileRenameInformation;
        }
        NativeRenameInfo->RootDirectory = RootDirectory;
        NativeRenameInfo->FileNameLength = NativeFileName->Length;
        RtlCopyMemory( NativeRenameInfo->FileName,
                       NativeFileName->Buffer,
                       NativeFileName->Length );

        Status = ZwSetInformationFile( hFile,
                                       &IoStatus,
                                       NativeRenameInfo,
                                       NativeRenameInfoSize,
                                       NativeInformationClass );

        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     NativeRenameInfo );
        if (FreeNtBuffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         FreeNtBuffer );
        }

        if (NT_SUCCESS(Status)) {
            return TRUE;
        }

        LdkSetLastNTError( Status );
        return FALSE;
    }

    if (FileInformationClass == FileBasicInfo) {
        PFILE_BASIC_INFO BasicInfo;
        FILE_BASIC_INFORMATION NativeBasicInfo;

        if (dwBufferSize < sizeof(FILE_BASIC_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        BasicInfo = (PFILE_BASIC_INFO)lpFileInformation;
        RtlZeroMemory( &NativeBasicInfo,
                       sizeof(NativeBasicInfo) );
        NativeBasicInfo.CreationTime = BasicInfo->CreationTime;
        NativeBasicInfo.LastAccessTime = BasicInfo->LastAccessTime;
        NativeBasicInfo.LastWriteTime = BasicInfo->LastWriteTime;
        NativeBasicInfo.ChangeTime = BasicInfo->ChangeTime;
        NativeBasicInfo.FileAttributes = BasicInfo->FileAttributes;

        Status = ZwSetInformationFile( hFile,
                                       &IoStatus,
                                       &NativeBasicInfo,
                                       sizeof(NativeBasicInfo),
                                       FileBasicInformation );
        if (NT_SUCCESS(Status)) {
            return TRUE;
        }

        LdkSetLastNTError( Status );
        return FALSE;
    }

    if (FileInformationClass == FileEndOfFileInfo) {
        PFILE_END_OF_FILE_INFO EndInfo;

        if (dwBufferSize < sizeof(FILE_END_OF_FILE_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        EndInfo = (PFILE_END_OF_FILE_INFO)lpFileInformation;
        if (! SetFilePointerEx( hFile,
                                EndInfo->EndOfFile,
                                NULL,
                                FILE_BEGIN )) {
            return FALSE;
        }
        return SetEndOfFile( hFile );
    }

    if (FileInformationClass == FileAllocationInfo) {
        PFILE_ALLOCATION_INFO AllocationInfo;
        FILE_ALLOCATION_INFORMATION NativeAllocationInfo;

        if (dwBufferSize < sizeof(FILE_ALLOCATION_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        AllocationInfo = (PFILE_ALLOCATION_INFO)lpFileInformation;
        NativeAllocationInfo.AllocationSize = AllocationInfo->AllocationSize;
        Status = ZwSetInformationFile( hFile,
                                       &IoStatus,
                                       &NativeAllocationInfo,
                                       sizeof(NativeAllocationInfo),
                                       FileAllocationInformation );
        if (NT_SUCCESS(Status)) {
            return TRUE;
        }

        LdkSetLastNTError( Status );
        return FALSE;
    }

    if (FileInformationClass == FileDispositionInfo) {
        PFILE_DISPOSITION_INFO DispositionInfo;
        FILE_DISPOSITION_INFORMATION NativeDispositionInfo;

        if (dwBufferSize < sizeof(FILE_DISPOSITION_INFO)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        DispositionInfo = (PFILE_DISPOSITION_INFO)lpFileInformation;
        NativeDispositionInfo.DeleteFile = DispositionInfo->DeleteFile;
        Status = ZwSetInformationFile( hFile,
                                       &IoStatus,
                                       &NativeDispositionInfo,
                                       sizeof(NativeDispositionInfo),
                                       FileDispositionInformation );
        if (NT_SUCCESS(Status)) {
            return TRUE;
        }

        LdkSetLastNTError( Status );
        return FALSE;
    }

    if (FileInformationClass == FileDispositionInfoEx) {
        PFILE_DISPOSITION_INFO_EX DispositionInfoEx;
        FILE_DISPOSITION_INFORMATION_EX NativeDispositionInfoEx;

        if (dwBufferSize < sizeof(FILE_DISPOSITION_INFO_EX)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        DispositionInfoEx = (PFILE_DISPOSITION_INFO_EX)lpFileInformation;
        NativeDispositionInfoEx.Flags = DispositionInfoEx->Flags;
        Status = ZwSetInformationFile( hFile,
                                       &IoStatus,
                                       &NativeDispositionInfoEx,
                                       sizeof(NativeDispositionInfoEx),
                                       FileDispositionInformationEx );
        if (NT_SUCCESS(Status)) {
            return TRUE;
        }

        LdkSetLastNTError( Status );
        return FALSE;
    }

    LdkpSetFileInformationClassError(
        LdkpIsSetFileInformationByHandleClass( FileInformationClass ) );
    return FALSE;
}

WINBASEAPI
DWORD
WINAPI
GetFinalPathNameByHandleW (
    _In_ HANDLE hFile,
    _Out_writes_(cchFilePath) LPWSTR lpszFilePath,
    _In_ DWORD cchFilePath,
    _In_ DWORD dwFlags
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PFILE_NAME_INFORMATION NativeFileNameInfo;
    PFILE_OBJECT FileObject;
    POBJECT_NAME_INFORMATION DosNameInfo;
    ULONG BufferLength;
    ULONG RequiredLength;
    ULONG CopyLength;
    ULONG RawLength;
    ULONG VolumeName;
    ULONG SourceOffset;
    ULONG RemainingLength;
    ULONG SourceNameOffset;
    ULONG PrefixLength = 0;
    ULONG PrefixCopyLength;
    ULONG PrefixOffset;
    PCWSTR SourceName;
    WCHAR PrefixBuffer[] = L"\\\\?\\";
    WCHAR DosDevicesPrefix[] = L"\\DosDevices\\";
    WCHAR NtDosDevicesPrefix[] = L"\\??\\";
    WCHAR Win32DosDevicesPrefix[] = L"\\\\?\\";

	PAGED_CODE();

    if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE)) {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    if ((cchFilePath != 0) && (lpszFilePath == NULL)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if ((dwFlags & ~(VOLUME_NAME_GUID |
                     VOLUME_NAME_NT |
                     VOLUME_NAME_NONE |
                     FILE_NAME_OPENED)) != 0) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    VolumeName = dwFlags & (VOLUME_NAME_GUID |
                            VOLUME_NAME_NT |
                            VOLUME_NAME_NONE);
    switch (VolumeName) {
    case VOLUME_NAME_DOS:
    case VOLUME_NAME_NT:
    case VOLUME_NAME_NONE:
        break;
    default:
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (VolumeName == VOLUME_NAME_DOS) {
        Status = ObReferenceObjectByHandle( hFile,
                                            0,
                                            *IoFileObjectType,
                                            KernelMode,
                                            (PVOID *)&FileObject,
                                            NULL );
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            return 0;
        }

        Status = IoQueryFileDosDeviceName( FileObject,
                                           &DosNameInfo );
        ObDereferenceObject( FileObject );
        if (! NT_SUCCESS(Status)) {
            LdkSetLastNTError( Status );
            return 0;
        }

        SourceName = DosNameInfo->Name.Buffer;
        RawLength = DosNameInfo->Name.Length / sizeof(WCHAR);
        SourceNameOffset = 0;

        if (RawLength >= (RTL_NUMBER_OF(DosDevicesPrefix) - 1) &&
            RtlCompareMemory( SourceName,
                              DosDevicesPrefix,
                              sizeof(DosDevicesPrefix) - sizeof(WCHAR) ) ==
                sizeof(DosDevicesPrefix) - sizeof(WCHAR)) {
            SourceNameOffset = RTL_NUMBER_OF(DosDevicesPrefix) - 1;
            PrefixLength = RTL_NUMBER_OF(PrefixBuffer) - 1;
        } else if (RawLength >= (RTL_NUMBER_OF(NtDosDevicesPrefix) - 1) &&
                   RtlCompareMemory( SourceName,
                                     NtDosDevicesPrefix,
                                     sizeof(NtDosDevicesPrefix) - sizeof(WCHAR) ) ==
                       sizeof(NtDosDevicesPrefix) - sizeof(WCHAR)) {
            SourceNameOffset = RTL_NUMBER_OF(NtDosDevicesPrefix) - 1;
            PrefixLength = RTL_NUMBER_OF(PrefixBuffer) - 1;
        } else if (RawLength >= (RTL_NUMBER_OF(Win32DosDevicesPrefix) - 1) &&
                   RtlCompareMemory( SourceName,
                                     Win32DosDevicesPrefix,
                                     sizeof(Win32DosDevicesPrefix) - sizeof(WCHAR) ) ==
                       sizeof(Win32DosDevicesPrefix) - sizeof(WCHAR)) {
            PrefixLength = 0;
        } else if (RawLength >= 3 &&
                   SourceName[1] == L':' &&
                   SourceName[2] == L'\\') {
            PrefixLength = RTL_NUMBER_OF(PrefixBuffer) - 1;
        }

        RequiredLength = PrefixLength + RawLength - SourceNameOffset;
        if (cchFilePath != 0) {
            CopyLength = RequiredLength < cchFilePath ? RequiredLength : cchFilePath;
            SourceOffset = 0;
            RemainingLength = CopyLength;

            if (PrefixLength != 0 && RemainingLength != 0) {
                PrefixCopyLength = PrefixLength < RemainingLength ? PrefixLength : RemainingLength;
                RtlCopyMemory( lpszFilePath,
                               PrefixBuffer,
                               PrefixCopyLength * sizeof(WCHAR) );
                SourceOffset += PrefixCopyLength;
                RemainingLength -= PrefixCopyLength;
            }

            if (RemainingLength != 0) {
                PrefixOffset = SourceNameOffset + SourceOffset - PrefixLength;
                RtlCopyMemory( lpszFilePath + SourceOffset,
                               SourceName + PrefixOffset,
                               RemainingLength * sizeof(WCHAR) );
            }

            if (RequiredLength < cchFilePath) {
                lpszFilePath[RequiredLength] = UNICODE_NULL;
            }
        }

        ExFreePool( DosNameInfo );
        return RequiredLength;
    }

    BufferLength = sizeof(FILE_NAME_INFORMATION) + (32 * 1024 * sizeof(WCHAR));
    NativeFileNameInfo = RtlAllocateHeap( RtlProcessHeap(),
                                          MAKE_TAG( TMP_TAG ),
                                          BufferLength );
    if (NativeFileNameInfo == NULL) {
        LdkSetLastNTError( STATUS_NO_MEMORY );
        return 0;
    }

    Status = ZwQueryInformationFile( hFile,
                                     &IoStatus,
                                     NativeFileNameInfo,
                                     BufferLength,
                                     FileNameInformation );
    if (! NT_SUCCESS(Status)) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     NativeFileNameInfo );
        LdkSetLastNTError( Status );
        return 0;
    }

    RawLength = NativeFileNameInfo->FileNameLength / sizeof(WCHAR);
    RequiredLength = RawLength;

    if (cchFilePath != 0) {
        CopyLength = RequiredLength < cchFilePath ? RequiredLength : cchFilePath;
        if (CopyLength != 0) {
            RtlCopyMemory( lpszFilePath,
                           NativeFileNameInfo->FileName,
                           CopyLength * sizeof(WCHAR) );
        }
        if (RequiredLength < cchFilePath) {
            lpszFilePath[RequiredLength] = UNICODE_NULL;
        }
    }

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 NativeFileNameInfo );

    return RequiredLength;
}

BOOL
LdkpCopyWideString (
    _Out_writes_(DestinationCch) PWSTR Destination,
    _In_ ULONG DestinationCch,
    _In_reads_(SourceCch) PCWSTR Source,
    _In_ ULONG SourceCch
    )
{
    if (Destination == NULL || Source == NULL || DestinationCch == 0 || SourceCch >= DestinationCch) {
        SetLastError( ERROR_INVALID_NAME );
        return FALSE;
    }

    if (SourceCch) {
        RtlCopyMemory( Destination,
                       Source,
                       SourceCch * sizeof(WCHAR) );
    }
    Destination[SourceCch] = UNICODE_NULL;
    return TRUE;
}

BOOL
LdkpSplitFindSpec (
    _In_ LPCWSTR FindSpec,
    _Out_writes_(DirectoryCch) PWSTR Directory,
    _In_ ULONG DirectoryCch,
    _Out_writes_(PatternCch) PWSTR Pattern,
    _In_ ULONG PatternCch
    )
{
    const WCHAR* Current;
    const WCHAR* LastSeparator = NULL;
    const WCHAR* PatternStart;
    ULONG DirectoryLength;
    ULONG PatternLength;
    WCHAR DotDirectory[] = L".";

    if (FindSpec == NULL || Directory == NULL || Pattern == NULL || FindSpec[0] == UNICODE_NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (Current = FindSpec; *Current != UNICODE_NULL; ++Current) {
        if (*Current == L'\\' || *Current == L'/') {
            LastSeparator = Current;
        }
    }

    PatternStart = FindSpec;
    if (LastSeparator == NULL) {
        if (! LdkpCopyWideString( Directory,
                                  DirectoryCch,
                                  DotDirectory,
                                  1 )) {
            return FALSE;
        }
    } else {
        PatternStart = LastSeparator + 1;
        if (*PatternStart == UNICODE_NULL) {
            SetLastError( ERROR_INVALID_NAME );
            return FALSE;
        }

        if (LastSeparator == FindSpec) {
            DirectoryLength = 1;
        } else if (LastSeparator == FindSpec + 2 && FindSpec[1] == L':') {
            DirectoryLength = 3;
        } else {
            DirectoryLength = (ULONG)(LastSeparator - FindSpec);
        }

        if (! LdkpCopyWideString( Directory,
                                  DirectoryCch,
                                  FindSpec,
                                  DirectoryLength )) {
            return FALSE;
        }
    }

    PatternLength = (ULONG)wcslen( PatternStart );
    return LdkpCopyWideString( Pattern,
                               PatternCch,
                               PatternStart,
                               PatternLength );
}

VOID
LdkpFillFindData (
    _In_ const FILE_BOTH_DIR_INFORMATION* DirectoryInformation,
    _Out_ LPWIN32_FIND_DATAW FindData
    )
{
    ULONG FileNameCch;
    ULONG ShortNameCch;

    RtlZeroMemory( FindData,
                   sizeof(*FindData) );
    FindData->dwFileAttributes = DirectoryInformation->FileAttributes;
    FindData->ftCreationTime = *(LPFILETIME)&DirectoryInformation->CreationTime;
    FindData->ftLastAccessTime = *(LPFILETIME)&DirectoryInformation->LastAccessTime;
    FindData->ftLastWriteTime = *(LPFILETIME)&DirectoryInformation->LastWriteTime;
    FindData->nFileSizeHigh = DirectoryInformation->EndOfFile.HighPart;
    FindData->nFileSizeLow = DirectoryInformation->EndOfFile.LowPart;
    if (FindData->dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) {
        FindData->dwReserved0 = DirectoryInformation->EaSize;
    }

    FileNameCch = DirectoryInformation->FileNameLength / sizeof(WCHAR);
    if (FileNameCch >= MAX_PATH) {
        FileNameCch = MAX_PATH - 1;
    }
    if (FileNameCch) {
        RtlCopyMemory( FindData->cFileName,
                       DirectoryInformation->FileName,
                       FileNameCch * sizeof(WCHAR) );
    }
    FindData->cFileName[FileNameCch] = UNICODE_NULL;

    ShortNameCch = DirectoryInformation->ShortNameLength / sizeof(WCHAR);
    if (ShortNameCch >= RTL_NUMBER_OF(FindData->cAlternateFileName)) {
        ShortNameCch = RTL_NUMBER_OF(FindData->cAlternateFileName) - 1;
    }
    if (ShortNameCch) {
        RtlCopyMemory( FindData->cAlternateFileName,
                       DirectoryInformation->ShortName,
                       ShortNameCch * sizeof(WCHAR) );
    }
    FindData->cAlternateFileName[ShortNameCch] = UNICODE_NULL;
}

BOOL
LdkpFindReadNext (
    _Inout_ PLDK_FIND_HANDLE FindHandle,
    _Out_ LPWIN32_FIND_DATAW FindData,
    _In_ DWORD NoMoreFilesError
    )
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    const FILE_BOTH_DIR_INFORMATION* DirectoryInformation;

    RtlZeroMemory( FindHandle->QueryBuffer.Buffer,
                   sizeof(FindHandle->QueryBuffer.Buffer) );

    Status = ZwQueryDirectoryFile( FindHandle->DirectoryHandle,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &IoStatus,
                                   FindHandle->QueryBuffer.Buffer,
                                   sizeof(FindHandle->QueryBuffer.Buffer),
                                   FileBothDirectoryInformation,
                                   TRUE,
                                   FindHandle->FirstQuery ? &FindHandle->Pattern : NULL,
                                   FindHandle->FirstQuery );

    FindHandle->FirstQuery = FALSE;

    if (NT_SUCCESS(Status)) {
        DirectoryInformation = (const FILE_BOTH_DIR_INFORMATION*)FindHandle->QueryBuffer.Buffer;
        LdkpFillFindData( DirectoryInformation,
                          FindData );
        return TRUE;
    }

    if (Status == STATUS_NO_MORE_FILES ||
        Status == STATUS_NO_MORE_ENTRIES ||
        Status == STATUS_NO_MORE_MATCHES ||
        Status == STATUS_NO_SUCH_FILE) {
        SetLastError( NoMoreFilesError );
    } else {
        LdkSetLastNTError( Status );
    }
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
FindClose (
    _Inout_ HANDLE hFindFile
    )
{
    PLDK_FIND_HANDLE FindHandle;

	PAGED_CODE();

    if (hFindFile == NULL || hFindFile == INVALID_HANDLE_VALUE) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    FindHandle = (PLDK_FIND_HANDLE)hFindFile;
    if (FindHandle->Magic != LDK_FIND_HANDLE_MAGIC) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    FindHandle->Magic = 0;
    if (FindHandle->DirectoryHandle != NULL && FindHandle->DirectoryHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( FindHandle->DirectoryHandle );
    }
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 FindHandle );
    return TRUE;
}

WINBASEAPI
HANDLE
WINAPI
FindFirstFileW (
    _In_ LPCWSTR lpFileName,
    _Out_ LPWIN32_FIND_DATAW lpFindFileData
    )
{
	PAGED_CODE();

    return FindFirstFileExW( lpFileName,
                             FindExInfoStandard,
                             lpFindFileData,
                             FindExSearchNameMatch,
                             NULL,
                             0 );
}

WINBASEAPI
HANDLE
WINAPI
FindFirstFileExW (
    _In_ LPCWSTR lpFileName,
    _In_ FINDEX_INFO_LEVELS fInfoLevelId,
    _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) LPVOID lpFindFileData,
    _In_ FINDEX_SEARCH_OPS fSearchOp,
    _Reserved_ LPVOID lpSearchFilter,
    _In_ DWORD dwAdditionalFlags
    )
{
    PLDK_FIND_HANDLE FindHandle;
    WCHAR DirectoryPath[MAX_PATH];

	PAGED_CODE();

    if (lpFindFileData == NULL ||
        (fInfoLevelId != FindExInfoStandard && fInfoLevelId != FindExInfoBasic) ||
        fSearchOp != FindExSearchNameMatch ||
        lpSearchFilter != NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    if ((dwAdditionalFlags & ~LDK_FIND_FIRST_EX_SUPPORTED_FLAGS) != 0) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return INVALID_HANDLE_VALUE;
    }

    FindHandle = RtlAllocateHeap( RtlProcessHeap(),
                                  MAKE_TAG( TMP_TAG ),
                                  sizeof(*FindHandle) );
    if (FindHandle == NULL) {
        LdkSetLastNTError( STATUS_NO_MEMORY );
        return INVALID_HANDLE_VALUE;
    }
    RtlZeroMemory( FindHandle,
                   sizeof(*FindHandle) );

    if (! LdkpSplitFindSpec( lpFileName,
                             DirectoryPath,
                             RTL_NUMBER_OF(DirectoryPath),
                             FindHandle->PatternBuffer,
                             RTL_NUMBER_OF(FindHandle->PatternBuffer) )) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FindHandle );
        return INVALID_HANDLE_VALUE;
    }

    FindHandle->DirectoryHandle = CreateFileW( DirectoryPath,
                                               FILE_LIST_DIRECTORY | FILE_READ_ATTRIBUTES,
                                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                               NULL,
                                               OPEN_EXISTING,
                                               FILE_FLAG_BACKUP_SEMANTICS,
                                               NULL );
    if (FindHandle->DirectoryHandle == INVALID_HANDLE_VALUE) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     FindHandle );
        return INVALID_HANDLE_VALUE;
    }

    FindHandle->Magic = LDK_FIND_HANDLE_MAGIC;
    FindHandle->FirstQuery = TRUE;
    RtlInitUnicodeString( &FindHandle->Pattern,
                          FindHandle->PatternBuffer );

    if (! LdkpFindReadNext( FindHandle,
                            (LPWIN32_FIND_DATAW)lpFindFileData,
                            ERROR_FILE_NOT_FOUND )) {
        FindClose( (HANDLE)FindHandle );
        return INVALID_HANDLE_VALUE;
    }

    return (HANDLE)FindHandle;
}

WINBASEAPI
BOOL
WINAPI
FindNextFileW (
    _In_ HANDLE hFindFile,
    _Out_ LPWIN32_FIND_DATAW lpFindFileData
    )
{
    PLDK_FIND_HANDLE FindHandle;

	PAGED_CODE();

    if (hFindFile == NULL || hFindFile == INVALID_HANDLE_VALUE || lpFindFileData == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    FindHandle = (PLDK_FIND_HANDLE)hFindFile;
    if (FindHandle->Magic != LDK_FIND_HANDLE_MAGIC) {
        SetLastError( ERROR_INVALID_HANDLE );
        return FALSE;
    }

    return LdkpFindReadNext( FindHandle,
                             lpFindFileData,
                             ERROR_NO_MORE_FILES );
}



WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetFullPathNameA (
    _In_ LPCSTR lpFileName,
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPSTR lpBuffer,
    _Outptr_opt_ LPSTR* lpFilePart
    )
{
    NTSTATUS Status;
    ULONG UnicodeLength;
    UNICODE_STRING UnicodeString;
    UNICODE_STRING UnicodeResult;
    PWSTR Ubuff;
    PWSTR FilePart = NULL;
    PWSTR *FilePartPtr;
    ULONG PrefixLength = 0;

	PAGED_CODE();

    if (ARGUMENT_PRESENT(lpFilePart)) {
        FilePartPtr = &FilePart;
    } else {
        FilePartPtr = NULL;
    }

	if (! Ldk8BitStringToDynamicUnicodeString( &UnicodeString,
											   lpFileName )) {
		return 0;
	}
    Ubuff = RtlAllocateHeap( RtlProcessHeap(),
							 MAKE_TAG( TMP_TAG ),
							 (MAX_PATH << 1) + sizeof(UNICODE_NULL));
    if (! Ubuff) {
        RtlFreeUnicodeString( &UnicodeString );
        LdkSetLastNTError( STATUS_NO_MEMORY );
        return 0;
    }

    UnicodeLength = RtlGetFullPathName_U( UnicodeString.Buffer,
										  (MAX_PATH << 1),
										  Ubuff,
										  FilePartPtr );

    if ( UnicodeLength <= ((MAX_PATH * sizeof(WCHAR) + sizeof(UNICODE_NULL))) ) {

        Status = RtlUnicodeToMultiByteSize( &UnicodeLength,
											Ubuff,
											UnicodeLength );

        if (NT_SUCCESS(Status)) {
            if (UnicodeLength && ARGUMENT_PRESENT(lpFilePart) && FilePart) {
                ULONG UnicodePrefixLength;

                UnicodePrefixLength = (ULONG)(FilePart - Ubuff) * sizeof(WCHAR);
                Status = RtlUnicodeToMultiByteSize( &PrefixLength,
                                                    Ubuff,
                                                    UnicodePrefixLength );
                if (! NT_SUCCESS(Status)) {
                    LdkSetLastNTError( Status );
                    UnicodeLength = 0;
                }
            }
        } else {
            LdkSetLastNTError( Status );
            UnicodeLength = 0;
        }
    } else {
        UnicodeLength = 0;
    }
    if (UnicodeLength && UnicodeLength < nBufferLength) {
        RtlInitUnicodeString( &UnicodeResult,Ubuff );
		ANSI_STRING AnsiResult;
		Status = LdkUnicodeStringTo8BitString( &AnsiResult,
											   &UnicodeResult,
											   TRUE );
        if (NT_SUCCESS(Status)) {
            RtlMoveMemory( lpBuffer,
						   AnsiResult.Buffer,
						   UnicodeLength + 1 );
			LdkFree8BitString( &AnsiResult );
            if (ARGUMENT_PRESENT(lpFilePart)) {
                if (FilePart == NULL) {
                	*lpFilePart = NULL;
                } else {
					*lpFilePart = lpBuffer + PrefixLength;
                }
            }
        } else {
            LdkSetLastNTError( Status );
            UnicodeLength = 0;
        }
    } else {
        if (UnicodeLength) {
            UnicodeLength++;
        }
    }

    RtlFreeUnicodeString( &UnicodeString );
    RtlFreeHeap( RtlProcessHeap(),
				 0,
				 Ubuff );

    return (DWORD)UnicodeLength;
}

WINBASEAPI
_Success_(return != 0 && return < nBufferLength)
DWORD
WINAPI
GetFullPathNameW (
    _In_ LPCWSTR lpFileName,
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPWSTR lpBuffer,
    _Outptr_opt_ LPWSTR* lpFilePart
    )
{
	PAGED_CODE();

    return (DWORD)RtlGetFullPathName_U( lpFileName,
										nBufferLength * sizeof(WCHAR),
										lpBuffer,
										lpFilePart ) /  sizeof(WCHAR);
}

WINBASEAPI
UINT
WINAPI
GetDriveTypeA (
    _In_opt_ LPCSTR lpRootPathName
    )
{
    PUNICODE_STRING Unicode;
    LPCWSTR lpRootPathName_U;

	PAGED_CODE();

    if (ARGUMENT_PRESENT(lpRootPathName)) {
        Unicode = Ldk8BitStringToStaticUnicodeString( lpRootPathName );
        if (Unicode == NULL) {
            return 1;
        }
        lpRootPathName_U = (LPCWSTR)Unicode->Buffer;
    } else {
        lpRootPathName_U = NULL;
    }

    return GetDriveTypeW( lpRootPathName_U );
}

WINBASEAPI
UINT
WINAPI
GetDriveTypeW (
    _In_opt_ LPCWSTR lpRootPathName
    )
{
    WCHAR wch;
    ULONG n, DriveNumber;
    WCHAR DefaultPath[MAX_PATH];
    PWSTR RootPathName;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    UNICODE_STRING FileName;
    IO_STATUS_BLOCK IoStatus;
    BOOLEAN TranslationStatus;
    PVOID FreeBuffer;
    DWORD ReturnValue;
    FILE_FS_DEVICE_INFORMATION DeviceInfo;
    PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo;

	PAGED_CODE();

    if (! ARGUMENT_PRESENT(lpRootPathName)) {
        n = RtlGetCurrentDirectory_U( sizeof(DefaultPath),
									  DefaultPath );
        RootPathName = DefaultPath;
        if (n > (3 * sizeof(WCHAR))) {
            RootPathName[3]=UNICODE_NULL;
        }
    } else if (lpRootPathName == (PWSTR)IntToPtr(0xFFFFFFFF)) {
        return 0;
    } else {
        RootPathName = (PWSTR)lpRootPathName;
        if (wcslen( RootPathName ) == 2) {
            wch = RtlUpcaseUnicodeChar( *RootPathName );
            if (wch >= (WCHAR)'A' &&
                wch <= (WCHAR)'Z' &&
                RootPathName[1] == (WCHAR)':'
               ) {
                RootPathName = wcscpy( DefaultPath,
									   lpRootPathName );
                RootPathName[2] = (WCHAR)'\\';
                RootPathName[3] = UNICODE_NULL;
            }
        }
    }

    wch = RtlUpcaseUnicodeChar( *RootPathName );
    if (wch >= (WCHAR)'A' &&
        wch <= (WCHAR)'Z' &&
        RootPathName[1]==(WCHAR)':' &&
        RootPathName[2]==(WCHAR)'\\' &&
        RootPathName[3]==UNICODE_NULL
       ) {
    	Status = ZwQueryInformationProcess( NtCurrentProcess(),
                                            ProcessDeviceMap,
                                            &ProcessDeviceMapInfo.Query,
                                            sizeof( ProcessDeviceMapInfo.Query ),
                                            NULL );
        if (! NT_SUCCESS(Status)) {
            RtlZeroMemory( &ProcessDeviceMapInfo,
						   sizeof(ProcessDeviceMapInfo) );
        }

        DriveNumber = wch - (WCHAR)'A';
        if (ProcessDeviceMapInfo.Query.DriveMap & (1 << DriveNumber)) {
            switch (ProcessDeviceMapInfo.Query.DriveType[DriveNumber]) {
			case DOSDEVICE_DRIVE_UNKNOWN:
				return DRIVE_UNKNOWN;
			case DOSDEVICE_DRIVE_REMOVABLE:
				return DRIVE_REMOVABLE;
			case DOSDEVICE_DRIVE_FIXED:
				return DRIVE_FIXED;
			case DOSDEVICE_DRIVE_REMOTE:
				return DRIVE_REMOTE;
			case DOSDEVICE_DRIVE_CDROM:
				return DRIVE_CDROM;
			case DOSDEVICE_DRIVE_RAMDISK:
				return DRIVE_RAMDISK;
			}
        }
    }

    if (! ARGUMENT_PRESENT(lpRootPathName)) {
        RootPathName = L"\\";
    }

    TranslationStatus = RtlDosPathNameToNtPathName_U( RootPathName,
                                                      &FileName,
                                                      NULL,
                                                      NULL );
    if (! TranslationStatus) {
        return DRIVE_NO_ROOT_DIR;
    }
    FreeBuffer = FileName.Buffer;

    if (FileName.Buffer[(FileName.Length >> 1)-1] != '\\') {
        RtlFreeHeap( RtlProcessHeap(),
					 0,
					 FreeBuffer );
        return DRIVE_NO_ROOT_DIR;
    }

    FileName.Length -= 2;
    InitializeObjectAttributes( &ObjectAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = ZwOpenFile( &Handle,
                         (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE );

    if (Status == STATUS_FILE_IS_A_DIRECTORY) {
        Status = ZwOpenFile( &Handle,
							 (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
							 &ObjectAttributes,
							 &IoStatus,
							 FILE_SHARE_READ | FILE_SHARE_WRITE,
							 FILE_SYNCHRONOUS_IO_NONALERT );
    } else {
        FileName.Length = FileName.Length + sizeof((WCHAR)'\\');
        if (! IsThisARootDirectory( NULL,
									&FileName )) {
            FileName.Length = FileName.Length - sizeof((WCHAR)'\\');
            if (NT_SUCCESS(Status)) {
                ZwClose(Handle);
            }
            Status = ZwOpenFile( &Handle,
								 (ACCESS_MASK)FILE_READ_ATTRIBUTES | SYNCHRONIZE,
								 &ObjectAttributes,
								 &IoStatus,
								 FILE_SHARE_READ | FILE_SHARE_WRITE,
								 FILE_SYNCHRONOUS_IO_NONALERT );
        }
    }
    RtlFreeHeap( RtlProcessHeap(),
				 0,
				 FreeBuffer );
    if (! NT_SUCCESS(Status)) {
        return DRIVE_NO_ROOT_DIR;
    }

    Status = ZwQueryVolumeInformationFile( Handle,
                                           &IoStatus,
                                           &DeviceInfo,
                                           sizeof(DeviceInfo),
                                           FileFsDeviceInformation );
    if (! NT_SUCCESS(Status)) {
        ReturnValue = DRIVE_UNKNOWN;
    } else if (DeviceInfo.Characteristics & FILE_REMOTE_DEVICE) {
        ReturnValue = DRIVE_REMOTE;
    } else {
    	switch (DeviceInfo.DeviceType) {
		case FILE_DEVICE_NETWORK:
		case FILE_DEVICE_NETWORK_FILE_SYSTEM:
			ReturnValue = DRIVE_REMOTE;
			break;
		case FILE_DEVICE_CD_ROM:
		case FILE_DEVICE_CD_ROM_FILE_SYSTEM:
			ReturnValue = DRIVE_CDROM;
			break;
		case FILE_DEVICE_VIRTUAL_DISK:
			ReturnValue = DRIVE_RAMDISK;
			break;
		case FILE_DEVICE_DISK:
		case FILE_DEVICE_DISK_FILE_SYSTEM:
			ReturnValue = (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA) ? DRIVE_REMOVABLE : DRIVE_FIXED;
			break;
		default:
			ReturnValue = DRIVE_UNKNOWN;
			break;
		}
    }

    ZwClose( Handle );
    return ReturnValue;
}

WINBASEAPI
BOOL
WINAPI
GetDiskFreeSpaceExW (
    _In_opt_ LPCWSTR lpDirectoryName,
    _Out_opt_ PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    _Out_opt_ PULARGE_INTEGER lpTotalNumberOfBytes,
    _Out_opt_ PULARGE_INTEGER lpTotalNumberOfFreeBytes
    )
{
    HANDLE Handle;
    IO_STATUS_BLOCK IoStatus;
    FILE_FS_SIZE_INFORMATION SizeInformation;
    NTSTATUS Status;
    ULONGLONG BytesPerAllocationUnit;
    LPCWSTR DirectoryName;

	PAGED_CODE();

    DirectoryName = ARGUMENT_PRESENT(lpDirectoryName) ? lpDirectoryName : L".";
    Handle = CreateFileW( DirectoryName,
                          FILE_READ_ATTRIBUTES,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          NULL,
                          OPEN_EXISTING,
                          FILE_FLAG_BACKUP_SEMANTICS,
                          NULL );
    if (Handle == INVALID_HANDLE_VALUE) {
        return FALSE;
    }

    Status = ZwQueryVolumeInformationFile( Handle,
                                           &IoStatus,
                                           &SizeInformation,
                                           sizeof(SizeInformation),
                                           FileFsSizeInformation );
    CloseHandle( Handle );

    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    BytesPerAllocationUnit = (ULONGLONG)SizeInformation.SectorsPerAllocationUnit *
                             (ULONGLONG)SizeInformation.BytesPerSector;

    if (ARGUMENT_PRESENT(lpFreeBytesAvailableToCaller)) {
        lpFreeBytesAvailableToCaller->QuadPart =
            (ULONGLONG)SizeInformation.AvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }
    if (ARGUMENT_PRESENT(lpTotalNumberOfBytes)) {
        lpTotalNumberOfBytes->QuadPart =
            (ULONGLONG)SizeInformation.TotalAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }
    if (ARGUMENT_PRESENT(lpTotalNumberOfFreeBytes)) {
        lpTotalNumberOfFreeBytes->QuadPart =
            (ULONGLONG)SizeInformation.AvailableAllocationUnits.QuadPart * BytesPerAllocationUnit;
    }

    return TRUE;
}

BOOL
LdkpEnsureTrailingPathSeparator (
    _Inout_updates_(PathCch) PWSTR Path,
    _In_ ULONG PathCch
    )
{
    ULONG Length;

    Length = (ULONG)wcslen( Path );
    if (Length == 0) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    if (Path[Length - 1] == L'\\' || Path[Length - 1] == L'/') {
        return TRUE;
    }

    if (Length + 1 >= PathCch) {
        SetLastError( ERROR_BUFFER_OVERFLOW );
        return FALSE;
    }

    Path[Length] = L'\\';
    Path[Length + 1] = UNICODE_NULL;
    return TRUE;
}

BOOL
LdkpTempPathExists (
    _In_ LPCWSTR Path
    )
{
    DWORD Attributes;

    Attributes = GetFileAttributesW( Path );
    return Attributes != INVALID_FILE_ATTRIBUTES &&
           FlagOn(Attributes, FILE_ATTRIBUTE_DIRECTORY);
}

BOOL
LdkpCopyTempPathCandidate (
    _Out_writes_(DestinationCch) PWSTR Destination,
    _In_ ULONG DestinationCch,
    _In_ LPCWSTR Source,
    _In_ BOOLEAN AppendTemp
    )
{
    ULONG Length;
    static const WCHAR TempComponent[] = L"Temp";

    if (Destination == NULL || Source == NULL || DestinationCch == 0) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    Length = (ULONG)wcslen( Source );
    if (Length == 0 || Length >= DestinationCch) {
        SetLastError( ERROR_BUFFER_OVERFLOW );
        return FALSE;
    }

    RtlCopyMemory( Destination,
                   Source,
                   (Length + 1) * sizeof(WCHAR) );

    if (AppendTemp) {
        if (Destination[Length - 1] != L'\\' && Destination[Length - 1] != L'/') {
            if (Length + 1 >= DestinationCch) {
                SetLastError( ERROR_BUFFER_OVERFLOW );
                return FALSE;
            }
            Destination[Length] = L'\\';
            Destination[Length + 1] = UNICODE_NULL;
            ++Length;
        }

        if (Length + RTL_NUMBER_OF(TempComponent) > DestinationCch) {
            SetLastError( ERROR_BUFFER_OVERFLOW );
            return FALSE;
        }

        RtlCopyMemory( Destination + Length,
                       TempComponent,
                       sizeof(TempComponent) );
    }

    if (! LdkpEnsureTrailingPathSeparator( Destination,
                                           DestinationCch )) {
        return FALSE;
    }

    if (! LdkpTempPathExists( Destination )) {
        SetLastError( ERROR_PATH_NOT_FOUND );
        return FALSE;
    }

    return TRUE;
}

BOOL
LdkpGetTempPathCandidate (
    _Out_writes_(DestinationCch) PWSTR Destination,
    _In_ ULONG DestinationCch
    )
{
    WCHAR Buffer[MAX_PATH];
    DWORD Length;
    static const LPCWSTR DirectVariables[] = {
        L"TMP",
        L"TEMP",
        L"USERPROFILE"
    };
    static const LPCWSTR WindowsVariables[] = {
        L"SystemRoot",
        L"windir"
    };

    for (ULONG Index = 0; Index < RTL_NUMBER_OF(DirectVariables); ++Index) {
        Length = GetEnvironmentVariableW( DirectVariables[Index],
                                          Buffer,
                                          RTL_NUMBER_OF(Buffer) );
        if (Length != 0 && Length < RTL_NUMBER_OF(Buffer)) {
            if (LdkpCopyTempPathCandidate( Destination,
                                           DestinationCch,
                                           Buffer,
                                           FALSE )) {
                return TRUE;
            }
        }
    }

    for (ULONG Index = 0; Index < RTL_NUMBER_OF(WindowsVariables); ++Index) {
        Length = GetEnvironmentVariableW( WindowsVariables[Index],
                                          Buffer,
                                          RTL_NUMBER_OF(Buffer) );
        if (Length != 0 && Length < RTL_NUMBER_OF(Buffer)) {
            if (LdkpCopyTempPathCandidate( Destination,
                                           DestinationCch,
                                           Buffer,
                                           TRUE )) {
                return TRUE;
            }
        }
    }

    Length = GetCurrentDirectoryW( RTL_NUMBER_OF(Buffer),
                                   Buffer );
    if (Length != 0 && Length < RTL_NUMBER_OF(Buffer)) {
        if (LdkpCopyTempPathCandidate( Destination,
                                       DestinationCch,
                                       Buffer,
                                       FALSE )) {
            return TRUE;
        }
    }

    SetLastError( ERROR_PATH_NOT_FOUND );
    return FALSE;
}

static
WCHAR
LdkpTempHexDigit (
    _In_ ULONG Value
    )
{
    Value &= 0x0f;
    if (Value < 10) {
        return (WCHAR)(L'0' + Value);
    }
    return (WCHAR)(L'A' + (Value - 10));
}

static
VOID
LdkpTempUniqueToHex (
    _In_ UINT Unique,
    _Out_writes_(4) PWSTR Buffer
    )
{
    ULONG Value;

    Value = Unique & 0xffff;
    Buffer[0] = LdkpTempHexDigit( Value >> 12 );
    Buffer[1] = LdkpTempHexDigit( Value >> 8 );
    Buffer[2] = LdkpTempHexDigit( Value >> 4 );
    Buffer[3] = LdkpTempHexDigit( Value );
}

static
BOOL
LdkpBuildTempFileNameW (
    _In_ LPCWSTR PathName,
    _In_ LPCWSTR PrefixString,
    _In_ UINT Unique,
    _Out_writes_(MAX_PATH) LPWSTR TempFileName
    )
{
    WCHAR FullPath[MAX_PATH];
    WCHAR UniqueText[4];
    DWORD PathLength;
    ULONG PrefixLength;
    ULONG TotalLength;
    static const WCHAR Extension[] = L".TMP";

    PathLength = GetFullPathNameW( PathName,
                                   RTL_NUMBER_OF(FullPath),
                                   FullPath,
                                   NULL );
    if (PathLength == 0) {
        return FALSE;
    }
    if (PathLength >= RTL_NUMBER_OF(FullPath)) {
        SetLastError( ERROR_BUFFER_OVERFLOW );
        return FALSE;
    }

    if (! LdkpEnsureTrailingPathSeparator( FullPath,
                                           RTL_NUMBER_OF(FullPath) )) {
        return FALSE;
    }

    PathLength = (DWORD)wcslen( FullPath );
    PrefixLength = 0;
    while (PrefixString[PrefixLength] != UNICODE_NULL && PrefixLength < 3) {
        PrefixLength++;
    }

    TotalLength = PathLength + PrefixLength + RTL_NUMBER_OF(UniqueText) +
                  (RTL_NUMBER_OF(Extension) - 1);
    if (TotalLength >= MAX_PATH) {
        SetLastError( ERROR_BUFFER_OVERFLOW );
        return FALSE;
    }

    RtlCopyMemory( TempFileName,
                   FullPath,
                   PathLength * sizeof(WCHAR) );
    RtlCopyMemory( TempFileName + PathLength,
                   PrefixString,
                   PrefixLength * sizeof(WCHAR) );

    LdkpTempUniqueToHex( Unique,
                         UniqueText );
    RtlCopyMemory( TempFileName + PathLength + PrefixLength,
                   UniqueText,
                   sizeof(UniqueText) );
    RtlCopyMemory( TempFileName + PathLength + PrefixLength + RTL_NUMBER_OF(UniqueText),
                   Extension,
                   sizeof(Extension) );
    return TRUE;
}

WINBASEAPI
DWORD
WINAPI
GetTempPathA (
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPSTR lpBuffer
    )
{
    WCHAR WidePath[MAX_PATH];
    UNICODE_STRING UnicodeString;
    ANSI_STRING AnsiString;
    ULONG MultiByteLength;
    NTSTATUS Status;

	PAGED_CODE();

    if (! LdkpGetTempPathCandidate( WidePath,
                                    RTL_NUMBER_OF(WidePath) )) {
        return 0;
    }

    RtlInitUnicodeString( &UnicodeString,
                          WidePath );
    Status = RtlUnicodeToMultiByteSize( &MultiByteLength,
                                        UnicodeString.Buffer,
                                        UnicodeString.Length );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return 0;
    }

    if (nBufferLength == 0) {
        return MultiByteLength + 1;
    }

    if (lpBuffer == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (nBufferLength <= MultiByteLength) {
        return MultiByteLength + 1;
    }

    AnsiString.Buffer = lpBuffer;
    AnsiString.Length = 0;
    AnsiString.MaximumLength = (USHORT)nBufferLength;
    Status = LdkUnicodeStringTo8BitString( &AnsiString,
                                           &UnicodeString,
                                           FALSE );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return 0;
    }

    lpBuffer[AnsiString.Length] = ANSI_NULL;
    return AnsiString.Length;
}

WINBASEAPI
DWORD
WINAPI
GetTempPathW (
    _In_ DWORD nBufferLength,
    _Out_writes_to_opt_(nBufferLength,return + 1) LPWSTR lpBuffer
    )
{
    WCHAR TempPath[MAX_PATH];
    DWORD Length;

	PAGED_CODE();

    if (! LdkpGetTempPathCandidate( TempPath,
                                    RTL_NUMBER_OF(TempPath) )) {
        return 0;
    }

    Length = (DWORD)wcslen( TempPath );
    if (nBufferLength == 0) {
        return Length + 1;
    }

    if (lpBuffer == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (nBufferLength <= Length) {
        return Length + 1;
    }

    RtlCopyMemory( lpBuffer,
                   TempPath,
                   (Length + 1) * sizeof(WCHAR) );
    return Length;
}

WINBASEAPI
DWORD
WINAPI
GetTempPath2W (
    _In_ DWORD BufferLength,
    _Out_writes_to_opt_(BufferLength, return + 1) LPWSTR Buffer
    )
{
	PAGED_CODE();

    return GetTempPathW( BufferLength,
                         Buffer );
}

WINBASEAPI
UINT
WINAPI
GetTempFileNameA (
    _In_ LPCSTR lpPathName,
    _In_ LPCSTR lpPrefixString,
    _In_ UINT uUnique,
    _Out_writes_(MAX_PATH) LPSTR lpTempFileName
    )
{
    UNICODE_STRING PathName;
    UNICODE_STRING PrefixString;
    UNICODE_STRING WideResultString;
    STRING AnsiResultString;
    WCHAR WideResult[MAX_PATH];
    UINT Result;
    NTSTATUS Status;

	PAGED_CODE();

    if (lpPathName == NULL ||
        lpPrefixString == NULL ||
        lpTempFileName == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (! Ldk8BitStringToDynamicUnicodeString( &PathName,
                                               lpPathName )) {
        return 0;
    }

    if (! Ldk8BitStringToDynamicUnicodeString( &PrefixString,
                                               lpPrefixString )) {
        RtlFreeUnicodeString( &PathName );
        return 0;
    }

    Result = GetTempFileNameW( PathName.Buffer,
                               PrefixString.Buffer,
                               uUnique,
                               WideResult );
    RtlFreeUnicodeString( &PrefixString );
    RtlFreeUnicodeString( &PathName );
    if (Result == 0) {
        return 0;
    }

    RtlInitUnicodeString( &WideResultString,
                          WideResult );
    AnsiResultString.Buffer = lpTempFileName;
    AnsiResultString.Length = 0;
    AnsiResultString.MaximumLength = MAX_PATH;
    Status = LdkUnicodeStringTo8BitString( &AnsiResultString,
                                           &WideResultString,
                                           FALSE );
    if (! NT_SUCCESS(Status)) {
        if (uUnique == 0) {
            DeleteFileW( WideResult );
        }
        LdkSetLastNTError( Status );
        return 0;
    }

    lpTempFileName[AnsiResultString.Length] = ANSI_NULL;
    return Result;
}

WINBASEAPI
UINT
WINAPI
GetTempFileNameW (
    _In_ LPCWSTR lpPathName,
    _In_ LPCWSTR lpPrefixString,
    _In_ UINT uUnique,
    _Out_writes_(MAX_PATH) LPWSTR lpTempFileName
    )
{
    UINT Unique;

	PAGED_CODE();

    if (lpPathName == NULL ||
        lpPrefixString == NULL ||
        lpTempFileName == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (uUnique != 0) {
        if (! LdkpBuildTempFileNameW( lpPathName,
                                      lpPrefixString,
                                      uUnique,
                                      lpTempFileName )) {
            return 0;
        }
        return uUnique;
    }

    for (ULONG Attempt = 0; Attempt < 0x10000; Attempt++) {
        HANDLE FileHandle;
        DWORD ErrorCode;

        Unique = (UINT)(USHORT)InterlockedIncrement( &LdkpTempFileNameCounter );
        if (Unique == 0) {
            continue;
        }

        if (! LdkpBuildTempFileNameW( lpPathName,
                                      lpPrefixString,
                                      Unique,
                                      lpTempFileName )) {
            return 0;
        }

        FileHandle = CreateFileW( lpTempFileName,
                                  GENERIC_READ | GENERIC_WRITE,
                                  0,
                                  NULL,
                                  CREATE_NEW,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );
        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle( FileHandle );
            return Unique;
        }

        ErrorCode = GetLastError();
        if (ErrorCode != ERROR_FILE_EXISTS &&
            ErrorCode != ERROR_ALREADY_EXISTS) {
            return 0;
        }
    }

    SetLastError( ERROR_FILE_EXISTS );
    return 0;
}

WINBASEAPI
BOOL
WINAPI
DeviceIoControl (
    _In_ HANDLE hDevice,
    _In_ DWORD dwIoControlCode,
    _In_reads_bytes_opt_(nInBufferSize) LPVOID lpInBuffer,
    _In_ DWORD nInBufferSize,
    _Out_writes_bytes_to_opt_(nOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer,
    _In_ DWORD nOutBufferSize,
    _Out_opt_ LPDWORD lpBytesReturned,
    _Inout_opt_ LPOVERLAPPED lpOverlapped
    )
{
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status;

	PAGED_CODE();

    if (ARGUMENT_PRESENT(lpBytesReturned)) {
        *lpBytesReturned = 0;
    }

    if (ARGUMENT_PRESENT(lpOverlapped)) {
        LDK_DIAGNOSTIC_BREAK();
        LdkSetLastNTError( STATUS_NOT_IMPLEMENTED );
        return FALSE;
    }

    if (DEVICE_TYPE_FROM_CTL_CODE(dwIoControlCode) == FILE_DEVICE_FILE_SYSTEM) {
        Status = ZwFsControlFile( hDevice,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatus,
                                  dwIoControlCode,
                                  lpInBuffer,
                                  nInBufferSize,
                                  lpOutBuffer,
                                  nOutBufferSize );
    } else {
        Status = ZwDeviceIoControlFile( hDevice,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &IoStatus,
                                        dwIoControlCode,
                                        lpInBuffer,
                                        nInBufferSize,
                                        lpOutBuffer,
                                        nOutBufferSize );
    }
    if (Status == STATUS_PENDING) {
        Status = ZwWaitForSingleObject( hDevice,
                                        FALSE,
                                        NULL );
        if (NT_SUCCESS(Status)) {
            Status = IoStatus.Status;
        }
    }

    if (NT_SUCCESS(Status)) {
        if (ARGUMENT_PRESENT(lpBytesReturned)) {
            *lpBytesReturned = (DWORD)IoStatus.Information;
        }
        return TRUE;
    }

    LdkSetLastNTError( Status );
    return FALSE;
}

BOOL
IsThisARootDirectory (
    _In_ HANDLE RootHandle,
    _In_opt_ PUNICODE_STRING FileName
    )
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;

    WCHAR Buffer[MAX_PATH + sizeof(FILE_NAME_INFORMATION)];
    PFILE_NAME_INFORMATION NativeFileNameInfo = (PFILE_NAME_INFORMATION)Buffer;

	PAGED_CODE();

    Status = ZwQueryInformationFile( RootHandle,
									 &IoStatus,
									 NativeFileNameInfo,
									 sizeof(Buffer),
									 FileNameInformation );
    if (NT_SUCCESS(Status)) {
		if (NativeFileNameInfo->FileName[(NativeFileNameInfo->FileNameLength >> 1) - 1] == (WCHAR)'\\') {
			return TRUE;
		}
	}

	if (ARGUMENT_PRESENT(FileName)) {
		OBJECT_ATTRIBUTES ObjectAttributes;
		HANDLE LinkHandle;
		WCHAR LinkValueBuffer[2 * MAX_PATH];
		UNICODE_STRING LinkValue;
		ULONG ReturnedLength;
		
		FileName->Length = FileName->Length - sizeof((WCHAR)'\\');
		InitializeObjectAttributes( &ObjectAttributes,
									FileName,
									OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
									NULL,
									NULL );
		Status = ZwOpenSymbolicLinkObject( &LinkHandle,
										   SYMBOLIC_LINK_QUERY,
										   &ObjectAttributes );
		FileName->Length = FileName->Length + sizeof((WCHAR)'\\');
		if (NT_SUCCESS(Status)) {
			LinkValue.Buffer = LinkValueBuffer;
			LinkValue.Length = 0;
			LinkValue.MaximumLength = (USHORT)(sizeof(LinkValueBuffer));
			ReturnedLength = 0;
			Status = ZwQuerySymbolicLinkObject( LinkHandle,
												&LinkValue,
												&ReturnedLength );
			ZwClose( LinkHandle );
			if (NT_SUCCESS(Status)) {
				return TRUE;
			}
		}
	}
	LdkSetLastNTError( Status );
	return FALSE;
}



ULONG
LdkpUnicodeStringToAnsiSize (
    _In_ PUNICODE_STRING UnicodeString
    );

ULONG
LdkpUnicodeStringToOemSize (
	_In_ PUNICODE_STRING UnicodeString
	);

ULONG
LdkpAnsiStringToUnicodeSize (
    _In_ PANSI_STRING AnsiString
    );

ULONG
LdkpOemStringToUnicodeSize(
    PANSI_STRING OemString
    );



_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
(*Ldk8BitStringToUnicodeString)(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
		PUNICODE_STRING DestinationString,
    _In_ PANSI_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
(*Ldk8BitStringToUnicodeSize)(
    _In_ PANSI_STRING AnsiString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
NTSTATUS
(*LdkUnicodeStringTo8BitString)(
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_)
    	PANSI_STRING DestinationString,
    _In_ PUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
(*LdkUnicodeStringTo8BitSize)(
    _In_ PUNICODE_STRING UnicodeString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
(*LdkFree8BitString)(
	_Inout_ _At_(String->Buffer, _Frees_ptr_opt_)
    _In_ PSTRING String
    );

WINBASEAPI
BOOL
WINAPI
AreFileApisANSI (
    VOID
    )
{
	return Ldk8BitStringToUnicodeString == RtlAnsiStringToUnicodeString;
}

WINBASEAPI
VOID
WINAPI
SetFileApisToOEM (
    VOID
    )
{
    Ldk8BitStringToUnicodeString = RtlOemStringToUnicodeString;
    Ldk8BitStringToUnicodeSize = LdkpOemStringToUnicodeSize;
    LdkUnicodeStringTo8BitString = RtlUnicodeStringToOemString;
    LdkUnicodeStringTo8BitSize  = LdkpUnicodeStringToOemSize;
	LdkFree8BitString = RtlFreeOemString;	
}

WINBASEAPI
VOID
WINAPI
SetFileApisToANSI (
    VOID
    )
{
    Ldk8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;
    Ldk8BitStringToUnicodeSize = LdkpAnsiStringToUnicodeSize;
	LdkUnicodeStringTo8BitString = RtlUnicodeStringToAnsiString;
    LdkUnicodeStringTo8BitSize  = LdkpUnicodeStringToAnsiSize;    
	LdkFree8BitString = RtlFreeAnsiString;
}
