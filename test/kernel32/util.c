#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
UtilityApiTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UtilityApiTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <stdio.h>
#define PAGED_CODE()
#endif

typedef
PVOID
(WINAPI *PLDK_ENCODE_POINTER_ROUTINE) (
    _In_opt_ PVOID Ptr
    );

typedef
PVOID
(WINAPI *PLDK_DECODE_POINTER_ROUTINE) (
    _In_opt_ PVOID Ptr
    );

static
BOOLEAN
LdkpTestPointerCodec (
    _In_z_ PCSTR Name,
    _In_ PLDK_ENCODE_POINTER_ROUTINE EncodeRoutine,
    _In_ PLDK_DECODE_POINTER_ROUTINE DecodeRoutine
    )
{
    ULONG LocalMarker = 0x12345678;
    PVOID Values[] = {
        NULL,
        (PVOID)(ULONG_PTR)1,
        (PVOID)(ULONG_PTR)0x12345678,
        &LocalMarker
    };

    for (ULONG Index = 0; Index < RTL_NUMBER_OF(Values); Index++) {
        PVOID Value = Values[Index];
        PVOID Encoded = EncodeRoutine( Value );
        PVOID EncodedAgain = EncodeRoutine( Value );
        PVOID Decoded = DecodeRoutine( Encoded );

        if (Encoded != EncodedAgain) {
            fprintf(stderr,
                    "[Failed] %s was not stable Value = %p Encoded = %p EncodedAgain = %p\n",
                    Name,
                    Value,
                    Encoded,
                    EncodedAgain );
            return FALSE;
        }

        if (Decoded != Value) {
            fprintf(stderr,
                    "[Failed] %s round-trip Value = %p Encoded = %p Decoded = %p\n",
                    Name,
                    Value,
                    Encoded,
                    Decoded );
            return FALSE;
        }

        if (Encoded == Value) {
            fprintf(stderr,
                    "[Failed] %s returned the original pointer Value = %p\n",
                    Name,
                    Value );
            return FALSE;
        }
    }

    return TRUE;
}

static
BOOLEAN
LdkpTestLogicalProcessorInformation (
    VOID
    )
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ProcessorInformation = NULL;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX ProcessorInformationEx = NULL;
    DWORD Length = 0;
    DWORD EntryCount;
    DWORD CoreCount = 0;
    DWORD NodeCount = 0;
    DWORD PackageCount = 0;
    DWORD GroupCount = 0;
    DWORD Offset;
    ULONG HighestNodeNumber;
    BOOL Result;

    Result = GetLogicalProcessorInformation( NULL,
                                             &Length );
    if (Result ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        Length < sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION)) {
        fprintf(stderr,
                "[Failed] GetLogicalProcessorInformation sizing Result = %lu ErrorCode = %lu Length = %lu\n",
                (DWORD)Result,
                GetLastError(),
                Length );
        return FALSE;
    }

    ProcessorInformation = HeapAlloc( GetProcessHeap(),
                                      HEAP_ZERO_MEMORY,
                                      Length );
    if (ProcessorInformation == NULL) {
        fprintf(stderr,
                "[Failed] HeapAlloc logical processor information Length = %lu ErrorCode = %lu\n",
                Length,
                GetLastError() );
        return FALSE;
    }

    Result = GetLogicalProcessorInformation( ProcessorInformation,
                                             &Length );
    if (! Result) {
        fprintf(stderr,
                "[Failed] GetLogicalProcessorInformation ErrorCode = %lu Length = %lu\n",
                GetLastError(),
                Length );
        HeapFree( GetProcessHeap(),
                  0,
                  ProcessorInformation );
        return FALSE;
    }

    EntryCount = Length / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    for (DWORD Index = 0; Index < EntryCount; Index++) {
        if (ProcessorInformation[Index].ProcessorMask == 0) {
            fprintf(stderr,
                    "[Failed] GetLogicalProcessorInformation empty mask Index = %lu Relationship = %lu\n",
                    Index,
                    (DWORD)ProcessorInformation[Index].Relationship );
            HeapFree( GetProcessHeap(),
                      0,
                      ProcessorInformation );
            return FALSE;
        }

        switch (ProcessorInformation[Index].Relationship) {
        case RelationProcessorCore:
            CoreCount++;
            break;

        case RelationNumaNode:
            NodeCount++;
            break;

        case RelationProcessorPackage:
            PackageCount++;
            break;

        default:
            break;
        }
    }

    HeapFree( GetProcessHeap(),
              0,
              ProcessorInformation );

    if (CoreCount == 0 ||
        NodeCount == 0 ||
        PackageCount == 0) {
        fprintf(stderr,
                "[Failed] GetLogicalProcessorInformation relationships Core = %lu Node = %lu Package = %lu Length = %lu\n",
                CoreCount,
                NodeCount,
                PackageCount,
                Length );
        return FALSE;
    }

    if (! GetNumaHighestNodeNumber( &HighestNodeNumber )) {
        fprintf(stderr,
                "[Failed] GetNumaHighestNodeNumber ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    Length = 0;
    Result = GetLogicalProcessorInformationEx( RelationAll,
                                               NULL,
                                               &Length );
    if (Result ||
        GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
        Length < sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)) {
        fprintf(stderr,
                "[Failed] GetLogicalProcessorInformationEx sizing Result = %lu ErrorCode = %lu Length = %lu\n",
                (DWORD)Result,
                GetLastError(),
                Length );
        return FALSE;
    }

    ProcessorInformationEx = HeapAlloc( GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        Length );
    if (ProcessorInformationEx == NULL) {
        fprintf(stderr,
                "[Failed] HeapAlloc logical processor information ex Length = %lu ErrorCode = %lu\n",
                Length,
                GetLastError() );
        return FALSE;
    }

    Result = GetLogicalProcessorInformationEx( RelationAll,
                                               ProcessorInformationEx,
                                               &Length );
    if (! Result) {
        fprintf(stderr,
                "[Failed] GetLogicalProcessorInformationEx ErrorCode = %lu Length = %lu\n",
                GetLastError(),
                Length );
        HeapFree( GetProcessHeap(),
                  0,
                  ProcessorInformationEx );
        return FALSE;
    }

    CoreCount = 0;
    NodeCount = 0;
    PackageCount = 0;
    GroupCount = 0;
    Offset = 0;

    while (Offset < Length) {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Entry;

        Entry = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)
            ((PBYTE)ProcessorInformationEx + Offset);
        if (Entry->Size < sizeof(LOGICAL_PROCESSOR_RELATIONSHIP) + sizeof(DWORD) ||
            Entry->Size > Length - Offset) {
            fprintf(stderr,
                    "[Failed] GetLogicalProcessorInformationEx entry size Offset = %lu Size = %lu Length = %lu\n",
                    Offset,
                    Entry->Size,
                    Length );
            HeapFree( GetProcessHeap(),
                      0,
                      ProcessorInformationEx );
            return FALSE;
        }

        switch (Entry->Relationship) {
        case RelationProcessorCore:
            if (Entry->Processor.GroupCount == 0 ||
                Entry->Processor.GroupMask[0].Mask == 0) {
                fprintf(stderr,
                        "[Failed] processor core relationship GroupCount = %hu Mask = %p\n",
                        Entry->Processor.GroupCount,
                        (PVOID)Entry->Processor.GroupMask[0].Mask );
                HeapFree( GetProcessHeap(),
                          0,
                          ProcessorInformationEx );
                return FALSE;
            }
            CoreCount++;
            break;

        case RelationNumaNode:
        case RelationNumaNodeEx:
            if (Entry->NumaNode.GroupCount == 0 ||
                Entry->NumaNode.GroupMask.Mask == 0) {
                fprintf(stderr,
                        "[Failed] NUMA relationship GroupCount = %hu Mask = %p\n",
                        Entry->NumaNode.GroupCount,
                        (PVOID)Entry->NumaNode.GroupMask.Mask );
                HeapFree( GetProcessHeap(),
                          0,
                          ProcessorInformationEx );
                return FALSE;
            }
            NodeCount++;
            break;

        case RelationProcessorPackage:
            if (Entry->Processor.GroupCount == 0 ||
                Entry->Processor.GroupMask[0].Mask == 0) {
                fprintf(stderr,
                        "[Failed] package relationship GroupCount = %hu Mask = %p\n",
                        Entry->Processor.GroupCount,
                        (PVOID)Entry->Processor.GroupMask[0].Mask );
                HeapFree( GetProcessHeap(),
                          0,
                          ProcessorInformationEx );
                return FALSE;
            }
            PackageCount++;
            break;

        case RelationGroup:
            if (Entry->Group.ActiveGroupCount == 0 ||
                Entry->Group.GroupInfo[0].ActiveProcessorMask == 0) {
                fprintf(stderr,
                        "[Failed] group relationship ActiveGroupCount = %hu Mask = %p\n",
                        Entry->Group.ActiveGroupCount,
                        (PVOID)Entry->Group.GroupInfo[0].ActiveProcessorMask );
                HeapFree( GetProcessHeap(),
                          0,
                          ProcessorInformationEx );
                return FALSE;
            }
            GroupCount++;
            break;

        default:
            break;
        }

        Offset += Entry->Size;
    }

    HeapFree( GetProcessHeap(),
              0,
              ProcessorInformationEx );

    if (CoreCount == 0 ||
        NodeCount == 0 ||
        PackageCount == 0 ||
        GroupCount == 0) {
        fprintf(stderr,
                "[Failed] GetLogicalProcessorInformationEx relationships Core = %lu Node = %lu Package = %lu Group = %lu Length = %lu\n",
                CoreCount,
                NodeCount,
                PackageCount,
                GroupCount,
                Length );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
LdkpTestVirtualMemory (
    VOID
    )
{
    PVOID BaseAddress = NULL;
    SIZE_T RegionSize = 0x1000;
    MEMORY_BASIC_INFORMATION MemoryInformation;
    SIZE_T QueryLength;

    if (VirtualFree( NULL,
                     0,
                     MEM_RELEASE ) ||
        GetLastError() != ERROR_INVALID_ADDRESS) {
        fprintf(stderr,
                "[Failed] VirtualFree accepted NULL address ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (VirtualFree( (PVOID)(ULONG_PTR)0x10000,
                     1,
                     MEM_RELEASE ) ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
        fprintf(stderr,
                "[Failed] VirtualFree accepted MEM_RELEASE size ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (VirtualFree( (PVOID)(ULONG_PTR)0x10000,
                     0,
                     MEM_DECOMMIT | MEM_RELEASE ) ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
        fprintf(stderr,
                "[Failed] VirtualFree accepted conflicting free flags ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

#if _KERNEL_MODE
    {
        NTSTATUS Status;

        Status = ZwAllocateVirtualMemory( NtCurrentProcess(),
                                          &BaseAddress,
                                          0,
                                          &RegionSize,
                                          MEM_RESERVE | MEM_COMMIT,
                                          PAGE_READWRITE );
        if (! NT_SUCCESS(Status)) {
            fprintf(stderr,
                    "[Failed] ZwAllocateVirtualMemory Status = 0x%08lx Size = %Iu\n",
                    Status,
                    RegionSize );
            return FALSE;
        }
    }
#else
    BaseAddress = VirtualAlloc( NULL,
                                RegionSize,
                                MEM_RESERVE | MEM_COMMIT,
                                PAGE_READWRITE );
    if (BaseAddress == NULL) {
        fprintf(stderr,
                "[Failed] VirtualAlloc ErrorCode = %lu Size = %Iu\n",
                GetLastError(),
                RegionSize );
        return FALSE;
    }
#endif

    QueryLength = VirtualQuery( BaseAddress,
                                &MemoryInformation,
                                sizeof(MemoryInformation) );
    if (QueryLength == 0 ||
        MemoryInformation.State != MEM_COMMIT) {
        fprintf(stderr,
                "[Failed] VirtualQuery allocated region QueryLength = %Iu State = 0x%08lx ErrorCode = %lu\n",
                QueryLength,
                MemoryInformation.State,
                GetLastError() );
        VirtualFree( BaseAddress,
                     0,
                     MEM_RELEASE );
        return FALSE;
    }

    if (! VirtualFree( BaseAddress,
                       0,
                       MEM_RELEASE )) {
        fprintf(stderr,
                "[Failed] VirtualFree MEM_RELEASE ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
LdkpTestProcessorAffinity (
    VOID
    )
{
    DWORD_PTR ProcessMask;
    DWORD_PTR SystemMask;
    HANDLE ProcessHandle;
    GROUP_AFFINITY GroupAffinity;
    GROUP_AFFINITY PreviousGroupAffinity;

    if (! GetProcessAffinityMask( GetCurrentProcess(),
                                  &ProcessMask,
                                  &SystemMask )) {
        fprintf(stderr,
                "[Failed] GetProcessAffinityMask ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (ProcessMask == 0 ||
        SystemMask == 0 ||
        (ProcessMask & ~SystemMask) != 0) {
        fprintf(stderr,
                "[Failed] process affinity ProcessMask = %p SystemMask = %p\n",
                (PVOID)ProcessMask,
                (PVOID)SystemMask );
        return FALSE;
    }

    if (! SetProcessAffinityMask( GetCurrentProcess(),
                                  ProcessMask )) {
        fprintf(stderr,
                "[Failed] SetProcessAffinityMask ErrorCode = %lu ProcessMask = %p\n",
                GetLastError(),
                (PVOID)ProcessMask );
        return FALSE;
    }

    ProcessHandle = OpenProcess( PROCESS_SET_INFORMATION,
                                 FALSE,
                                 GetCurrentProcessId() );
    if (ProcessHandle == NULL) {
        fprintf(stderr,
                "[Failed] OpenProcess(PROCESS_SET_INFORMATION) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (! SetProcessAffinityMask( ProcessHandle,
                                  ProcessMask )) {
        fprintf(stderr,
                "[Failed] SetProcessAffinityMask opened handle ErrorCode = %lu ProcessMask = %p\n",
                GetLastError(),
                (PVOID)ProcessMask );
        CloseHandle( ProcessHandle );
        return FALSE;
    }

    CloseHandle( ProcessHandle );

    if (SetProcessAffinityMask( GetCurrentProcess(),
                                0 )) {
        fprintf(stderr,
                "[Failed] SetProcessAffinityMask accepted zero mask\n" );
        return FALSE;
    }

    RtlZeroMemory( &GroupAffinity,
                   sizeof(GroupAffinity) );
    if (! GetThreadGroupAffinity( GetCurrentThread(),
                                  &GroupAffinity )) {
        fprintf(stderr,
                "[Failed] GetThreadGroupAffinity ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (GroupAffinity.Mask == 0) {
        fprintf(stderr,
                "[Failed] GetThreadGroupAffinity empty mask Group = %hu\n",
                GroupAffinity.Group );
        return FALSE;
    }

    RtlZeroMemory( &PreviousGroupAffinity,
                   sizeof(PreviousGroupAffinity) );
    if (! SetThreadGroupAffinity( GetCurrentThread(),
                                  &GroupAffinity,
                                  &PreviousGroupAffinity )) {
        fprintf(stderr,
                "[Failed] SetThreadGroupAffinity ErrorCode = %lu Group = %hu Mask = %p\n",
                GetLastError(),
                GroupAffinity.Group,
                (PVOID)GroupAffinity.Mask );
        return FALSE;
    }

    if (PreviousGroupAffinity.Mask != 0) {
        SetThreadGroupAffinity( GetCurrentThread(),
                                &PreviousGroupAffinity,
                                NULL );
    }

    return TRUE;
}

static
BOOLEAN
LdkpTestProcessorInformation (
    VOID
    )
{
    SYSTEM_INFO SystemInfo;
    PROCESSOR_NUMBER ProcessorNumber;
    WORD ActiveGroupCount;
    WORD MaximumGroupCount;
    DWORD ActiveProcessorCount;
    DWORD MaximumProcessorCount;
    DWORD ActiveGroupProcessorCount;
    DWORD MaximumGroupProcessorCount;
    DWORD CurrentProcessorNumber;

    RtlZeroMemory( &SystemInfo,
                   sizeof(SystemInfo) );
    RtlZeroMemory( &ProcessorNumber,
                   sizeof(ProcessorNumber) );

    GetNativeSystemInfo( &SystemInfo );
    if (SystemInfo.dwNumberOfProcessors == 0 ||
        SystemInfo.dwPageSize == 0) {
        fprintf(stderr,
                "[Failed] GetNativeSystemInfo Processors = %lu PageSize = %lu\n",
                SystemInfo.dwNumberOfProcessors,
                SystemInfo.dwPageSize );
        return FALSE;
    }

    GetCurrentProcessorNumberEx( &ProcessorNumber );
    CurrentProcessorNumber = GetCurrentProcessorNumber();
    ActiveGroupCount = GetActiveProcessorGroupCount();
    MaximumGroupCount = GetMaximumProcessorGroupCount();
    ActiveProcessorCount = GetActiveProcessorCount( ALL_PROCESSOR_GROUPS );
    MaximumProcessorCount = GetMaximumProcessorCount( ALL_PROCESSOR_GROUPS );

    if (ActiveGroupCount == 0 ||
        MaximumGroupCount < ActiveGroupCount ||
        ActiveProcessorCount == 0 ||
        MaximumProcessorCount < ActiveProcessorCount) {
        fprintf(stderr,
                "[Failed] processor group summary ActiveGroups = %hu MaximumGroups = %hu ActiveProcessors = %lu MaximumProcessors = %lu\n",
                ActiveGroupCount,
                MaximumGroupCount,
                ActiveProcessorCount,
                MaximumProcessorCount );
        return FALSE;
    }

    if (ProcessorNumber.Group >= ActiveGroupCount) {
        fprintf(stderr,
                "[Failed] current processor group Group = %hu ActiveGroups = %hu\n",
                ProcessorNumber.Group,
                ActiveGroupCount );
        return FALSE;
    }

    ActiveGroupProcessorCount = GetActiveProcessorCount( ProcessorNumber.Group );
    MaximumGroupProcessorCount = GetMaximumProcessorCount( ProcessorNumber.Group );
    if (ActiveGroupProcessorCount == 0 ||
        MaximumGroupProcessorCount < ActiveGroupProcessorCount ||
        ProcessorNumber.Number >= ActiveGroupProcessorCount ||
        CurrentProcessorNumber >= ActiveProcessorCount) {
        fprintf(stderr,
                "[Failed] current processor summary Group = %hu Number = %lu Current = %lu ActiveGroupProcessors = %lu MaximumGroupProcessors = %lu ActiveProcessors = %lu\n",
                ProcessorNumber.Group,
                (DWORD)ProcessorNumber.Number,
                CurrentProcessorNumber,
                ActiveGroupProcessorCount,
                MaximumGroupProcessorCount,
                ActiveProcessorCount );
        return FALSE;
    }

    if (! LdkpTestLogicalProcessorInformation()) {
        return FALSE;
    }

    if (! LdkpTestProcessorAffinity()) {
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
LdkpTestTimeAndVersionInformation (
    VOID
    )
{
    FILETIME SystemTime;
    FILETIME PreciseSystemTime;
    TIME_ZONE_INFORMATION TimeZoneInformation;
    DWORD TimeZoneId;
    OSVERSIONINFOEXW CurrentVersion;
    OSVERSIONINFOEXW MinimumVersion;
    OSVERSIONINFOEXA MinimumVersionA;
    OSVERSIONINFOEXW FutureVersion;
    OSVERSIONINFOEXW InvalidVersion;
    DWORDLONG ConditionMask;
    BOOL VersionResult;

    RtlZeroMemory( &SystemTime,
                   sizeof(SystemTime) );
    RtlZeroMemory( &PreciseSystemTime,
                   sizeof(PreciseSystemTime) );

    GetSystemTimeAsFileTime( &SystemTime );
    GetSystemTimePreciseAsFileTime( &PreciseSystemTime );
    if ((SystemTime.dwLowDateTime == 0 &&
         SystemTime.dwHighDateTime == 0) ||
        (PreciseSystemTime.dwLowDateTime == 0 &&
         PreciseSystemTime.dwHighDateTime == 0)) {
        fprintf(stderr,
                "[Failed] system file time System = %08lx:%08lx Precise = %08lx:%08lx\n",
                SystemTime.dwHighDateTime,
                SystemTime.dwLowDateTime,
                PreciseSystemTime.dwHighDateTime,
                PreciseSystemTime.dwLowDateTime );
        return FALSE;
    }

    RtlZeroMemory( &TimeZoneInformation,
                   sizeof(TimeZoneInformation) );
    TimeZoneId = GetTimeZoneInformation( &TimeZoneInformation );
    if (TimeZoneId != TIME_ZONE_ID_UNKNOWN &&
        TimeZoneId != TIME_ZONE_ID_STANDARD &&
        TimeZoneId != TIME_ZONE_ID_DAYLIGHT) {
        fprintf(stderr,
                "[Failed] GetTimeZoneInformation TimeZoneId = %lu ErrorCode = %lu\n",
                TimeZoneId,
                GetLastError() );
        return FALSE;
    }

    RtlZeroMemory( &CurrentVersion,
                   sizeof(CurrentVersion) );
    CurrentVersion.dwOSVersionInfoSize = sizeof(CurrentVersion);
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4996)
#endif
    VersionResult = GetVersionExW( (LPOSVERSIONINFOW)&CurrentVersion );
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
    if (!VersionResult) {
        fprintf(stderr,
                "[Failed] GetVersionExW ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    RtlZeroMemory( &MinimumVersion,
                   sizeof(MinimumVersion) );
    MinimumVersion.dwOSVersionInfoSize = sizeof(MinimumVersion);
    MinimumVersion.dwMajorVersion = 6;
    MinimumVersion.dwMinorVersion = 1;
    MinimumVersion.wServicePackMajor = 0;
    ConditionMask = 0;
    ConditionMask = VerSetConditionMask( ConditionMask,
                                         VER_MAJORVERSION,
                                         VER_GREATER_EQUAL );
    ConditionMask = VerSetConditionMask( ConditionMask,
                                         VER_MINORVERSION,
                                         VER_GREATER_EQUAL );
    ConditionMask = VerSetConditionMask( ConditionMask,
                                         VER_SERVICEPACKMAJOR,
                                         VER_GREATER_EQUAL );
    if (!VerifyVersionInfoW( &MinimumVersion,
                             VER_MAJORVERSION |
                             VER_MINORVERSION |
                             VER_SERVICEPACKMAJOR,
                             ConditionMask )) {
        fprintf(stderr,
                "[Failed] VerifyVersionInfoW minimum version ErrorCode = %lu Current = %lu.%lu\n",
                GetLastError(),
                CurrentVersion.dwMajorVersion,
                CurrentVersion.dwMinorVersion );
        return FALSE;
    }

    RtlZeroMemory( &MinimumVersionA,
                   sizeof(MinimumVersionA) );
    MinimumVersionA.dwOSVersionInfoSize = sizeof(MinimumVersionA);
    MinimumVersionA.dwMajorVersion = MinimumVersion.dwMajorVersion;
    MinimumVersionA.dwMinorVersion = MinimumVersion.dwMinorVersion;
    MinimumVersionA.wServicePackMajor = MinimumVersion.wServicePackMajor;
    if (!VerifyVersionInfoA( &MinimumVersionA,
                             VER_MAJORVERSION |
                             VER_MINORVERSION |
                             VER_SERVICEPACKMAJOR,
                             ConditionMask )) {
        fprintf(stderr,
                "[Failed] VerifyVersionInfoA minimum version ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    RtlZeroMemory( &InvalidVersion,
                   sizeof(InvalidVersion) );
    InvalidVersion.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXW) - 1;
    InvalidVersion.dwMajorVersion = 1;
    ConditionMask = VerSetConditionMask( 0,
                                         VER_MAJORVERSION,
                                         VER_GREATER_EQUAL );
    if (!VerifyVersionInfoW( &InvalidVersion,
                             VER_MAJORVERSION,
                             ConditionMask )) {
        fprintf(stderr,
                "[Failed] VerifyVersionInfoW rejected native-compatible size-tolerant basic check ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    RtlZeroMemory( &FutureVersion,
                   sizeof(FutureVersion) );
    FutureVersion.dwOSVersionInfoSize = sizeof(FutureVersion);
    FutureVersion.dwMajorVersion = CurrentVersion.dwMajorVersion + 1;
    ConditionMask = 0;
    VER_SET_CONDITION( ConditionMask,
                       VER_MAJORVERSION,
                       VER_GREATER_EQUAL );
    if (VerifyVersionInfoW( &FutureVersion,
                            VER_MAJORVERSION,
                            ConditionMask )) {
        fprintf(stderr,
                "[Failed] VerifyVersionInfoW accepted future major version Current = %lu Future = %lu\n",
                CurrentVersion.dwMajorVersion,
                FutureVersion.dwMajorVersion );
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
UtilityApiTest (
    VOID
    )
{
    PAGED_CODE();

    printf("Utility API Test\n");

    if (! LdkpTestPointerCodec( "EncodePointer/DecodePointer",
                                EncodePointer,
                                DecodePointer )) {
        return FALSE;
    }

    if (! LdkpTestPointerCodec( "EncodeSystemPointer/DecodeSystemPointer",
                                EncodeSystemPointer,
                                DecodeSystemPointer )) {
        return FALSE;
    }

    if (! LdkpTestProcessorInformation()) {
        return FALSE;
    }

    if (! LdkpTestTimeAndVersionInformation()) {
        return FALSE;
    }

    if (! LdkpTestVirtualMemory()) {
        return FALSE;
    }

    printf("[Success] Utility API Test\n\n");
    return TRUE;
}
