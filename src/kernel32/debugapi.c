#include "winbase.h"



WINBASEAPI
BOOL
WINAPI
IsDebuggerPresent(
    VOID
    )
{
	return (KdRefreshDebuggerNotPresent() == FALSE);
}

WINBASEAPI
VOID
WINAPI
OutputDebugStringA(
    _In_opt_ LPCSTR lpOutputString
    )
{
    if (ARGUMENT_PRESENT(lpOutputString)) {
        DbgPrint(lpOutputString);
    }
}

WINBASEAPI
VOID
WINAPI
OutputDebugStringW(
    _In_opt_ LPCWSTR lpOutputString
    )
{
    if (ARGUMENT_PRESENT(lpOutputString)) {
        DbgPrint("%ws", lpOutputString);
    }
}