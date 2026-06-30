#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GlobalMemoryStatusEx)
#pragma alloc_text(PAGE, GetActiveProcessorCount)
#pragma alloc_text(PAGE, GetActiveProcessorGroupCount)
#pragma alloc_text(PAGE, GetCurrentProcessorNumber)
#pragma alloc_text(PAGE, GetCurrentProcessorNumberEx)
#pragma alloc_text(PAGE, GetLogicalProcessorInformation)
#pragma alloc_text(PAGE, GetLogicalProcessorInformationEx)
#pragma alloc_text(PAGE, GetMaximumProcessorCount)
#pragma alloc_text(PAGE, GetMaximumProcessorGroupCount)
#pragma alloc_text(PAGE, GetNativeSystemInfo)
#pragma alloc_text(PAGE, GetNumaHighestNodeNumber)
#pragma alloc_text(PAGE, GetSystemInfo)
#pragma alloc_text(PAGE, GetVersion)
#pragma alloc_text(PAGE, GetVersionExA)
#pragma alloc_text(PAGE, GetVersionExW)
#endif



#define LDK_AFFINITY_BITS ((ULONG)(sizeof(KAFFINITY) * 8))

static
KAFFINITY
LdkpBuildProcessorMaskFromCount (
    _In_ ULONG ProcessorCount
    )
{
    if (ProcessorCount == 0) {
        return 0;
    }

    if (ProcessorCount >= LDK_AFFINITY_BITS) {
        return ~(KAFFINITY)0;
    }

    return (((KAFFINITY)1) << ProcessorCount) - 1;
}

static
ULONG
LdkpCountProcessorsInMask (
    _In_ KAFFINITY Mask
    )
{
    ULONG Count = 0;

    while (Mask != 0) {
        Count += (ULONG)(Mask & 1);
        Mask >>= 1;
    }

    return Count;
}

static
WORD
LdkpQueryActiveProcessorGroupCount (
    VOID
    )
{
    WORD GroupCount = KeQueryActiveGroupCount();

    return GroupCount != 0 ? GroupCount : 1;
}

static
KAFFINITY
LdkpQueryActiveProcessorMask (
    _In_ WORD GroupNumber
    )
{
    KAFFINITY Mask;
    ULONG ProcessorCount;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;

    if (GroupNumber >= LdkpQueryActiveProcessorGroupCount()) {
        return 0;
    }

    Mask = KeQueryGroupAffinity( GroupNumber );
    if (Mask != 0) {
        return Mask;
    }

    ProcessorCount = KeQueryActiveProcessorCountEx( GroupNumber );
    Mask = LdkpBuildProcessorMaskFromCount( ProcessorCount );
    if (Mask != 0) {
        return Mask;
    }

    if (GroupNumber == 0) {
        RtlZeroMemory( &BasicInfo,
                       sizeof(BasicInfo) );
        Status = ZwQuerySystemInformation( SystemBasicInformation,
                                           &BasicInfo,
                                           sizeof(BasicInfo),
                                           NULL );
        if (NT_SUCCESS(Status) &&
            BasicInfo.ActiveProcessorsAffinityMask != 0) {
            return BasicInfo.ActiveProcessorsAffinityMask;
        }
    }

    return GroupNumber == 0 ? (KAFFINITY)1 : 0;
}

static
DWORD
LdkpQueryActiveProcessorCountInGroup (
    _In_ WORD GroupNumber
    )
{
    DWORD ProcessorCount;

    ProcessorCount = KeQueryActiveProcessorCountEx( GroupNumber );
    if (ProcessorCount == 0) {
        ProcessorCount = LdkpCountProcessorsInMask(
            LdkpQueryActiveProcessorMask( GroupNumber ) );
    }

    return ProcessorCount != 0 ? ProcessorCount : 1;
}

static
DWORD
LdkpQueryMaximumProcessorCountInGroup (
    _In_ WORD GroupNumber
    )
{
    DWORD ProcessorCount;

    ProcessorCount = KeQueryMaximumProcessorCountEx( GroupNumber );
    if (ProcessorCount == 0) {
        ProcessorCount = LdkpQueryActiveProcessorCountInGroup( GroupNumber );
    }

    return ProcessorCount;
}

static
DWORD
LdkpQueryLogicalProcessorCount (
    VOID
    )
{
    WORD GroupCount = LdkpQueryActiveProcessorGroupCount();
    DWORD ProcessorCount = 0;

    for (WORD GroupIndex = 0; GroupIndex < GroupCount; GroupIndex++) {
        ProcessorCount += LdkpCountProcessorsInMask(
            LdkpQueryActiveProcessorMask( GroupIndex ) );
    }

    return ProcessorCount != 0 ? ProcessorCount : 1;
}

static
DWORD
LdkpQueryLegacyLogicalProcessorInformationLength (
    VOID
    )
{
    DWORD ProcessorCount = LdkpCountProcessorsInMask(
        LdkpQueryActiveProcessorMask( 0 ) );

    if (ProcessorCount == 0) {
        ProcessorCount = 1;
    }

    return (ProcessorCount + 2) * sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
}

static
VOID
LdkpFillGroupAffinity (
    _Out_ PGROUP_AFFINITY GroupAffinity,
    _In_ WORD GroupNumber,
    _In_ KAFFINITY Mask
    )
{
    RtlZeroMemory( GroupAffinity,
                   sizeof(*GroupAffinity) );
    GroupAffinity->Mask = Mask;
    GroupAffinity->Group = GroupNumber;
}

static
DWORD
LdkpProcessorRelationshipSize (
    _In_ WORD GroupCount
    )
{
    return (DWORD)FIELD_OFFSET(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
                               Processor.GroupMask) +
           ((DWORD)GroupCount * sizeof(GROUP_AFFINITY));
}

static
DWORD
LdkpNumaNodeRelationshipSize (
    _In_ WORD GroupCount
    )
{
    return (DWORD)FIELD_OFFSET(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
                               NumaNode.GroupMasks) +
           ((DWORD)GroupCount * sizeof(GROUP_AFFINITY));
}

static
DWORD
LdkpGroupRelationshipSize (
    _In_ WORD GroupCount
    )
{
    return (DWORD)FIELD_OFFSET(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX,
                               Group.GroupInfo) +
           ((DWORD)GroupCount * sizeof(PROCESSOR_GROUP_INFO));
}

static
BOOL
LdkpIsSupportedProcessorRelationshipType (
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType
    )
{
    switch (RelationshipType) {
    case RelationProcessorCore:
    case RelationNumaNode:
    case RelationCache:
    case RelationProcessorPackage:
    case RelationGroup:
    case RelationProcessorDie:
    case RelationNumaNodeEx:
    case RelationProcessorModule:
    case RelationAll:
        return TRUE;

    default:
        return FALSE;
    }
}

static
DWORD
LdkpQueryLogicalProcessorInformationExLength (
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType
    )
{
    WORD GroupCount = LdkpQueryActiveProcessorGroupCount();
    DWORD ProcessorCount = LdkpQueryLogicalProcessorCount();

    switch (RelationshipType) {
    case RelationProcessorCore:
        return ProcessorCount * LdkpProcessorRelationshipSize( 1 );

    case RelationNumaNode:
    case RelationNumaNodeEx:
        return LdkpNumaNodeRelationshipSize( GroupCount );

    case RelationProcessorPackage:
        return LdkpProcessorRelationshipSize( GroupCount );

    case RelationGroup:
        return LdkpGroupRelationshipSize( GroupCount );

    case RelationAll:
        return LdkpGroupRelationshipSize( GroupCount ) +
               LdkpNumaNodeRelationshipSize( GroupCount ) +
               LdkpProcessorRelationshipSize( GroupCount ) +
               (ProcessorCount * LdkpProcessorRelationshipSize( 1 ));

    case RelationCache:
    case RelationProcessorDie:
    case RelationProcessorModule:
        return 0;

    default:
        return 0;
    }
}

static
PBYTE
LdkpAppendProcessorRelationship (
    _In_ PBYTE Cursor,
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP Relationship,
    _In_ WORD GroupCount,
    _In_opt_ const GROUP_AFFINITY *GroupMasks
    )
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
    DWORD Size;

    Size = LdkpProcessorRelationshipSize( GroupCount );
    Entry = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)Cursor;
    RtlZeroMemory( Entry,
                   Size );

    Entry->Relationship = Relationship;
    Entry->Size = Size;
    Entry->Processor.GroupCount = GroupCount;

    for (WORD GroupIndex = 0; GroupIndex < GroupCount; GroupIndex++) {
        if (GroupMasks != NULL) {
            Entry->Processor.GroupMask[GroupIndex] = GroupMasks[GroupIndex];
        } else {
            LdkpFillGroupAffinity( &Entry->Processor.GroupMask[GroupIndex],
                                   GroupIndex,
                                   LdkpQueryActiveProcessorMask( GroupIndex ) );
        }
    }

    return Cursor + Size;
}

static
PBYTE
LdkpAppendNumaNodeRelationship (
    _In_ PBYTE Cursor,
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP Relationship,
    _In_ WORD GroupCount
    )
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
    DWORD Size;

    Size = LdkpNumaNodeRelationshipSize( GroupCount );
    Entry = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)Cursor;
    RtlZeroMemory( Entry,
                   Size );

    Entry->Relationship = Relationship;
    Entry->Size = Size;
    Entry->NumaNode.NodeNumber = 0;
    Entry->NumaNode.GroupCount = GroupCount;

    for (WORD GroupIndex = 0; GroupIndex < GroupCount; GroupIndex++) {
        LdkpFillGroupAffinity( &Entry->NumaNode.GroupMasks[GroupIndex],
                               GroupIndex,
                               LdkpQueryActiveProcessorMask( GroupIndex ) );
    }

    return Cursor + Size;
}

static
PBYTE
LdkpAppendGroupRelationship (
    _In_ PBYTE Cursor,
    _In_ WORD GroupCount
    )
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;
    DWORD Size;
    WORD MaximumGroupCount;

    Size = LdkpGroupRelationshipSize( GroupCount );
    Entry = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)Cursor;
    RtlZeroMemory( Entry,
                   Size );

    MaximumGroupCount = KeQueryMaximumGroupCount();

    Entry->Relationship = RelationGroup;
    Entry->Size = Size;
    Entry->Group.ActiveGroupCount = GroupCount;
    Entry->Group.MaximumGroupCount = MaximumGroupCount != 0 ?
                                     MaximumGroupCount :
                                     GroupCount;

    for (WORD GroupIndex = 0; GroupIndex < GroupCount; GroupIndex++) {
        DWORD ActiveCount = LdkpQueryActiveProcessorCountInGroup( GroupIndex );
        DWORD MaximumCount = LdkpQueryMaximumProcessorCountInGroup( GroupIndex );

        Entry->Group.GroupInfo[GroupIndex].ActiveProcessorCount =
            (BYTE)min( ActiveCount, MAXUCHAR );
        Entry->Group.GroupInfo[GroupIndex].MaximumProcessorCount =
            (BYTE)min( MaximumCount, MAXUCHAR );
        Entry->Group.GroupInfo[GroupIndex].ActiveProcessorMask =
            LdkpQueryActiveProcessorMask( GroupIndex );
    }

    return Cursor + Size;
}

static
PBYTE
LdkpAppendProcessorCoreRelationships (
    _In_ PBYTE Cursor
    )
{
    WORD GroupCount = LdkpQueryActiveProcessorGroupCount();

    for (WORD GroupIndex = 0; GroupIndex < GroupCount; GroupIndex++) {
        KAFFINITY Mask = LdkpQueryActiveProcessorMask( GroupIndex );

        for (ULONG BitIndex = 0; BitIndex < LDK_AFFINITY_BITS; BitIndex++) {
            GROUP_AFFINITY GroupMask;
            KAFFINITY ProcessorMask = ((KAFFINITY)1) << BitIndex;

            if ((Mask & ProcessorMask) == 0) {
                continue;
            }

            LdkpFillGroupAffinity( &GroupMask,
                                   GroupIndex,
                                   ProcessorMask );
            Cursor = LdkpAppendProcessorRelationship( Cursor,
                                                      RelationProcessorCore,
                                                      1,
                                                      &GroupMask );
        }
    }

    return Cursor;
}

static
VOID
LdkpFillLogicalProcessorInformationEx (
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_writes_bytes_(Length) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer,
    _In_ DWORD Length
    )
{
    WORD GroupCount = LdkpQueryActiveProcessorGroupCount();
    PBYTE Cursor = (PBYTE)Buffer;

    RtlZeroMemory( Buffer,
                   Length );

    if (RelationshipType == RelationAll ||
        RelationshipType == RelationGroup) {
        Cursor = LdkpAppendGroupRelationship( Cursor,
                                              GroupCount );
    }

    if (RelationshipType == RelationAll ||
        RelationshipType == RelationNumaNode) {
        Cursor = LdkpAppendNumaNodeRelationship( Cursor,
                                                 RelationNumaNode,
                                                 GroupCount );
    } else if (RelationshipType == RelationNumaNodeEx) {
        Cursor = LdkpAppendNumaNodeRelationship( Cursor,
                                                 RelationNumaNodeEx,
                                                 GroupCount );
    }

    if (RelationshipType == RelationAll ||
        RelationshipType == RelationProcessorPackage) {
        Cursor = LdkpAppendProcessorRelationship( Cursor,
                                                  RelationProcessorPackage,
                                                  GroupCount,
                                                  NULL );
    }

    if (RelationshipType == RelationAll ||
        RelationshipType == RelationProcessorCore) {
        Cursor = LdkpAppendProcessorCoreRelationships( Cursor );
    }

    UNREFERENCED_PARAMETER( Cursor );
}

static
VOID
LdkpFillLogicalProcessorInformation (
    _Out_writes_bytes_(Length) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
    _In_ DWORD Length
    )
{
    KAFFINITY Mask = LdkpQueryActiveProcessorMask( 0 );
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Entry = Buffer;

    RtlZeroMemory( Buffer,
                   Length );

    Entry->Relationship = RelationNumaNode;
    Entry->ProcessorMask = (ULONG_PTR)Mask;
    Entry->NumaNode.NodeNumber = 0;
    Entry++;

    Entry->Relationship = RelationProcessorPackage;
    Entry->ProcessorMask = (ULONG_PTR)Mask;
    Entry++;

    for (ULONG BitIndex = 0; BitIndex < LDK_AFFINITY_BITS; BitIndex++) {
        KAFFINITY ProcessorMask = ((KAFFINITY)1) << BitIndex;

        if ((Mask & ProcessorMask) == 0) {
            continue;
        }

        Entry->Relationship = RelationProcessorCore;
        Entry->ProcessorMask = (ULONG_PTR)ProcessorMask;
        Entry->ProcessorCore.Flags = 0;
        Entry++;
    }
}

WINBASEAPI
BOOL
WINAPI
GlobalMemoryStatusEx (
    _Inout_ LPMEMORYSTATUSEX lpBuffer
    )
{
    NTSTATUS Status;
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
	SYSTEM_BASIC_INFORMATION BasicInfo;
    VM_COUNTERS VmCounters;
    QUOTA_LIMITS QuotaLimits;
    DWORDLONG AvailPageFile;
    DWORDLONG PhysicalMemory;

    PAGED_CODE();

    if (lpBuffer == NULL || lpBuffer->dwLength != sizeof(*lpBuffer)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

	Status = ZwQuerySystemInformation( SystemBasicInformation,
                                       &BasicInfo,
                                       sizeof(BasicInfo),
                                       NULL );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }
    Status = ZwQuerySystemInformation( SystemPerformanceInformation,
                                       &PerfInfo,
                                       sizeof(PerfInfo),
                                       NULL );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    PhysicalMemory = (DWORDLONG)BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;
    if (PerfInfo.AvailablePages < 100) {
        lpBuffer->dwMemoryLoad = 100;
    } else {
        lpBuffer->dwMemoryLoad = ((DWORD)(BasicInfo.NumberOfPhysicalPages - PerfInfo.AvailablePages) * 100) / BasicInfo.NumberOfPhysicalPages;
    }
    lpBuffer->ullTotalPhys = PhysicalMemory;
    PhysicalMemory = PerfInfo.AvailablePages;
    PhysicalMemory *= BasicInfo.PageSize;
    lpBuffer->ullAvailPhys = PhysicalMemory;

    RtlZeroMemory( &QuotaLimits,
                   sizeof(QUOTA_LIMITS));
    Status = ZwQueryInformationProcess (NtCurrentProcess(),
                                        ProcessQuotaLimits,
                                        &QuotaLimits,
                                        sizeof(QUOTA_LIMITS),
                                        NULL );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    RtlZeroMemory( &VmCounters,
                   sizeof(VM_COUNTERS));
    Status = ZwQueryInformationProcess (NtCurrentProcess(),
                                        ProcessVmCounters,
                                        &VmCounters,
                                        sizeof(VM_COUNTERS),
                                        NULL );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    lpBuffer->ullTotalPageFile = PerfInfo.CommitLimit;
    if (QuotaLimits.PagefileLimit < PerfInfo.CommitLimit) {
        lpBuffer->ullTotalPageFile = QuotaLimits.PagefileLimit;
    }
    lpBuffer->ullTotalPageFile *= BasicInfo.PageSize;
    AvailPageFile = PerfInfo.CommitLimit - PerfInfo.CommittedPages;
    lpBuffer->ullAvailPageFile =QuotaLimits.PagefileLimit - VmCounters.PagefileUsage;
    if ((ULONG)lpBuffer->ullTotalPageFile > (ULONG)AvailPageFile) {
        lpBuffer->ullAvailPageFile = AvailPageFile;
    }
    lpBuffer->ullAvailPageFile *= BasicInfo.PageSize;
    lpBuffer->ullTotalVirtual = (BasicInfo.MaximumUserModeAddress - BasicInfo.MinimumUserModeAddress) + 1;
    lpBuffer->ullAvailVirtual = lpBuffer->ullTotalVirtual - VmCounters.VirtualSize;
    lpBuffer->ullAvailExtendedVirtual = 0;
    return TRUE;
}



WINBASEAPI
VOID
WINAPI
GetSystemTime (
    _Out_ LPSYSTEMTIME lpSystemTime
    )
{
	LARGE_INTEGER SystemTime;
	TIME_FIELDS TimeFields;
	
	KeQuerySystemTime( &SystemTime );

	RtlTimeToTimeFields( &SystemTime,
						 &TimeFields );
	
	lpSystemTime->wYear = TimeFields.Year;
	lpSystemTime->wMonth = TimeFields.Month;
	lpSystemTime->wDayOfWeek = TimeFields.Weekday;
	lpSystemTime->wDay = TimeFields.Day;
	lpSystemTime->wHour = TimeFields.Hour;
	lpSystemTime->wMinute = TimeFields.Minute;
	lpSystemTime->wSecond = TimeFields.Second;
	lpSystemTime->wMilliseconds = TimeFields.Milliseconds;
}

WINBASEAPI
VOID
WINAPI
GetSystemTimePreciseAsFileTime (
    _Out_ LPFILETIME lpSystemTimeAsFileTime
    )
{
    LARGE_INTEGER SystemTime;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    KeQuerySystemTimePrecise(&SystemTime);
#else
    KeQuerySystemTime(&SystemTime);
#endif

    lpSystemTimeAsFileTime->dwLowDateTime = SystemTime.LowPart;
    lpSystemTimeAsFileTime->dwHighDateTime = SystemTime.HighPart;
}

WINBASEAPI
VOID
WINAPI
GetSystemTimeAsFileTime (
    _Out_ LPFILETIME lpSystemTimeAsFileTime
    )
{
	LARGE_INTEGER SystemTime;
	KeQuerySystemTime( &SystemTime );

	lpSystemTimeAsFileTime->dwLowDateTime = SystemTime.LowPart;
	lpSystemTimeAsFileTime->dwHighDateTime = SystemTime.HighPart;
}

WINBASEAPI
VOID
WINAPI
GetLocalTime (
    _Out_ LPSYSTEMTIME lpSystemTime
    )
{
	LARGE_INTEGER SystemTime;
	TIME_FIELDS TimeFields;
	
	KeQuerySystemTime( &SystemTime );

	RtlTimeToTimeFields( &SystemTime,
						 &TimeFields );
	
	lpSystemTime->wYear = TimeFields.Year;
	lpSystemTime->wMonth = TimeFields.Month;
	lpSystemTime->wDayOfWeek = TimeFields.Weekday;
	lpSystemTime->wDay = TimeFields.Day;
	lpSystemTime->wHour = TimeFields.Hour;
	lpSystemTime->wMinute = TimeFields.Minute;
	lpSystemTime->wSecond = TimeFields.Second;
	lpSystemTime->wMilliseconds = TimeFields.Milliseconds;
}

WINBASEAPI
BOOL
WINAPI
SetLocalTime (
    _In_ CONST SYSTEMTIME* lpSystemTime
    )
{
	TIME_FIELDS TimeFields;
	TimeFields.Year = lpSystemTime->wYear;
	TimeFields.Month = lpSystemTime->wMonth;
	TimeFields.Weekday = lpSystemTime->wDayOfWeek;
	TimeFields.Day = lpSystemTime->wDay;
	TimeFields.Hour = lpSystemTime->wHour;
	TimeFields.Minute = lpSystemTime->wMinute;
	TimeFields.Second = lpSystemTime->wSecond;
	TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

	LARGE_INTEGER SystemTime;
    if (! RtlTimeFieldsToTime( &TimeFields,
                               &SystemTime )) {
        return FALSE;
    }
    NTSTATUS Status = ZwSetSystemTime( &SystemTime,
                                       NULL );
    if (NT_SUCCESS(Status)) {
        return Status;
    }
    LdkSetLastNTError( Status );
    return Status;
}

WINBASEAPI
VOID
WINAPI
GetNativeSystemInfo (
    _Out_ LPSYSTEM_INFO lpSystemInfo
    )
{
    PAGED_CODE();

    GetSystemInfo( lpSystemInfo );
}

WINBASEAPI
BOOL
WINAPI
GetLogicalProcessorInformation (
    _Out_writes_bytes_to_opt_(*ReturnedLength, *ReturnedLength) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
    _Inout_ PDWORD ReturnedLength
    )
{
    DWORD RequiredLength;

    PAGED_CODE();

    if (ReturnedLength == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    RequiredLength = LdkpQueryLegacyLogicalProcessorInformationLength();
    if (Buffer == NULL ||
        *ReturnedLength < RequiredLength) {
        *ReturnedLength = RequiredLength;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    LdkpFillLogicalProcessorInformation( Buffer,
                                         RequiredLength );
    *ReturnedLength = RequiredLength;
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
GetLogicalProcessorInformationEx (
    _In_ LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType,
    _Out_writes_bytes_to_opt_(*ReturnedLength, *ReturnedLength) PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Buffer,
    _Inout_ PDWORD ReturnedLength
    )
{
    DWORD RequiredLength;

    PAGED_CODE();

    if (ReturnedLength == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (! LdkpIsSupportedProcessorRelationshipType( RelationshipType )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    RequiredLength = LdkpQueryLogicalProcessorInformationExLength(
        RelationshipType );
    if (RequiredLength == 0) {
        *ReturnedLength = 0;
        return TRUE;
    }

    if (Buffer == NULL ||
        *ReturnedLength < RequiredLength) {
        *ReturnedLength = RequiredLength;
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
    }

    LdkpFillLogicalProcessorInformationEx( RelationshipType,
                                           Buffer,
                                           RequiredLength );
    *ReturnedLength = RequiredLength;
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
GetNumaHighestNodeNumber (
    _Out_ PULONG HighestNodeNumber
    )
{
    PAGED_CODE();

    if (HighestNodeNumber == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *HighestNodeNumber = 0;
    return TRUE;
}

WINBASEAPI
VOID
WINAPI
GetSystemInfo (
    _Out_ LPSYSTEM_INFO lpSystemInfo
    )
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;

    PAGED_CODE();

    RtlZeroMemory( lpSystemInfo,
                   sizeof(SYSTEM_INFO) );

	Status = ZwQuerySystemInformation( SystemBasicInformation,
                                       &BasicInfo,
                                       sizeof(BasicInfo),
                                       NULL );
	if (! NT_SUCCESS(Status)) {
		return;
	}

    Status = ZwQuerySystemInformation( SystemProcessorInformation,
                                       &ProcessorInfo,
                                       sizeof(ProcessorInfo),
                                       NULL );
	if (! NT_SUCCESS(Status)) {
		return;
	}

    lpSystemInfo->wProcessorArchitecture = ProcessorInfo.ProcessorArchitecture;
    lpSystemInfo->wReserved = 0;
    lpSystemInfo->dwPageSize = BasicInfo.PageSize;
    lpSystemInfo->lpMinimumApplicationAddress = (LPVOID)BasicInfo.MinimumUserModeAddress;
    lpSystemInfo->lpMaximumApplicationAddress = (LPVOID)BasicInfo.MaximumUserModeAddress;
    lpSystemInfo->dwActiveProcessorMask = BasicInfo.ActiveProcessorsAffinityMask;
    lpSystemInfo->dwNumberOfProcessors = BasicInfo.NumberOfProcessors;
    lpSystemInfo->wProcessorLevel = ProcessorInfo.ProcessorLevel;
    lpSystemInfo->wProcessorRevision = ProcessorInfo.ProcessorRevision;
	if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) {
		if (ProcessorInfo.ProcessorLevel == 3) {
			lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_386;
		} else if (ProcessorInfo.ProcessorLevel == 4) {
			lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_486;
		} else {
			lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_PENTIUM;
		}
	} else if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_MIPS) {
		lpSystemInfo->dwProcessorType = PROCESSOR_MIPS_R4000;
	} else if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_ALPHA) {
        lpSystemInfo->dwProcessorType = PROCESSOR_ALPHA_21064;
    } else if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_PPC) {
        lpSystemInfo->dwProcessorType = PROCESSOR_PPC_604;
	} else if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
		lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_IA64;
	} else {
		lpSystemInfo->dwProcessorType = PROCESSOR_ARCHITECTURE_INTEL;
	}
	lpSystemInfo->dwAllocationGranularity = BasicInfo.AllocationGranularity;
}

WINBASEAPI
DWORD
WINAPI
GetCurrentProcessorNumber (
    VOID
    )
{
    PAGED_CODE();

    return KeGetCurrentProcessorNumber();
}

WINBASEAPI
VOID
WINAPI
GetCurrentProcessorNumberEx (
    _Out_ PPROCESSOR_NUMBER ProcNumber
    )
{
    PAGED_CODE();

    if (ProcNumber == NULL) {
        return;
    }

    KeGetCurrentProcessorNumberEx( ProcNumber );
}

WINBASEAPI
WORD
WINAPI
GetActiveProcessorGroupCount (
    VOID
    )
{
    PAGED_CODE();

    return KeQueryActiveGroupCount();
}

WINBASEAPI
WORD
WINAPI
GetMaximumProcessorGroupCount (
    VOID
    )
{
    PAGED_CODE();

    return KeQueryMaximumGroupCount();
}

WINBASEAPI
DWORD
WINAPI
GetActiveProcessorCount (
    _In_ WORD GroupNumber
    )
{
    PAGED_CODE();

    if (GroupNumber != ALL_PROCESSOR_GROUPS &&
        GroupNumber >= KeQueryActiveGroupCount()) {
        return 0;
    }

    return KeQueryActiveProcessorCountEx( GroupNumber );
}

WINBASEAPI
DWORD
WINAPI
GetMaximumProcessorCount (
    _In_ WORD GroupNumber
    )
{
    PAGED_CODE();

    if (GroupNumber != ALL_PROCESSOR_GROUPS &&
        GroupNumber >= KeQueryMaximumGroupCount()) {
        return 0;
    }

    return KeQueryMaximumProcessorCountEx( GroupNumber );
}



__drv_preferredFunction("GetTickCount64", "GetTickCount overflows roughly every 49 days.  Code that does not take that into account can loop indefinitely.  GetTickCount64 operates on 64 bit values and does not have that problem")
WINBASEAPI
DWORD
WINAPI
GetTickCount (
    VOID
    )
{
	LARGE_INTEGER TickCount;
	KeQueryTickCount( &TickCount );
	return (DWORD)((TickCount.QuadPart * SharedUserData->TickCountMultiplier) >> 24);
}

WINBASEAPI
ULONGLONG
WINAPI
GetTickCount64 (
    VOID
    )
{
	LARGE_INTEGER TickCount;
	KeQueryTickCount(&TickCount);
	return ((TickCount.QuadPart * SharedUserData->TickCountMultiplier) >> 24);
}


WINBASEAPI
DWORD
WINAPI
GetVersion (
    VOID
    )
{
    RTL_OSVERSIONINFOW vi;

    PAGED_CODE();

    if (! NT_SUCCESS(RtlGetVersion( &vi ))) {
        return 0;
    }

    return (((vi.dwPlatformId ^ 0x2) << 30) |
            (vi.dwBuildNumber << 16) |
            (vi.dwMinorVersion << 8) |
             vi.dwMajorVersion
           );
}

WINBASEAPI
BOOL
WINAPI
GetVersionExA (
    _Inout_ LPOSVERSIONINFOA lpVersionInformation
    )
{
	STRING CSDVersionA;
	UNICODE_STRING CSDVersionW;
	OSVERSIONINFOEXW VersionInformationU;

    PAGED_CODE();

	if (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA) &&
		lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA)) {
		SetLastError( ERROR_INSUFFICIENT_BUFFER );
		return FALSE;
	}

	VersionInformationU.dwOSVersionInfoSize = sizeof(VersionInformationU);
	if (GetVersionExW( (LPOSVERSIONINFOW)&VersionInformationU) ) {
		lpVersionInformation->dwMajorVersion = VersionInformationU.dwMajorVersion;
		lpVersionInformation->dwMinorVersion = VersionInformationU.dwMinorVersion;
		lpVersionInformation->dwBuildNumber = VersionInformationU.dwBuildNumber;
		lpVersionInformation->dwPlatformId = VersionInformationU.dwPlatformId;

		if (lpVersionInformation->dwOSVersionInfoSize == sizeof(OSVERSIONINFOEXA)) {
			((POSVERSIONINFOEXA)lpVersionInformation)->wServicePackMajor = VersionInformationU.wServicePackMajor;
			((POSVERSIONINFOEXA)lpVersionInformation)->wServicePackMinor = VersionInformationU.wServicePackMinor;
			((POSVERSIONINFOEXA)lpVersionInformation)->wSuiteMask = VersionInformationU.wSuiteMask;
			((POSVERSIONINFOEXA)lpVersionInformation)->wProductType = VersionInformationU.wProductType;
			((POSVERSIONINFOEXA)lpVersionInformation)->wReserved = VersionInformationU.wReserved;			
			if (VersionInformationU.szCSDVersion[0] == UNICODE_NULL) {
				lpVersionInformation->szCSDVersion[0] = ANSI_NULL;
				return TRUE;
			}
			CSDVersionA.Length = 0;
			CSDVersionA.MaximumLength = sizeof(lpVersionInformation->szCSDVersion);
			CSDVersionA.Buffer = lpVersionInformation->szCSDVersion;
			RtlInitUnicodeString( &CSDVersionW,
                                  VersionInformationU.szCSDVersion );
			return NT_SUCCESS(LdkUnicodeStringToAnsiString( &CSDVersionA,
                                                            &CSDVersionW,
                                                            FALSE ));
		}
		return TRUE;
	}
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetVersionExW (
    _Inout_ LPOSVERSIONINFOW lpVersionInformation
    )
{
    PAGED_CODE();

    if (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW) &&
		lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW)) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
	}

	RtlZeroMemory( lpVersionInformation,
                   lpVersionInformation->dwOSVersionInfoSize );
	return NT_SUCCESS(RtlGetVersion( lpVersionInformation ));
}
