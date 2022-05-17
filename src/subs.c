#include "ldk.h"

_Must_inspect_result_
_At_(String->Length, _Out_range_(==, 0))
_At_(String->MaximumLength, _In_) 
_At_(String->Buffer, _Pre_maybenull_ _Post_notnull_ _Post_writable_byte_size_(String->MaximumLength))
NTSTATUS
LdkAllocateUnicodeString (
    _Inout_ _At_(String->Buffer, __drv_allocatesMem(Mem))
		PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine allocates a unicode string

Arguments:

    String - supplies the size of the string to be allocated in the MaximumLength field
             return the unicode string

Return Value:

    STATUS_SUCCESS                  - success
    STATUS_INSUFFICIENT_RESOURCES   - failure

--*/
{	
    String->Buffer = ExAllocatePoolWithTag( LdkpDefaultPoolType,
                                            String->MaximumLength,
											TAG_UNICODE_POOL );

    if (String->Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
	
    String->Length = 0;
	
    return STATUS_SUCCESS;
}

VOID
LdkFreeUnicodeString (
	_Inout_ _At_(String->Buffer, _Frees_ptr_opt_)
		PUNICODE_STRING String
    )
/*++

Routine Description:

    This routine frees a unicode string

Arguments:

    String - supplies the string to be freed

Return Value:

    None

--*/
{
	if (! String->Buffer) {
		return;
	}

    ExFreePoolWithTag( String->Buffer,
                       TAG_UNICODE_POOL );

    String->Length = String->MaximumLength = 0;
    String->Buffer = NULL;
}

_Must_inspect_result_
NTSTATUS
LdkDuplicateUnicodeString (
    _In_ PCUNICODE_STRING StringIn,
    _Out_ _At_(StringOut->Buffer, __drv_allocatesMem(Mem))
		PUNICODE_STRING StringOut
    )
/*++

Routine Description:

    This routine allocates a unicode string that is a duplicate of the specified source unicode string.

Arguments:
	
	StringIn - A pointer to the source string buffer.
	StringOut - A pointer to the destination string buffer.

Return Value:

    STATUS_SUCCESS                  - success
    STATUS_INSUFFICIENT_RESOURCES   - failure

--*/
{
	NTSTATUS Status;

    StringOut->MaximumLength = StringIn->MaximumLength;

	Status = LdkAllocateUnicodeString( StringOut );

    if (NT_SUCCESS(Status)) {
        RtlCopyUnicodeString( StringOut,
                              StringIn );
    }

    return Status;
}



_Must_inspect_result_
_At_(String->Length, _Out_range_(==, 0))
_At_(String->MaximumLength, _In_) 
_At_(String->Buffer, _Pre_maybenull_ _Post_notnull_ _Post_writable_byte_size_(String->MaximumLength))
NTSTATUS
LdkAllocateAnsiString (
    _Inout_ _At_(String->Buffer, __drv_allocatesMem(Mem))
		PANSI_STRING String
    )
/*++

Routine Description:

    This routine allocates a ansi string

Arguments:

    String - supplies the size of the string to be allocated in the MaximumLength field
             return the ansi string

Return Value:

    STATUS_SUCCESS                  - success
    STATUS_INSUFFICIENT_RESOURCES   - failure

--*/
{	
    String->Buffer = ExAllocatePoolWithTag( LdkpDefaultPoolType,
                                            String->MaximumLength,
											TAG_ANSI_POOL );

    if (String->Buffer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
	
    String->Length = 0;
	
    return STATUS_SUCCESS;
}

VOID
LdkFreeAnsiString (
	_Inout_ _At_(String->Buffer, _Frees_ptr_opt_)
		PANSI_STRING String
    )
/*++

Routine Description:

    This routine frees a ansi string

Arguments:

    String - supplies the string to be freed

Return Value:

    None

--*/
{
	if (! String->Buffer) {
		return;
	}

    ExFreePoolWithTag( String->Buffer,
                       TAG_ANSI_POOL );

    String->Length = String->MaximumLength = 0;
    String->Buffer = NULL;
}

_Must_inspect_result_
NTSTATUS
LdkDuplicateAnsiString (
    _In_ PCANSI_STRING StringIn,
    _Out_ _At_(StringOut->Buffer, __drv_allocatesMem(Mem))
		PANSI_STRING StringOut
    )
/*++

Routine Description:

    This routine allocates a ansi string that is a duplicate of the specified source ansi string.

Arguments:
	
	StringIn - A pointer to the source string buffer.
	StringOut - A pointer to the destination string buffer.

Return Value:

    STATUS_SUCCESS                  - success
    STATUS_INSUFFICIENT_RESOURCES   - failure

--*/
{
	NTSTATUS Status;

    StringOut->MaximumLength = StringIn->MaximumLength;
	
	Status = LdkAllocateAnsiString( StringOut );
	
    if (NT_SUCCESS(Status)) {
        RtlCopyString( StringOut,
                       StringIn );
    }
	
    return Status;
}



_Must_inspect_result_
NTSTATUS
NTAPI
LdkAnsiStringToUnicodeString (
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_) PUNICODE_STRING DestinationString,
    _In_ PCANSI_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    )
{
	NTSTATUS Status;
	if (AllocateDestinationString) {
        DestinationString->MaximumLength = (USHORT)RtlAnsiStringToUnicodeSize( SourceString );
		Status = LdkAllocateUnicodeString( DestinationString );
		if (! NT_SUCCESS(Status)) {
			return Status;
		}
        Status = RtlAnsiStringToUnicodeString( DestinationString,
                                               SourceString,
                                               FALSE );
        if (! NT_SUCCESS(Status)) {
            LdkFreeUnicodeString( DestinationString );
        }
	    return Status;
	} else {
        return RtlAnsiStringToUnicodeString( DestinationString,
                                             SourceString,
                                             FALSE );
	}
}

_When_(AllocateDestinationString,
       _At_(DestinationString->MaximumLength,
            _Out_range_(<=, (SourceString->MaximumLength / sizeof(WCHAR)))))
_When_(!AllocateDestinationString,
       _At_(DestinationString->Buffer, _Const_)
       _At_(DestinationString->MaximumLength, _Const_))
_When_(AllocateDestinationString, _Must_inspect_result_)
NTSTATUS
NTAPI
LdkUnicodeStringToAnsiString (
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_) PANSI_STRING DestinationString,
    _In_ PCUNICODE_STRING SourceString,
    _In_ BOOLEAN AllocateDestinationString
    )
{
	NTSTATUS Status;
	if (AllocateDestinationString) {
        DestinationString->MaximumLength = (USHORT)RtlUnicodeStringToAnsiSize( SourceString );
		Status = LdkAllocateAnsiString( DestinationString );
		if (! NT_SUCCESS(Status)) {
			return Status;
		}
        Status = RtlUnicodeStringToAnsiString( DestinationString,
                                               SourceString,
                                               FALSE );
        if (! NT_SUCCESS(Status)) {
            LdkFreeAnsiString( DestinationString );
        }
	    return Status;
	} else {
        return RtlUnicodeStringToAnsiString( DestinationString,
                                             SourceString,
                                             FALSE );
	}
}



NTSTATUS
LdkQueryModuleInformationFromAddress (
	_In_ PVOID Address,
	_Out_ PRTL_PROCESS_MODULE_INFORMATION ModuleInformation
	)
{
    NTSTATUS Status;
    PRTL_PROCESS_MODULES Modules;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    PVOID Buffer;
    ULONG BufferSize = 65536;
    ULONG ReturnLength;
    ULONG i;

retry:
    Buffer = ExAllocatePoolWithTag(LdkpDefaultPoolType, BufferSize, TAG_TMP_POOL);

    if (!Buffer) {
        return STATUS_NO_MEMORY;
    }
	
    Status = ZwQuerySystemInformation( SystemModuleInformation,
                                       Buffer,
                                       BufferSize,
                                       &ReturnLength );

	if (Status == STATUS_INFO_LENGTH_MISMATCH) {
		ExFreePoolWithTag(Buffer, TAG_TMP_POOL);
		BufferSize = ReturnLength;
		goto retry;
	}

	if (NT_SUCCESS(Status)) {

		Status = STATUS_NOT_FOUND;

		Modules = (PRTL_PROCESS_MODULES)Buffer;
		for (i = 0, ModuleInfo = Modules->Modules;
			 i < Modules->NumberOfModules;
			 i++, ModuleInfo++) {

			if (((ULONG_PTR)ModuleInfo->ImageBase < (ULONG_PTR)Address &&
				((ULONG_PTR)(Add2Ptr(ModuleInfo->ImageBase, ModuleInfo->ImageSize)) >= (ULONG_PTR)Address))) {

				RtlCopyMemory(ModuleInformation, ModuleInfo, sizeof(RTL_PROCESS_MODULE_INFORMATION));
				Status = STATUS_SUCCESS;
				break;
			}
		}
	} 

	ExFreePoolWithTag(Buffer, TAG_TMP_POOL);

	return Status;
}