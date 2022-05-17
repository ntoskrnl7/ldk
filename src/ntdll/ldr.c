#include "ntdll.h"
#include <ntimage.h>



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdrLoadDll)
#pragma alloc_text(PAGE, LdrUnloadDll)
#pragma alloc_text(PAGE, LdrGetProcedureAddress)
#pragma alloc_text(PAGE, LdrGetDllHandle)
#endif



BOOLEAN LdrpInLdrInit = FALSE;
BOOLEAN LdrpShutdownInProgress = FALSE;
HANDLE LdrpShutdownThreadId = NULL;



NTSTATUS
NTAPI
LdrLoadDll (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID *DllHandle
    )
{
    PAGED_CODE()

    if (LdrpInLdrInit) {
        return STATUS_UNSUCCESSFUL;
    }

    return LdkLoadDll( DllPath,
                       DllCharacteristics,
                       DllName,
                       DllHandle );
}

NTSTATUS
NTAPI
LdrUnloadDll (
    _In_ PVOID DllHandle
    )
{
    PAGED_CODE();

    return LdkUnloadDll( DllHandle );
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
    PAGED_CODE();

    return LdkGetProcedureAddress( DllHandle,
                                   ProcedureName, 
                                   ProcedureNumber,
                                   ProcedureAddress );
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
    PAGED_CODE();

    return LdkGetDllHandle( DllPath,
                            DllCharacteristics,
                            DllName,
                            DllHandle );
}