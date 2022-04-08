#include "winbase.h"



WINBASEAPI
BOOL
WINAPI
QueueUserWorkItem(
    _In_ LPTHREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_ ULONG Flags
    )
{
    UNREFERENCED_PARAMETER(Function);
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Flags);

	// TODO

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
	return FALSE;
}
