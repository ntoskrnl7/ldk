#pragma once

EXTERN_C_START



#define DOS_MAX_COMPONENT_LENGTH    255
#define DOS_MAX_PATH_LENGTH         (DOS_MAX_COMPONENT_LENGTH + 5)



#define RTL_USER_PROC_CURDIR_CLOSE      0x00000002
#define RTL_USER_PROC_CURDIR_INHERIT    0x00000003



#define RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END        (0x00000001)
#define RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET (0x00000002)
#define RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE    (0x00000004)



#define HEAP_MAKE_TAG_FLAGS( b, o ) ((ULONG)((b) + ((o) << 18)))  // winnt



NTSTATUS
NTAPI
RtlFindCharInUnicodeString (
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING StringToSearch,
    _In_ PCUNICODE_STRING CharSet,
    _Out_ USHORT *NonInclusivePrefixLength
    );



NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U (
    _Inout_opt_ PVOID Environment,
    _In_ PCUNICODE_STRING Name,
    _Inout_ PUNICODE_STRING Value
    );

NTSTATUS
NTAPI
RtlSetEnvironmentVariable (
    _Inout_opt_ PVOID *Environment,
    _In_ PCUNICODE_STRING Name,
    _In_opt_ PCUNICODE_STRING Value
    );



VOID
NTAPI
RtlAcquirePebLock (
    VOID
    );

VOID
NTAPI
RtlReleasePebLock (
    VOID
    );


typedef struct _RTL_TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    TIME_FIELDS StandardStart;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    TIME_FIELDS DaylightStart;
    LONG DaylightBias;
} RTL_TIME_ZONE_INFORMATION, *PRTL_TIME_ZONE_INFORMATION;

BOOLEAN
NTAPI
RtlCutoverTimeToSystemTime (
    _In_ PTIME_FIELDS CutoverTime,
    _Out_ PLARGE_INTEGER SystemTime,
    _Out_ PLARGE_INTEGER CurrentSystemTime,
    _In_ BOOLEAN ThisYear
    );



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
PVOID
NTAPI
RtlImageDirectoryEntryToData(
    PVOID BaseOfImage,
    BOOLEAN MappedAsImage,
    USHORT DirectoryEntry,
    PULONG Size
    );

#if defined(_WIN64)
NTSYSAPI
PVOID
RtlImageDirectoryEntryToData32 (
    IN PVOID Base,
    IN BOOLEAN MappedAsImage,
    IN USHORT DirectoryEntry,
    OUT PULONG Size
    );
#else
    #define RtlImageDirectoryEntryToData32 RtlImageDirectoryEntryToData
#endif

#define RTL_ERRORMODE_FAILCRITICALERRORS (0x0010)

EXTERN_C_END