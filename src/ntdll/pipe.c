#include "ntdll.h"
#include "../ldk.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtCreateNamedPipeFile)
#endif

NTSTATUS
NtCreateNamedPipeFile (
    _Out_ PHANDLE FileHandle,
    _In_ ULONG DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes,
    _Out_ PIO_STATUS_BLOCK IoStatusBlock,
    _In_ ULONG ShareAccess,
    _In_ ULONG CreateDisposition,
    _In_ ULONG CreateOptions,
    _In_ ULONG NamedPipeType,
    _In_ ULONG ReadMode,
    _In_ ULONG CompletionMode,
    _In_ ULONG MaximumInstances,
    _In_ ULONG InboundQuota,
    _In_ ULONG OutboundQuota,
    _In_opt_ PLARGE_INTEGER DefaultTimeout
    )
{
    NAMED_PIPE_CREATE_PARAMETERS namedPipeCreateParameters;

    PAGED_CODE();

    if (ARGUMENT_PRESENT(DefaultTimeout)) {
        namedPipeCreateParameters.TimeoutSpecified = TRUE;
        namedPipeCreateParameters.DefaultTimeout = *DefaultTimeout;
    } else {
        namedPipeCreateParameters.TimeoutSpecified = FALSE;
    }

    namedPipeCreateParameters.NamedPipeType = NamedPipeType;
    namedPipeCreateParameters.ReadMode = ReadMode;
    namedPipeCreateParameters.CompletionMode = CompletionMode;
    namedPipeCreateParameters.MaximumInstances = MaximumInstances;
    namedPipeCreateParameters.InboundQuota = InboundQuota;
    namedPipeCreateParameters.OutboundQuota = OutboundQuota;

    return IoCreateFile( FileHandle,
                         DesiredAccess,
                         ObjectAttributes,
                         IoStatusBlock,
                         (PLARGE_INTEGER) NULL,
                         0L,
                         ShareAccess,
                         CreateDisposition,
                         CreateOptions,
                         (PVOID) NULL,
                         0L,
                         CreateFileTypeNamedPipe,
                         &namedPipeCreateParameters,
                         0 );
}