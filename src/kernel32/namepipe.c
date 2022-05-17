#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"
#include <stdio.h>


LONG PipeSerialNumber = 0;

#define WIN32_SS_PIPE_FORMAT_STRING    "\\Device\\NamedPipe\\Win32Pipes.%08x.%08x"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CreatePipe)
#pragma alloc_text(PAGE, PeekNamedPipe)
#endif

WINBASEAPI
BOOL
WINAPI
CreatePipe (
    _Out_ PHANDLE hReadPipe,
    _Out_ PHANDLE hWritePipe,
    _In_opt_ LPSECURITY_ATTRIBUTES lpPipeAttributes,
    _In_ DWORD nSize
    )
{
    CHAR PipeNameBuffer[MAX_PATH];
    ANSI_STRING PipeName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ReadPipeHandle, WritePipeHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    LARGE_INTEGER Timeout;
    PUNICODE_STRING Unicode;

    PAGED_CODE();

    Timeout.QuadPart = - 10 * 1000 * 1000 * 120;
    if (nSize == 0) {
        nSize = 4096;
    }
    sprintf( PipeNameBuffer,
             WIN32_SS_PIPE_FORMAT_STRING,
             HandleToUlong(NtCurrentTeb()->ClientId.UniqueProcess),
             InterlockedIncrement(&PipeSerialNumber) );

    RtlInitAnsiString( &PipeName,
                       PipeNameBuffer );
    Unicode = &NtCurrentTeb()->StaticUnicodeString;
    Status = RtlAnsiStringToUnicodeString(Unicode,&PipeName,FALSE);
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError(Status);
        return FALSE;
    }

    InitializeObjectAttributes( &ObjectAttributes,
                                Unicode,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );
	if (ARGUMENT_PRESENT(lpPipeAttributes)) {
		ObjectAttributes.SecurityDescriptor = lpPipeAttributes->lpSecurityDescriptor;
		if (lpPipeAttributes->bInheritHandle) {
			SetFlag(ObjectAttributes.Attributes, OBJ_INHERIT);
		}
	}

    Status = ZwCreateNamedPipeFile( &ReadPipeHandle,
                                    GENERIC_READ | FILE_WRITE_ATTRIBUTES | SYNCHRONIZE,
                                    &ObjectAttributes,
                                    &IoStatus,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    FILE_CREATE,
                                    FILE_SYNCHRONOUS_IO_NONALERT,
                                    FILE_PIPE_BYTE_STREAM_TYPE,
                                    FILE_PIPE_BYTE_STREAM_MODE,
                                    FILE_PIPE_QUEUE_OPERATION,
                                    1,
                                    nSize,
                                    nSize,
                                    (PLARGE_INTEGER) &Timeout
                                  );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    Status = ZwOpenFile( &WritePipeHandle,
                         GENERIC_WRITE | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE,
                         FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE );
    if (! NT_SUCCESS(Status)) {
        ZwClose( ReadPipeHandle );
        LdkSetLastNTError( Status );
        return FALSE;
        }

    *hReadPipe = ReadPipeHandle;
    *hWritePipe = WritePipeHandle;
    return TRUE;
}



WINBASEAPI
BOOL
WINAPI
PeekNamedPipe (
    _In_ HANDLE hNamedPipe,
    _Out_writes_bytes_to_opt_(nBufferSize,*lpBytesRead) LPVOID lpBuffer,
    _In_ DWORD nBufferSize,
    _Out_opt_ LPDWORD lpBytesRead,
    _Out_opt_ LPDWORD lpTotalBytesAvail,
    _Out_opt_ LPDWORD lpBytesLeftThisMessage
    )
{
    PAGED_CODE();

    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    IO_STATUS_BLOCK IoStatus;
    ULONG BufferLength = nBufferSize + FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]);
    PFILE_PIPE_PEEK_BUFFER PeekBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                                         MAKE_TAG( TMP_TAG ),
                                                         BufferLength );
    try {
        Status = ZwFsControlFile( hNamedPipe,
                                  NULL,
                                  NULL,
                                  NULL,
                                  &IoStatus,
                                  FSCTL_PIPE_PEEK,
                                  NULL,
                                  0,
                                  PeekBuffer,
                                  BufferLength );

        if ( Status == STATUS_PENDING) {
            Status = ZwWaitForSingleObject( hNamedPipe,
                                            FALSE,
                                            NULL );
            if (NT_SUCCESS(Status)) {
                Status = IoStatus.Status;
            }
        }
        if ( Status == STATUS_BUFFER_OVERFLOW ) {
            Status = STATUS_SUCCESS;
        }
        if (NT_SUCCESS(Status)) {
            try {
                if (ARGUMENT_PRESENT(lpBuffer)) {
                    RtlCopyMemory( lpBuffer,
                                   PeekBuffer->Data,
                                   IoStatus.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]));
                }                
                if (ARGUMENT_PRESENT(lpBytesRead)) {
                    *lpBytesRead = (ULONG)(IoStatus.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]));
                }
                if (ARGUMENT_PRESENT(lpTotalBytesAvail)) {
                    *lpTotalBytesAvail = PeekBuffer->ReadDataAvailable;
                }
                if (ARGUMENT_PRESENT(lpBytesLeftThisMessage)) {
                    *lpBytesLeftThisMessage = PeekBuffer->MessageLength - (ULONG)(IoStatus.Information - FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[0]));
                }
            } except (EXCEPTION_EXECUTE_HANDLER) {
                Status = STATUS_ACCESS_VIOLATION;
            }
        }
    } finally {
        if (PeekBuffer) {
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         PeekBuffer );
        }
    }

    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    LdkSetLastNTError( Status );
    return FALSE;
}