#include "winbase.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, DuplicateHandle)
#endif



WINBASEAPI
BOOL
WINAPI
CloseHandle (
    _In_ HANDLE hObject
    )
{
	BOOL Handled;

	if (LdkCloseProcessHandle( hObject,
							   &Handled )) {
		return TRUE;
	}
	if (Handled) {
		return FALSE;
	}

	if (hObject == NULL ||
		hObject == INVALID_HANDLE_VALUE ||
		hObject == ZwCurrentThread()) {
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	ASSERT(ObIsKernelHandle( hObject ));

	NTSTATUS Status = ZwClose( hObject );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
DuplicateHandle (
    _In_ HANDLE hSourceProcessHandle,
    _In_ HANDLE hSourceHandle,
    _In_ HANDLE hTargetProcessHandle,
    _Outptr_ LPHANDLE lpTargetHandle,
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwOptions
    )
{
    PAGED_CODE();

	BOOL Handled;
	HANDLE NativeSourceProcessHandle;
	HANDLE NativeTargetProcessHandle;

	if (! LdkResolveProcessHandleForNativeDuplicate( hSourceProcessHandle,
													 &NativeSourceProcessHandle ) ||
		! LdkResolveProcessHandleForNativeDuplicate( hTargetProcessHandle,
													 &NativeTargetProcessHandle )) {
		return FALSE;
	}

	if (LdkDuplicateProcessHandle( hSourceHandle,
								   lpTargetHandle,
								   dwDesiredAccess,
								   bInheritHandle,
								   dwOptions,
								   &Handled )) {
		return TRUE;
	}
	if (Handled) {
		return FALSE;
	}

    NTSTATUS Status = ZwDuplicateObject( NativeSourceProcessHandle,
                                         hSourceHandle,
                                         NativeTargetProcessHandle,
                                         lpTargetHandle,
                                         (ACCESS_MASK)dwDesiredAccess,
                                         bInheritHandle ? OBJ_INHERIT | OBJ_KERNEL_HANDLE : OBJ_KERNEL_HANDLE,
                                         dwOptions );
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}
    
    LdkSetLastNTError( Status );
    return FALSE;
}
