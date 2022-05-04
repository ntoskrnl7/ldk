#include "winbase.h"
#include "../peb.h"



WINBASEAPI
HANDLE
WINAPI
GetStdHandle (
    _In_ DWORD nStdHandle
    )
{ 
    switch (nStdHandle) {
    case STD_INPUT_HANDLE:
        return NtCurrentPeb()->ProcessParameters->StandardInput;
    case STD_OUTPUT_HANDLE:
        return NtCurrentPeb()->ProcessParameters->StandardOutput;
    case STD_ERROR_HANDLE:
        return NtCurrentPeb()->ProcessParameters->StandardError;
    }
    return INVALID_HANDLE_VALUE;
}

WINBASEAPI
BOOL
WINAPI
SetStdHandle (
    _In_ DWORD nStdHandle,
    _In_ HANDLE hHandle
    )
{
    return SetStdHandleEx( nStdHandle,
                           hHandle,
                           NULL );
}

WINBASEAPI
BOOL
WINAPI
SetStdHandleEx (
    _In_ DWORD nStdHandle,
    _In_ HANDLE hHandle,
    _Out_opt_ PHANDLE phPrevValue
    )
{
    HANDLE hPrevValue = NULL;
    switch (nStdHandle) {
    case STD_INPUT_HANDLE:
        hPrevValue = InterlockedExchangePointer( &NtCurrentPeb()->ProcessParameters->StandardInput,
                                                 hHandle );
        break;
    case STD_OUTPUT_HANDLE:
        hPrevValue = InterlockedExchangePointer( &NtCurrentPeb()->ProcessParameters->StandardOutput,
                                                 hHandle );
        break;
    case STD_ERROR_HANDLE:
        hPrevValue = InterlockedExchangePointer( &NtCurrentPeb()->ProcessParameters->StandardError,
                                                 hHandle );
        break;
    default:
        return FALSE;
    }

    if (ARGUMENT_PRESENT(phPrevValue)) {
        *phPrevValue = hPrevValue;
    }

    return TRUE;
}