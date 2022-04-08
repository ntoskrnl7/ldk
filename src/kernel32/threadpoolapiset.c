#include "winbase.h"



WINBASEAPI
_Must_inspect_result_
PTP_WORK
WINAPI
CreateThreadpoolWork(
    _In_ PTP_WORK_CALLBACK pfnwk,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe
    )
{
	UNREFERENCED_PARAMETER(pfnwk);
	UNREFERENCED_PARAMETER(pv);
	UNREFERENCED_PARAMETER(pcbe);

	// TODO

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
	return NULL;
}

WINBASEAPI
VOID
WINAPI
FreeLibraryWhenCallbackReturns(
    _Inout_ PTP_CALLBACK_INSTANCE pci,
    _In_ HMODULE mod
    )
{
	UNREFERENCED_PARAMETER(pci);
	UNREFERENCED_PARAMETER(mod);

	// TODO

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
}

WINBASEAPI
VOID
WINAPI
SubmitThreadpoolWork(
    _Inout_ PTP_WORK pwk
    )
{
	UNREFERENCED_PARAMETER(pwk);

	// TODO

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
}

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWork(
    _Inout_ PTP_WORK pwk
    )
{
	UNREFERENCED_PARAMETER(pwk);

	// TODO

	BaseSetLastNTError(STATUS_NOT_IMPLEMENTED);
}
