#pragma once

EXTERN_C_START

NTSTATUS
NTAPI
LdrLoadDll (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

NTSTATUS
NTAPI
LdrUnloadDll (
    _In_ PVOID DllHandle
    );

NTSTATUS
NTAPI
LdrGetProcedureAddress (
    _In_ PVOID DllHandle,
    _In_opt_ PANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress
    );

NTSTATUS
NTAPI
LdrGetDllHandle (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

NTSTATUS
NTAPI
LdrGetDllHandle (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    );

EXTERN_C_END