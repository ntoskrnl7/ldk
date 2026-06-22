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
Kernel32TimeTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Kernel32TimeTest)
#endif

#define printf(...)         DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)
#define fprintf(_f_, ...)   DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__)
#define stderr              DPFLTR_ERROR_LEVEL

static
BOOLEAN
TimeExpect (
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
TimeSameCalendarFields (
    _In_ const SYSTEMTIME *Left,
    _In_ const SYSTEMTIME *Right
    )
{
    return Left->wYear == Right->wYear &&
           Left->wMonth == Right->wMonth &&
           Left->wDay == Right->wDay &&
           Left->wHour == Right->wHour &&
           Left->wMinute == Right->wMinute &&
           Left->wSecond == Right->wSecond &&
           Left->wMilliseconds == Right->wMilliseconds;
}

BOOLEAN
Kernel32TimeTest (
    VOID
    )
{
    SYSTEMTIME SystemTime;
    SYSTEMTIME RoundTripTime;
    SYSTEMTIME LocalTime;
    SYSTEMTIME CurrentLocalTime;
    FILETIME FileTime;
    TIME_ZONE_INFORMATION TimeZoneInformation;
    BOOL Result = TRUE;

    PAGED_CODE();

    printf("Kernel32 Time Test\n");

    RtlZeroMemory( &SystemTime,
                   sizeof(SystemTime) );
    SystemTime.wYear = 2024;
    SystemTime.wMonth = 2;
    SystemTime.wDay = 3;
    SystemTime.wHour = 4;
    SystemTime.wMinute = 5;
    SystemTime.wSecond = 6;
    SystemTime.wMilliseconds = 7;

    Result &= TimeExpect( SystemTimeToFileTime( &SystemTime,
                                                &FileTime ),
                          "SystemTimeToFileTime(valid time)" );
    Result &= TimeExpect( FileTimeToSystemTime( &FileTime,
                                                &RoundTripTime ),
                          "FileTimeToSystemTime(valid file time)" );
    Result &= TimeExpect( TimeSameCalendarFields( &SystemTime,
                                                  &RoundTripTime ),
                          "SystemTime/FileTime round-trip" );

    RtlZeroMemory( &TimeZoneInformation,
                   sizeof(TimeZoneInformation) );
    TimeZoneInformation.Bias = -60;

    RtlZeroMemory( &LocalTime,
                   sizeof(LocalTime) );
    Result &= TimeExpect( SystemTimeToTzSpecificLocalTime( &TimeZoneInformation,
                                                           &SystemTime,
                                                           &LocalTime ),
                          "SystemTimeToTzSpecificLocalTime(fixed bias)" );
    Result &= TimeExpect( LocalTime.wYear == 2024 &&
                          LocalTime.wMonth == 2 &&
                          LocalTime.wDay == 3 &&
                          LocalTime.wHour == 5 &&
                          LocalTime.wMinute == 5,
                          "SystemTimeToTzSpecificLocalTime applies bias" );

    RtlZeroMemory( &SystemTime,
                   sizeof(SystemTime) );
    SystemTime.wYear = 2024;
    SystemTime.wMonth = 13;
    SystemTime.wDay = 1;
    SetLastError( ERROR_SUCCESS );
    Result &= TimeExpect( ! SystemTimeToFileTime( &SystemTime,
                                                  &FileTime ),
                          "SystemTimeToFileTime(invalid time)" );

    GetLocalTime( &CurrentLocalTime );
    Result &= TimeExpect( CurrentLocalTime.wYear >= 2020 &&
                          CurrentLocalTime.wMonth >= 1 &&
                          CurrentLocalTime.wMonth <= 12 &&
                          CurrentLocalTime.wDay >= 1 &&
                          CurrentLocalTime.wDay <= 31,
                          "GetLocalTime returns plausible calendar fields" );

    if (Result) {
        printf("[Success] Kernel32 Time Test\n\n");
    } else {
        printf("[Failed] Kernel32 Time Test\n\n");
    }

    return Result ? TRUE : FALSE;
}
