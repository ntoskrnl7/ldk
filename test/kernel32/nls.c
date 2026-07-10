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
NlsCheckLocaleCodePage (
    _In_z_ PCWSTR LocaleName,
    _In_ LCID Lcid,
    _In_ LCTYPE Type,
    _In_ DWORD Expected,
    _In_z_ PCWSTR ExpectedText
    )
{
    DWORD Number = 0;
    int Chars;

    if (! NlsCheckLocaleString( LocaleName,
                                Type,
                                ExpectedText )) {
        return FALSE;
    }

    Chars = GetLocaleInfoW( Lcid,
                            Type | LOCALE_RETURN_NUMBER,
                            (LPWSTR)&Number,
                            sizeof(Number) / sizeof(WCHAR) );
    if (Chars == 0 ||
        Number != Expected) {
        fprintf(stderr,
                "[Failed] GetLocaleInfoW(0x%08lx, 0x%08lx | LOCALE_RETURN_NUMBER) Chars = %d Value = %lu Expected = %lu ErrorCode = %lu\n",
                Lcid,
                Type,
                Chars,
                Number,
                Expected,
                GetLastError() );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
NlsCheckLocaleCodePages (
    _In_z_ PCWSTR LocaleName,
    _In_ LCID Lcid,
    _In_ DWORD ExpectedAnsi,
    _In_z_ PCWSTR ExpectedAnsiText,
    _In_ DWORD ExpectedOem,
    _In_z_ PCWSTR ExpectedOemText,
    _In_ DWORD ExpectedMac,
    _In_z_ PCWSTR ExpectedMacText,
    _In_ DWORD ExpectedEbcdic,
    _In_z_ PCWSTR ExpectedEbcdicText
    )
{
    return NlsCheckLocaleCodePage( LocaleName,
                                   Lcid,
                                   LOCALE_IDEFAULTANSICODEPAGE,
                                   ExpectedAnsi,
                                   ExpectedAnsiText ) &&
           NlsCheckLocaleCodePage( LocaleName,
                                   Lcid,
                                   LOCALE_IDEFAULTCODEPAGE,
                                   ExpectedOem,
                                   ExpectedOemText ) &&
           NlsCheckLocaleCodePage( LocaleName,
                                   Lcid,
                                   LOCALE_IDEFAULTMACCODEPAGE,
                                   ExpectedMac,
                                   ExpectedMacText ) &&
           NlsCheckLocaleCodePage( LocaleName,
                                   Lcid,
                                   LOCALE_IDEFAULTEBCDICCODEPAGE,
                                   ExpectedEbcdic,
                                   ExpectedEbcdicText );
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
    WCHAR DateBufferEx[128];
    WCHAR TimeBuffer[64];
    WCHAR TimeBufferEx[64];
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

    Chars = GetDateFormatEx( LocaleName,
                             DATE_LONGDATE,
                             &Sample,
                             NULL,
                             DateBufferEx,
                             RTL_NUMBER_OF(DateBufferEx),
                             NULL );
    if (Chars == 0 ||
        wcscmp( DateBufferEx,
                DateBuffer ) != 0) {
        fprintf(stderr,
                "[Failed] GetDateFormatEx(%ws) Chars = %d ErrorCode = %lu Date = %ws Expected = %ws\n",
                LocaleName,
                Chars,
                GetLastError(),
                DateBufferEx,
                DateBuffer );
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

    Chars = GetTimeFormatEx( LocaleName,
                             0,
                             &Sample,
                             NULL,
                             TimeBufferEx,
                             RTL_NUMBER_OF(TimeBufferEx) );
    if (Chars == 0 ||
        wcscmp( TimeBufferEx,
                TimeBuffer ) != 0) {
        fprintf(stderr,
                "[Failed] GetTimeFormatEx(%ws) Chars = %d ErrorCode = %lu Time = %ws Expected = %ws\n",
                LocaleName,
                Chars,
                GetLastError(),
                TimeBufferEx,
                TimeBuffer );
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

static
BOOLEAN
NlsCheckStringApis (
    VOID
    )
{
    WCHAR WideBuffer[32];
    CHAR AnsiBuffer[32];
    BYTE SortKey[32];
    WORD Types[4];
    int Chars;

    if (CompareStringEx( L"en-US",
                         NORM_IGNORECASE,
                         L"Runtime",
                         -1,
                         L"runtime",
                         -1,
                         NULL,
                         NULL,
                         0 ) != CSTR_EQUAL) {
        fprintf(stderr,
                "[Failed] CompareStringEx(NORM_IGNORECASE) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (CompareStringW( 0x0409,
                        0,
                        L"abc",
                        3,
                        L"abd",
                        3 ) != CSTR_LESS_THAN) {
        fprintf(stderr,
                "[Failed] CompareStringW ordinal order ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (CompareStringA( 0x0409,
                        NORM_IGNORECASE,
                        "Kernel",
                        -1,
                        "kernel",
                        -1 ) != CSTR_EQUAL) {
        fprintf(stderr,
                "[Failed] CompareStringA(NORM_IGNORECASE) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (CompareStringOrdinal( L"Loader",
                              -1,
                              L"loader",
                              -1,
                              TRUE ) != CSTR_EQUAL) {
        fprintf(stderr,
                "[Failed] CompareStringOrdinal(ignore case) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    Chars = LCMapStringEx( L"en-US",
                           LCMAP_UPPERCASE,
                           L"crt",
                           -1,
                           WideBuffer,
                           RTL_NUMBER_OF(WideBuffer),
                           NULL,
                           NULL,
                           0 );
    if (Chars != 4 ||
        wcscmp( WideBuffer,
                L"CRT" ) != 0) {
        fprintf(stderr,
                "[Failed] LCMapStringEx(UPPERCASE) Chars = %d ErrorCode = %lu\n",
                Chars,
                GetLastError() );
        return FALSE;
    }

    Chars = LCMapStringW( 0x0409,
                          LCMAP_LOWERCASE,
                          L"STL",
                          3,
                          WideBuffer,
                          3 );
    if (Chars != 3 ||
        wcsncmp( WideBuffer,
                 L"stl",
                 3 ) != 0) {
        fprintf(stderr,
                "[Failed] LCMapStringW(LOWERCASE) Chars = %d ErrorCode = %lu\n",
                Chars,
                GetLastError() );
        return FALSE;
    }

    Chars = LCMapStringA( 0x0409,
                          LCMAP_UPPERCASE,
                          "api",
                          -1,
                          AnsiBuffer,
                          RTL_NUMBER_OF(AnsiBuffer) );
    if (Chars != 4 ||
        strcmp( AnsiBuffer,
                "API" ) != 0) {
        fprintf(stderr,
                "[Failed] LCMapStringA(UPPERCASE) Chars = %d ErrorCode = %lu\n",
                Chars,
                GetLastError() );
        return FALSE;
    }

    Chars = LCMapStringEx( L"en-US",
                           LCMAP_SORTKEY | NORM_IGNORECASE,
                           L"AbC",
                           -1,
                           (LPWSTR)SortKey,
                           RTL_NUMBER_OF(SortKey),
                           NULL,
                           NULL,
                           0 );
    if (Chars <= 1 ||
        SortKey[Chars - 1] != 0) {
        fprintf(stderr,
                "[Failed] LCMapStringEx(SORTKEY) Chars = %d ErrorCode = %lu\n",
                Chars,
                GetLastError() );
        return FALSE;
    }

    Chars = GetLocaleInfoA( 0x0412,
                            LOCALE_SISO639LANGNAME,
                            AnsiBuffer,
                            RTL_NUMBER_OF(AnsiBuffer) );
    if (Chars != 3 ||
        strcmp( AnsiBuffer,
                "ko" ) != 0) {
        fprintf(stderr,
                "[Failed] GetLocaleInfoA(ko-KR) Chars = %d ErrorCode = %lu\n",
                Chars,
                GetLastError() );
        return FALSE;
    }

    if (! IsValidLocaleName( L"ko-KR" )) {
        fprintf(stderr,
                "[Failed] IsValidLocaleName ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (LocaleNameToLCID( LOCALE_NAME_INVARIANT,
                          0 ) != LOCALE_INVARIANT) {
        fprintf(stderr,
                "[Failed] LocaleNameToLCID(LOCALE_NAME_INVARIANT) = 0x%08lx Expected = 0x%08lx\n",
                LocaleNameToLCID( LOCALE_NAME_INVARIANT,
                                  0 ),
                LOCALE_INVARIANT );
        return FALSE;
    }

    Chars = LCIDToLocaleName( LOCALE_INVARIANT,
                              WideBuffer,
                              RTL_NUMBER_OF(WideBuffer),
                              0 );
    if (Chars != 1 ||
        WideBuffer[0] != L'\0') {
        fprintf(stderr,
                "[Failed] LCIDToLocaleName(LOCALE_INVARIANT) Chars = %d ErrorCode = %lu\n",
                Chars,
                GetLastError() );
        return FALSE;
    }

    if (! IsValidLocale( LOCALE_INVARIANT,
                         LCID_SUPPORTED ) ||
        ! IsValidLocaleName( LOCALE_NAME_INVARIANT )) {
        fprintf(stderr,
                "[Failed] invariant locale validation ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (! GetStringTypeA( 0x0409,
                          CT_CTYPE1,
                          "Az0",
                          3,
                          Types ) ||
        ! FlagOn(Types[0], C1_ALPHA) ||
        ! FlagOn(Types[0], C1_UPPER) ||
        ! FlagOn(Types[1], C1_ALPHA) ||
        ! FlagOn(Types[1], C1_LOWER) ||
        ! FlagOn(Types[2], C1_DIGIT)) {
        fprintf(stderr,
                "[Failed] GetStringTypeA(CT_CTYPE1) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (! GetStringTypeExW( LOCALE_INVARIANT,
                            CT_CTYPE1,
                            L"A1 ",
                            3,
                            Types ) ||
        ! FlagOn(Types[0], C1_UPPER) ||
        ! FlagOn(Types[1], C1_DIGIT) ||
        ! FlagOn(Types[2], C1_SPACE)) {
        fprintf(stderr,
                "[Failed] GetStringTypeExW(LOCALE_INVARIANT, CT_CTYPE1) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    if (! GetStringTypeExW( 0x0412,
                            CT_CTYPE3,
                            L"\xd55c",
                            1,
                            Types ) ||
        ! FlagOn(Types[0], C3_ALPHA)) {
        fprintf(stderr,
                "[Failed] GetStringTypeExW(CT_CTYPE3) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    SetLastError( ERROR_SUCCESS );
    Chars = MultiByteToWideChar( CP_UTF8,
                                 0x10,
                                 "A",
                                 -1,
                                 WideBuffer,
                                 RTL_NUMBER_OF(WideBuffer) );
    if (Chars != 0 ||
        GetLastError() != ERROR_INVALID_FLAGS) {
        fprintf(stderr,
                "[Failed] MultiByteToWideChar(CP_UTF8, invalid flag 0x10) Chars = %d ErrorCode = %lu\n",
                Chars,
                GetLastError() );
        return FALSE;
    }

    SetLastError( ERROR_SUCCESS );
    Chars = WideCharToMultiByte( CP_UTF8,
                                 0x100,
                                 L"A",
                                 -1,
                                 AnsiBuffer,
                                 RTL_NUMBER_OF(AnsiBuffer),
                                 NULL,
                                 NULL );
    if (Chars != 0 ||
        GetLastError() != ERROR_INVALID_FLAGS) {
        fprintf(stderr,
                "[Failed] WideCharToMultiByte(CP_UTF8, invalid flag 0x100) Chars = %d ErrorCode = %lu\n",
                Chars,
                GetLastError() );
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
             NlsCheckLocaleCodePages( L"en-US",
                                      0x0409,
                                      1252,
                                      L"1252",
                                      437,
                                      L"437",
                                      10000,
                                      L"10000",
                                      37,
                                      L"037" ) &&
             NlsCheckLocaleCodePages( L"de-DE",
                                      0x0407,
                                      1252,
                                      L"1252",
                                      850,
                                      L"850",
                                      10000,
                                      L"10000",
                                      20273,
                                      L"20273" ) &&
             NlsCheckLocaleCodePages( L"en-GB",
                                      0x0809,
                                      1252,
                                      L"1252",
                                      850,
                                      L"850",
                                      10000,
                                      L"10000",
                                      20285,
                                      L"20285" ) &&
             NlsCheckLocaleCodePages( L"ko-KR",
                                      0x0412,
                                      949,
                                      L"949",
                                      949,
                                      L"949",
                                      10003,
                                      L"10003",
                                      20833,
                                      L"20833" ) &&
             NlsCheckLocaleCodePages( L"zh-CN",
                                      0x0804,
                                      936,
                                      L"936",
                                      936,
                                      L"936",
                                      10008,
                                      L"10008",
                                      500,
                                      L"500" ) &&
             NlsCheckLocaleCodePages( L"ja-JP",
                                      0x0411,
                                      932,
                                      L"932",
                                      932,
                                      L"932",
                                      10001,
                                      L"10001",
                                      20290,
                                      L"20290" ) &&
             NlsCheckLocaleCodePages( L"ru-RU",
                                      0x0419,
                                      1251,
                                      L"1251",
                                      866,
                                      L"866",
                                      10007,
                                      L"10007",
                                      20880,
                                      L"20880" ) &&
             NlsCheckLocaleEnumeration() &&
             NlsCheckCharacterTypes() &&
             NlsCheckStringApis();

    if (Result) {
        printf("[Success] NLS locale and script test\n\n");
    } else {
        printf("[Failed] NLS locale and script test\n\n");
    }

    return Result;
}
