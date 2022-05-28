#pragma once

#ifndef _TIMEZONEAPI_H_
#define _TIMEZONEAPI_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "minwinbase.h"

EXTERN_C_START

#define TIME_ZONE_ID_INVALID ((DWORD)0xFFFFFFFF)

typedef struct _TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

typedef struct _TIME_DYNAMIC_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
    WCHAR TimeZoneKeyName[ 128 ];
    BOOLEAN DynamicDaylightTimeDisabled;
} DYNAMIC_TIME_ZONE_INFORMATION, *PDYNAMIC_TIME_ZONE_INFORMATION;

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
SystemTimeToTzSpecificLocalTime(
    _In_opt_ CONST TIME_ZONE_INFORMATION* lpTimeZoneInformation,
    _In_ CONST SYSTEMTIME* lpUniversalTime,
    _Out_ LPSYSTEMTIME lpLocalTime
    );



WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
FileTimeToSystemTime(
    _In_ CONST FILETIME* lpFileTime,
    _Out_ LPSYSTEMTIME lpSystemTime
    );

WINBASEAPI
_Success_(return != FALSE)
BOOL
WINAPI
SystemTimeToFileTime(
    _In_ CONST SYSTEMTIME* lpSystemTime,
    _Out_ LPFILETIME lpFileTime
    );

WINBASEAPI
_Success_(return != TIME_ZONE_ID_INVALID)
DWORD
WINAPI
GetTimeZoneInformation(
    _Out_ LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    );

EXTERN_C_END

#endif // _TIMEZONEAPI_H_