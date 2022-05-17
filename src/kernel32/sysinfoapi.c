#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GlobalMemoryStatusEx)
#pragma alloc_text(PAGE, GetNativeSystemInfo)
#pragma alloc_text(PAGE, GetSystemInfo)
#pragma alloc_text(PAGE, GetVersion)
#pragma alloc_text(PAGE, GetVersionExA)
#pragma alloc_text(PAGE, GetVersionExW)
#endif



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

#if (NTDDI_VERSION >= NTDDI_WIN8)
WINBASEAPI
VOID
WINAPI
GetSystemTimePreciseAsFileTime (
    _Out_ LPFILETIME lpSystemTimeAsFileTime
    )
{
    LARGE_INTEGER SystemTime;
    KeQuerySystemTimePrecise(&SystemTime);

    lpSystemTimeAsFileTime->dwLowDateTime = SystemTime.LowPart;
    lpSystemTimeAsFileTime->dwHighDateTime = SystemTime.HighPart;
}
#endif

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
