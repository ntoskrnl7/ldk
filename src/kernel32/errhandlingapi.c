#include "winbase.h"
#include "../ldk.h"
#include "../nt/ntexapi.h"


WINBASEAPI
_Check_return_ _Post_equals_last_error_
DWORD
WINAPI
GetLastError(
    VOID
    )
{
	return NtCurrentTeb()->LastErrorValue;
}

WINBASEAPI
VOID
WINAPI
SetLastError(
    _In_ DWORD dwErrCode
    )
{
	NtCurrentTeb()->LastErrorValue = (LONG)dwErrCode;
}