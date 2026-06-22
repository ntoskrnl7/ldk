#if _KERNEL_MODE
#include <Ldk/Windows.h>
#else
#include <windows.h>
#include <stdio.h>
#define PAGED_CODE()
#define DbgPrintEx(_id_, _level_, ...) printf(__VA_ARGS__)
#define DPFLTR_IHVDRIVER_ID 0
#define DPFLTR_INFO_LEVEL 0
#define DPFLTR_ERROR_LEVEL 1
#endif

BOOLEAN
Kernel32RuntimeTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Kernel32RuntimeTest)
#endif

#define printf(...)         DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)
#define fprintf(_f_, ...)   DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__)
#define stderr              DPFLTR_ERROR_LEVEL

static
BOOLEAN
RuntimeExpect (
    _In_ BOOL Condition,
    _In_ PCSTR Message
    )
{
    if (! Condition) {
        fprintf(stderr, "[Failed] %s (LastError = %lu)\n", Message, GetLastError());
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
RuntimeIsFilled (
    _In_reads_bytes_(Length) const UCHAR *Buffer,
    _In_ SIZE_T Length,
    _In_ UCHAR Value
    )
{
    for (SIZE_T Index = 0; Index < Length; Index++) {
        if (Buffer[Index] != Value) {
            return FALSE;
        }
    }

    return TRUE;
}

static
LONG
WINAPI
RuntimeExceptionFilter (
    _In_ struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    UNREFERENCED_PARAMETER(ExceptionInfo);
    return EXCEPTION_CONTINUE_SEARCH;
}

static
BOOLEAN
RuntimeHeapTest (
    VOID
    )
{
    HANDLE Heap;
    UCHAR *Bytes;
    BOOL Result = TRUE;

    PAGED_CODE();

    Result &= RuntimeExpect( GetProcessHeap() != NULL,
                             "GetProcessHeap" );

    Heap = HeapCreate( 0,
                       0,
                       0 );
    Result &= RuntimeExpect( Heap != NULL,
                             "HeapCreate" );
    if (Heap == NULL) {
        return FALSE;
    }

    Bytes = HeapAlloc( Heap,
                       HEAP_ZERO_MEMORY,
                       32 );
    Result &= RuntimeExpect( Bytes != NULL,
                             "HeapAlloc(HEAP_ZERO_MEMORY)" );
    Result &= RuntimeExpect( RuntimeIsFilled( Bytes,
                                              32,
                                              0 ),
                             "HeapAlloc zeroes memory" );
    Result &= RuntimeExpect( HeapSize( Heap,
                                       0,
                                       Bytes ) >= 32,
                             "HeapSize(allocated block)" );
    Result &= RuntimeExpect( HeapValidate( Heap,
                                           0,
                                           Bytes ),
                             "HeapValidate(allocated block)" );

    if (Bytes != NULL) {
        for (SIZE_T Index = 0; Index < 32; Index++) {
            Bytes[Index] = 0x5a;
        }

        Bytes = HeapReAlloc( Heap,
                             HEAP_ZERO_MEMORY,
                             Bytes,
                             64 );
        Result &= RuntimeExpect( Bytes != NULL,
                                 "HeapReAlloc(HEAP_ZERO_MEMORY)" );
        Result &= RuntimeExpect( RuntimeIsFilled( Bytes,
                                                  32,
                                                  0x5a ),
                                 "HeapReAlloc preserves memory" );
        Result &= RuntimeExpect( RuntimeIsFilled( Bytes + 32,
                                                  32,
                                                  0 ),
                                 "HeapReAlloc zeroes new memory" );
        Result &= RuntimeExpect( HeapFree( Heap,
                                           0,
                                           Bytes ),
                                 "HeapFree" );
    }

    Result &= RuntimeExpect( HeapDestroy( Heap ),
                             "HeapDestroy" );

    return Result ? TRUE : FALSE;
}

static
BOOLEAN
RuntimeProfileAndUtilTest (
    VOID
    )
{
    LARGE_INTEGER Frequency;
    LARGE_INTEGER First;
    LARGE_INTEGER Second;
    PVOID Pointer = &Frequency;
    PVOID Encoded;
    BOOL Result = TRUE;

    PAGED_CODE();

    Result &= RuntimeExpect( QueryPerformanceFrequency( &Frequency ) &&
                             Frequency.QuadPart > 0,
                             "QueryPerformanceFrequency" );
    Result &= RuntimeExpect( QueryPerformanceCounter( &First ),
                             "QueryPerformanceCounter(first)" );
    Result &= RuntimeExpect( QueryPerformanceCounter( &Second ) &&
                             Second.QuadPart >= First.QuadPart,
                             "QueryPerformanceCounter(second)" );

    Result &= RuntimeExpect( EncodePointer( NULL ) == NULL &&
                             DecodePointer( NULL ) == NULL,
                             "EncodePointer/DecodePointer(NULL)" );

    Encoded = EncodePointer( Pointer );
    Result &= RuntimeExpect( DecodePointer( Encoded ) == Pointer,
                             "DecodePointer(EncodePointer(pointer))" );

    return Result ? TRUE : FALSE;
}

static
BOOLEAN
RuntimeErrorTest (
    VOID
    )
{
    LPTOP_LEVEL_EXCEPTION_FILTER PreviousFilter;
    LPTOP_LEVEL_EXCEPTION_FILTER RestoredFilter;
    BOOL Result = TRUE;

    PAGED_CODE();

    SetLastError( 0x1234 );
    Result &= RuntimeExpect( GetLastError() == 0x1234,
                             "SetLastError/GetLastError" );

    PreviousFilter = SetUnhandledExceptionFilter( RuntimeExceptionFilter );
    RestoredFilter = SetUnhandledExceptionFilter( PreviousFilter );
    Result &= RuntimeExpect( RestoredFilter == RuntimeExceptionFilter,
                             "SetUnhandledExceptionFilter returns previous filter" );

    return Result ? TRUE : FALSE;
}

static
BOOLEAN
RuntimeSystemInfoTest (
    VOID
    )
{
    SYSTEM_INFO SystemInfo;
    SYSTEM_INFO NativeSystemInfo;
    MEMORYSTATUSEX MemoryStatus;
    OSVERSIONINFOEXW VersionInfoW;
    OSVERSIONINFOEXA VersionInfoA;
    DWORD Version;
    BOOL Result = TRUE;

    PAGED_CODE();

    RtlZeroMemory( &SystemInfo,
                   sizeof(SystemInfo) );
    GetSystemInfo( &SystemInfo );
    Result &= RuntimeExpect( SystemInfo.dwPageSize != 0 &&
                             SystemInfo.dwNumberOfProcessors != 0 &&
                             SystemInfo.lpMinimumApplicationAddress < SystemInfo.lpMaximumApplicationAddress,
                             "GetSystemInfo" );

    RtlZeroMemory( &NativeSystemInfo,
                   sizeof(NativeSystemInfo) );
    GetNativeSystemInfo( &NativeSystemInfo );
    Result &= RuntimeExpect( NativeSystemInfo.dwPageSize == SystemInfo.dwPageSize &&
                             NativeSystemInfo.dwNumberOfProcessors == SystemInfo.dwNumberOfProcessors,
                             "GetNativeSystemInfo" );

    RtlZeroMemory( &MemoryStatus,
                   sizeof(MemoryStatus) );
    MemoryStatus.dwLength = sizeof(MemoryStatus);
    Result &= RuntimeExpect( GlobalMemoryStatusEx( &MemoryStatus ),
                             "GlobalMemoryStatusEx" );
    Result &= RuntimeExpect( MemoryStatus.dwMemoryLoad <= 100 &&
                             MemoryStatus.ullTotalPhys >= MemoryStatus.ullAvailPhys,
                             "GlobalMemoryStatusEx fields" );

    RtlZeroMemory( &MemoryStatus,
                   sizeof(MemoryStatus) );
    SetLastError( ERROR_SUCCESS );
    Result &= RuntimeExpect( ! GlobalMemoryStatusEx( &MemoryStatus ) &&
                             GetLastError() == ERROR_INVALID_PARAMETER,
                             "GlobalMemoryStatusEx rejects missing length" );

    RtlZeroMemory( &VersionInfoW,
                   sizeof(VersionInfoW) );
    VersionInfoW.dwOSVersionInfoSize = sizeof(VersionInfoW);
    Result &= RuntimeExpect( GetVersionExW( (LPOSVERSIONINFOW)&VersionInfoW ) &&
                             VersionInfoW.dwMajorVersion != 0,
                             "GetVersionExW" );

    RtlZeroMemory( &VersionInfoA,
                   sizeof(VersionInfoA) );
    VersionInfoA.dwOSVersionInfoSize = sizeof(VersionInfoA);
    Result &= RuntimeExpect( GetVersionExA( (LPOSVERSIONINFOA)&VersionInfoA ) &&
                             VersionInfoA.dwMajorVersion == VersionInfoW.dwMajorVersion &&
                             VersionInfoA.dwMinorVersion == VersionInfoW.dwMinorVersion,
                             "GetVersionExA" );

    Version = GetVersion();
    Result &= RuntimeExpect( (Version & 0xff) != 0,
                             "GetVersion" );

    return Result ? TRUE : FALSE;
}

BOOLEAN
Kernel32RuntimeTest (
    VOID
    )
{
    BOOL Result = TRUE;

    PAGED_CODE();

    printf("Kernel32 Runtime Test\n");

    Result &= RuntimeHeapTest();
    Result &= RuntimeProfileAndUtilTest();
    Result &= RuntimeErrorTest();
    Result &= RuntimeSystemInfoTest();

    if (Result) {
        printf("[Success] Kernel32 Runtime Test\n\n");
    } else {
        printf("[Failed] Kernel32 Runtime Test\n\n");
    }

    return Result ? TRUE : FALSE;
}
