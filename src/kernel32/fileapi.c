#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"



BOOL
IsThisARootDirectory (
    _In_ HANDLE RootHandle,
    _In_opt_ PUNICODE_STRING FileName
    );



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
#pragma alloc_text(PAGE, GetFileSize)
#pragma alloc_text(PAGE, GetFileSizeEx)
#pragma alloc_text(PAGE, GetFileType)
#pragma alloc_text(PAGE, SetFileTime)
#pragma alloc_text(PAGE, DeleteFileA)
#pragma alloc_text(PAGE, DeleteFileW)
#pragma alloc_text(PAGE, GetFileInformationByHandle)
#pragma alloc_text(PAGE, GetFullPathNameA)
#pragma alloc_text(PAGE, GetFullPathNameW)
#pragma alloc_text(PAGE, GetDriveTypeA)
#pragma alloc_text(PAGE, GetDriveTypeW)
#pragma alloc_text(PAGE, IsThisARootDirectory)
#endif



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

	PAGED_CODE();

	RtlInitUnicodeString( &FileName,
						  lpPathName );

	InitializeObjectAttributes( &ObjectAttributes,
								&FileName,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
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

		// untested :-(
		KdBreakPoint();

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
			SetLastError( ERROR_PATH_NOT_FOUND );
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
	NTSTATUS Status;
	UNICODE_STRING FileName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	FILE_NETWORK_OPEN_INFORMATION FileInformation;

	PAGED_CODE();

	RtlInitUnicodeString( &FileName,
						  lpFileName );

	InitializeObjectAttributes( &ObjectAttributes,
								&FileName,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								NULL );

	Status = ZwQueryFullAttributesFile( &ObjectAttributes,
										&FileInformation );

	if (NT_SUCCESS(Status)) {
		return FileInformation.FileAttributes;
	}

	LdkSetLastNTError( Status );
	return (DWORD)-1;
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
            if (FileTagInformation.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT) {
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

BOOL
IsThisARootDirectory (
    _In_ HANDLE RootHandle,
    _In_opt_ PUNICODE_STRING FileName
    )
{
	NTSTATUS Status;
	IO_STATUS_BLOCK IoStatus;

    WCHAR Buffer[MAX_PATH + sizeof(FileNameInfo)];
    PFILE_NAME_INFORMATION FileNameInfo = (PFILE_NAME_INFORMATION)Buffer;

	PAGED_CODE();

    Status = ZwQueryInformationFile( RootHandle,
									 &IoStatus,
									 FileNameInfo,
									 sizeof(Buffer),
									 FileNameInformation );
    if (NT_SUCCESS(Status)) {
		if (FileNameInfo->FileName[(FileNameInfo->FileNameLength >> 1) - 1] == (WCHAR)'\\') {
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