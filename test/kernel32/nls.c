#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
NlsTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NlsTest)
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

#ifndef LOCALE_NAME_MAX_LENGTH
#define LOCALE_NAME_MAX_LENGTH 85
#endif

typedef struct _NLS_ENUM_CONTEXT {
    BOOLEAN FoundKorean;
    BOOLEAN FoundChinese;
    BOOLEAN FoundJapanese;
    BOOLEAN FoundRussian;
} NLS_ENUM_CONTEXT, *PNLS_ENUM_CONTEXT;

static
BOOLEAN
NlsCheckLocaleString (
    _In_z_ PCWSTR LocaleName,
    _In_ LCTYPE Type,
    _In_z_ PCWSTR Expected
    )
{
    WCHAR Buffer[96];
    int Chars;

    Chars = GetLocaleInfoEx( LocaleName,
                             Type,
                             Buffer,
                             RTL_NUMBER_OF(Buffer) );
    if (Chars == 0 ||
        wcscmp( Buffer,
                Expected ) != 0) {
        fprintf(stderr,
                "[Failed] GetLocaleInfoEx(%ws, 0x%08lx) Chars = %d ErrorCode = %lu\n",
                LocaleName,
                Type,
                Chars,
                GetLastError() );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
NlsCheckLocale (
    _In_z_ PCWSTR LocaleName,
    _In_ LCID ExpectedLcid,
    _In_z_ PCWSTR IsoLanguage,
    _In_z_ PCWSTR IsoCountry,
    _In_z_ PCWSTR Scripts,
    _In_z_ PCWSTR OpenTypeTag,
    _In_z_ PCWSTR NativeDisplayName,
    _In_z_ PCWSTR NativeLanguageName,
    _In_z_ PCWSTR NativeCountryName,
    _In_z_ PCWSTR FirstMonth,
    _In_z_ PCWSTR FirstDay,
    _In_z_ PCWSTR CurrencySymbol
    )
{
    WCHAR LocaleNameBuffer[LOCALE_NAME_MAX_LENGTH];
    WCHAR DateBuffer[128];
    WCHAR TimeBuffer[64];
    SYSTEMTIME Sample = { 0 };
    LCID Lcid;
    int Chars;

    Lcid = LocaleNameToLCID( LocaleName,
                             0 );
    if (Lcid != ExpectedLcid) {
        fprintf(stderr,
                "[Failed] LocaleNameToLCID(%ws) = 0x%08lx Expected = 0x%08lx\n",
                LocaleName,
                Lcid,
                ExpectedLcid );
        return FALSE;
    }

    Chars = LCIDToLocaleName( ExpectedLcid,
                              LocaleNameBuffer,
                              RTL_NUMBER_OF(LocaleNameBuffer),
                              0 );
    if (Chars == 0 ||
        wcscmp( LocaleNameBuffer,
                LocaleName ) != 0) {
        fprintf(stderr,
                "[Failed] LCIDToLocaleName(0x%08lx) Chars = %d ErrorCode = %lu\n",
                ExpectedLcid,
                Chars,
                GetLastError() );
        return FALSE;
    }

    if (! NlsCheckLocaleString( LocaleName,
                                LOCALE_SISO639LANGNAME,
                                IsoLanguage ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SISO3166CTRYNAME,
                                IsoCountry ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SSCRIPTS,
                                Scripts ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SOPENTYPELANGUAGETAG,
                                OpenTypeTag ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SNATIVEDISPLAYNAME,
                                NativeDisplayName ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SNATIVELANGUAGENAME,
                                NativeLanguageName ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SNATIVECOUNTRYNAME,
                                NativeCountryName ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SMONTHNAME1,
                                FirstMonth ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SDAYNAME1,
                                FirstDay ) ||
        ! NlsCheckLocaleString( LocaleName,
                                LOCALE_SCURRENCY,
                                CurrencySymbol )) {
        return FALSE;
    }

    Sample.wYear = 2026;
    Sample.wMonth = 6;
    Sample.wDay = 24;
    Sample.wDayOfWeek = 3;
    Sample.wHour = 13;
    Sample.wMinute = 45;
    Sample.wSecond = 9;

    Chars = GetDateFormatW( ExpectedLcid,
                            DATE_LONGDATE,
                            &Sample,
                            NULL,
                            DateBuffer,
                            RTL_NUMBER_OF(DateBuffer) );
    if (Chars == 0 ||
        DateBuffer[0] == UNICODE_NULL) {
        fprintf(stderr,
                "[Failed] GetDateFormatW(%ws) Chars = %d ErrorCode = %lu\n",
                LocaleName,
                Chars,
                GetLastError() );
        return FALSE;
    }

    Chars = GetTimeFormatW( ExpectedLcid,
                            0,
                            &Sample,
                            NULL,
                            TimeBuffer,
                            RTL_NUMBER_OF(TimeBuffer) );
    if (Chars == 0 ||
        TimeBuffer[0] == UNICODE_NULL) {
        fprintf(stderr,
                "[Failed] GetTimeFormatW(%ws) Chars = %d ErrorCode = %lu\n",
                LocaleName,
                Chars,
                GetLastError() );
        return FALSE;
    }

    return TRUE;
}

static
BOOL
CALLBACK
NlsEnumLocaleNameCallback (
    _In_ LPWSTR LocaleString,
    _In_ DWORD Flags,
    _In_ LPARAM Parameter
    )
{
    PNLS_ENUM_CONTEXT Context = (PNLS_ENUM_CONTEXT)Parameter;

    UNREFERENCED_PARAMETER(Flags);

    if (wcscmp( LocaleString,
                L"ko-KR" ) == 0) {
        Context->FoundKorean = TRUE;
    } else if (wcscmp( LocaleString,
                       L"zh-CN" ) == 0) {
        Context->FoundChinese = TRUE;
    } else if (wcscmp( LocaleString,
                       L"ja-JP" ) == 0) {
        Context->FoundJapanese = TRUE;
    } else if (wcscmp( LocaleString,
                       L"ru-RU" ) == 0) {
        Context->FoundRussian = TRUE;
    }

    return TRUE;
}

static
BOOL
CALLBACK
NlsEnumLocaleHexCallback (
    _In_ LPWSTR LocaleString
    )
{
    UNREFERENCED_PARAMETER(LocaleString);
    return TRUE;
}

static
BOOLEAN
NlsCheckLocaleEnumeration (
    VOID
    )
{
    NLS_ENUM_CONTEXT Context = { 0 };

    if (! EnumSystemLocalesW( NlsEnumLocaleHexCallback,
                              0 )) {
        fprintf(stderr,
                "[Failed] EnumSystemLocalesW ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (! EnumSystemLocalesEx( NlsEnumLocaleNameCallback,
                               0,
                               (LPARAM)&Context,
                               NULL )) {
        fprintf(stderr,
                "[Failed] EnumSystemLocalesEx ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (!Context.FoundKorean ||
        !Context.FoundChinese ||
        !Context.FoundJapanese ||
        !Context.FoundRussian) {
        fprintf(stderr,
                "[Failed] EnumSystemLocalesEx missing locales: ko=%d zh=%d ja=%d ru=%d\n",
                Context.FoundKorean,
                Context.FoundChinese,
                Context.FoundJapanese,
                Context.FoundRussian );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
NlsCheckCharacterTypes (
    VOID
    )
{
    WCHAR Characters[] = {
        L'\xd55c',
        L'\x4e2d',
        L'\x3042',
        L'\x30a2',
        L'\x042f'
    };
    WORD Types[RTL_NUMBER_OF(Characters)];

    if (! GetStringTypeW( CT_CTYPE1,
                          Characters,
                          RTL_NUMBER_OF(Characters),
                          Types )) {
        fprintf(stderr,
                "[Failed] GetStringTypeW(CT_CTYPE1) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    for (ULONG Index = 0; Index < RTL_NUMBER_OF(Characters); Index++) {
        if (! FlagOn(Types[Index], C1_DEFINED) ||
            ! FlagOn(Types[Index], C1_ALPHA)) {
            fprintf(stderr,
                    "[Failed] GetStringTypeW(CT_CTYPE1)[%lu] = 0x%04x\n",
                    Index,
                    Types[Index] );
            return FALSE;
        }
    }

    if (! GetStringTypeW( CT_CTYPE3,
                          Characters,
                          RTL_NUMBER_OF(Characters),
                          Types )) {
        fprintf(stderr,
                "[Failed] GetStringTypeW(CT_CTYPE3) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (! FlagOn(Types[0], C3_ALPHA) ||
        ! FlagOn(Types[1], C3_IDEOGRAPH) ||
        ! FlagOn(Types[2], C3_HIRAGANA) ||
        ! FlagOn(Types[3], C3_KATAKANA) ||
        ! FlagOn(Types[4], C3_ALPHA)) {
        fprintf(stderr,
                "[Failed] GetStringTypeW(CT_CTYPE3) = 0x%04x 0x%04x 0x%04x 0x%04x 0x%04x\n",
                Types[0],
                Types[1],
                Types[2],
                Types[3],
                Types[4] );
        return FALSE;
    }

    return TRUE;
}

BOOLEAN
NlsTest (
    VOID
    )
{
    BOOLEAN Result;

    PAGED_CODE();

    printf("NLS locale and script test\n");

    Result = NlsCheckLocale( L"ko-KR",
                             0x0412,
                             L"ko",
                             L"KR",
                             L"Hang;Hani;Kore;",
                             L"KOR ",
                             L"\xd55c\xad6d\xc5b4(\xb300\xd55c\xbbfc\xad6d)",
                             L"\xd55c\xad6d\xc5b4",
                             L"\xb300\xd55c\xbbfc\xad6d",
                             L"1" L"\xc6d4",
                             L"\xc6d4\xc694\xc77c",
                             L"\x20a9" ) &&
             NlsCheckLocale( L"zh-CN",
                             0x0804,
                             L"zh",
                             L"CN",
                             L"Hani;Hans;",
                             L"ZHS ",
                             L"\x4e2d\x6587(\x4e2d\x56fd)",
                             L"\x4e2d\x6587(\x7b80\x4f53)",
                             L"\x4e2d\x56fd",
                             L"\x4e00\x6708",
                             L"\x661f\x671f\x4e00",
                             L"\x00a5" ) &&
             NlsCheckLocale( L"ja-JP",
                             0x0411,
                             L"ja",
                             L"JP",
                             L"Hani;Hira;Jpan;Kana;",
                             L"JAN ",
                             L"\x65e5\x672c\x8a9e (\x65e5\x672c)",
                             L"\x65e5\x672c\x8a9e",
                             L"\x65e5\x672c",
                             L"1" L"\x6708",
                             L"\x6708\x66dc\x65e5",
                             L"\x00a5" ) &&
             NlsCheckLocale( L"ru-RU",
                             0x0419,
                             L"ru",
                             L"RU",
                             L"Cyrl;",
                             L"RUS ",
                             L"\x0440\x0443\x0441\x0441\x043a\x0438\x0439 (\x0420\x043e\x0441\x0441\x0438\x044f)",
                             L"\x0440\x0443\x0441\x0441\x043a\x0438\x0439",
                             L"\x0420\x043e\x0441\x0441\x0438\x044f",
                             L"\x042f\x043d\x0432\x0430\x0440\x044c",
                             L"\x043f\x043e\x043d\x0435\x0434\x0435\x043b\x044c\x043d\x0438\x043a",
                             L"\x20bd" ) &&
             NlsCheckLocaleEnumeration() &&
             NlsCheckCharacterTypes();

    if (Result) {
        printf("[Success] NLS locale and script test\n\n");
    } else {
        printf("[Failed] NLS locale and script test\n\n");
    }

    return Result;
}
