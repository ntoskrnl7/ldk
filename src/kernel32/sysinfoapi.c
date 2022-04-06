#include "winbase.h"
#include "../nt/ntpsapi.h"
#include "../nt/ntexapi.h"
#include "../ldk.h"



WINBASEAPI
BOOL
WINAPI
GlobalMemoryStatusEx(
    _Inout_ LPMEMORYSTATUSEX lpBuffer
    )
{
    SYSTEM_PERFORMANCE_INFORMATION PerfInfo;
	SYSTEM_BASIC_INFORMATION BasicInfo;
    VM_COUNTERS VmCounters;
    QUOTA_LIMITS QuotaLimits;
    DWORDLONG AvailPageFile;
    DWORDLONG PhysicalMemory;
    NTSTATUS Status;

    if (lpBuffer == NULL || lpBuffer->dwLength != sizeof(*lpBuffer)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

	EXIT_WHEN_DPC_WITH_RETURN(FALSE);

	Status = ZwQuerySystemInformation(
            SystemBasicInformation,
            &BasicInfo,
            sizeof(BasicInfo),
            NULL
            );

    Status = ZwQuerySystemInformation(
                SystemPerformanceInformation,
                &PerfInfo,
                sizeof(PerfInfo),
                NULL
                );

    ASSERT(NT_SUCCESS(Status));

    PhysicalMemory =  (DWORDLONG)BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;

    if (PerfInfo.AvailablePages < 100) {
        lpBuffer->dwMemoryLoad = 100;
        }
    else {
        lpBuffer->dwMemoryLoad =
            ((DWORD)(BasicInfo.NumberOfPhysicalPages -
                     PerfInfo.AvailablePages
                    ) * 100
            ) / BasicInfo.NumberOfPhysicalPages;
        }

    lpBuffer->ullTotalPhys = PhysicalMemory;

    PhysicalMemory = PerfInfo.AvailablePages;

    PhysicalMemory *= BasicInfo.PageSize;

    lpBuffer->ullAvailPhys = PhysicalMemory;

    RtlZeroMemory(&QuotaLimits, sizeof(QUOTA_LIMITS));
    RtlZeroMemory(&VmCounters, sizeof(VM_COUNTERS));

    Status = ZwQueryInformationProcess (NtCurrentProcess(),
                                        ProcessQuotaLimits,
                                        &QuotaLimits,
                                        sizeof(QUOTA_LIMITS),
                                        NULL );

    Status = ZwQueryInformationProcess (NtCurrentProcess(),
                                        ProcessVmCounters,
                                        &VmCounters,
                                        sizeof(VM_COUNTERS),
                                        NULL );

    lpBuffer->ullTotalPageFile = PerfInfo.CommitLimit;
    if (QuotaLimits.PagefileLimit < PerfInfo.CommitLimit) {
        lpBuffer->ullTotalPageFile = QuotaLimits.PagefileLimit;
    }

    lpBuffer->ullTotalPageFile *= BasicInfo.PageSize;

    AvailPageFile = PerfInfo.CommitLimit - PerfInfo.CommittedPages;

    lpBuffer->ullAvailPageFile =
                    QuotaLimits.PagefileLimit - VmCounters.PagefileUsage;

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
GetSystemTime(
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
GetSystemTimePreciseAsFileTime(
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
GetSystemTimeAsFileTime(
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
GetLocalTime(
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
GetNativeSystemInfo(
    _Out_ LPSYSTEM_INFO lpSystemInfo
    )
{
    GetSystemInfo(lpSystemInfo);
}

WINBASEAPI
VOID
WINAPI
GetSystemInfo(
    _Out_ LPSYSTEM_INFO lpSystemInfo
    )
{
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcessorInfo;

    RtlZeroMemory(lpSystemInfo,sizeof(SYSTEM_INFO));

	EXIT_WHEN_DPC_WITH_NO_RETURN();

	Status = ZwQuerySystemInformation(
				SystemBasicInformation,
				&BasicInfo,
				sizeof(BasicInfo),
				NULL
				);
	if (!NT_SUCCESS(Status)) {
		return;
	}

    Status = ZwQuerySystemInformation(
                SystemProcessorInformation,
                &ProcessorInfo,
                sizeof(ProcessorInfo),
                NULL
                );
	if (!NT_SUCCESS(Status)) {
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
        lpSystemInfo->dwProcessorType = 604;  // backward compatibility
	} else if (ProcessorInfo.ProcessorArchitecture == PROCESSOR_ARCHITECTURE_IA64) {
		lpSystemInfo->dwProcessorType = PROCESSOR_INTEL_IA64;
	} else {
		lpSystemInfo->dwProcessorType = 0;
	}

	lpSystemInfo->dwAllocationGranularity = BasicInfo.AllocationGranularity;
}



__drv_preferredFunction("GetTickCount64", "GetTickCount overflows roughly every 49 days.  Code that does not take that into account can loop indefinitely.  GetTickCount64 operates on 64 bit values and does not have that problem")
WINBASEAPI
DWORD
WINAPI
GetTickCount(
    VOID
    )
{
	LARGE_INTEGER TickCount;
	KeQueryTickCount(&TickCount);
	return (DWORD)((TickCount.QuadPart * SharedUserData->TickCountMultiplier) >> 24);
}
#if (_WIN32_WINNT >= 0x0600)
WINBASEAPI
ULONGLONG
WINAPI
GetTickCount64(
    VOID
    )
{
	LARGE_INTEGER TickCount;
	KeQueryTickCount(&TickCount);
	return ((TickCount.QuadPart * SharedUserData->TickCountMultiplier) >> 24);
}
#endif



WINBASEAPI
DWORD
WINAPI
GetVersion(
    VOID
    )
{
    RTL_OSVERSIONINFOW vi;

    if (!NT_SUCCESS(RtlGetVersion(&vi))) {
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
GetVersionExA(
    _Inout_ LPOSVERSIONINFOA lpVersionInformation
    )
{
	STRING CSDVersionA;
	UNICODE_STRING CSDVersionW;
	OSVERSIONINFOEXW VersionInformationU;

	if (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXA) &&
		lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA)) {

		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return FALSE;
	}

	VersionInformationU.dwOSVersionInfoSize = sizeof(VersionInformationU);

	if (GetVersionExW((LPOSVERSIONINFOW)&VersionInformationU)) {

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

			RtlInitUnicodeString(&CSDVersionW, VersionInformationU.szCSDVersion);

			return NT_SUCCESS(LdkUnicodeStringToAnsiString(&CSDVersionA, &CSDVersionW, FALSE));
		}

		return TRUE;
	}

	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetVersionExW(
    _Inout_ LPOSVERSIONINFOW lpVersionInformation
    )
{
    if (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOEXW) &&
		lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOW)) {

        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return FALSE;
	}
	
	RtlZeroMemory(lpVersionInformation, lpVersionInformation->dwOSVersionInfoSize);

	return NT_SUCCESS(RtlGetVersion(lpVersionInformation));
}
