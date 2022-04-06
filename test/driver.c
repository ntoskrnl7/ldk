#include <wdm.h>

#include <ldk/ldk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

BOOLEAN
ConditionVariableTest (
    VOID
    );

BOOLEAN
LibraryTest (
    VOID
    );

BOOLEAN
LdrTest (
    VOID
    );

BOOLEAN
KeyedEventTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#endif

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    PAGED_CODE();

    NTSTATUS status = LdkInitialize(DriverObject, RegistryPath, 0);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    if (!ConditionVariableTest()) {
        LdkTerminate();
        return STATUS_UNSUCCESSFUL;
    }

    if (!LibraryTest()) {
        LdkTerminate();
        return STATUS_UNSUCCESSFUL;
    }
    
    if (!KeyedEventTest()) {
        LdkTerminate();
        return STATUS_UNSUCCESSFUL;
    }

    if (!LdrTest()) {
        LdkTerminate();
        return STATUS_UNSUCCESSFUL;
    }

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}

VOID
DriverUnload (
    _In_ PDRIVER_OBJECT driverObject
    )
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(driverObject);
    LdkTerminate();
}