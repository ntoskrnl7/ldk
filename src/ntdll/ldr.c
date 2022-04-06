#include "ntdll.h"
#include "../nt/ntpsapi.h"

#include <ntimage.h>
#include "../nt/ntrtl.h"

BOOLEAN LdrpInLdrInit = FALSE;
BOOLEAN LdrpShutdownInProgress = FALSE;
HANDLE LdrpShutdownThreadId = NULL;

NTSTATUS
NTAPI
LdrLoadDll(
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    )
{
    if (LdrpInLdrInit == FALSE) {
        return STATUS_UNSUCCESSFUL;
    }

    return LdkLoadDll(DllPath, DllCharacteristics, DllName, DllHandle);
}

NTSTATUS
NTAPI
LdrUnloadDll(
    _In_ PVOID DllHandle
    )
{
    return LdkUnloadDll(DllHandle);
}

NTSTATUS
NTAPI
LdrGetProcedureAddress (
    _In_ PVOID DllHandle,
    _In_opt_ PANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID *ProcedureAddress
    )
{
    return LdkGetProcedureAddress(DllHandle, ProcedureName, ProcedureNumber, ProcedureAddress);
}

NTSTATUS
NTAPI
LdrGetDllHandle (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    )
{
    return LdkGetDllHandle(DllPath, DllCharacteristics, DllName, DllHandle);
}