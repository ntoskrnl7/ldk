#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, GetTimeZoneInformation)
#pragma alloc_text(PAGE, SystemTimeToTzSpecificLocalTime)
#endif



WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
FileTimeToSystemTime (
    _In_ CONST FILETIME * lpFileTime,
    _Out_ LPSYSTEMTIME lpSystemTime
    )
{
	LARGE_INTEGER FileTime;
	TIME_FIELDS TimeFields;

	FileTime.LowPart = lpFileTime->dwLowDateTime;
	FileTime.HighPart = lpFileTime->dwHighDateTime;

	if (FileTime.QuadPart < 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	RtlTimeToTimeFields( &FileTime,
                         &TimeFields );
	lpSystemTime->wYear = TimeFields.Year;
	lpSystemTime->wMonth = TimeFields.Month;
	lpSystemTime->wDay = TimeFields.Day;
	lpSystemTime->wDayOfWeek = TimeFields.Weekday;
	lpSystemTime->wHour = TimeFields.Hour;
	lpSystemTime->wMinute = TimeFields.Minute;
	lpSystemTime->wSecond = TimeFields.Second;
	lpSystemTime->wMilliseconds = TimeFields.Milliseconds;
	return TRUE;
}

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
SystemTimeToFileTime (
    _In_ CONST SYSTEMTIME *lpSystemTime,
    _Out_ LPFILETIME lpFileTime
    )
{
	TIME_FIELDS TimeFields;
	LARGE_INTEGER FileTime;

	TimeFields.Year = lpSystemTime->wYear;
	TimeFields.Month = lpSystemTime->wMonth;
	TimeFields.Day = lpSystemTime->wDay;
	TimeFields.Hour = lpSystemTime->wHour;
	TimeFields.Minute = lpSystemTime->wMinute;
	TimeFields.Second = lpSystemTime->wSecond;
	TimeFields.Milliseconds = lpSystemTime->wMilliseconds;

	if (! RtlTimeFieldsToTime( &TimeFields,
                               &FileTime )) {
		LdkSetLastNTError( STATUS_INVALID_PARAMETER );
		return FALSE;
	} else {
		lpFileTime->dwLowDateTime = FileTime.LowPart;
		lpFileTime->dwHighDateTime = FileTime.HighPart;
		return TRUE;
	}
}

WINBASEAPI
_Success_(return != TIME_ZONE_ID_INVALID)
DWORD
WINAPI
GetTimeZoneInformation (
    _Out_ LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    )
{
	NTSTATUS Status;
	RTL_TIME_ZONE_INFORMATION TimeZoneInfo;

    PAGED_CODE();

	Status = ZwQuerySystemInformation( SystemCurrentTimeZoneInformation,
                                       (PVOID)&TimeZoneInfo,
                                       sizeof(TimeZoneInfo),
                                       NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return TIME_ZONE_ID_INVALID;
	}

	lpTimeZoneInformation->Bias = TimeZoneInfo.Bias;
	lpTimeZoneInformation->StandardBias = TimeZoneInfo.StandardBias;
	lpTimeZoneInformation->DaylightBias = TimeZoneInfo.DaylightBias;
	RtlMoveMemory( &lpTimeZoneInformation->StandardName,
                   &TimeZoneInfo.StandardName,
                   sizeof(TimeZoneInfo.StandardName) );
	RtlMoveMemory( &lpTimeZoneInformation->DaylightName,
                   &TimeZoneInfo.DaylightName,
                   sizeof(TimeZoneInfo.DaylightName) );
	lpTimeZoneInformation->DaylightDate.wYear = TimeZoneInfo.DaylightStart.Year;
	lpTimeZoneInformation->DaylightDate.wMonth = TimeZoneInfo.DaylightStart.Month;
	lpTimeZoneInformation->DaylightDate.wDayOfWeek = TimeZoneInfo.DaylightStart.Weekday;
	lpTimeZoneInformation->DaylightDate.wDay = TimeZoneInfo.DaylightStart.Day;
	lpTimeZoneInformation->DaylightDate.wHour = TimeZoneInfo.DaylightStart.Hour;
	lpTimeZoneInformation->DaylightDate.wMinute = TimeZoneInfo.DaylightStart.Minute;
	lpTimeZoneInformation->DaylightDate.wSecond = TimeZoneInfo.DaylightStart.Second;
	lpTimeZoneInformation->DaylightDate.wMilliseconds = TimeZoneInfo.DaylightStart.Milliseconds;
	lpTimeZoneInformation->StandardDate.wYear = TimeZoneInfo.StandardStart.Year;
	lpTimeZoneInformation->StandardDate.wMonth = TimeZoneInfo.StandardStart.Month;
	lpTimeZoneInformation->StandardDate.wDayOfWeek = TimeZoneInfo.StandardStart.Weekday;
	lpTimeZoneInformation->StandardDate.wDay = TimeZoneInfo.StandardStart.Day;
	lpTimeZoneInformation->StandardDate.wHour = TimeZoneInfo.StandardStart.Hour;
	lpTimeZoneInformation->StandardDate.wMinute = TimeZoneInfo.StandardStart.Minute;
	lpTimeZoneInformation->StandardDate.wSecond = TimeZoneInfo.StandardStart.Second;
	lpTimeZoneInformation->StandardDate.wMilliseconds = TimeZoneInfo.StandardStart.Milliseconds;
	return SharedUserData->TimeZoneId;
}



WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
SystemTimeToTzSpecificLocalTime (
    _In_opt_ CONST TIME_ZONE_INFORMATION* lpTimeZoneInformation,
    _In_ CONST SYSTEMTIME* lpUniversalTime,
    _Out_ LPSYSTEMTIME lpLocalTime
    )
{
    TIME_ZONE_INFORMATION TziData;
    LPTIME_ZONE_INFORMATION Tzi;
    RTL_TIME_ZONE_INFORMATION tzi;
    LARGE_INTEGER TimeZoneBias;
    LARGE_INTEGER NewTimeZoneBias;
    LARGE_INTEGER LocalCustomBias;
    LARGE_INTEGER StandardTime;
    LARGE_INTEGER DaylightTime;
    LARGE_INTEGER UtcStandardTime;
    LARGE_INTEGER UtcDaylightTime;
    LARGE_INTEGER CurrentUniversalTime;
    LARGE_INTEGER ComputedLocalTime;
    ULONG CurrentTimeZoneId = 0xffffffff;

    PAGED_CODE();

    if (! ARGUMENT_PRESENT(lpTimeZoneInformation) ) {
        if (GetTimeZoneInformation( &TziData ) == TIME_ZONE_ID_INVALID) {
            return FALSE;
        }
        Tzi = &TziData;
    } else {
        Tzi = (TIME_ZONE_INFORMATION *)lpTimeZoneInformation;
    }

    tzi.Bias = Tzi->Bias;
    tzi.StandardBias = Tzi->StandardBias;
    tzi.DaylightBias = Tzi->DaylightBias;
    RtlMoveMemory( &tzi.StandardName,
                   &Tzi->StandardName,
                   sizeof(tzi.StandardName) );
    RtlMoveMemory( &tzi.DaylightName,
                   &Tzi->DaylightName,
                   sizeof(tzi.DaylightName) );
    tzi.StandardStart.Year = Tzi->StandardDate.wYear;
    tzi.StandardStart.Month = Tzi->StandardDate.wMonth;
    tzi.StandardStart.Weekday = Tzi->StandardDate.wDayOfWeek;
    tzi.StandardStart.Day = Tzi->StandardDate.wDay;
    tzi.StandardStart.Hour = Tzi->StandardDate.wHour;
    tzi.StandardStart.Minute = Tzi->StandardDate.wMinute;
    tzi.StandardStart.Second = Tzi->StandardDate.wSecond;
    tzi.StandardStart.Milliseconds = Tzi->StandardDate.wMilliseconds;

    tzi.DaylightStart.Year = Tzi->DaylightDate.wYear;
    tzi.DaylightStart.Month = Tzi->DaylightDate.wMonth;
    tzi.DaylightStart.Weekday = Tzi->DaylightDate.wDayOfWeek;
    tzi.DaylightStart.Day = Tzi->DaylightDate.wDay;
    tzi.DaylightStart.Hour = Tzi->DaylightDate.wHour;
    tzi.DaylightStart.Minute = Tzi->DaylightDate.wMinute;
    tzi.DaylightStart.Second = Tzi->DaylightDate.wSecond;
    tzi.DaylightStart.Milliseconds = Tzi->DaylightDate.wMilliseconds;

    if (! SystemTimeToFileTime( lpUniversalTime,
                                (LPFILETIME)&CurrentUniversalTime )) {
        return FALSE;
    }

    NewTimeZoneBias.QuadPart = Int32x32To64(tzi.Bias*60, 10000000);

    if (tzi.StandardStart.Month && tzi.DaylightStart.Month) {
        if (! RtlCutoverTimeToSystemTime( &tzi.StandardStart,
                                          &StandardTime,
                                          &CurrentUniversalTime,
                                          TRUE )) {
            LdkSetLastNTError( STATUS_INVALID_PARAMETER );
            return FALSE;
        }

        if (! RtlCutoverTimeToSystemTime( &tzi.DaylightStart,
                                          &DaylightTime,
                                          &CurrentUniversalTime,
                                          TRUE )) {
            LdkSetLastNTError( STATUS_INVALID_PARAMETER );
            return FALSE;
        }

        LocalCustomBias.QuadPart = Int32x32To64( tzi.StandardBias * 60, 10000000 );
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
        UtcDaylightTime.QuadPart = DaylightTime.QuadPart + TimeZoneBias.QuadPart;
        LocalCustomBias.QuadPart = Int32x32To64( tzi.DaylightBias * 60, 10000000 );
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
        UtcStandardTime.QuadPart = StandardTime.QuadPart + TimeZoneBias.QuadPart;
        if (UtcDaylightTime.QuadPart < UtcStandardTime.QuadPart) {
            if ((CurrentUniversalTime.QuadPart >= UtcDaylightTime.QuadPart) && (CurrentUniversalTime.QuadPart < UtcStandardTime.QuadPart)) {
                CurrentTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
            } else {
                CurrentTimeZoneId = TIME_ZONE_ID_STANDARD;
            }
        } else {
            if ((CurrentUniversalTime.QuadPart >= UtcStandardTime.QuadPart) && (CurrentUniversalTime.QuadPart < UtcDaylightTime.QuadPart)) {
                CurrentTimeZoneId = TIME_ZONE_ID_STANDARD;
            } else {
                CurrentTimeZoneId = TIME_ZONE_ID_DAYLIGHT;
            }
        }
        LocalCustomBias.QuadPart = Int32x32To64( (CurrentTimeZoneId == TIME_ZONE_ID_DAYLIGHT ? tzi.DaylightBias : tzi.StandardBias) * 60,
                                                 10000000 );
        TimeZoneBias.QuadPart = NewTimeZoneBias.QuadPart + LocalCustomBias.QuadPart;
    } else {
        TimeZoneBias = NewTimeZoneBias;
    }
    ComputedLocalTime.QuadPart = CurrentUniversalTime.QuadPart - TimeZoneBias.QuadPart;

    return FileTimeToSystemTime( (LPFILETIME)&ComputedLocalTime,lpLocalTime );
}
