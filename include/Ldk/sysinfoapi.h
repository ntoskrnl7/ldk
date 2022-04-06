#pragma once

EXTERN_C_START

#pragma warning(disable:4201)
typedef struct _SYSTEM_INFO {
    union {
        DWORD dwOemId;          // Obsolete field...do not use
        struct {
            WORD wProcessorArchitecture;
            WORD wReserved;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
    DWORD dwPageSize;
    LPVOID lpMinimumApplicationAddress;
    LPVOID lpMaximumApplicationAddress;
    DWORD_PTR dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD wProcessorLevel;
    WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;
#pragma warning(default:4201)

WINBASEAPI
VOID
WINAPI
GetSystemInfo(
    _Out_ LPSYSTEM_INFO lpSystemInfo
    );

WINBASEAPI
VOID
WINAPI
GetNativeSystemInfo(
    _Out_ LPSYSTEM_INFO lpSystemInfo
    );



WINBASEAPI
DWORD
WINAPI
GetVersion(
    VOID
    );


typedef struct _MEMORYSTATUSEX {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORDLONG ullTotalPhys;
    DWORDLONG ullAvailPhys;
    DWORDLONG ullTotalPageFile;
    DWORDLONG ullAvailPageFile;
    DWORDLONG ullTotalVirtual;
    DWORDLONG ullAvailVirtual;
    DWORDLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

WINBASEAPI
BOOL
WINAPI
GlobalMemoryStatusEx(
    _Inout_ LPMEMORYSTATUSEX lpBuffer
    );



WINBASEAPI
VOID
WINAPI
GetSystemTime(
    _Out_ LPSYSTEMTIME lpSystemTime
    );

#if (NTDDI_VERSION >= NTDDI_WIN8)
WINBASEAPI
VOID
WINAPI
GetSystemTimePreciseAsFileTime(
    _Out_ LPFILETIME lpSystemTimeAsFileTime
    );
#endif

WINBASEAPI
VOID
WINAPI
GetSystemTimeAsFileTime(
    _Out_ LPFILETIME lpSystemTimeAsFileTime
    );

WINBASEAPI
VOID
WINAPI
GetLocalTime(
    _Out_ LPSYSTEMTIME lpSystemTime
    );



__drv_preferredFunction("GetTickCount64", "GetTickCount overflows roughly every 49 days.  Code that does not take that into account can loop indefinitely.  GetTickCount64 operates on 64 bit values and does not have that problem")
WINBASEAPI
DWORD
WINAPI
GetTickCount(
    VOID
    );

#if (_WIN32_WINNT >= 0x0600)

WINBASEAPI
ULONGLONG
WINAPI
GetTickCount64(
    VOID
    );

#endif



WINBASEAPI
BOOL
WINAPI
GetVersionExA(
    _Inout_ LPOSVERSIONINFOA lpVersionInformation
    );

WINBASEAPI
BOOL
WINAPI
GetVersionExW(
    _Inout_ LPOSVERSIONINFOW lpVersionInformation
    );

EXTERN_C_END