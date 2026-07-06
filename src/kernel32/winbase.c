#include "winbase.h"
#include "../ldk.h"

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpAnsiStringToUnicodeSize (
    _In_ PANSI_STRING AnsiString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpUnicodeStringToAnsiSize (
    _In_ PUNICODE_STRING UnicodeString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpOemStringToUnicodeSize(
    _In_ PANSI_STRING OemString
    );

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpUnicodeStringToOemSize (
	_In_ PUNICODE_STRING UnicodeString
	);

PVOID
LdkpMapModuleHandle (
    _In_opt_ HMODULE hModule,
    _In_ BOOLEAN bResourcesOnly
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ldk8BitStringToStaticUnicodeString)
#pragma alloc_text(PAGE, Ldk8BitStringToDynamicUnicodeString)
#endif

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpAnsiStringToUnicodeSize (
    _In_ PANSI_STRING AnsiString
    )
{
	PAGED_CODE();
    return RtlAnsiStringToUnicodeSize( AnsiString );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpUnicodeStringToAnsiSize (
    _In_ PUNICODE_STRING UnicodeString
    )
{
	PAGED_CODE();
    return RtlUnicodeStringToAnsiSize( UnicodeString );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpOemStringToUnicodeSize(
    _In_ PANSI_STRING OemString
    )
{
	PAGED_CODE();
    return RtlOemStringToUnicodeSize( OemString );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
ULONG
LdkpUnicodeStringToOemSize (
	_In_ PUNICODE_STRING UnicodeString
	)
{
	PAGED_CODE();
    return RtlUnicodeStringToOemSize( UnicodeString );
}



ULONG
LdkSetLastNTError (
	_In_ NTSTATUS Status
	)
{
	ULONG dwErrorCode = RtlNtStatusToDosError( Status );
	SetLastError( dwErrorCode );
	return dwErrorCode;
}

PLARGE_INTEGER
LdkFormatTimeout (
	_Out_ PLARGE_INTEGER Timeout,
	_In_ DWORD Milliseconds
	)
{
	if (Timeout == NULL) {
		return NULL;
	}
	if (Milliseconds == INFINITE) {
		return NULL;
	}
	Timeout->QuadPart = UInt32x32To64(Milliseconds, 10000);
	Timeout->QuadPart *= -1;
	return Timeout;
}

#ifndef ERROR_MR_MID_NOT_FOUND
#define ERROR_MR_MID_NOT_FOUND 317L
#endif

#define LDK_RT_MESSAGETABLE 11
#define LDK_MESSAGE_RESOURCE_UNICODE 0x0001

typedef struct _LDK_MESSAGE_RESOURCE_ENTRY {
	USHORT Length;
	USHORT Flags;
	UCHAR Text[1];
} LDK_MESSAGE_RESOURCE_ENTRY, *PLDK_MESSAGE_RESOURCE_ENTRY;

typedef struct _LDK_MESSAGE_RESOURCE_BLOCK {
	ULONG LowId;
	ULONG HighId;
	ULONG OffsetToEntries;
} LDK_MESSAGE_RESOURCE_BLOCK, *PLDK_MESSAGE_RESOURCE_BLOCK;

typedef struct _LDK_MESSAGE_RESOURCE_DATA {
	ULONG NumberOfBlocks;
	LDK_MESSAGE_RESOURCE_BLOCK Blocks[1];
} LDK_MESSAGE_RESOURCE_DATA, *PLDK_MESSAGE_RESOURCE_DATA;

typedef struct _LDK_SYSTEM_MESSAGE_IMAGE {
	PCWSTR FileName;
	PVOID ImageBase;
	ULONG ImageSize;
} LDK_SYSTEM_MESSAGE_IMAGE, *PLDK_SYSTEM_MESSAGE_IMAGE;

static LDK_SYSTEM_MESSAGE_IMAGE LdkpSystemMessageImages[] = {
	{ L"\\SystemRoot\\System32\\kernel32.dll", NULL, 0 },
	{ L"\\SystemRoot\\System32\\KernelBase.dll", NULL, 0 },
};

static
LPCSTR
LdkpLookupSystemErrorMessage (
	_In_ DWORD MessageId
	)
{
	switch (MessageId) {
	case ERROR_SUCCESS:
		return "The operation completed successfully.\r\n";
	case ERROR_INVALID_FUNCTION:
		return "Incorrect function.\r\n";
	case ERROR_FILE_NOT_FOUND:
		return "The system cannot find the file specified.\r\n";
	case ERROR_PATH_NOT_FOUND:
		return "The system cannot find the path specified.\r\n";
	case ERROR_TOO_MANY_OPEN_FILES:
		return "The system cannot open the file.\r\n";
	case ERROR_ACCESS_DENIED:
		return "Access is denied.\r\n";
	case ERROR_INVALID_HANDLE:
		return "The handle is invalid.\r\n";
	case ERROR_NOT_ENOUGH_MEMORY:
	case ERROR_OUTOFMEMORY:
		return "Not enough memory resources are available to process this command.\r\n";
	case ERROR_INVALID_DATA:
		return "The data is invalid.\r\n";
	case ERROR_INVALID_DRIVE:
		return "The system cannot find the drive specified.\r\n";
	case ERROR_NO_MORE_FILES:
		return "There are no more files.\r\n";
	case ERROR_WRITE_PROTECT:
		return "The media is write protected.\r\n";
	case ERROR_BAD_UNIT:
		return "The system cannot find the device specified.\r\n";
	case ERROR_NOT_READY:
		return "The device is not ready.\r\n";
	case ERROR_BAD_COMMAND:
		return "The device does not recognize the command.\r\n";
	case ERROR_CRC:
		return "Data error (cyclic redundancy check).\r\n";
	case ERROR_BAD_LENGTH:
		return "The program issued a command but the command length is incorrect.\r\n";
	case ERROR_SEEK:
		return "The drive cannot locate a specific area or track on the disk.\r\n";
	case ERROR_NOT_DOS_DISK:
		return "The specified disk or diskette cannot be accessed.\r\n";
	case ERROR_SECTOR_NOT_FOUND:
		return "The drive cannot find the sector requested.\r\n";
	case ERROR_OUT_OF_PAPER:
		return "The printer is out of paper.\r\n";
	case ERROR_WRITE_FAULT:
		return "The system cannot write to the specified device.\r\n";
	case ERROR_READ_FAULT:
		return "The system cannot read from the specified device.\r\n";
	case ERROR_GEN_FAILURE:
		return "A device attached to the system is not functioning.\r\n";
	case ERROR_SHARING_VIOLATION:
		return "The process cannot access the file because it is being used by another process.\r\n";
	case ERROR_LOCK_VIOLATION:
		return "The process cannot access the file because another process has locked a portion of the file.\r\n";
	case ERROR_HANDLE_EOF:
		return "Reached the end of the file.\r\n";
	case ERROR_HANDLE_DISK_FULL:
		return "The disk is full.\r\n";
	case ERROR_NOT_SUPPORTED:
		return "The request is not supported.\r\n";
	case ERROR_FILE_EXISTS:
		return "The file exists.\r\n";
	case ERROR_CANNOT_MAKE:
		return "The directory or file cannot be created.\r\n";
	case ERROR_INVALID_PARAMETER:
		return "The parameter is incorrect.\r\n";
	case ERROR_BROKEN_PIPE:
		return "The pipe has been ended.\r\n";
	case ERROR_CALL_NOT_IMPLEMENTED:
		return "This function is not supported on this system.\r\n";
	case ERROR_INSUFFICIENT_BUFFER:
		return "The data area passed to a system call is too small.\r\n";
	case ERROR_INVALID_NAME:
		return "The filename, directory name, or volume label syntax is incorrect.\r\n";
	case ERROR_DIR_NOT_EMPTY:
		return "The directory is not empty.\r\n";
	case ERROR_ALREADY_EXISTS:
		return "Cannot create a file when that file already exists.\r\n";
	case ERROR_ENVVAR_NOT_FOUND:
		return "The system could not find the environment option that was entered.\r\n";
	case ERROR_FILENAME_EXCED_RANGE:
		return "The filename or extension is too long.\r\n";
	case ERROR_NEGATIVE_SEEK:
		return "An attempt was made to move the file pointer before the beginning of the file.\r\n";
	case ERROR_DIRECTORY:
		return "The directory name is invalid.\r\n";
	case ERROR_NOT_FOUND:
		return "Element not found.\r\n";
	case ERROR_INVALID_FLAGS:
		return "Invalid flags.\r\n";
	case ERROR_NO_UNICODE_TRANSLATION:
		return "No mapping for the Unicode character exists in the target multi-byte code page.\r\n";
	default:
		return NULL;
	}
}

static
NTSTATUS
LdkpReadWholeFile (
	_In_ PCWSTR FileName,
	_Outptr_result_bytebuffer_(*DataSize) PVOID *Data,
	_Out_ PULONG DataSize
	)
{
	UNICODE_STRING NtFileName;
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatus;
	FILE_STANDARD_INFORMATION StandardInfo;
	LARGE_INTEGER ByteOffset;
	HANDLE FileHandle = NULL;
	PVOID Buffer = NULL;
	NTSTATUS Status;

	PAGED_CODE();

	*Data = NULL;
	*DataSize = 0;

	RtlInitUnicodeString( &NtFileName,
						  FileName );
	InitializeObjectAttributes( &ObjectAttributes,
								&NtFileName,
								OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
								NULL,
								NULL );

	Status = ZwOpenFile( &FileHandle,
						 FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
						 &ObjectAttributes,
						 &IoStatus,
						 FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
						 FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
	if (! NT_SUCCESS(Status)) {
		return Status;
	}

	try {
		Status = ZwQueryInformationFile( FileHandle,
										 &IoStatus,
										 &StandardInfo,
										 sizeof(StandardInfo),
										 FileStandardInformation );
		if (! NT_SUCCESS(Status)) {
			leave;
		}

		if ((StandardInfo.EndOfFile.HighPart != 0) ||
			(StandardInfo.EndOfFile.LowPart < sizeof(IMAGE_DOS_HEADER))) {
			Status = STATUS_INVALID_IMAGE_FORMAT;
			leave;
		}

		Buffer = RtlAllocateHeap( RtlProcessHeap(),
								  0,
								  StandardInfo.EndOfFile.LowPart );
		if (Buffer == NULL) {
			Status = STATUS_NO_MEMORY;
			leave;
		}

		ByteOffset.QuadPart = 0;
		Status = ZwReadFile( FileHandle,
							 NULL,
							 NULL,
							 NULL,
							 &IoStatus,
							 Buffer,
							 StandardInfo.EndOfFile.LowPart,
							 &ByteOffset,
							 NULL );
		if (! NT_SUCCESS(Status)) {
			leave;
		}

		*Data = Buffer;
		*DataSize = StandardInfo.EndOfFile.LowPart;
		Buffer = NULL;
	} finally {
		if (Buffer != NULL) {
			RtlFreeHeap( RtlProcessHeap(),
						 0,
						 Buffer );
		}

		ZwClose( FileHandle );
	}

	return Status;
}

VOID
LdkpInitializeWinBaseMessageResources (
	VOID
	)
{
	ULONG Index;

	PAGED_CODE();

	for (Index = 0; Index < RTL_NUMBER_OF(LdkpSystemMessageImages); Index++) {
		PLDK_SYSTEM_MESSAGE_IMAGE Image = &LdkpSystemMessageImages[Index];
		PVOID Data = NULL;
		ULONG DataSize = 0;

		if (Image->ImageBase != NULL) {
			continue;
		}

		if (NT_SUCCESS(LdkpReadWholeFile( Image->FileName,
										  &Data,
										  &DataSize ))) {
			Image->ImageBase = Data;
			Image->ImageSize = DataSize;
		}
	}
}

VOID
LdkpTerminateWinBaseMessageResources (
	VOID
	)
{
	ULONG Index;

	PAGED_CODE();

	for (Index = 0; Index < RTL_NUMBER_OF(LdkpSystemMessageImages); Index++) {
		PLDK_SYSTEM_MESSAGE_IMAGE Image = &LdkpSystemMessageImages[Index];

		if (Image->ImageBase != NULL) {
			RtlFreeHeap( RtlProcessHeap(),
						 0,
						 Image->ImageBase );
			Image->ImageBase = NULL;
			Image->ImageSize = 0;
		}
	}
}

static
PVOID
LdkpPeRvaToFilePointer (
	_In_reads_bytes_(ImageSize) PVOID ImageBase,
	_In_ ULONG ImageSize,
	_In_ ULONG Rva,
	_In_ ULONG RequiredSize
	)
{
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;
	USHORT SectionIndex;

	if (RequiredSize > ImageSize) {
		return NULL;
	}

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return NULL;
	}

	if ((DosHeader->e_lfanew < 0) ||
		((ULONG)DosHeader->e_lfanew > ImageSize - sizeof(IMAGE_NT_HEADERS))) {
		return NULL;
	}

	NtHeader = (PIMAGE_NT_HEADERS)Add2Ptr(ImageBase, DosHeader->e_lfanew);
	if (NtHeader->Signature != IMAGE_NT_SIGNATURE) {
		return NULL;
	}

	SectionHeader = IMAGE_FIRST_SECTION( NtHeader );
	for (SectionIndex = 0;
		 SectionIndex < NtHeader->FileHeader.NumberOfSections;
		 SectionIndex++) {
		ULONG VirtualAddress = SectionHeader[SectionIndex].VirtualAddress;
		ULONG VirtualSize = SectionHeader[SectionIndex].Misc.VirtualSize;
		ULONG RawSize = SectionHeader[SectionIndex].SizeOfRawData;
		ULONG Span = max(VirtualSize, RawSize);
		ULONG RawPointer = SectionHeader[SectionIndex].PointerToRawData;

		if (Span == 0) {
			continue;
		}

		if (RequiredSize > Span) {
			continue;
		}

		if ((Rva >= VirtualAddress) &&
			(Rva - VirtualAddress <= Span - RequiredSize)) {
			ULONG FileOffset = RawPointer + (Rva - VirtualAddress);
			if ((RawPointer > ImageSize) ||
				(FileOffset > ImageSize - RequiredSize)) {
				return NULL;
			}

			return Add2Ptr(ImageBase, FileOffset);
		}
	}

	return NULL;
}

static
BOOLEAN
LdkpGetPeResourceDirectory (
	_In_reads_bytes_(ImageSize) PVOID ImageBase,
	_In_ ULONG ImageSize,
	_Outptr_result_bytebuffer_(*ResourceSize) PIMAGE_RESOURCE_DIRECTORY *ResourceDirectory,
	_Out_ PULONG ResourceSize
	)
{
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeader;
	IMAGE_DATA_DIRECTORY Directory;

	*ResourceDirectory = NULL;
	*ResourceSize = 0;

	DosHeader = (PIMAGE_DOS_HEADER)ImageBase;
	if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
		return FALSE;
	}

	if ((DosHeader->e_lfanew < 0) ||
		((ULONG)DosHeader->e_lfanew > ImageSize - sizeof(IMAGE_NT_HEADERS))) {
		return FALSE;
	}

	NtHeader = (PIMAGE_NT_HEADERS)Add2Ptr(ImageBase, DosHeader->e_lfanew);
	if (NtHeader->Signature != IMAGE_NT_SIGNATURE) {
		return FALSE;
	}

	Directory = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_RESOURCE];
	if ((Directory.VirtualAddress == 0) || (Directory.Size < sizeof(IMAGE_RESOURCE_DIRECTORY))) {
		return FALSE;
	}

	*ResourceDirectory = (PIMAGE_RESOURCE_DIRECTORY)LdkpPeRvaToFilePointer(
		ImageBase,
		ImageSize,
		Directory.VirtualAddress,
		sizeof(IMAGE_RESOURCE_DIRECTORY) );
	if (*ResourceDirectory == NULL) {
		return FALSE;
	}

	*ResourceSize = Directory.Size;
	return TRUE;
}

static
BOOLEAN
LdkpResourceOffsetIsValid (
	_In_ ULONG Offset,
	_In_ ULONG ResourceSize,
	_In_ ULONG RequiredSize
	)
{
	return (RequiredSize <= ResourceSize) && (Offset <= ResourceSize - RequiredSize);
}

static
PIMAGE_RESOURCE_DIRECTORY_ENTRY
LdkpFindResourceDirectoryEntryById (
	_In_reads_bytes_(ResourceSize) PIMAGE_RESOURCE_DIRECTORY ResourceBase,
	_In_ ULONG ResourceSize,
	_In_ ULONG DirectoryOffset,
	_In_ USHORT Id
	)
{
	PIMAGE_RESOURCE_DIRECTORY Directory;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY Entry;
	ULONG EntryCount;
	ULONG Index;

	if (! LdkpResourceOffsetIsValid( DirectoryOffset,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		return NULL;
	}

	Directory = (PIMAGE_RESOURCE_DIRECTORY)Add2Ptr(ResourceBase, DirectoryOffset);
	EntryCount = Directory->NumberOfNamedEntries + Directory->NumberOfIdEntries;
	if (EntryCount == 0) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( DirectoryOffset + sizeof(IMAGE_RESOURCE_DIRECTORY),
									 ResourceSize,
									 EntryCount * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) )) {
		return NULL;
	}

	Entry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(Directory + 1);
	for (Index = 0; Index < EntryCount; Index++) {
		if (! Entry[Index].NameIsString && (Entry[Index].Id == Id)) {
			return &Entry[Index];
		}
	}

	return NULL;
}

static
PIMAGE_RESOURCE_DIRECTORY_ENTRY
LdkpGetFirstResourceDirectoryEntry (
	_In_reads_bytes_(ResourceSize) PIMAGE_RESOURCE_DIRECTORY ResourceBase,
	_In_ ULONG ResourceSize,
	_In_ ULONG DirectoryOffset
	)
{
	PIMAGE_RESOURCE_DIRECTORY Directory;
	ULONG EntryCount;

	if (! LdkpResourceOffsetIsValid( DirectoryOffset,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		return NULL;
	}

	Directory = (PIMAGE_RESOURCE_DIRECTORY)Add2Ptr(ResourceBase, DirectoryOffset);
	EntryCount = Directory->NumberOfNamedEntries + Directory->NumberOfIdEntries;
	if (EntryCount == 0) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( DirectoryOffset + sizeof(IMAGE_RESOURCE_DIRECTORY),
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) )) {
		return NULL;
	}

	return (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(Directory + 1);
}

static
BOOLEAN
LdkpResourceIdMatches (
	_In_ LPCWSTR Resource,
	_In_ USHORT Id
	)
{
	return IS_INTRESOURCE(Resource) && ((USHORT)(ULONG_PTR)Resource == Id);
}

static
BOOLEAN
LdkpResourceStringMatches (
	_In_reads_bytes_(ResourceSize) PIMAGE_RESOURCE_DIRECTORY ResourceBase,
	_In_ ULONG ResourceSize,
	_In_ PIMAGE_RESOURCE_DIRECTORY_ENTRY Entry,
	_In_ LPCWSTR Name
	)
{
	PIMAGE_RESOURCE_DIR_STRING_U ResourceName;
	ULONG NameOffset;
	ULONG RequiredSize;
	USHORT Length;
	ULONG Index;

	if (! Entry->NameIsString || IS_INTRESOURCE(Name)) {
		return FALSE;
	}

	NameOffset = Entry->NameOffset;
	if (! LdkpResourceOffsetIsValid( NameOffset,
									 ResourceSize,
									 FIELD_OFFSET(IMAGE_RESOURCE_DIR_STRING_U, NameString) )) {
		return FALSE;
	}

	ResourceName = (PIMAGE_RESOURCE_DIR_STRING_U)Add2Ptr(ResourceBase, NameOffset);
	Length = ResourceName->Length;
	RequiredSize = FIELD_OFFSET(IMAGE_RESOURCE_DIR_STRING_U, NameString) +
				   Length * sizeof(WCHAR);
	if (! LdkpResourceOffsetIsValid( NameOffset,
									 ResourceSize,
									 RequiredSize )) {
		return FALSE;
	}

	for (Index = 0; Index < Length; Index++) {
		if (Name[Index] == UNICODE_NULL ||
			Name[Index] != ResourceName->NameString[Index]) {
			return FALSE;
		}
	}

	return Name[Length] == UNICODE_NULL;
}

static
PIMAGE_RESOURCE_DIRECTORY_ENTRY
LdkpFindResourceDirectoryEntry (
	_In_reads_bytes_(ResourceSize) PIMAGE_RESOURCE_DIRECTORY ResourceBase,
	_In_ ULONG ResourceSize,
	_In_ ULONG DirectoryOffset,
	_In_ LPCWSTR Name
	)
{
	PIMAGE_RESOURCE_DIRECTORY Directory;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY Entry;
	ULONG EntryCount;
	ULONG Index;

	if (! LdkpResourceOffsetIsValid( DirectoryOffset,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		return NULL;
	}

	Directory = (PIMAGE_RESOURCE_DIRECTORY)Add2Ptr(ResourceBase, DirectoryOffset);
	EntryCount = Directory->NumberOfNamedEntries + Directory->NumberOfIdEntries;
	if (EntryCount == 0) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( DirectoryOffset + sizeof(IMAGE_RESOURCE_DIRECTORY),
									 ResourceSize,
									 EntryCount * sizeof(IMAGE_RESOURCE_DIRECTORY_ENTRY) )) {
		return NULL;
	}

	Entry = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)(Directory + 1);
	for (Index = 0; Index < EntryCount; Index++) {
		if (! Entry[Index].NameIsString) {
			if (LdkpResourceIdMatches( Name,
									   Entry[Index].Id )) {
				return &Entry[Index];
			}
		} else if (LdkpResourceStringMatches( ResourceBase,
											  ResourceSize,
											  &Entry[Index],
											  Name )) {
			return &Entry[Index];
		}
	}

	return NULL;
}

static
PVOID
LdkpMappedImageRvaToPointer (
	_In_ PVOID ImageBase,
	_In_ ULONG Rva,
	_In_ ULONG RequiredSize
	)
{
	PIMAGE_NT_HEADERS NtHeader;
	ULONG SizeOfImage;

	NtHeader = RtlImageNtHeader( ImageBase );
	if (NtHeader == NULL) {
		return NULL;
	}

	SizeOfImage = NtHeader->OptionalHeader.SizeOfImage;
	if ((RequiredSize > SizeOfImage) ||
		(Rva > SizeOfImage - RequiredSize)) {
		return NULL;
	}

	return Add2Ptr(ImageBase, Rva);
}

static
PIMAGE_RESOURCE_DATA_ENTRY
LdkpFindMappedResourceDataEntry (
	_In_ PVOID ImageBase,
	_In_ LPCWSTR Type,
	_In_ LPCWSTR Name,
	_In_ WORD Language,
	_Out_ PDWORD ErrorCode
	)
{
	PIMAGE_RESOURCE_DIRECTORY ResourceBase;
	ULONG ResourceSize;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY TypeEntry;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY NameEntry;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY LanguageEntry;
	PIMAGE_RESOURCE_DATA_ENTRY DataEntry;

	*ErrorCode = ERROR_RESOURCE_TYPE_NOT_FOUND;

	if (Type == NULL) {
		*ErrorCode = ERROR_RESOURCE_TYPE_NOT_FOUND;
		return NULL;
	}

	if (Name == NULL) {
		*ErrorCode = ERROR_RESOURCE_NAME_NOT_FOUND;
		return NULL;
	}

	ResourceBase = RtlImageDirectoryEntryToData(
		ImageBase,
		TRUE,
		IMAGE_DIRECTORY_ENTRY_RESOURCE,
		&ResourceSize );
	if ((ResourceBase == NULL) || (ResourceSize == 0)) {
		return NULL;
	}

	TypeEntry = LdkpFindResourceDirectoryEntry(
		ResourceBase,
		ResourceSize,
		0,
		Type );
	if ((TypeEntry == NULL) || ! TypeEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( TypeEntry->OffsetToDirectory,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		*ErrorCode = ERROR_RESOURCE_DATA_NOT_FOUND;
		return NULL;
	}

	*ErrorCode = ERROR_RESOURCE_NAME_NOT_FOUND;
	NameEntry = LdkpFindResourceDirectoryEntry(
		ResourceBase,
		ResourceSize,
		TypeEntry->OffsetToDirectory,
		Name );
	if ((NameEntry == NULL) || ! NameEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( NameEntry->OffsetToDirectory,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		*ErrorCode = ERROR_RESOURCE_DATA_NOT_FOUND;
		return NULL;
	}

	*ErrorCode = ERROR_RESOURCE_LANG_NOT_FOUND;
	LanguageEntry = NULL;
	if (Language != 0) {
		LanguageEntry = LdkpFindResourceDirectoryEntryById(
			ResourceBase,
			ResourceSize,
			NameEntry->OffsetToDirectory,
			Language );
	} else {
		LanguageEntry = LdkpGetFirstResourceDirectoryEntry(
			ResourceBase,
			ResourceSize,
			NameEntry->OffsetToDirectory );
	}

	if ((LanguageEntry == NULL) || LanguageEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( LanguageEntry->OffsetToData,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DATA_ENTRY) )) {
		*ErrorCode = ERROR_RESOURCE_DATA_NOT_FOUND;
		return NULL;
	}

	DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)Add2Ptr(
		ResourceBase,
		LanguageEntry->OffsetToData );
	if (LdkpMappedImageRvaToPointer( ImageBase,
									 DataEntry->OffsetToData,
									 DataEntry->Size ) == NULL) {
		*ErrorCode = ERROR_RESOURCE_DATA_NOT_FOUND;
		return NULL;
	}

	return DataEntry;
}

static
PLDK_MESSAGE_RESOURCE_DATA
LdkpFindMessageResourceData (
	_In_reads_bytes_(ImageSize) PVOID ImageBase,
	_In_ ULONG ImageSize,
	_In_ LANGID LanguageId,
	_Out_ PULONG MessageResourceSize
	)
{
	PIMAGE_RESOURCE_DIRECTORY ResourceBase;
	ULONG ResourceSize;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY TypeEntry;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY NameEntry;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY LanguageEntry;
	PIMAGE_RESOURCE_DATA_ENTRY DataEntry;

	*MessageResourceSize = 0;

	if (! LdkpGetPeResourceDirectory( ImageBase,
									  ImageSize,
									  &ResourceBase,
									  &ResourceSize )) {
		return NULL;
	}

	TypeEntry = LdkpFindResourceDirectoryEntryById(
		ResourceBase,
		ResourceSize,
		0,
		LDK_RT_MESSAGETABLE );
	if ((TypeEntry == NULL) || ! TypeEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( TypeEntry->OffsetToDirectory,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		return NULL;
	}

	NameEntry = LdkpGetFirstResourceDirectoryEntry(
		ResourceBase,
		ResourceSize,
		TypeEntry->OffsetToDirectory );
	if ((NameEntry == NULL) || ! NameEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( NameEntry->OffsetToDirectory,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		return NULL;
	}

	LanguageEntry = NULL;
	if (LanguageId != 0) {
		LanguageEntry = LdkpFindResourceDirectoryEntryById(
			ResourceBase,
			ResourceSize,
			NameEntry->OffsetToDirectory,
			LanguageId );
	}
	if (LanguageEntry == NULL) {
		LanguageEntry = LdkpGetFirstResourceDirectoryEntry(
			ResourceBase,
			ResourceSize,
			NameEntry->OffsetToDirectory );
	}

	if ((LanguageEntry == NULL) || LanguageEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( LanguageEntry->OffsetToData,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DATA_ENTRY) )) {
		return NULL;
	}

	DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)Add2Ptr(
		ResourceBase,
		LanguageEntry->OffsetToData );
	if (DataEntry->Size < sizeof(LDK_MESSAGE_RESOURCE_DATA)) {
		return NULL;
	}

	*MessageResourceSize = DataEntry->Size;
	return (PLDK_MESSAGE_RESOURCE_DATA)LdkpPeRvaToFilePointer(
		ImageBase,
		ImageSize,
		DataEntry->OffsetToData,
		DataEntry->Size );
}

static
PLDK_MESSAGE_RESOURCE_DATA
LdkpFindMappedMessageResourceData (
	_In_ PVOID ImageBase,
	_In_ LANGID LanguageId,
	_Out_ PULONG MessageResourceSize
	)
{
	PIMAGE_RESOURCE_DIRECTORY ResourceBase;
	ULONG ResourceSize;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY TypeEntry;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY NameEntry;
	PIMAGE_RESOURCE_DIRECTORY_ENTRY LanguageEntry;
	PIMAGE_RESOURCE_DATA_ENTRY DataEntry;

	*MessageResourceSize = 0;

	ResourceBase = RtlImageDirectoryEntryToData(
		ImageBase,
		TRUE,
		IMAGE_DIRECTORY_ENTRY_RESOURCE,
		&ResourceSize );
	if ((ResourceBase == NULL) || (ResourceSize == 0)) {
		return NULL;
	}

	TypeEntry = LdkpFindResourceDirectoryEntryById(
		ResourceBase,
		ResourceSize,
		0,
		LDK_RT_MESSAGETABLE );
	if ((TypeEntry == NULL) || ! TypeEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( TypeEntry->OffsetToDirectory,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		return NULL;
	}

	NameEntry = LdkpGetFirstResourceDirectoryEntry(
		ResourceBase,
		ResourceSize,
		TypeEntry->OffsetToDirectory );
	if ((NameEntry == NULL) || ! NameEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( NameEntry->OffsetToDirectory,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DIRECTORY) )) {
		return NULL;
	}

	LanguageEntry = NULL;
	if (LanguageId != 0) {
		LanguageEntry = LdkpFindResourceDirectoryEntryById(
			ResourceBase,
			ResourceSize,
			NameEntry->OffsetToDirectory,
			LanguageId );
	}
	if (LanguageEntry == NULL) {
		LanguageEntry = LdkpGetFirstResourceDirectoryEntry(
			ResourceBase,
			ResourceSize,
			NameEntry->OffsetToDirectory );
	}

	if ((LanguageEntry == NULL) || LanguageEntry->DataIsDirectory) {
		return NULL;
	}

	if (! LdkpResourceOffsetIsValid( LanguageEntry->OffsetToData,
									 ResourceSize,
									 sizeof(IMAGE_RESOURCE_DATA_ENTRY) )) {
		return NULL;
	}

	DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)Add2Ptr(
		ResourceBase,
		LanguageEntry->OffsetToData );
	if (DataEntry->Size < sizeof(LDK_MESSAGE_RESOURCE_DATA)) {
		return NULL;
	}

	*MessageResourceSize = DataEntry->Size;
	return (PLDK_MESSAGE_RESOURCE_DATA)Add2Ptr(
		ImageBase,
		DataEntry->OffsetToData );
}

static
PLDK_MESSAGE_RESOURCE_ENTRY
LdkpFindMessageResourceEntry (
	_In_reads_bytes_(MessageResourceSize) PLDK_MESSAGE_RESOURCE_DATA MessageData,
	_In_ ULONG MessageResourceSize,
	_In_ DWORD MessageId
	)
{
	ULONG BlockIndex;

	if (MessageResourceSize < sizeof(ULONG)) {
		return NULL;
	}

	if (MessageData->NumberOfBlocks >
		(MessageResourceSize - sizeof(ULONG)) / sizeof(LDK_MESSAGE_RESOURCE_BLOCK)) {
		return NULL;
	}

	for (BlockIndex = 0;
		 BlockIndex < MessageData->NumberOfBlocks;
		 BlockIndex++) {
		PLDK_MESSAGE_RESOURCE_BLOCK Block = &MessageData->Blocks[BlockIndex];
		PLDK_MESSAGE_RESOURCE_ENTRY Entry;
		ULONG Id;
		ULONG EntryOffset;

		if ((MessageId < Block->LowId) || (MessageId > Block->HighId)) {
			continue;
		}

		EntryOffset = Block->OffsetToEntries;
		if (! LdkpResourceOffsetIsValid( EntryOffset,
										 MessageResourceSize,
										 sizeof(LDK_MESSAGE_RESOURCE_ENTRY) )) {
			return NULL;
		}

		Entry = (PLDK_MESSAGE_RESOURCE_ENTRY)Add2Ptr(MessageData, EntryOffset);
		for (Id = Block->LowId; Id <= Block->HighId; Id++) {
			ULONG TextBytes;

			if (! LdkpResourceOffsetIsValid( (ULONG)((PUCHAR)Entry - (PUCHAR)MessageData),
											 MessageResourceSize,
											 sizeof(LDK_MESSAGE_RESOURCE_ENTRY) ) ||
				(Entry->Length < FIELD_OFFSET(LDK_MESSAGE_RESOURCE_ENTRY, Text))) {
				return NULL;
			}

			if (! LdkpResourceOffsetIsValid( (ULONG)((PUCHAR)Entry - (PUCHAR)MessageData),
											 MessageResourceSize,
											 Entry->Length )) {
				return NULL;
			}

			if (Id == MessageId) {
				return Entry;
			}

			TextBytes = Entry->Length - FIELD_OFFSET(LDK_MESSAGE_RESOURCE_ENTRY, Text);
			Entry = (PLDK_MESSAGE_RESOURCE_ENTRY)Add2Ptr(
				Entry,
				FIELD_OFFSET(LDK_MESSAGE_RESOURCE_ENTRY, Text) + TextBytes );
		}
	}

	return NULL;
}

static
PLDK_MESSAGE_RESOURCE_ENTRY
LdkpFindMessageInMappedImage (
	_In_ PVOID ImageBase,
	_In_ DWORD MessageId,
	_In_ LANGID LanguageId
	)
{
	PLDK_MESSAGE_RESOURCE_DATA MessageData;
	ULONG MessageDataSize;

	MessageData = LdkpFindMappedMessageResourceData( ImageBase,
													 LanguageId,
													 &MessageDataSize );
	if (MessageData == NULL) {
		return NULL;
	}

	return LdkpFindMessageResourceEntry( MessageData,
										 MessageDataSize,
										 MessageId );
}

static
PVOID
LdkpNormalizeFormatMessageModuleHandle (
	_In_ LPCVOID Source
	)
{
	ULONG_PTR Value;

	Value = (ULONG_PTR)Source;
	Value &= ~(ULONG_PTR)3;
	return (PVOID)Value;
}

static
PLDK_MESSAGE_RESOURCE_ENTRY
LdkpFindSystemMessageInCachedImages (
	_In_ DWORD MessageId,
	_In_ LANGID LanguageId
	)
{
	ULONG Index;

	for (Index = 0; Index < RTL_NUMBER_OF(LdkpSystemMessageImages); Index++) {
		PLDK_SYSTEM_MESSAGE_IMAGE Image = &LdkpSystemMessageImages[Index];
		PLDK_MESSAGE_RESOURCE_DATA MessageData;
		ULONG MessageDataSize;
		PLDK_MESSAGE_RESOURCE_ENTRY MessageEntry;

		if (Image->ImageBase == NULL) {
			continue;
		}

		MessageData = LdkpFindMessageResourceData( Image->ImageBase,
												   Image->ImageSize,
												   LanguageId,
												   &MessageDataSize );
		if (MessageData == NULL) {
			continue;
		}

		MessageEntry = LdkpFindMessageResourceEntry( MessageData,
													 MessageDataSize,
													 MessageId );
		if (MessageEntry != NULL) {
			return MessageEntry;
		}
	}

	return NULL;
}

static
DWORD
LdkpStringLengthA (
	_In_z_ LPCSTR String
	)
{
	DWORD Length = 0;

	while (String[Length] != ANSI_NULL) {
		Length++;
	}
	return Length;
}

static
DWORD
LdkpStringLengthW (
	_In_z_ LPCWSTR String
	)
{
	DWORD Length = 0;

	while (String[Length] != UNICODE_NULL) {
		Length++;
	}
	return Length;
}

static
DWORD
LdkpFormatMessageAFromAnsiCounted (
	_In_reads_(Length) LPCSTR Message,
	_In_ DWORD Length,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPSTR Buffer,
	_In_ DWORD nSize
	)
{
	DWORD Index;
	LPSTR Target;

	if (AllocateBuffer) {
		if (Buffer == NULL) {
			SetLastError( ERROR_INVALID_PARAMETER );
			return 0;
		}

		Target = (LPSTR)LocalAlloc( LMEM_FIXED,
									Length + 1 );
		if (Target == NULL) {
			SetLastError( ERROR_NOT_ENOUGH_MEMORY );
			return 0;
		}

		*((LPSTR*)Buffer) = Target;
	} else {
		if ((Buffer == NULL) || (nSize == 0)) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		if (Length >= nSize) {
			Buffer[0] = ANSI_NULL;
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}

		Target = Buffer;
	}

	for (Index = 0; Index < Length; Index++) {
		Target[Index] = Message[Index];
	}
	Target[Length] = ANSI_NULL;
	return Length;
}

static
DWORD
LdkpFormatMessageAFromAnsi (
	_In_z_ LPCSTR Message,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPSTR Buffer,
	_In_ DWORD nSize
	)
{
	return LdkpFormatMessageAFromAnsiCounted(
		Message,
		LdkpStringLengthA( Message ),
		AllocateBuffer,
		Buffer,
		nSize );
}

static
DWORD
LdkpFormatMessageAFromWideCounted (
	_In_reads_(Length) LPCWSTR Message,
	_In_ DWORD Length,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPSTR Buffer,
	_In_ DWORD nSize
	)
{
	DWORD Index;
	LPSTR Target;

	if (AllocateBuffer) {
		if (Buffer == NULL) {
			SetLastError( ERROR_INVALID_PARAMETER );
			return 0;
		}

		Target = (LPSTR)LocalAlloc( LMEM_FIXED,
									Length + 1 );
		if (Target == NULL) {
			SetLastError( ERROR_NOT_ENOUGH_MEMORY );
			return 0;
		}

		*((LPSTR*)Buffer) = Target;
	} else {
		if ((Buffer == NULL) || (nSize == 0)) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		if (Length >= nSize) {
			Buffer[0] = ANSI_NULL;
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}

		Target = Buffer;
	}

	for (Index = 0; Index < Length; Index++) {
		Target[Index] = (CHAR)(Message[Index] <= 0x7f ? Message[Index] : '?');
	}
	Target[Length] = ANSI_NULL;
	return Length;
}

static
DWORD
LdkpFormatMessageWFromAnsiCounted (
	_In_reads_(Length) LPCSTR Message,
	_In_ DWORD Length,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPWSTR Buffer,
	_In_ DWORD nSize
	)
{
	DWORD Index;
	LPWSTR Target;

	if (AllocateBuffer) {
		if (Buffer == NULL) {
			SetLastError( ERROR_INVALID_PARAMETER );
			return 0;
		}

		Target = (LPWSTR)LocalAlloc( LMEM_FIXED,
									 (Length + 1) * sizeof(WCHAR) );
		if (Target == NULL) {
			SetLastError( ERROR_NOT_ENOUGH_MEMORY );
			return 0;
		}

		*((LPWSTR*)Buffer) = Target;
	} else {
		if ((Buffer == NULL) || (nSize == 0)) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		if (Length >= nSize) {
			Buffer[0] = UNICODE_NULL;
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}

		Target = Buffer;
	}

	for (Index = 0; Index < Length; Index++) {
		Target[Index] = (WCHAR)(UCHAR)Message[Index];
	}
	Target[Length] = UNICODE_NULL;
	return Length;
}

static
DWORD
LdkpFormatMessageWFromAnsi (
	_In_z_ LPCSTR Message,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPWSTR Buffer,
	_In_ DWORD nSize
	)
{
	return LdkpFormatMessageWFromAnsiCounted(
		Message,
		LdkpStringLengthA( Message ),
		AllocateBuffer,
		Buffer,
		nSize );
}

static
DWORD
LdkpFormatMessageWFromWideCounted (
	_In_reads_(Length) LPCWSTR Message,
	_In_ DWORD Length,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPWSTR Buffer,
	_In_ DWORD nSize
	)
{
	DWORD Index;
	LPWSTR Target;

	if (AllocateBuffer) {
		if (Buffer == NULL) {
			SetLastError( ERROR_INVALID_PARAMETER );
			return 0;
		}

		Target = (LPWSTR)LocalAlloc( LMEM_FIXED,
									 (Length + 1) * sizeof(WCHAR) );
		if (Target == NULL) {
			SetLastError( ERROR_NOT_ENOUGH_MEMORY );
			return 0;
		}

		*((LPWSTR*)Buffer) = Target;
	} else {
		if ((Buffer == NULL) || (nSize == 0)) {
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}
		if (Length >= nSize) {
			Buffer[0] = UNICODE_NULL;
			SetLastError( ERROR_INSUFFICIENT_BUFFER );
			return 0;
		}

		Target = Buffer;
	}

	for (Index = 0; Index < Length; Index++) {
		Target[Index] = Message[Index];
	}
	Target[Length] = UNICODE_NULL;
	return Length;
}

static
DWORD
LdkpFormatMessageWFromWide (
	_In_z_ LPCWSTR Message,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPWSTR Buffer,
	_In_ DWORD nSize
	)
{
	return LdkpFormatMessageWFromWideCounted(
		Message,
		LdkpStringLengthW( Message ),
		AllocateBuffer,
		Buffer,
		nSize );
}

static
DWORD
LdkpFormatMessageAFromResourceEntry (
	_In_ PLDK_MESSAGE_RESOURCE_ENTRY MessageEntry,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPSTR Buffer,
	_In_ DWORD nSize
	)
{
	ULONG TextBytes;

	if (MessageEntry->Length < FIELD_OFFSET(LDK_MESSAGE_RESOURCE_ENTRY, Text)) {
		SetLastError( ERROR_INVALID_DATA );
		return 0;
	}

	TextBytes = MessageEntry->Length - FIELD_OFFSET(LDK_MESSAGE_RESOURCE_ENTRY, Text);
	if (FlagOn(MessageEntry->Flags, LDK_MESSAGE_RESOURCE_UNICODE)) {
		return LdkpFormatMessageAFromWideCounted(
			(LPCWSTR)MessageEntry->Text,
			TextBytes / sizeof(WCHAR),
			AllocateBuffer,
			Buffer,
			nSize );
	}

	return LdkpFormatMessageAFromAnsiCounted(
		(LPCSTR)MessageEntry->Text,
		TextBytes,
		AllocateBuffer,
		Buffer,
		nSize );
}

static
DWORD
LdkpFormatMessageWFromResourceEntry (
	_In_ PLDK_MESSAGE_RESOURCE_ENTRY MessageEntry,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPWSTR Buffer,
	_In_ DWORD nSize
	)
{
	ULONG TextBytes;

	if (MessageEntry->Length < FIELD_OFFSET(LDK_MESSAGE_RESOURCE_ENTRY, Text)) {
		SetLastError( ERROR_INVALID_DATA );
		return 0;
	}

	TextBytes = MessageEntry->Length - FIELD_OFFSET(LDK_MESSAGE_RESOURCE_ENTRY, Text);
	if (FlagOn(MessageEntry->Flags, LDK_MESSAGE_RESOURCE_UNICODE)) {
		return LdkpFormatMessageWFromWideCounted(
			(LPCWSTR)MessageEntry->Text,
			TextBytes / sizeof(WCHAR),
			AllocateBuffer,
			Buffer,
			nSize );
	}

	return LdkpFormatMessageWFromAnsiCounted(
		(LPCSTR)MessageEntry->Text,
		TextBytes,
		AllocateBuffer,
		Buffer,
		nSize );
}

static
DWORD
LdkpFormatSystemMessageAFromImages (
	_In_ DWORD MessageId,
	_In_ LANGID LanguageId,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPSTR Buffer,
	_In_ DWORD nSize
	)
{
	PLDK_MESSAGE_RESOURCE_ENTRY MessageEntry;

	MessageEntry = LdkpFindSystemMessageInCachedImages( MessageId,
														LanguageId );
	if (MessageEntry != NULL) {
		return LdkpFormatMessageAFromResourceEntry(
			MessageEntry,
			AllocateBuffer,
			Buffer,
			nSize );
	}

	SetLastError( ERROR_MR_MID_NOT_FOUND );
	return 0;
}

static
DWORD
LdkpFormatSystemMessageWFromImages (
	_In_ DWORD MessageId,
	_In_ LANGID LanguageId,
	_In_ BOOL AllocateBuffer,
	_Out_writes_z_(nSize) LPWSTR Buffer,
	_In_ DWORD nSize
	)
{
	PLDK_MESSAGE_RESOURCE_ENTRY MessageEntry;

	MessageEntry = LdkpFindSystemMessageInCachedImages( MessageId,
														LanguageId );
	if (MessageEntry != NULL) {
		return LdkpFormatMessageWFromResourceEntry(
			MessageEntry,
			AllocateBuffer,
			Buffer,
			nSize );
	}

	SetLastError( ERROR_MR_MID_NOT_FOUND );
	return 0;
}



_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PUNICODE_STRING
LdkAnsiStringToStaticUnicodeString (
    _In_ LPCSTR SourceString
    )
{
    ANSI_STRING Ansi;

	PAGED_CODE();

    RtlInitAnsiString( &Ansi,
                       SourceString );
	PUNICODE_STRING Unicode = &NtCurrentTeb()->StaticUnicodeString;
    NTSTATUS Status = LdkAnsiStringToUnicodeString( Unicode,
                                                    &Ansi,
                                                    FALSE );
    if (NT_SUCCESS(Status)) {
		return Unicode;
	}
	if (Status == STATUS_BUFFER_OVERFLOW) {
		SetLastError( ERROR_FILENAME_EXCED_RANGE );
	} else {
		LdkSetLastNTError( Status );
	}
	return NULL;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
PUNICODE_STRING
Ldk8BitStringToStaticUnicodeString (
    _In_ PCSTR String
    )
{
    ANSI_STRING Ansi;

	PAGED_CODE();

    RtlInitAnsiString( &Ansi,
                       String );
    PUNICODE_STRING Unicode = &NtCurrentTeb()->StaticUnicodeString;
	NTSTATUS Status = Ldk8BitStringToUnicodeString( Unicode,
													&Ansi,
													FALSE );
    if (NT_SUCCESS(Status)) {
		return Unicode;
	}
	if (Status == STATUS_BUFFER_OVERFLOW) {
		SetLastError( ERROR_FILENAME_EXCED_RANGE );
	} else {
		LdkSetLastNTError( Status );
	}
	return NULL;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_Must_inspect_result_
BOOL
NTAPI
Ldk8BitStringToDynamicUnicodeString (
    _When_(AllocateDestinationString, _Out_ _At_(DestinationString->Buffer, __drv_allocatesMem(Mem)))
    _When_(!AllocateDestinationString, _Inout_) PUNICODE_STRING DestinationString,
    _In_ PCSTR SourceString
    )
{
	STRING String;

	PAGED_CODE();

	RtlInitString( &String,
				   SourceString );
	NTSTATUS Status = Ldk8BitStringToUnicodeString( DestinationString,
													&String,
													TRUE );
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}
	if (Status == STATUS_BUFFER_OVERFLOW) {
		SetLastError( ERROR_FILENAME_EXCED_RANGE );
	} else {
		LdkSetLastNTError( Status );
	}
	return FALSE;
}

static
BOOL
LdkpAnsiResourceToUnicode (
	_Out_ PUNICODE_STRING UnicodeString,
	_In_ LPCSTR AnsiResource,
	_Out_ LPCWSTR *UnicodeResource
	)
{
	UnicodeString->Length = 0;
	UnicodeString->MaximumLength = 0;
	UnicodeString->Buffer = NULL;

	if (AnsiResource == NULL) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (IS_INTRESOURCE(AnsiResource)) {
		*UnicodeResource = (LPCWSTR)(ULONG_PTR)(USHORT)(ULONG_PTR)AnsiResource;
		return TRUE;
	}

	if (! Ldk8BitStringToDynamicUnicodeString( UnicodeString,
											   AnsiResource )) {
		return FALSE;
	}

	*UnicodeResource = UnicodeString->Buffer;
	return TRUE;
}

WINBASEAPI
HRSRC
WINAPI
FindResourceExW (
    _In_opt_ HMODULE hModule,
    _In_ LPCWSTR lpType,
    _In_ LPCWSTR lpName,
    _In_ WORD wLanguage
    )
{
	PVOID ImageBase;
	PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
	DWORD ErrorCode;

	PAGED_CODE();

	ImageBase = LdkpMapModuleHandle( hModule,
									 TRUE );
	if (ImageBase == NULL) {
		SetLastError( ERROR_INVALID_HANDLE );
		return NULL;
	}

	DataEntry = LdkpFindMappedResourceDataEntry( ImageBase,
												 lpType,
												 lpName,
												 wLanguage,
												 &ErrorCode );
	if (DataEntry == NULL) {
		SetLastError( ErrorCode );
		return NULL;
	}

	return (HRSRC)DataEntry;
}

WINBASEAPI
HRSRC
WINAPI
FindResourceW (
    _In_opt_ HMODULE hModule,
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpType
    )
{
	return FindResourceExW( hModule,
							lpType,
							lpName,
							0 );
}

WINBASEAPI
HRSRC
WINAPI
FindResourceExA (
    _In_opt_ HMODULE hModule,
    _In_ LPCSTR lpType,
    _In_ LPCSTR lpName,
    _In_ WORD wLanguage
    )
{
	UNICODE_STRING TypeString;
	UNICODE_STRING NameString;
	LPCWSTR Type;
	LPCWSTR Name;
	HRSRC Resource;

	PAGED_CODE();

	if (! LdkpAnsiResourceToUnicode( &TypeString,
									 lpType,
									 &Type )) {
		return NULL;
	}
	if (! LdkpAnsiResourceToUnicode( &NameString,
									 lpName,
									 &Name )) {
		if (TypeString.Buffer != NULL) {
			RtlFreeUnicodeString( &TypeString );
		}
		return NULL;
	}

	Resource = FindResourceExW( hModule,
								Type,
								Name,
								wLanguage );

	if (NameString.Buffer != NULL) {
		RtlFreeUnicodeString( &NameString );
	}
	if (TypeString.Buffer != NULL) {
		RtlFreeUnicodeString( &TypeString );
	}

	return Resource;
}

WINBASEAPI
HRSRC
WINAPI
FindResourceA (
    _In_opt_ HMODULE hModule,
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpType
    )
{
	return FindResourceExA( hModule,
							lpType,
							lpName,
							0 );
}

WINBASEAPI
_Ret_maybenull_
HGLOBAL
WINAPI
LoadResource (
    _In_opt_ HMODULE hModule,
    _In_ HRSRC hResInfo
    )
{
	PVOID ImageBase;
	PIMAGE_RESOURCE_DATA_ENTRY DataEntry;
	PVOID ResourceData;

	PAGED_CODE();

	if (hResInfo == NULL) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	ImageBase = LdkpMapModuleHandle( hModule,
									 TRUE );
	if (ImageBase == NULL) {
		SetLastError( ERROR_INVALID_HANDLE );
		return NULL;
	}

	DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)hResInfo;
	ResourceData = LdkpMappedImageRvaToPointer( ImageBase,
												DataEntry->OffsetToData,
												DataEntry->Size );
	if (ResourceData == NULL) {
		SetLastError( ERROR_RESOURCE_DATA_NOT_FOUND );
		return NULL;
	}

	return (HGLOBAL)ResourceData;
}

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
LockResource (
    _In_ HGLOBAL hResData
    )
{
	return (LPVOID)hResData;
}

WINBASEAPI
DWORD
WINAPI
SizeofResource (
    _In_opt_ HMODULE hModule,
    _In_ HRSRC hResInfo
    )
{
	PVOID ImageBase;
	PIMAGE_RESOURCE_DATA_ENTRY DataEntry;

	PAGED_CODE();

	if (hResInfo == NULL) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return 0;
	}

	ImageBase = LdkpMapModuleHandle( hModule,
									 TRUE );
	if (ImageBase == NULL) {
		SetLastError( ERROR_INVALID_HANDLE );
		return 0;
	}

	DataEntry = (PIMAGE_RESOURCE_DATA_ENTRY)hResInfo;
	if (LdkpMappedImageRvaToPointer( ImageBase,
									 DataEntry->OffsetToData,
									 DataEntry->Size ) == NULL) {
		SetLastError( ERROR_RESOURCE_DATA_NOT_FOUND );
		return 0;
	}

	return DataEntry->Size;
}

WINBASEAPI
BOOL
WINAPI
FreeResource (
    _In_ HGLOBAL hResData
    )
{
	UNREFERENCED_PARAMETER(hResData);
	return FALSE;
}



WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageA (
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) != 0, _At_((LPSTR*)lpBuffer, _Outptr_result_z_))
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) == 0, _Out_writes_z_(nSize))
             LPSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    )
{
	UNREFERENCED_PARAMETER(Arguments);

	PAGED_CODE();

	if (FlagOn(dwFlags, FORMAT_MESSAGE_FROM_HMODULE)) {
		PLDK_MESSAGE_RESOURCE_ENTRY MessageEntry;
		PVOID ImageBase;

		if (lpSource == NULL) {
			SetLastError( ERROR_INVALID_HANDLE );
			return 0;
		}

		ImageBase = LdkpNormalizeFormatMessageModuleHandle( lpSource );
		MessageEntry = LdkpFindMessageInMappedImage(
			ImageBase,
			dwMessageId,
			(LANGID)dwLanguageId );
		if (MessageEntry != NULL) {
			return LdkpFormatMessageAFromResourceEntry(
				MessageEntry,
				BooleanFlagOn(dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER),
				lpBuffer,
				nSize );
		}

		if (! FlagOn(dwFlags, FORMAT_MESSAGE_FROM_SYSTEM)) {
			SetLastError( ERROR_MR_MID_NOT_FOUND );
			return 0;
		}
	}

	if (FlagOn(dwFlags, FORMAT_MESSAGE_FROM_SYSTEM)) {
		LPCSTR Message = LdkpLookupSystemErrorMessage( dwMessageId );
		DWORD Chars = LdkpFormatSystemMessageAFromImages(
			dwMessageId,
			(LANGID)dwLanguageId,
			BooleanFlagOn(dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER),
			lpBuffer,
			nSize );
		if (Chars != 0) {
			return Chars;
		}

		if (Message == NULL) {
			SetLastError( ERROR_MR_MID_NOT_FOUND );
			return 0;
		}

		return LdkpFormatMessageAFromAnsi(
			Message,
			BooleanFlagOn(dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER),
			lpBuffer,
			nSize );
	}

	if (FlagOn(dwFlags, FORMAT_MESSAGE_FROM_STRING) && (lpSource != NULL)) {
		return LdkpFormatMessageAFromAnsi(
			(LPCSTR)lpSource,
			BooleanFlagOn(dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER),
			lpBuffer,
			nSize );
	}

	SetLastError( ERROR_INVALID_PARAMETER );
	return 0;
}

WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageW (
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) != 0, _At_((LPWSTR*)lpBuffer, _Outptr_result_z_))
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) == 0, _Out_writes_z_(nSize))
             LPWSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    )
{
	UNREFERENCED_PARAMETER(Arguments);

	PAGED_CODE();

	if (FlagOn(dwFlags, FORMAT_MESSAGE_FROM_HMODULE)) {
		PLDK_MESSAGE_RESOURCE_ENTRY MessageEntry;
		PVOID ImageBase;

		if (lpSource == NULL) {
			SetLastError( ERROR_INVALID_HANDLE );
			return 0;
		}

		ImageBase = LdkpNormalizeFormatMessageModuleHandle( lpSource );
		MessageEntry = LdkpFindMessageInMappedImage(
			ImageBase,
			dwMessageId,
			(LANGID)dwLanguageId );
		if (MessageEntry != NULL) {
			return LdkpFormatMessageWFromResourceEntry(
				MessageEntry,
				BooleanFlagOn(dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER),
				lpBuffer,
				nSize );
		}

		if (! FlagOn(dwFlags, FORMAT_MESSAGE_FROM_SYSTEM)) {
			SetLastError( ERROR_MR_MID_NOT_FOUND );
			return 0;
		}
	}

	if (FlagOn(dwFlags, FORMAT_MESSAGE_FROM_SYSTEM)) {
		LPCSTR Message = LdkpLookupSystemErrorMessage( dwMessageId );
		DWORD Chars = LdkpFormatSystemMessageWFromImages(
			dwMessageId,
			(LANGID)dwLanguageId,
			BooleanFlagOn(dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER),
			lpBuffer,
			nSize );
		if (Chars != 0) {
			return Chars;
		}

		if (Message == NULL) {
			SetLastError( ERROR_MR_MID_NOT_FOUND );
			return 0;
		}

		return LdkpFormatMessageWFromAnsi(
			Message,
			BooleanFlagOn(dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER),
			lpBuffer,
			nSize );
	}

	if (FlagOn(dwFlags, FORMAT_MESSAGE_FROM_STRING) && (lpSource != NULL)) {
		return LdkpFormatMessageWFromWide(
			(LPCWSTR)lpSource,
			BooleanFlagOn(dwFlags, FORMAT_MESSAGE_ALLOCATE_BUFFER),
			lpBuffer,
			nSize );
	}

	SetLastError( ERROR_INVALID_PARAMETER );
	return 0;
}



WINBASEAPI
_Success_(return != NULL)
_Post_writable_byte_size_(uBytes)
DECLSPEC_ALLOCATOR
HLOCAL
WINAPI
LocalAlloc (
    _In_ UINT uFlags,
    _In_ SIZE_T uBytes
    )
{
	DWORD dwFlags = 0;
	if (FlagOn(uFlags, LMEM_ZEROINIT)) {
		SetFlag(dwFlags, HEAP_ZERO_MEMORY);
	}
	return (HLOCAL)HeapAlloc( GetProcessHeap(),
							  dwFlags,
							  (DWORD)uBytes );
}

WINBASEAPI
_Ret_reallocated_bytes_(hMem, uBytes)
DECLSPEC_ALLOCATOR
HLOCAL
WINAPI
LocalReAlloc (
    _Frees_ptr_opt_ HLOCAL hMem,
    _In_ SIZE_T uBytes,
    _In_ UINT uFlags
    )
{
	DWORD dwFlags = 0;
	if (FlagOn(uFlags, LMEM_ZEROINIT)) {
		SetFlag(dwFlags, HEAP_ZERO_MEMORY);
	}
	return (HLOCAL)HeapReAlloc( GetProcessHeap(),
								dwFlags,
								(PVOID)hMem,
								(DWORD)uBytes );
}

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
LocalLock (
    _In_ HLOCAL hMem
    )
{
	return (LPVOID)hMem;
}

WINBASEAPI
_Ret_maybenull_
HLOCAL
WINAPI
LocalHandle (
    _In_ LPCVOID pMem
    )
{
	return (HLOCAL)pMem;
}

WINBASEAPI
BOOL
WINAPI
LocalUnlock (
    _In_ HLOCAL hMem
    )
{
	return hMem != NULL;
}

WINBASEAPI
SIZE_T
WINAPI
LocalSize (
    _In_ HLOCAL hMem
    )
{
	return HeapSize( GetProcessHeap(),
					 0,
					 (LPCVOID)hMem );
}

WINBASEAPI
_Success_(return==0)
_Ret_maybenull_
HLOCAL
WINAPI
LocalFree (
    _Frees_ptr_opt_ HLOCAL hMem
    )
{
	UNREFERENCED_PARAMETER(hMem);
	if (HeapFree( GetProcessHeap(),
				  0,
				  (PVOID)hMem)) {
		return NULL;
	}
	return hMem;
}
