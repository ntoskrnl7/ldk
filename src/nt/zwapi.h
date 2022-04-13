#pragma once

EXTERN_C_START

NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution(
    VOID
    );



_IRQL_requires_max_(DISPATCH_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwClearEvent (
    _In_ HANDLE EventHandle
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
NTSYSAPI
NTSTATUS
NTAPI
ZwPulseEvent (
    _In_ HANDLE EventHandle,
    _Out_opt_ PLONG PreviousState
    );



_When_(Timeout == NULL, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart != 0, _IRQL_requires_max_(APC_LEVEL))
_When_(Timeout->QuadPart == 0, _IRQL_requires_max_(DISPATCH_LEVEL))
NTSYSAPI
NTSTATUS
NTAPI
ZwWaitForMultipleObjects (
    _In_ ULONG Count,
    _In_reads_(Count) HANDLE Handles[],
    _In_ _Strict_type_match_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );
EXTERN_C_END