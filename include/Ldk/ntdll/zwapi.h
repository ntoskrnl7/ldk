#pragma once

EXTERN_C_START

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationJobObject (
    __in_opt HANDLE JobHandle,
    __in JOBOBJECTINFOCLASS JobObjectInformationClass,
    __out_bcount(JobObjectInformationLength) PVOID JobObjectInformation,
    __in ULONG JobObjectInformationLength,
    __out_opt PULONG ReturnLength
    );



NTSYSAPI
NTSTATUS
NTAPI
ZwProtectVirtualMemory (
    __in HANDLE ProcessHandle,
    __inout PVOID *BaseAddress,
    __inout PSIZE_T RegionSize,
    __in ULONG NewProtect,
    __out PULONG OldProtect
    );



NTSYSAPI
NTSTATUS
NTAPI
ZwOpenThread (
    _In_ PHANDLE ThreadHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_opt_ PCLIENT_ID ClientId
    );



NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution (
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


NTSYSAPI
NTSTATUS
NTAPI
ZwSetSystemTime (
    _In_ PLARGE_INTEGER SystemTime,
    _Out_opt_ PLARGE_INTEGER PreviousTime
    );

EXTERN_C_END