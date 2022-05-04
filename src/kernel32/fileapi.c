#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"

#define TAG_EA_BUFFER						'fBaE'

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreateDirectoryA)
#pragma alloc_text(PAGE, CreateDirectoryW)
#pragma alloc_text(PAGE, CreateFileA)
#pragma alloc_text(PAGE, CreateFileW)
#pragma alloc_text(PAGE, ReadFile)
#pragma alloc_text(PAGE, WriteFile)
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
#endif



WINBASEAPI
BOOL
WINAPI
CreateDirectoryA (
    _In_ LPCSTR lpPathName,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
	BOOL bSuccess;
    UNICODE_STRING Unicode;
	ANSI_STRING Ansi;

	PAGED_CODE();

	RtlInitAnsiString( &Ansi,
					   lpPathName );

	LdkAnsiStringToUnicodeString( &Unicode,
								  &Ansi,
								  TRUE );

	bSuccess = CreateDirectoryW( Unicode.Buffer,
								lpSecurityAttributes );

	LdkFreeUnicodeString( &Unicode );
	return bSuccess;
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
		BaseSetLastNTError( Status );
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
	HANDLE hFile;
    UNICODE_STRING Unicode;
	ANSI_STRING Ansi;

	PAGED_CODE();

	RtlInitAnsiString( &Ansi,
					   lpFileName );

	LdkAnsiStringToUnicodeString( &Unicode,
								  &Ansi,
								  TRUE );

	hFile = CreateFileW( Unicode.Buffer,
						 dwDesiredAccess,
						 dwShareMode,
						 lpSecurityAttributes,
						 dwCreationDisposition,
						 dwFlagsAndAttributes,
						 hTemplateFile );

	LdkFreeUnicodeString( &Unicode );
	return hFile;
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
			BaseSetLastNTError( STATUS_INVALID_PARAMETER );
			return INVALID_HANDLE_VALUE;
		}
		break;

	default:
		BaseSetLastNTError( STATUS_INVALID_PARAMETER );
		return INVALID_HANDLE_VALUE;
	}
	
	RtlInitUnicodeString( &FileName,
						  lpFileName );
	
	InitializeObjectAttributes( &ObjectAttributes,
								&FileName,
								OBJ_KERNEL_HANDLE | (dwFlagsAndAttributes & FILE_FLAG_POSIX_SEMANTICS ? 0 : OBJ_CASE_INSENSITIVE),
								NULL,
								NULL );

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
	
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_BACKUP_SEMANTICS) ? FILE_OPEN_FOR_BACKUP_INTENT : 0 ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_SEQUENTIAL_SCAN) ? FILE_SEQUENTIAL_ONLY : 0 ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_RANDOM_ACCESS) ? FILE_RANDOM_ACCESS : 0 ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_NO_BUFFERING) ? FILE_NO_INTERMEDIATE_BUFFERING : 0 ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_OVERLAPPED) ? 0 : FILE_SYNCHRONOUS_IO_NONALERT ));
	SetFlag(CreateFlags, (FlagOn(dwFlagsAndAttributes, FILE_FLAG_WRITE_THROUGH) ? FILE_WRITE_THROUGH : 0 ));


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
	ObjectAttributes.SecurityQualityOfService = &SecurityQualityOfService;

	if (ARGUMENT_PRESENT(lpSecurityAttributes)) {		
		ObjectAttributes.SecurityDescriptor = lpSecurityAttributes->lpSecurityDescriptor;
		if (lpSecurityAttributes->bInheritHandle) {
			ObjectAttributes.Attributes |= OBJ_INHERIT;
		}
	}

	if (ObjectAttributes.SecurityDescriptor) {
		KdBreakPoint();
	}
	if (ARGUMENT_PRESENT(hTemplateFile)) {
		KdBreakPoint();
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
				EaBuffer = HeapAlloc( GetProcessHeap(),
									  TAG_EA_BUFFER,
									  EaSize );

				if (! EaBuffer) {
					BaseSetLastNTError( STATUS_NO_MEMORY );
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
					HeapFree( GetProcessHeap(),
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

	if (EaBuffer) {
		HeapFree( GetProcessHeap(),
				  0,
				  EaBuffer );
	}

	if (! NT_SUCCESS( Status )) {
		if (Status == STATUS_OBJECT_NAME_COLLISION) {
			SetLastError( ERROR_FILE_EXISTS );
		} else if (Status == STATUS_FILE_IS_A_DIRECTORY) {
			SetLastError( ERROR_PATH_NOT_FOUND );
		} else {
			BaseSetLastNTError( Status );
		}
		return INVALID_HANDLE_VALUE;
	}

	if (((dwCreationDisposition == CREATE_ALWAYS) && (IoStatus.Information == FILE_OVERWRITTEN)) ||
		((dwCreationDisposition == OPEN_ALWAYS) && (IoStatus.Information == FILE_OPENED))) {
		SetLastError( ERROR_ALREADY_EXISTS );
	} else {		
		SetLastError( 0 );
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
			BaseSetLastNTError( Status );
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
				BaseSetLastNTError( Status );
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
			BaseSetLastNTError( Status );
			return FALSE;
		} else {
			BaseSetLastNTError( Status );
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
			BaseSetLastNTError( Status );
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
			BaseSetLastNTError( Status );
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
			BaseSetLastNTError( Status );
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
	} else {
		BaseSetLastNTError( Status );
		return FALSE;
	}
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
		BaseSetLastNTError( Status );
		return FALSE;
	}

	EndOfFile.EndOfFile = PositionInfo.CurrentByteOffset;
	Status = ZwSetInformationFile( hFile,
								   &IoStatus,
								   &EndOfFile,
								   sizeof(FILE_END_OF_FILE_INFORMATION),
								   FileEndOfFileInformation );
	
	if (! NT_SUCCESS(Status)) {
		BaseSetLastNTError( Status );
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
		BaseSetLastNTError( Status );
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
			BaseSetLastNTError( Status );
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
			BaseSetLastNTError( Status );
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
		BaseSetLastNTError( Status );

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
			BaseSetLastNTError( Status );
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
			BaseSetLastNTError( Status );
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
		BaseSetLastNTError( Status );
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
	DWORD dwFileAttributes;
    UNICODE_STRING Unicode;
	ANSI_STRING Ansi;

	PAGED_CODE();

	RtlInitAnsiString( &Ansi,
					   lpFileName );

	LdkAnsiStringToUnicodeString( &Unicode,
								  &Ansi,
								  TRUE );

	dwFileAttributes = GetFileAttributesW( Unicode.Buffer );

	LdkFreeUnicodeString(&Unicode);

	return dwFileAttributes;
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
								OBJ_CASE_INSENSITIVE,
								NULL,
								NULL );

	Status = ZwQueryFullAttributesFile( &ObjectAttributes,
										&FileInformation );

	if (NT_SUCCESS(Status)) {
		return FileInformation.FileAttributes;
	} else {
		BaseSetLastNTError( Status );
		return (DWORD)-1;
	}
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
    IO_STATUS_BLOCK IoStatusBlock;
    FILE_STANDARD_INFORMATION StandardInfo;

	PAGED_CODE();

    Status = ZwQueryInformationFile( hFile,
									 &IoStatusBlock,
									 &StandardInfo,
									 sizeof(StandardInfo),
									 FileStandardInformation );

    if (! NT_SUCCESS(Status)) {
		BaseSetLastNTError( Status );
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
	IO_STATUS_BLOCK IoStatusBlock;

	PAGED_CODE();

	LdkGetConsoleHandle( hFile,
						 &hFile );

	if (LdkIsConsoleHandle( hFile )) {
		return FILE_TYPE_CHAR;
	}

	Status = ZwQueryVolumeInformationFile( hFile,
										   &IoStatusBlock,
										   &DeviceInformation,
										   sizeof(FILE_FS_DEVICE_INFORMATION),
										   FileFsDeviceInformation );
	if (! NT_SUCCESS(Status)) {
		BaseSetLastNTError( Status );
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
		SetLastError(NO_ERROR);
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

	RtlZeroMemory( &BasicInfo, sizeof(BasicInfo) );

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
		BaseSetLastNTError( Status );
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