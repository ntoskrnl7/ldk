#include "../ldk.h"

typedef char UBool;
typedef WCHAR UChar;
typedef double UDate;
typedef INT32 UErrorCode;

int LdkpIcuFltused = 0;
#if defined(_M_IX86)
#pragma comment(linker, "/alternatename:__fltused=_LdkpIcuFltused")
#else
#pragma comment(linker, "/alternatename:_fltused=LdkpIcuFltused")
#endif

#define U_ZERO_ERROR 0
#define U_BUFFER_OVERFLOW_ERROR 15

#define UCAL_ZONE_OFFSET 15
#define UCAL_DST_OFFSET 16

typedef enum _LDK_UCALENDAR_TYPE {
    UCAL_TRADITIONAL,
    UCAL_DEFAULT = UCAL_TRADITIONAL
} LDK_UCALENDAR_TYPE;

typedef enum _LDK_USYSTEM_TIME_ZONE_TYPE {
    UCAL_ZONE_TYPE_ANY,
    UCAL_ZONE_TYPE_CANONICAL,
    UCAL_ZONE_TYPE_CANONICAL_LOCATION
} LDK_USYSTEM_TIME_ZONE_TYPE;

typedef enum _LDK_UCALENDAR_DISPLAY_NAME_TYPE {
    UCAL_STANDARD,
    UCAL_SHORT_STANDARD,
    UCAL_DST,
    UCAL_SHORT_DST
} LDK_UCALENDAR_DISPLAY_NAME_TYPE;

typedef enum _LDK_UTIME_ZONE_TRANSITION_TYPE {
    UCAL_TZ_TRANSITION_NEXT,
    UCAL_TZ_TRANSITION_NEXT_INCLUSIVE,
    UCAL_TZ_TRANSITION_PREVIOUS,
    UCAL_TZ_TRANSITION_PREVIOUS_INCLUSIVE
} LDK_UTIME_ZONE_TRANSITION_TYPE;

typedef struct _LDK_ICU_TIME_ZONE {
    const UChar* Id;
    const UChar* CanonicalId;
    const UChar* StandardName;
    const UChar* ShortStandardName;
    INT32 RawOffsetMilliseconds;
} LDK_ICU_TIME_ZONE, *PLDK_ICU_TIME_ZONE;

typedef struct _LDK_ICU_CALENDAR {
    const LDK_ICU_TIME_ZONE* TimeZone;
} LDK_ICU_CALENDAR, *PLDK_ICU_CALENDAR;

typedef struct _LDK_ICU_ENUMERATION {
    INT32 Index;
} LDK_ICU_ENUMERATION, *PLDK_ICU_ENUMERATION;

static const UChar LdkpIcuUtcAlias[] = L"UTC";
static const UChar LdkpIcuGmtAlias[] = L"GMT";
static const UChar LdkpIcuUtcZone[] = L"Etc/UTC";
static const UChar LdkpIcuUtcDisplayName[] = L"UTC";
static const UChar LdkpIcuSeoulZone[] = L"Asia/Seoul";
static const UChar LdkpIcuSeoulDisplayName[] = L"Korea Standard Time";
static const UChar LdkpIcuSeoulShortName[] = L"KST";
static const UChar LdkpIcuTokyoZone[] = L"Asia/Tokyo";
static const UChar LdkpIcuTokyoDisplayName[] = L"Tokyo Standard Time";
static const UChar LdkpIcuTokyoShortName[] = L"JST";
static const UChar LdkpIcuShanghaiZone[] = L"Asia/Shanghai";
static const UChar LdkpIcuShanghaiDisplayName[] = L"China Standard Time";
static const UChar LdkpIcuShanghaiShortName[] = L"CST";
static const UChar LdkpIcuNewYorkZone[] = L"America/New_York";
static const UChar LdkpIcuNewYorkDisplayName[] = L"Eastern Standard Time";
static const UChar LdkpIcuNewYorkShortName[] = L"EST";
static const UChar LdkpIcuBerlinZone[] = L"Europe/Berlin";
static const UChar LdkpIcuBerlinDisplayName[] = L"Central European Standard Time";
static const UChar LdkpIcuBerlinShortName[] = L"CET";
static const UChar LdkpIcuMoscowZone[] = L"Europe/Moscow";
static const UChar LdkpIcuMoscowDisplayName[] = L"Russian Standard Time";
static const UChar LdkpIcuMoscowShortName[] = L"MSK";
static const CHAR LdkpIcuTzDataVersion[] = "ldk-minimal-iana-subset";

/* MSVC STL builds std::chrono::tzdb vectors from this enumeration and then
 * uses lower_bound() for lookup, so the exported IDs must stay sorted. */
static const LDK_ICU_TIME_ZONE LdkpIcuTimeZones[] = {
    { LdkpIcuNewYorkZone, LdkpIcuNewYorkZone, LdkpIcuNewYorkDisplayName, LdkpIcuNewYorkShortName, -5 * 60 * 60 * 1000 },
    { LdkpIcuSeoulZone, LdkpIcuSeoulZone, LdkpIcuSeoulDisplayName, LdkpIcuSeoulShortName, 9 * 60 * 60 * 1000 },
    { LdkpIcuShanghaiZone, LdkpIcuShanghaiZone, LdkpIcuShanghaiDisplayName, LdkpIcuShanghaiShortName, 8 * 60 * 60 * 1000 },
    { LdkpIcuTokyoZone, LdkpIcuTokyoZone, LdkpIcuTokyoDisplayName, LdkpIcuTokyoShortName, 9 * 60 * 60 * 1000 },
    { LdkpIcuUtcZone, LdkpIcuUtcZone, LdkpIcuUtcDisplayName, LdkpIcuUtcDisplayName, 0 },
    { LdkpIcuBerlinZone, LdkpIcuBerlinZone, LdkpIcuBerlinDisplayName, LdkpIcuBerlinShortName, 60 * 60 * 1000 },
    { LdkpIcuMoscowZone, LdkpIcuMoscowZone, LdkpIcuMoscowDisplayName, LdkpIcuMoscowShortName, 3 * 60 * 60 * 1000 },
    { LdkpIcuGmtAlias, LdkpIcuUtcZone, LdkpIcuUtcDisplayName, LdkpIcuUtcDisplayName, 0 },
    { LdkpIcuUtcAlias, LdkpIcuUtcZone, LdkpIcuUtcDisplayName, LdkpIcuUtcDisplayName, 0 },
};

static
VOID
LdkpIcuSetStatus(
    _Out_opt_ UErrorCode* Status,
    _In_ UErrorCode Value
    )
{
    if (ARGUMENT_PRESENT(Status)) {
        *Status = Value;
    }
}

static
INT32
LdkpIcuStringLength(
    _In_z_ const UChar* Value
    )
{
    return (INT32)wcslen(Value);
}

static
INT32
LdkpIcuCopyString(
    _In_z_ const UChar* Source,
    _Out_writes_opt_(DestinationCapacity) UChar* Destination,
    _In_ INT32 DestinationCapacity,
    _Out_opt_ UErrorCode* Status
    )
{
    INT32 Length = LdkpIcuStringLength(Source);

    if (DestinationCapacity <= Length) {
        if (Destination != NULL && DestinationCapacity > 0) {
            RtlCopyMemory(Destination,
                          Source,
                          (SIZE_T)DestinationCapacity * sizeof(UChar));
        }
        LdkpIcuSetStatus(Status, U_BUFFER_OVERFLOW_ERROR);
        return Length;
    }

    if (Destination != NULL) {
        RtlCopyMemory(Destination,
                      Source,
                      ((SIZE_T)Length + 1) * sizeof(UChar));
    }
    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    return Length;
}

static
BOOLEAN
LdkpIcuEqualString(
    _In_z_ const UChar* Left,
    _In_reads_opt_(RightLength) const UChar* Right,
    _In_ INT32 RightLength
    )
{
    SIZE_T LeftLength;

    if (Right == NULL) {
        return FALSE;
    }

    LeftLength = wcslen(Left);
    if (RightLength < 0) {
        return wcscmp(Left, Right) == 0;
    }

    if (LeftLength != (SIZE_T)RightLength) {
        return FALSE;
    }

    return wcsncmp(Left, Right, (SIZE_T)RightLength) == 0;
}

static
const LDK_ICU_TIME_ZONE*
LdkpIcuFindTimeZone(
    _In_reads_opt_(IdLength) const UChar* Id,
    _In_ INT32 IdLength
    )
{
    for (SIZE_T Index = 0; Index < RTL_NUMBER_OF(LdkpIcuTimeZones); Index++) {
        if (LdkpIcuEqualString(LdkpIcuTimeZones[Index].Id, Id, IdLength)) {
            return &LdkpIcuTimeZones[Index];
        }
    }

    return NULL;
}

static
const LDK_ICU_TIME_ZONE*
LdkpIcuDefaultTimeZone(
    VOID
    )
{
    return &LdkpIcuTimeZones[0];
}

VOID
__cdecl
LdkpIcuUcalClose(
    _In_opt_ PLDK_ICU_CALENDAR Calendar
    )
{
    if (Calendar != NULL) {
        RtlFreeHeap(RtlProcessHeap(),
                    0,
                    Calendar);
    }
}

INT32
__cdecl
LdkpIcuUcalGet(
    _In_opt_ const LDK_ICU_CALENDAR* Calendar,
    _In_ INT32 Field,
    _Out_opt_ UErrorCode* Status
    )
{
    const LDK_ICU_TIME_ZONE* TimeZone;

    TimeZone = Calendar != NULL && Calendar->TimeZone != NULL
                   ? Calendar->TimeZone
                   : LdkpIcuDefaultTimeZone();

    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    if (Field == UCAL_ZONE_OFFSET) {
        return TimeZone->RawOffsetMilliseconds;
    }
    if (Field == UCAL_DST_OFFSET) {
        return 0;
    }
    return 0;
}

INT32
__cdecl
LdkpIcuUcalGetCanonicalTimeZoneID(
    _In_reads_opt_(IdLength) const UChar* Id,
    _In_ INT32 IdLength,
    _Out_writes_opt_(ResultCapacity) UChar* Result,
    _In_ INT32 ResultCapacity,
    _Out_opt_ UBool* IsSystemId,
    _Out_opt_ UErrorCode* Status
    )
{
    const LDK_ICU_TIME_ZONE* TimeZone;

    TimeZone = LdkpIcuFindTimeZone(Id, IdLength);
    if (TimeZone == NULL) {
        if (ARGUMENT_PRESENT(IsSystemId)) {
            *IsSystemId = FALSE;
        }
        return LdkpIcuCopyString(LdkpIcuUtcZone,
                                 Result,
                                 ResultCapacity,
                                 Status);
    }

    if (ARGUMENT_PRESENT(IsSystemId)) {
        *IsSystemId = TRUE;
    }
    return LdkpIcuCopyString(TimeZone->CanonicalId, Result, ResultCapacity, Status);
}

INT32
__cdecl
LdkpIcuUcalGetDefaultTimeZone(
    _Out_writes_opt_(ResultCapacity) UChar* Result,
    _In_ INT32 ResultCapacity,
    _Out_opt_ UErrorCode* Status
    )
{
    return LdkpIcuCopyString(LdkpIcuUtcZone, Result, ResultCapacity, Status);
}

INT32
__cdecl
LdkpIcuUcalGetTimeZoneDisplayName(
    _In_opt_ const LDK_ICU_CALENDAR* Calendar,
    _In_ INT32 Type,
    _In_opt_ const CHAR* Locale,
    _Out_writes_opt_(ResultLength) UChar* Result,
    _In_ INT32 ResultLength,
    _Out_opt_ UErrorCode* Status
    )
{
    const LDK_ICU_TIME_ZONE* TimeZone;
    const UChar* DisplayName;

    UNREFERENCED_PARAMETER(Locale);

    TimeZone = Calendar != NULL && Calendar->TimeZone != NULL
                   ? Calendar->TimeZone
                   : LdkpIcuDefaultTimeZone();
    DisplayName = (Type == UCAL_SHORT_STANDARD || Type == UCAL_SHORT_DST)
                      ? TimeZone->ShortStandardName
                      : TimeZone->StandardName;
    return LdkpIcuCopyString(DisplayName, Result, ResultLength, Status);
}

UBool
__cdecl
LdkpIcuUcalGetTimeZoneTransitionDate(
    _In_opt_ const LDK_ICU_CALENDAR* Calendar,
    _In_ INT32 Type,
    _Out_opt_ UDate* Transition,
    _Out_opt_ UErrorCode* Status
    )
{
    UNREFERENCED_PARAMETER(Calendar);
    UNREFERENCED_PARAMETER(Type);

    if (ARGUMENT_PRESENT(Transition)) {
        RtlZeroMemory(Transition, sizeof(*Transition));
    }
    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    return FALSE;
}

const CHAR*
__cdecl
LdkpIcuUcalGetTZDataVersion(
    _Out_opt_ UErrorCode* Status
    )
{
    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    return LdkpIcuTzDataVersion;
}

UBool
__cdecl
LdkpIcuUcalInDaylightTime(
    _In_opt_ const LDK_ICU_CALENDAR* Calendar,
    _Out_opt_ UErrorCode* Status
    )
{
    UNREFERENCED_PARAMETER(Calendar);
    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    return FALSE;
}

PLDK_ICU_CALENDAR
__cdecl
LdkpIcuUcalOpen(
    _In_reads_opt_(ZoneIdLength) const UChar* ZoneId,
    _In_ INT32 ZoneIdLength,
    _In_opt_ const CHAR* Locale,
    _In_ LDK_UCALENDAR_TYPE Type,
    _Out_opt_ UErrorCode* Status
    )
{
    PLDK_ICU_CALENDAR Calendar;
    const LDK_ICU_TIME_ZONE* TimeZone;

    UNREFERENCED_PARAMETER(Locale);
    UNREFERENCED_PARAMETER(Type);

    TimeZone = ZoneId != NULL ? LdkpIcuFindTimeZone(ZoneId, ZoneIdLength) : LdkpIcuDefaultTimeZone();
    if (TimeZone == NULL) {
        LdkpIcuSetStatus(Status, 1);
        return NULL;
    }

    Calendar = RtlAllocateHeap(RtlProcessHeap(),
                               HEAP_ZERO_MEMORY,
                               sizeof(*Calendar));
    if (Calendar == NULL) {
        LdkpIcuSetStatus(Status, 7);
        return NULL;
    }

    Calendar->TimeZone = TimeZone;
    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    return Calendar;
}

PLDK_ICU_ENUMERATION
__cdecl
LdkpIcuUcalOpenTimeZoneIDEnumeration(
    _In_ LDK_USYSTEM_TIME_ZONE_TYPE ZoneType,
    _In_opt_ const CHAR* Region,
    _In_opt_ const INT32* RawOffset,
    _Out_opt_ UErrorCode* Status
    )
{
    PLDK_ICU_ENUMERATION Enumeration;

    UNREFERENCED_PARAMETER(ZoneType);
    UNREFERENCED_PARAMETER(Region);
    UNREFERENCED_PARAMETER(RawOffset);

    Enumeration = RtlAllocateHeap(RtlProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  sizeof(*Enumeration));
    if (Enumeration == NULL) {
        LdkpIcuSetStatus(Status, 7);
        return NULL;
    }

    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    return Enumeration;
}

VOID
__cdecl
LdkpIcuUcalSetMillis(
    _In_opt_ PLDK_ICU_CALENDAR Calendar,
    _In_ UDate DateTime,
    _Out_opt_ UErrorCode* Status
    )
{
    UNREFERENCED_PARAMETER(Calendar);
    UNREFERENCED_PARAMETER(DateTime);
    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
}

VOID
__cdecl
LdkpIcuUenumClose(
    _In_opt_ PLDK_ICU_ENUMERATION Enumeration
    )
{
    if (Enumeration != NULL) {
        RtlFreeHeap(RtlProcessHeap(),
                    0,
                    Enumeration);
    }
}

INT32
__cdecl
LdkpIcuUenumCount(
    _In_opt_ PLDK_ICU_ENUMERATION Enumeration,
    _Out_opt_ UErrorCode* Status
    )
{
    UNREFERENCED_PARAMETER(Enumeration);
    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    return (INT32)RTL_NUMBER_OF(LdkpIcuTimeZones);
}

const UChar*
__cdecl
LdkpIcuUenumUnext(
    _Inout_opt_ PLDK_ICU_ENUMERATION Enumeration,
    _Out_opt_ INT32* ResultLength,
    _Out_opt_ UErrorCode* Status
    )
{
    const LDK_ICU_TIME_ZONE* TimeZone;

    if (Enumeration == NULL ||
        Enumeration->Index < 0 ||
        Enumeration->Index >= (INT32)RTL_NUMBER_OF(LdkpIcuTimeZones)) {
        if (ARGUMENT_PRESENT(ResultLength)) {
            *ResultLength = 0;
        }
        LdkpIcuSetStatus(Status, U_ZERO_ERROR);
        return NULL;
    }

    TimeZone = &LdkpIcuTimeZones[Enumeration->Index];
    Enumeration->Index++;
    if (ARGUMENT_PRESENT(ResultLength)) {
        *ResultLength = LdkpIcuStringLength(TimeZone->Id);
    }
    LdkpIcuSetStatus(Status, U_ZERO_ERROR);
    return TimeZone->Id;
}

#pragma warning(disable:4152)
LDK_FUNCTION_REGISTRATION LdkpIcuFunctionRegistration[] = {
    { "ucal_close", LdkpIcuUcalClose },
    { "ucal_get", LdkpIcuUcalGet },
    { "ucal_getCanonicalTimeZoneID", LdkpIcuUcalGetCanonicalTimeZoneID },
    { "ucal_getDefaultTimeZone", LdkpIcuUcalGetDefaultTimeZone },
    { "ucal_getTimeZoneDisplayName", LdkpIcuUcalGetTimeZoneDisplayName },
    { "ucal_getTimeZoneTransitionDate", LdkpIcuUcalGetTimeZoneTransitionDate },
    { "ucal_getTZDataVersion", (PVOID)LdkpIcuUcalGetTZDataVersion },
    { "ucal_inDaylightTime", LdkpIcuUcalInDaylightTime },
    { "ucal_open", LdkpIcuUcalOpen },
    { "ucal_openTimeZoneIDEnumeration", LdkpIcuUcalOpenTimeZoneIDEnumeration },
    { "ucal_setMillis", LdkpIcuUcalSetMillis },
    { "uenum_close", LdkpIcuUenumClose },
    { "uenum_count", LdkpIcuUenumCount },
    { "uenum_unext", (PVOID)LdkpIcuUenumUnext },
    { NULL, NULL }
};
#pragma warning(default:4152)

LDK_MODULE LdkpIcuModule = {
    RTL_CONSTANT_STRING("ICU.DLL"),
    RTL_CONSTANT_STRING("\\SystemRoot\\system32\\ICU.DLL"),
    LdkpIcuFunctionRegistration,
    NULL
};
