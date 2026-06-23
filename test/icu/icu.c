#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
IcuTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, IcuTest)
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

static PCSTR IcuRequiredExports[] = {
    "ucal_close",
    "ucal_get",
    "ucal_getCanonicalTimeZoneID",
    "ucal_getDefaultTimeZone",
    "ucal_getTimeZoneDisplayName",
    "ucal_getTimeZoneTransitionDate",
    "ucal_getTZDataVersion",
    "ucal_inDaylightTime",
    "ucal_open",
    "ucal_openTimeZoneIDEnumeration",
    "ucal_setMillis",
    "uenum_close",
    "uenum_count",
    "uenum_unext"
};

typedef PVOID (__cdecl *PFN_UCAL_OPEN)(const WCHAR*, INT32, const CHAR*, INT32, PINT32);
typedef VOID (__cdecl *PFN_UCAL_CLOSE)(PVOID);
typedef INT32 (__cdecl *PFN_UCAL_GET)(const VOID*, INT32, PINT32);
typedef PVOID (__cdecl *PFN_UCAL_OPEN_TIME_ZONE_ID_ENUMERATION)(INT32, const CHAR*, const INT32*, PINT32);
typedef VOID (__cdecl *PFN_UENUM_CLOSE)(PVOID);
typedef INT32 (__cdecl *PFN_UENUM_COUNT)(PVOID, PINT32);
typedef const WCHAR* (__cdecl *PFN_UENUM_UNEXT)(PVOID, PINT32, PINT32);

#define UCAL_ZONE_OFFSET 15
#define UCAL_DST_OFFSET 16

static
BOOLEAN
IcuCheckOffset (
    _In_ HMODULE Module,
    _In_z_ PCWSTR ZoneName,
    _In_ INT32 ExpectedOffset
    )
{
    PFN_UCAL_OPEN UcalOpen;
    PFN_UCAL_CLOSE UcalClose;
    PFN_UCAL_GET UcalGet;
    PVOID Calendar;
    INT32 Status = 0;
    INT32 Offset;
    INT32 DstOffset;

    UcalOpen = (PFN_UCAL_OPEN)GetProcAddress( Module, "ucal_open" );
    UcalClose = (PFN_UCAL_CLOSE)GetProcAddress( Module, "ucal_close" );
    UcalGet = (PFN_UCAL_GET)GetProcAddress( Module, "ucal_get" );

    if (UcalOpen == NULL || UcalClose == NULL || UcalGet == NULL) {
        fprintf(stderr, "[Failed] ICU calendar exports are incomplete\n");
        return FALSE;
    }

    Calendar = UcalOpen( ZoneName, -1, NULL, 0, &Status );
    if (Calendar == NULL || Status != 0) {
        fprintf(stderr,
                "[Failed] ucal_open(%ws) Status = %d\n",
                ZoneName,
                Status );
        return FALSE;
    }

    Offset = UcalGet( Calendar, UCAL_ZONE_OFFSET, &Status );
    if (Status != 0 || Offset != ExpectedOffset) {
        fprintf(stderr,
                "[Failed] ucal_get(%ws, UCAL_ZONE_OFFSET) = %d, Status = %d\n",
                ZoneName,
                Offset,
                Status );
        UcalClose( Calendar );
        return FALSE;
    }

    DstOffset = UcalGet( Calendar, UCAL_DST_OFFSET, &Status );
    if (Status != 0 || DstOffset != 0) {
        fprintf(stderr,
                "[Failed] ucal_get(%ws, UCAL_DST_OFFSET) = %d, Status = %d\n",
                ZoneName,
                DstOffset,
                Status );
        UcalClose( Calendar );
        return FALSE;
    }

    UcalClose( Calendar );
    return TRUE;
}

static
BOOLEAN
IcuCheckEnumeration (
    _In_ HMODULE Module
    )
{
    PFN_UCAL_OPEN_TIME_ZONE_ID_ENUMERATION UcalOpenTimeZoneIDEnumeration;
    PFN_UENUM_CLOSE UenumClose;
    PFN_UENUM_COUNT UenumCount;
    PFN_UENUM_UNEXT UenumUnext;
    PVOID Enumeration;
    BOOLEAN FoundSeoul = FALSE;
    BOOLEAN FoundTokyo = FALSE;
    BOOLEAN FoundShanghai = FALSE;
    BOOLEAN FoundMoscow = FALSE;
    INT32 Count;
    INT32 Status = 0;

    UcalOpenTimeZoneIDEnumeration =
        (PFN_UCAL_OPEN_TIME_ZONE_ID_ENUMERATION)GetProcAddress(
            Module,
            "ucal_openTimeZoneIDEnumeration" );
    UenumClose = (PFN_UENUM_CLOSE)GetProcAddress( Module, "uenum_close" );
    UenumCount = (PFN_UENUM_COUNT)GetProcAddress( Module, "uenum_count" );
    UenumUnext = (PFN_UENUM_UNEXT)GetProcAddress( Module, "uenum_unext" );

    if (UcalOpenTimeZoneIDEnumeration == NULL ||
        UenumClose == NULL ||
        UenumCount == NULL ||
        UenumUnext == NULL) {
        fprintf(stderr, "[Failed] ICU enumeration exports are incomplete\n");
        return FALSE;
    }

    Enumeration = UcalOpenTimeZoneIDEnumeration( 0, NULL, NULL, &Status );
    if (Enumeration == NULL || Status != 0) {
        fprintf(stderr,
                "[Failed] ucal_openTimeZoneIDEnumeration Status = %d\n",
                Status );
        return FALSE;
    }

    Count = UenumCount( Enumeration, &Status );
    if (Status != 0 || Count < 7) {
        fprintf(stderr,
                "[Failed] uenum_count = %d, Status = %d\n",
                Count,
                Status );
        UenumClose( Enumeration );
        return FALSE;
    }

    for (INT32 Index = 0; Index < Count; Index++) {
        INT32 NameLength = 0;
        const WCHAR* Name = UenumUnext( Enumeration, &NameLength, &Status );
        if (Status != 0 || Name == NULL) {
            fprintf(stderr,
                    "[Failed] uenum_unext[%d] Status = %d\n",
                    Index,
                    Status );
            UenumClose( Enumeration );
            return FALSE;
        }

        if (wcscmp( Name, L"Asia/Seoul" ) == 0) {
            FoundSeoul = TRUE;
        } else if (wcscmp( Name, L"Asia/Tokyo" ) == 0) {
            FoundTokyo = TRUE;
        } else if (wcscmp( Name, L"Asia/Shanghai" ) == 0) {
            FoundShanghai = TRUE;
        } else if (wcscmp( Name, L"Europe/Moscow" ) == 0) {
            FoundMoscow = TRUE;
        }
    }

    UenumClose( Enumeration );
    if (!FoundSeoul || !FoundTokyo || !FoundShanghai || !FoundMoscow) {
        fprintf(stderr,
                "[Failed] ICU timezone enumeration missing zones: Seoul=%d Tokyo=%d Shanghai=%d Moscow=%d\n",
                FoundSeoul,
                FoundTokyo,
                FoundShanghai,
                FoundMoscow );
    }
    return FoundSeoul && FoundTokyo && FoundShanghai && FoundMoscow;
}

BOOLEAN
IcuTest (
    VOID
    )
{
    HMODULE Module;
    BOOLEAN Result = TRUE;

    PAGED_CODE();

    printf("ICU virtual module test\n");

    Module = LoadLibraryExW( L"icu.dll",
                             NULL,
                             LOAD_LIBRARY_SEARCH_SYSTEM32 );
    if (Module == NULL) {
        fprintf(stderr,
                "[Failed] LoadLibraryExW(icu.dll) ErrorCode = %d\n",
                GetLastError() );
        return FALSE;
    }

    for (SIZE_T Index = 0; Index < RTL_NUMBER_OF(IcuRequiredExports); Index++) {
        FARPROC Procedure = GetProcAddress( Module,
                                            IcuRequiredExports[Index] );
        if (Procedure == NULL) {
            fprintf(stderr,
                    "[Failed] GetProcAddress(icu.dll, %s) ErrorCode = %d\n",
                    IcuRequiredExports[Index],
                    GetLastError() );
            Result = FALSE;
        }
    }

    if (Result) {
        Result = IcuCheckEnumeration( Module );
    }

    if (Result) {
        Result = IcuCheckOffset( Module, L"Etc/UTC", 0 ) &&
                 IcuCheckOffset( Module, L"Asia/Seoul", 9 * 60 * 60 * 1000 ) &&
                 IcuCheckOffset( Module, L"Asia/Tokyo", 9 * 60 * 60 * 1000 ) &&
                 IcuCheckOffset( Module, L"Asia/Shanghai", 8 * 60 * 60 * 1000 ) &&
                 IcuCheckOffset( Module, L"America/New_York", -5 * 60 * 60 * 1000 ) &&
                 IcuCheckOffset( Module, L"Europe/Berlin", 60 * 60 * 1000 ) &&
                 IcuCheckOffset( Module, L"Europe/Moscow", 3 * 60 * 60 * 1000 );
    }

    if (Result) {
        printf("[Success] ICU virtual module timezone subset works\n\n");
    } else {
        printf("[Failed] ICU virtual module test\n\n");
    }

    return Result;
}
