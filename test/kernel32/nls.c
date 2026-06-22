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
NlsTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NlsTest)
#endif

#define printf(...)         DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)
#define fprintf(_f_, ...)   DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__)
#define stderr              DPFLTR_ERROR_LEVEL

static
BOOLEAN
NlsExpect (
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
NlsWideStringEquals (
    _In_z_ PCWSTR Left,
    _In_z_ PCWSTR Right
    )
{
    while (*Left || *Right) {
        if (*Left != *Right) {
            return FALSE;
        }

        Left++;
        Right++;
    }

    return TRUE;
}

static
BOOLEAN
NlsTestCodePageInfo (
    VOID
    )
{
    CPINFO Info;
    CPINFOEXA InfoA;
    CPINFOEXW InfoW;
    BOOL Result = TRUE;

    PAGED_CODE();

    Result &= NlsExpect( ! IsValidCodePage( CP_ACP ),
                         "IsValidCodePage(CP_ACP) follows Win32 pseudo-codepage behavior" );
    Result &= NlsExpect( IsValidCodePage( 500 ),
                         "IsValidCodePage(500)" );
    Result &= NlsExpect( IsValidCodePage( 1252 ),
                         "IsValidCodePage(1252)" );
    Result &= NlsExpect( IsValidCodePage( 932 ),
                         "IsValidCodePage(932)" );

    Result &= NlsExpect( GetCPInfo( CP_ACP,
                                    &Info ),
                         "GetCPInfo(CP_ACP)" );
    Result &= NlsExpect( GetCPInfo( CP_MACCP,
                                    &Info ),
                         "GetCPInfo(CP_MACCP)" );

    Result &= NlsExpect( GetCPInfoExW( 500,
                                       0,
                                       &InfoW ),
                         "GetCPInfoExW(500)" );
    Result &= NlsExpect( InfoW.CodePage == 500,
                         "GetCPInfoExW(500).CodePage" );
    Result &= NlsExpect( InfoW.MaxCharSize == 1,
                         "GetCPInfoExW(500).MaxCharSize" );
    Result &= NlsExpect( InfoW.CodePageName[0] != UNICODE_NULL,
                         "GetCPInfoExW(500).CodePageName" );

    Result &= NlsExpect( GetCPInfoExA( 500,
                                       0,
                                       &InfoA ),
                         "GetCPInfoExA(500)" );
    Result &= NlsExpect( InfoA.CodePage == 500,
                         "GetCPInfoExA(500).CodePage" );
    Result &= NlsExpect( InfoA.CodePageName[0] != '\0',
                         "GetCPInfoExA(500).CodePageName" );

    Result &= NlsExpect( GetCPInfoExW( 1252,
                                       0,
                                       &InfoW ),
                         "GetCPInfoExW(1252)" );
    Result &= NlsExpect( InfoW.MaxCharSize == 1,
                         "GetCPInfoExW(1252).MaxCharSize" );

    Result &= NlsExpect( GetCPInfoExW( 932,
                                       0,
                                       &InfoW ),
                         "GetCPInfoExW(932)" );
    Result &= NlsExpect( InfoW.MaxCharSize == 2,
                         "GetCPInfoExW(932).MaxCharSize" );
    Result &= NlsExpect( InfoW.LeadByte[0] != 0,
                         "GetCPInfoExW(932).LeadByte" );

    SetLastError( ERROR_SUCCESS );
    Result &= NlsExpect( ! GetCPInfoExW( 999999,
                                         0,
                                         &InfoW ) &&
                         GetLastError() == ERROR_INVALID_PARAMETER,
                         "GetCPInfoExW(invalid codepage)" );

    return Result ? TRUE : FALSE;
}

static
BOOLEAN
NlsTestUtf8Conversion (
    VOID
    )
{
    CHAR MultiByte[16];
    WCHAR Wide[16];
    BOOL UsedDefaultChar = FALSE;
    int Count;
    BOOL Result = TRUE;

    PAGED_CODE();

    Count = MultiByteToWideChar( CP_UTF8,
                                 0,
                                 "abc",
                                 -1,
                                 NULL,
                                 0 );
    Result &= NlsExpect( Count == 4,
                         "MultiByteToWideChar(CP_UTF8) size query includes terminator" );

    Count = MultiByteToWideChar( CP_UTF8,
                                 0,
                                 "abc",
                                 -1,
                                 Wide,
                                 RTL_NUMBER_OF(Wide) );
    Result &= NlsExpect( Count == 4 && NlsWideStringEquals( Wide, L"abc" ),
                         "MultiByteToWideChar(CP_UTF8) converts ASCII" );

    SetLastError( ERROR_SUCCESS );
    Count = MultiByteToWideChar( CP_UTF8,
                                 0,
                                 "abc",
                                 -1,
                                 Wide,
                                 2 );
    Result &= NlsExpect( Count == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                         "MultiByteToWideChar(CP_UTF8) reports small buffer" );

    Count = MultiByteToWideChar( CP_UTF8,
                                 0,
                                 "\xE2\x82\xAC",
                                 -1,
                                 Wide,
                                 RTL_NUMBER_OF(Wide) );
    Result &= NlsExpect( Count == 2 && Wide[0] == 0x20ac && Wide[1] == UNICODE_NULL,
                         "MultiByteToWideChar(CP_UTF8) converts multibyte sequence" );

    Count = WideCharToMultiByte( CP_UTF8,
                                 0,
                                 L"\x20ac",
                                 -1,
                                 MultiByte,
                                 sizeof(MultiByte),
                                 NULL,
                                 NULL );
    Result &= NlsExpect( Count == 4 &&
                         (UCHAR)MultiByte[0] == 0xe2 &&
                         (UCHAR)MultiByte[1] == 0x82 &&
                         (UCHAR)MultiByte[2] == 0xac &&
                         MultiByte[3] == '\0',
                         "WideCharToMultiByte(CP_UTF8) converts multibyte sequence" );

    SetLastError( ERROR_SUCCESS );
    Count = WideCharToMultiByte( CP_UTF8,
                                 0,
                                 L"A",
                                 -1,
                                 MultiByte,
                                 sizeof(MultiByte),
                                 NULL,
                                 &UsedDefaultChar );
    Result &= NlsExpect( Count == 0 && GetLastError() == ERROR_INVALID_PARAMETER,
                         "WideCharToMultiByte(CP_UTF8) rejects lpUsedDefaultChar" );

    return Result ? TRUE : FALSE;
}

static
BOOLEAN
NlsTestCustomCodePageConversion (
    VOID
    )
{
    static const WCHAR Source[] = L"ABCxyz09";
    CHAR MultiByte[64];
    WCHAR Wide[64];
    BOOL UsedDefaultChar = TRUE;
    int Bytes;
    int Chars;
    BOOL Result = TRUE;

    PAGED_CODE();

    Bytes = WideCharToMultiByte( 500,
                                 0,
                                 Source,
                                 -1,
                                 MultiByte,
                                 sizeof(MultiByte),
                                 NULL,
                                 &UsedDefaultChar );
    Result &= NlsExpect( Bytes > 0 && ! UsedDefaultChar,
                         "WideCharToMultiByte(500) converts representable text" );

    Chars = MultiByteToWideChar( 500,
                                 0,
                                 MultiByte,
                                 -1,
                                 NULL,
                                 0 );
    Result &= NlsExpect( Chars == RTL_NUMBER_OF(Source),
                         "MultiByteToWideChar(500) size query includes terminator" );

    Chars = MultiByteToWideChar( 500,
                                 0,
                                 MultiByte,
                                 Bytes,
                                 Wide,
                                 RTL_NUMBER_OF(Wide) );
    Result &= NlsExpect( Chars == RTL_NUMBER_OF(Source) &&
                         NlsWideStringEquals( Wide, Source ),
                         "MultiByteToWideChar(500) round-trips text" );

    SetLastError( ERROR_SUCCESS );
    Chars = MultiByteToWideChar( 500,
                                 0,
                                 MultiByte,
                                 Bytes,
                                 Wide,
                                 1 );
    Result &= NlsExpect( Chars == 0 && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                         "MultiByteToWideChar(500) reports small buffer" );

    return Result ? TRUE : FALSE;
}

BOOLEAN
NlsTest (
    VOID
    )
{
    BOOL Result = TRUE;

    PAGED_CODE();

    printf("NLS Test\n");

    Result &= NlsTestCodePageInfo();
    Result &= NlsTestUtf8Conversion();
    Result &= NlsTestCustomCodePageConversion();

    if (Result) {
        printf("[Success] NLS Test\n\n");
    } else {
        printf("[Failed] NLS Test\n\n");
    }

    return Result ? TRUE : FALSE;
}
