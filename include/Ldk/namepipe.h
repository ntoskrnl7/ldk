#pragma once

EXTERN_C_START

WINBASEAPI
BOOL
WINAPI
CreatePipe(
    _Out_ PHANDLE hReadPipe,
    _Out_ PHANDLE hWritePipe,
    _In_opt_ LPSECURITY_ATTRIBUTES lpPipeAttributes,
    _In_ DWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
PeekNamedPipe(
    _In_ HANDLE hNamedPipe,
    _Out_writes_bytes_to_opt_(nBufferSize,*lpBytesRead) LPVOID lpBuffer,
    _In_ DWORD nBufferSize,
    _Out_opt_ LPDWORD lpBytesRead,
    _Out_opt_ LPDWORD lpTotalBytesAvail,
    _Out_opt_ LPDWORD lpBytesLeftThisMessage
    );

EXTERN_C_END