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

    NTSTATUS Status = ZwDuplicateObject( hSourceProcessHandle,
                                         hSourceHandle,
                                         hTargetProcessHandle,
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