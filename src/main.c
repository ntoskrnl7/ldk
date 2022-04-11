#include "ldk.h"

DRIVER_INITIALIZE LdkDriverEntry;
DRIVER_UNLOAD LdkDriverUnload;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkDriverEntry)
#pragma alloc_text(PAGE, LdkDriverUnload)
#endif

DRIVER_INITIALIZE DriverEntry;

PDRIVER_UNLOAD LdkUserDriverUnload = NULL;

NTSTATUS
LdkDriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS status = LdkInitialize( DriverObject,
                                     RegistryPath,
                                     0 );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    status = DriverEntry( DriverObject,
                          RegistryPath );
    if (!NT_SUCCESS(status)) {
        return status;
    }

    LdkUserDriverUnload = DriverObject->DriverUnload;

    DriverObject->DriverUnload = LdkDriverUnload;
    return status;
}

VOID
LdkDriverUnload (
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();

    if (LdkUserDriverUnload) {
        LdkUserDriverUnload( DriverObject );
    }

    LdkTerminate();
}