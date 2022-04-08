#include "winbase.h"

WINBASEAPI
BOOL
WINAPI
CloseHandle(
    _In_ HANDLE hObject
    )
{
	ASSERT(ObIsKernelHandle(hObject) == TRUE);

	NTSTATUS Status = ZwClose(hObject);
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

	BaseSetLastNTError(Status);
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
DuplicateHandle(
    _In_ HANDLE hSourceProcessHandle,
    _In_ HANDLE hSourceHandle,
    _In_ HANDLE hTargetProcessHandle,
    _Outptr_ LPHANDLE lpTargetHandle,
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwOptions
    )
{
    NTSTATUS Status = ZwDuplicateObject(
                hSourceProcessHandle,
                hSourceHandle,
                hTargetProcessHandle,
                lpTargetHandle,
                (ACCESS_MASK)dwDesiredAccess,
                bInheritHandle ? OBJ_INHERIT : 0,
                dwOptions
                );
	if (NT_SUCCESS(Status)) {
		return TRUE;
	} else {
		BaseSetLastNTError(Status);
		return FALSE;
	}
}
