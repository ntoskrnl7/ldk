#include <Ldk/Windows.h>
#include <ntddk.h>

#include "multi_instance_shared.h"

#ifndef LDK_MULTI_INSTANCE_ID
#error LDK_MULTI_INSTANCE_ID must be defined.
#endif

#if LDK_MULTI_INSTANCE_ID == 1
#define LDK_MULTI_INSTANCE_DEVICE_NAME L"\\Device\\LdkMultiA"
#define LDK_MULTI_INSTANCE_SYMBOLIC_LINK L"\\DosDevices\\LdkMultiA"
#elif LDK_MULTI_INSTANCE_ID == 2
#define LDK_MULTI_INSTANCE_DEVICE_NAME L"\\Device\\LdkMultiB"
#define LDK_MULTI_INSTANCE_SYMBOLIC_LINK L"\\DosDevices\\LdkMultiB"
#else
#error Unsupported LDK_MULTI_INSTANCE_ID.
#endif

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD LdkMultiInstanceUnload;

static PDEVICE_OBJECT LdkMultiInstanceDeviceObject;
static UNICODE_STRING LdkMultiInstanceSymbolicLink;
static DWORD LdkMultiInstanceTlsIndex = TLS_OUT_OF_INDEXES;

static NTSTATUS
LdkMultiInstanceCompleteIrp (
    _Inout_ PIRP Irp,
    _In_ NTSTATUS Status,
    _In_ ULONG_PTR Information
    )
{
    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Information;
    IoCompleteRequest( Irp,
                       IO_NO_INCREMENT );
    return Status;
}

static NTSTATUS
LdkMultiInstanceCreateClose (
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    UNREFERENCED_PARAMETER(DeviceObject);
    return LdkMultiInstanceCompleteIrp( Irp,
                                        STATUS_SUCCESS,
                                        0 );
}

static NTSTATUS
LdkMultiInstanceDeviceControl (
    _In_ PDEVICE_OBJECT DeviceObject,
    _Inout_ PIRP Irp
    )
{
    PIO_STACK_LOCATION IrpSp;
    PLDK_MULTI_INSTANCE_REQUEST Request;
    PLDK_MULTI_INSTANCE_RESPONSE Response;
    DWORD LastErrorValue;
    PVOID TlsValue;
    NTSTATUS Status;
    ULONG_PTR Information;

    UNREFERENCED_PARAMETER(DeviceObject);

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    Status = STATUS_SUCCESS;
    Information = sizeof(*Response);

    if (IrpSp->Parameters.DeviceIoControl.IoControlCode != LDK_MULTI_INSTANCE_IOCTL) {
        return LdkMultiInstanceCompleteIrp( Irp,
                                            STATUS_INVALID_DEVICE_REQUEST,
                                            0 );
    }

    if (IrpSp->Parameters.DeviceIoControl.InputBufferLength < sizeof(*Request) ||
        IrpSp->Parameters.DeviceIoControl.OutputBufferLength < sizeof(*Response)) {
        return LdkMultiInstanceCompleteIrp( Irp,
                                            STATUS_BUFFER_TOO_SMALL,
                                            0 );
    }

    Request = (PLDK_MULTI_INSTANCE_REQUEST)Irp->AssociatedIrp.SystemBuffer;
    Response = (PLDK_MULTI_INSTANCE_RESPONSE)Irp->AssociatedIrp.SystemBuffer;

    if (Request->Command == LDK_MULTI_INSTANCE_SET_QUERY) {
        if (! TlsSetValue( LdkMultiInstanceTlsIndex,
                           (PVOID)(ULONG_PTR)Request->TlsValue )) {
            Status = STATUS_UNSUCCESSFUL;
        }
        SetLastError( Request->LastErrorValue );
    } else if (Request->Command != LDK_MULTI_INSTANCE_QUERY) {
        Status = STATUS_INVALID_PARAMETER;
        Information = 0;
    }

    if (NT_SUCCESS(Status)) {
        LastErrorValue = GetLastError();
        TlsValue = TlsGetValue( LdkMultiInstanceTlsIndex );
        SetLastError( LastErrorValue );

        Response->InstanceId = LDK_MULTI_INSTANCE_ID;
        Response->TlsIndex = LdkMultiInstanceTlsIndex;
        Response->Teb = (unsigned __int64)(ULONG_PTR)LdkCurrentTeb();
        Response->Peb = (unsigned __int64)(ULONG_PTR)LdkCurrentPeb();
        Response->Thread = (unsigned __int64)(ULONG_PTR)PsGetCurrentThread();
        Response->TlsValue = (unsigned __int64)(ULONG_PTR)TlsValue;
        Response->LastErrorValue = LastErrorValue;
    }

    return LdkMultiInstanceCompleteIrp( Irp,
                                        Status,
                                        Information );
}

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNICODE_STRING DeviceName;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RegistryPath);

    RtlInitUnicodeString( &DeviceName,
                          LDK_MULTI_INSTANCE_DEVICE_NAME );
    RtlInitUnicodeString( &LdkMultiInstanceSymbolicLink,
                          LDK_MULTI_INSTANCE_SYMBOLIC_LINK );

    LdkMultiInstanceTlsIndex = TlsAlloc();
    if (LdkMultiInstanceTlsIndex == TLS_OUT_OF_INDEXES) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = IoCreateDevice( DriverObject,
                             0,
                             &DeviceName,
                             FILE_DEVICE_UNKNOWN,
                             FILE_DEVICE_SECURE_OPEN,
                             FALSE,
                             &LdkMultiInstanceDeviceObject );
    if (! NT_SUCCESS(Status)) {
        TlsFree( LdkMultiInstanceTlsIndex );
        LdkMultiInstanceTlsIndex = TLS_OUT_OF_INDEXES;
        return Status;
    }

    Status = IoCreateSymbolicLink( &LdkMultiInstanceSymbolicLink,
                                   &DeviceName );
    if (! NT_SUCCESS(Status)) {
        IoDeleteDevice( LdkMultiInstanceDeviceObject );
        LdkMultiInstanceDeviceObject = NULL;
        TlsFree( LdkMultiInstanceTlsIndex );
        LdkMultiInstanceTlsIndex = TLS_OUT_OF_INDEXES;
        return Status;
    }

    DriverObject->MajorFunction[IRP_MJ_CREATE] = LdkMultiInstanceCreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = LdkMultiInstanceCreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = LdkMultiInstanceDeviceControl;
    DriverObject->DriverUnload = LdkMultiInstanceUnload;

    return STATUS_SUCCESS;
}

VOID
LdkMultiInstanceUnload (
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    UNREFERENCED_PARAMETER(DriverObject);

    IoDeleteSymbolicLink( &LdkMultiInstanceSymbolicLink );

    if (LdkMultiInstanceDeviceObject) {
        IoDeleteDevice( LdkMultiInstanceDeviceObject );
        LdkMultiInstanceDeviceObject = NULL;
    }

    if (LdkMultiInstanceTlsIndex != TLS_OUT_OF_INDEXES) {
        TlsFree( LdkMultiInstanceTlsIndex );
        LdkMultiInstanceTlsIndex = TLS_OUT_OF_INDEXES;
    }
}
