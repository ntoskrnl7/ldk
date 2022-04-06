#pragma once

NTSYSAPI
PIMAGE_NT_HEADERS
NTAPI
RtlImageNtHeader (
	_In_ PVOID Base
    );

NTSYSAPI
PVOID
NTAPI
RtlImageDirectoryEntryToData (
	_In_ PVOID Base,
	_In_ BOOLEAN MappedAsImage,
	_In_ USHORT DirectoryEntry,
	_Out_ PULONG Size
	);



NTSYSAPI
SIZE_T
NTAPI
RtlSizeHeap(
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ PVOID BaseAddress
    );