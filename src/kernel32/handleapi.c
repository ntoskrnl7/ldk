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