#include "ldk.h"
#include "peb.h"
#include "nt/ntexapi.h"
#include "nt/ntldr.h"

BOOLEAN LdkPebInitialized = FALSE;

RTL_USER_PROCESS_PARAMETERS LdkpProcessParameters;

LDK_PEB LdkpPeb;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializePeb)
#pragma alloc_text(PAGE, LdkpTerminatePeb)
#endif

HANDLE LdkpStdInHandle = NULL;
HANDLE LdkpStdOutHandle = NULL;
HANDLE LdkpStdErrHandle = NULL;

PDEVICE_OBJECT LdkpStdInDeviceObject = NULL;
PDEVICE_OBJECT LdkpStdOutDeviceObject = NULL;
PDEVICE_OBJECT LdkpStdErrorDeviceObject = NULL;

_Function_class_(DRIVER_DISPATCH)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS
LdkpDriverDispatchCreateClose (
    _In_ struct _DEVICE_OBJECT *DeviceObject,
    _Inout_ struct _IRP *Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    return STATUS_SUCCESS;
}

_Function_class_(DRIVER_DISPATCH)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS
LdkpDriverDispatchReadWrite (
    _In_ struct _DEVICE_OBJECT *DeviceObject,
    _Inout_ struct _IRP *Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    return STATUS_SUCCESS;
}

_Function_class_(DRIVER_DISPATCH)
_IRQL_requires_max_(DISPATCH_LEVEL)
_IRQL_requires_same_
NTSTATUS
LdkpDriverDispatchOther (
    _In_ struct _DEVICE_OBJECT *DeviceObject,
    _Inout_ struct _IRP *Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    UNREFERENCED_PARAMETER(Irp);
    return STATUS_INVALID_DEVICE_REQUEST;
}

VOID
LdkpUninitializeStdio (
    VOID
    )
{
    PAGED_CODE();

    if (LdkpStdInHandle) {
        ZwClose(LdkpStdInHandle);
    }
    if (LdkpStdInDeviceObject) {
        IoDeleteDevice( LdkpStdInDeviceObject );
    }

    if (LdkpStdOutHandle) {
        ZwClose(LdkpStdOutHandle);
    }
    if (LdkpStdOutDeviceObject) {
        IoDeleteDevice( LdkpStdOutDeviceObject );
    }

    if (LdkpStdErrHandle) {
        ZwClose(LdkpStdErrHandle);
    }
    if (LdkpStdErrorDeviceObject) {
        IoDeleteDevice( LdkpStdErrorDeviceObject );
    }
}

NTSTATUS
LdkpInitializeStdio (
    _Inout_ PDRIVER_OBJECT DriverObject
    )
{
    NTSTATUS status;

    PAGED_CODE();

    status = IoCreateDevice( DriverObject,
                             0,
                             NULL,
                             FILE_DEVICE_CONSOLE,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &LdkpStdInDeviceObject );
    if (! NT_SUCCESS(status)) {
        LdkpUninitializeStdio();
        return status;
    }
    status = ObOpenObjectByPointer( LdkpStdInDeviceObject,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    0,
                                    NULL,
                                    KernelMode,
                                    &LdkpStdInHandle );
    if (! NT_SUCCESS(status)) {
        LdkpUninitializeStdio();
        return status;
    }
    status = IoCreateDevice( DriverObject,
                             0,
                             NULL,
                             FILE_DEVICE_CONSOLE,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &LdkpStdOutDeviceObject );
    if (! NT_SUCCESS(status)) {
        LdkpUninitializeStdio();
        return status;
    }
    status = ObOpenObjectByPointer( LdkpStdOutDeviceObject,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    0,
                                    NULL,
                                    KernelMode,
                                    &LdkpStdOutHandle );
    if (! NT_SUCCESS(status)) {
        LdkpUninitializeStdio();
        return status;
    }
    status = IoCreateDevice( DriverObject,
                             0,
                             NULL,
                             FILE_DEVICE_CONSOLE,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &LdkpStdErrorDeviceObject );
    if (! NT_SUCCESS(status)) {
        LdkpUninitializeStdio();
        return status;
    }
    status = ObOpenObjectByPointer( LdkpStdErrorDeviceObject,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    0,
                                    NULL,
                                    KernelMode,
                                    &LdkpStdErrHandle );
    if (! NT_SUCCESS(status)) {
        LdkpUninitializeStdio();
        return status;
    }
    for (int i = 0 ; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = LdkpDriverDispatchOther;
    }
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = LdkpDriverDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = DriverObject->MajorFunction[IRP_MJ_WRITE] = LdkpDriverDispatchReadWrite;
    return status;
}



PLDK_PEB
LDKAPI
LdkCurrentPeb (
	VOID
	)
{
	return &LdkpPeb;
}

NTSTATUS
LdkpInitializeProcessParameters (
	_In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
	)
{
	PAGED_CODE();
    UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS status = LdkpInitializeStdio( DriverObject );
	if (! NT_SUCCESS(status)) {
		return status;
	}

    LdkpProcessParameters.Length = sizeof(LdkpProcessParameters);
    LdkpProcessParameters.MaximumLength = sizeof(LdkpProcessParameters);

    LdkpProcessParameters.StandardInput = LdkpStdInHandle;
    LdkpProcessParameters.StandardOutput = LdkpStdOutHandle;
    LdkpProcessParameters.StandardError = LdkpStdErrHandle;

	status = LdkAnsiStringToUnicodeString( &LdkpProcessParameters.ImagePathName,
                                           &LdkpPeb.FullPathName,
                                           TRUE );
	if (! NT_SUCCESS(status)) {
        LdkpUninitializeStdio();
		return status;
	}
    return status;
}

VOID
LdkpUninitializeProcessParameters (
    VOID
    )
{
    if (! LdkIsConsoleHandle( LdkpProcessParameters.StandardInput )) {
        ZwClose( LdkpProcessParameters.StandardInput );
    }
    if (! LdkIsConsoleHandle( LdkpProcessParameters.StandardOutput )) {
        ZwClose( LdkpProcessParameters.StandardOutput );
    }
    if (! LdkIsConsoleHandle( LdkpProcessParameters.StandardError )) {
        ZwClose( LdkpProcessParameters.StandardError );
    }

    LdkFreeUnicodeString( &LdkpProcessParameters.ImagePathName );
    LdkpUninitializeStdio();
}

NTSTATUS
LdkpInitializePeb (
	_In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
	)
{
	PAGED_CODE();

	RTL_PROCESS_MODULE_INFORMATION moduleInfo;
	NTSTATUS status = LdkQueryModuleInformationFromAddress( &LdkpPeb,
                                                            &moduleInfo );
	if (! NT_SUCCESS(status)) {
		return status;
	}
    
    LdkpPeb.DriverObject = DriverObject;
	LdkpPeb.ImageBaseAddress = moduleInfo.ImageBase;
	LdkpPeb.ImageBaseSize = moduleInfo.ImageSize;
	
	status = LdkDuplicateUnicodeString( RegistryPath,
                                        &LdkpPeb.RegistryPath );
	if (! NT_SUCCESS(status)) {
		return status;
	}

	ANSI_STRING ImagePathName;
	RtlInitAnsiString(&ImagePathName, (PSZ)moduleInfo.FullPathName);
	status = LdkAnsiStringToUnicodeString( &LdkpProcessParameters.ImagePathName,
                                           &ImagePathName,
                                           TRUE );
	if (! NT_SUCCESS(status)) {
		LdkFreeUnicodeString( &LdkpPeb.RegistryPath );
		return status;
	}


    status = LdkpInitializeProcessParameters( DriverObject,
                                              RegistryPath );
	if (! NT_SUCCESS(status)) {
		LdkFreeAnsiString( &LdkpPeb.FullPathName );
		LdkFreeUnicodeString( &LdkpPeb.RegistryPath );
		return status;
	}
    LdkpPeb.ProcessParameters = &LdkpProcessParameters;


	InitializeListHead(&LdkpPeb.ModuleListHead);

	extern LDK_MODULE LdkpKernel32Module;
	InsertTailList(&LdkpPeb.ModuleListHead, &LdkpKernel32Module.ActiveLinks);

	extern LDK_MODULE LdkpNtdllModule;
	InsertTailList(&LdkpPeb.ModuleListHead, &LdkpNtdllModule.ActiveLinks);

	status = ExInitializeResourceLite( &LdkpPeb.ModuleListResource );
	if (! NT_SUCCESS(status)) {
		LdkpUninitializeProcessParameters();
		LdkFreeAnsiString( &LdkpPeb.FullPathName );
		LdkFreeUnicodeString( &LdkpPeb.RegistryPath );
		return status;
	}

	ExInitializeNPagedLookasideList( &LdkpPeb.ModuleListLookaside,
                                    NULL,
                                    NULL,
                                    POOL_NX_ALLOCATION,
                                    sizeof(LDK_MODULE),
                                    'MkdL',
                                    0 );

	LdkpPeb.NumberOfProcessors = KeNumberProcessors;
	LdkpPeb.ProcessHeap = NULL;
	LdkpPeb.NtGlobalFlag = 0;
	LdkpPeb.CriticalSectionTimeout.QuadPart = Int32x32To64(2592000, -10000000);

	RtlInitializeBitMap( &LdkpPeb.TlsBitmap,
                         &LdkpPeb.TlsBitmapBits[0],
                         LDK_TLS_SLOTS_SIZE );

	RtlInitializeBitMap( &LdkpPeb.FlsBitmap,
                         &LdkpPeb.FlsBitmapBits[0],
                         LDK_FLS_SLOTS_SIZE );

	LdkPebInitialized = NT_SUCCESS(status);
	return status;
}

VOID
LdkpTerminatePeb (
	VOID
	)
{
	PAGED_CODE();

	if (! LdkPebInitialized) {
        return;
    }

	ExDeleteResourceLite( &LdkpPeb.ModuleListResource );
	ExDeleteNPagedLookasideList( &LdkpPeb.ModuleListLookaside );

	LdkpUninitializeProcessParameters();

	LdkFreeAnsiString( &LdkpPeb.FullPathName );
	LdkFreeUnicodeString( &LdkpPeb.RegistryPath );

	LdkPebInitialized = FALSE;
}

BOOLEAN
LdkIsConsoleHandle (
	_In_ HANDLE Handle
	)
{
	return (LdkpStdInHandle == Handle) || (LdkpStdOutHandle == Handle) || (LdkpStdErrHandle == Handle);
}