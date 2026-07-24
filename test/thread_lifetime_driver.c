#include <Ldk/Windows.h>

BOOLEAN
ThreadLifetimeDrainTest (
	VOID
	);

BOOLEAN
ThreadLifetimeDrainFinish (
	VOID
	);

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

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
	UNREFERENCED_PARAMETER(RegistryPath);

	if (! ThreadLifetimeDrainTest() ||
		! ThreadLifetimeDrainFinish()) {
		return STATUS_UNSUCCESSFUL;
	}

	DriverObject->DriverUnload = DriverUnload;
	return STATUS_SUCCESS;
}

VOID
DriverUnload (
	_In_ PDRIVER_OBJECT DriverObject
	)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(DriverObject);
}
