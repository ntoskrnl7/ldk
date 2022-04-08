#pragma once

EXTERN_C_START

WINBASEAPI
BOOL
WINAPI
QueueUserWorkItem(
    _In_ LPTHREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_ ULONG Flags
    );

EXTERN_C_END
