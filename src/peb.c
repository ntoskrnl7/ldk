#include "ldk.h"
#include "peb.h"



BOOLEAN LdkPebInitialized = FALSE;

RTL_USER_PROCESS_PARAMETERS LdkpProcessParameters;

LDK_PEB LdkpPeb;



UNICODE_STRING NtSystemRoot;



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
    NTSTATUS Status;

    PAGED_CODE();

    Status = IoCreateDevice( DriverObject,
                             0,
                             NULL,
                             FILE_DEVICE_CONSOLE,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &LdkpStdInDeviceObject );
    if (! NT_SUCCESS(Status)) {
        LdkpUninitializeStdio();
        return Status;
    }
    Status = ObOpenObjectByPointer( LdkpStdInDeviceObject,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    0,
                                    NULL,
                                    KernelMode,
                                    &LdkpStdInHandle );
    if (! NT_SUCCESS(Status)) {
        LdkpUninitializeStdio();
        return Status;
    }
    Status = IoCreateDevice( DriverObject,
                             0,
                             NULL,
                             FILE_DEVICE_CONSOLE,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &LdkpStdOutDeviceObject );
    if (! NT_SUCCESS(Status)) {
        LdkpUninitializeStdio();
        return Status;
    }
    Status = ObOpenObjectByPointer( LdkpStdOutDeviceObject,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    0,
                                    NULL,
                                    KernelMode,
                                    &LdkpStdOutHandle );
    if (! NT_SUCCESS(Status)) {
        LdkpUninitializeStdio();
        return Status;
    }
    Status = IoCreateDevice( DriverObject,
                             0,
                             NULL,
                             FILE_DEVICE_CONSOLE,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &LdkpStdErrorDeviceObject );
    if (! NT_SUCCESS(Status)) {
        LdkpUninitializeStdio();
        return Status;
    }
    Status = ObOpenObjectByPointer( LdkpStdErrorDeviceObject,
                                    OBJ_KERNEL_HANDLE,
                                    NULL,
                                    0,
                                    NULL,
                                    KernelMode,
                                    &LdkpStdErrHandle );
    if (! NT_SUCCESS(Status)) {
        LdkpUninitializeStdio();
        return Status;
    }
    for (int i = 0 ; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
        DriverObject->MajorFunction[i] = LdkpDriverDispatchOther;
    }
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverObject->MajorFunction[IRP_MJ_CLOSE] = LdkpDriverDispatchCreateClose;
    DriverObject->MajorFunction[IRP_MJ_READ] = DriverObject->MajorFunction[IRP_MJ_WRITE] = LdkpDriverDispatchReadWrite;
    return Status;
}



PLDK_PEB
LDKAPI
LdkCurrentPeb (
	VOID
	)
{
	return &LdkpPeb;
}


VOID
LdkpUninitializeProcessParameters (
    VOID
    );

NTSTATUS
LdkpInitializeProcessParameters (
	_In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
	)
{
	PAGED_CODE();
    UNREFERENCED_PARAMETER(RegistryPath);

	NTSTATUS Status = LdkpInitializeStdio( DriverObject );
	if (! NT_SUCCESS(Status)) {
		return Status;
	}

    LdkpProcessParameters.Length = sizeof(LdkpProcessParameters);
    LdkpProcessParameters.MaximumLength = sizeof(LdkpProcessParameters);

    LdkpProcessParameters.StandardInput = LdkpStdInHandle;
    LdkpProcessParameters.StandardOutput = LdkpStdOutHandle;
    LdkpProcessParameters.StandardError = LdkpStdErrHandle;

    LdkpProcessParameters.EnvironmentSize = PAGE_SIZE;
    LdkpProcessParameters.Environment = RtlAllocateHeap( RtlProcessHeap(),
                                                         HEAP_ZERO_MEMORY,
                                                         LdkpProcessParameters.EnvironmentSize );
	if (! LdkpProcessParameters.Environment) {
        LdkpUninitializeStdio();
		return Status;
	}

    UNICODE_STRING Name;
    RtlInitUnicodeString( &Name,
                          L"windir" );
    Status = RtlSetEnvironmentVariable( &LdkpProcessParameters.Environment,
                                        &Name,
                                        &NtSystemRoot );
	if (! NT_SUCCESS(Status)) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     LdkpProcessParameters.Environment );
        LdkpUninitializeStdio();
		return Status;
	}

    RtlInitUnicodeString( &Name,
                          L"SystemDrive" );
    UNICODE_STRING Value;
    Value = NtSystemRoot;
    Value.Length = 2 * sizeof(WCHAR);
    Status = RtlSetEnvironmentVariable( &LdkpProcessParameters.Environment,
                                        &Name,
                                        &Value );
	if (! NT_SUCCESS(Status)) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     LdkpProcessParameters.Environment );
        LdkpUninitializeStdio();
		return Status;
	}

    RtlInitUnicodeString( &Name,
                          L"SystemRoot" );
    Status = RtlSetEnvironmentVariable( &LdkpProcessParameters.Environment,
                                        &Name,
                                        &NtSystemRoot );
	if (! NT_SUCCESS(Status)) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     LdkpProcessParameters.Environment );
        LdkpUninitializeStdio();
		return Status;
	}

    ANSI_STRING FullPathName = LdkpPeb.FullPathName;
    extern UNICODE_STRING RtlpDosDevicesPrefix;
    FullPathName.Buffer += (RtlpDosDevicesPrefix.Length / sizeof(WCHAR));
    FullPathName.Length -= (RtlpDosDevicesPrefix.Length / sizeof(WCHAR));
	Status = LdkAnsiStringToUnicodeString( &LdkpProcessParameters.ImagePathName,
                                           &FullPathName,
                                           TRUE );
	if (! NT_SUCCESS(Status)) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     LdkpProcessParameters.Environment );
        LdkpUninitializeStdio();
		return Status;
	}
    LdkpProcessParameters.CommandLine = LdkpProcessParameters.ImagePathName;

    UNICODE_STRING Tmp = NtSystemRoot;
    Tmp.MaximumLength = DOS_MAX_PATH_LENGTH * sizeof(WCHAR);
    Status = LdkDuplicateUnicodeString( &Tmp,
                                        &LdkpProcessParameters.CurrentDirectory.DosPath );
	if (! NT_SUCCESS(Status)) {
        LdkFreeUnicodeString( &LdkpProcessParameters.ImagePathName );
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     LdkpProcessParameters.Environment );
        LdkpUninitializeStdio();
		return Status;
	}
    RtlAppendUnicodeToString( &LdkpProcessParameters.CurrentDirectory.DosPath,
                              L"\\" );

    Status = LdkDuplicateUnicodeString( &Tmp,
                                        &LdkpProcessParameters.DllPath );
	if (! NT_SUCCESS(Status)) {
        LdkFreeUnicodeString( &LdkpProcessParameters.CurrentDirectory.DosPath );
        LdkFreeUnicodeString( &LdkpProcessParameters.ImagePathName );
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     LdkpProcessParameters.Environment );
        LdkpUninitializeStdio();
		return Status;
	}
    RtlAppendUnicodeToString( &LdkpProcessParameters.DllPath,
                              L"\\System32\\" );

    RtlInitUnicodeString( &Name,
                          L"PATH" );
    UNICODE_STRING Path;
    Path.MaximumLength = LdkpProcessParameters.DllPath.Length + sizeof(L';') + LdkpProcessParameters.DllPath.Length + sizeof(L"\\downlevel\\");
    Path.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                   HEAP_ZERO_MEMORY,
                                   Path.MaximumLength );
    if (! Path.Buffer) {
        LdkpUninitializeProcessParameters();
    }
    RtlCopyUnicodeString( &Path,
                          &LdkpProcessParameters.DllPath );
    RtlAppendUnicodeToString( &Path,
                              L";" );
    RtlAppendUnicodeStringToString( &Path,
                                    &LdkpProcessParameters.DllPath );
    RtlAppendUnicodeToString( &Path,
                              L"\\downlevel\\;" );

    Status = RtlSetEnvironmentVariable( &LdkpProcessParameters.Environment,
                                        &Name,
                                        &Path );
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 Path.Buffer );
	if (! NT_SUCCESS(Status)) {
        LdkpUninitializeProcessParameters();
		return Status;
	}

    return Status;
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

    LdkFreeUnicodeString( &LdkpProcessParameters.DllPath );
    LdkFreeUnicodeString( &LdkpProcessParameters.CurrentDirectory.DosPath );
    LdkFreeUnicodeString( &LdkpProcessParameters.ImagePathName );
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 LdkpProcessParameters.Environment );
    LdkpUninitializeStdio();
}

NTSTATUS
LdkpInitializePeb (
	_In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
	)
{
	PAGED_CODE();

    RtlInitUnicodeString( &NtSystemRoot,
                          SharedUserData->NtSystemRoot );

	RTL_PROCESS_MODULE_INFORMATION moduleInfo;
	NTSTATUS Status = LdkQueryModuleInformationFromAddress( &LdkpPeb,
                                                            &moduleInfo );
	if (! NT_SUCCESS(Status)) {
		return Status;
	}
    
    LdkpPeb.DriverObject = DriverObject;
	LdkpPeb.ImageBaseAddress = moduleInfo.ImageBase;
	LdkpPeb.ImageBaseSize = moduleInfo.ImageSize;
	LdkpPeb.CriticalSectionTimeout.QuadPart = Int32x32To64(2592000, -10000000);

	Status = LdkDuplicateUnicodeString( RegistryPath,
                                        &LdkpPeb.RegistryPath );
	if (! NT_SUCCESS(Status)) {
		return Status;
	}

	ANSI_STRING FullPathName;
	RtlInitAnsiString( &FullPathName,
                       (PSZ)moduleInfo.FullPathName );
    Status = LdkDuplicateAnsiString( &FullPathName,
                                     &LdkpPeb.FullPathName );
	if (! NT_SUCCESS(Status)) {
		LdkFreeUnicodeString( &LdkpPeb.RegistryPath );
		return Status;
	}

    PSTR FilePart = strrchr( FullPathName.Buffer,
                             OBJ_NAME_PATH_SEPARATOR );
    if (! FilePart) {
		LdkFreeAnsiString( &LdkpPeb.FullPathName );
		LdkFreeUnicodeString( &LdkpPeb.RegistryPath );
        return STATUS_UNSUCCESSFUL;
    }

    ULONG Size;
    PIMAGE_LOAD_CONFIG_DIRECTORY ImageConfigData = RtlImageDirectoryEntryToData( LdkpPeb.ImageBaseAddress,
                                                                                 TRUE,
                                                                                 IMAGE_DIRECTORY_ENTRY_LOAD_CONFIG,
                                                                                 &Size );

    RTL_HEAP_PARAMETERS HeapParameters;
    RtlZeroMemory( &HeapParameters,
                   sizeof(HeapParameters) );
    HeapParameters.Length = sizeof(HeapParameters);
    ULONG ProcessHeapFlags = HEAP_GROWABLE | HEAP_CLASS_0;
    if (ImageConfigData && Size == sizeof(IMAGE_LOAD_CONFIG_DIRECTORY)) {
        if (ImageConfigData->ProcessHeapFlags) {
            ProcessHeapFlags = ImageConfigData->ProcessHeapFlags;
        }
        if (ImageConfigData->DeCommitFreeBlockThreshold) {
            HeapParameters.DeCommitFreeBlockThreshold = ImageConfigData->DeCommitFreeBlockThreshold;
        }
        if (ImageConfigData->DeCommitTotalFreeThreshold) {
            HeapParameters.DeCommitTotalFreeThreshold = ImageConfigData->DeCommitTotalFreeThreshold;
        }
        if (ImageConfigData->MaximumAllocationSize) {
            HeapParameters.MaximumAllocationSize = ImageConfigData->MaximumAllocationSize;
        }
        if (ImageConfigData->VirtualMemoryThreshold) {
            HeapParameters.VirtualMemoryThreshold = ImageConfigData->VirtualMemoryThreshold;
        }
        if (ImageConfigData->CriticalSectionDefaultTimeout) {
            LdkpPeb.CriticalSectionTimeout.QuadPart = Int32x32To64((LONG)ImageConfigData->CriticalSectionDefaultTimeout, -10000);
        }
    }
	LdkpPeb.ProcessHeap = RtlCreateHeap( ProcessHeapFlags,
                                         NULL,
                                         0,
                                         0,
                                         NULL,
                                         &HeapParameters );
    if (LdkpPeb.ProcessHeap == NULL) {
		LdkFreeAnsiString( &LdkpPeb.FullPathName );
		LdkFreeUnicodeString( &LdkpPeb.RegistryPath );
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    Status = LdkpInitializeProcessParameters( DriverObject,
                                              RegistryPath );
	if (! NT_SUCCESS(Status)) {
        RtlDestroyHeap( LdkpPeb.ProcessHeap );
		LdkFreeAnsiString( &LdkpPeb.FullPathName );
		LdkFreeUnicodeString( &LdkpPeb.RegistryPath );
		return Status;
	}
    LdkpPeb.ProcessParameters = &LdkpProcessParameters;

	InitializeListHead(&LdkpPeb.ModuleListHead);

	extern LDK_MODULE LdkpKernel32Module;
	InsertTailList(&LdkpPeb.ModuleListHead, &LdkpKernel32Module.ActiveLinks);

	extern LDK_MODULE LdkpNtdllModule;
	InsertTailList(&LdkpPeb.ModuleListHead, &LdkpNtdllModule.ActiveLinks);

	Status = ExInitializeResourceLite( &LdkpPeb.ModuleListResource );
	if (! NT_SUCCESS(Status)) {
		LdkpUninitializeProcessParameters();
        RtlDestroyHeap( LdkpPeb.ProcessHeap );
		LdkFreeAnsiString( &LdkpPeb.FullPathName );
		LdkFreeUnicodeString( &LdkpPeb.RegistryPath );
		return Status;
	}

	ExInitializeNPagedLookasideList( &LdkpPeb.ModuleListLookaside,
                                     NULL,
                                     NULL,
                                     POOL_NX_ALLOCATION,
                                     sizeof(LDK_MODULE),
                                     'MkdL',
                                     0 );

	LdkpPeb.NumberOfProcessors = KeNumberProcessors;
	LdkpPeb.NtGlobalFlag = 0;

	RtlInitializeBitMap( &LdkpPeb.TlsBitmap,
                         &LdkpPeb.TlsBitmapBits[0],
                         LDK_TLS_SLOTS_SIZE );

	RtlInitializeBitMap( &LdkpPeb.FlsBitmap,
                         &LdkpPeb.FlsBitmapBits[0],
                         LDK_FLS_SLOTS_SIZE );

	LdkPebInitialized = TRUE;
	return STATUS_SUCCESS;
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
    RtlDestroyHeap( LdkpPeb.ProcessHeap );
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