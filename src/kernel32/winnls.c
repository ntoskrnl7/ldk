#include "winbase.h"
#include "../peb.h"


LCID LdkpSystemLocale;

extern USHORT *NlsAnsiCodePage;
extern USHORT *NlsOemCodePage;
extern PUSHORT *NlsLeadByteInfo;

NTSTATUS
LdkInitializeNls (
	VOID
	)
{
	NTSTATUS Status;
    PAGED_CODE();

	Status = ZwQueryDefaultLocale( FALSE,
                                   &LdkpSystemLocale );
	return Status;
}

WINBASEAPI
BOOL
WINAPI
IsValidCodePage (
    _In_ UINT CodePage
    )
{
    if ((CodePage == *NlsAnsiCodePage) || (CodePage == *NlsOemCodePage) || (CodePage == CP_UTF7) || (CodePage == CP_UTF8)) {
        return TRUE;
    }

    KdBreakPoint();
    return FALSE;
}

WINBASEAPI
UINT
WINAPI
GetACP (
    VOID
    )
{
    return *NlsAnsiCodePage;
}

WINBASEAPI
UINT
WINAPI
GetOEMCP (
    VOID
    )
{
    return *NlsOemCodePage;
}


BOOL
WINAPI
UTFCPInfo (
	_In_ UINT CodePage,
	_Out_ LPCPINFO lpCPInfo,
    _In_ BOOL fExVer
    )
{
    if ((CodePage < CP_UTF7) || (CodePage > CP_UTF8) || (lpCPInfo == NULL)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    switch (CodePage) {
    case CP_UTF7:
        lpCPInfo->MaxCharSize = 5;
        break;
	case CP_UTF8:
        lpCPInfo->MaxCharSize = 4;
        break;
    }

    (lpCPInfo->DefaultChar)[0] = '?';
    (lpCPInfo->DefaultChar)[1] = (BYTE)0;

    for (int ctr = 0; ctr < MAX_LEADBYTES; ctr++) {
        (lpCPInfo->LeadByte)[ctr] = (BYTE)0;
    }

    if (fExVer) {
        LPCPINFOEXW lpCPInfoEx = (LPCPINFOEXW)lpCPInfo;
        lpCPInfoEx->UnicodeDefaultChar = L'?';
        lpCPInfoEx->CodePage = CodePage;
    }

    return TRUE;
}

#define NLS_CP_ALGORITHM_RANGE    60000

WINBASEAPI
BOOL
WINAPI
GetCPInfo (
    _In_ UINT CodePage,
    _Out_ LPCPINFO lpCPInfo
    )
{
    if (CodePage >= NLS_CP_ALGORITHM_RANGE) {
        return UTFCPInfo( CodePage,
                          lpCPInfo,
                          FALSE );
    }

    KdBreakPoint();
    return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetCPInfoExA (
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _Out_ LPCPINFOEXA lpCPInfoEx
    )
{
	BOOL bSuccess;
    CPINFOEXW CPInfoExW;

	PAGED_CODE();

	bSuccess = GetCPInfoExW( CodePage,
                             dwFlags,
                             &CPInfoExW );

    UNICODE_STRING Unicode;
	ANSI_STRING Ansi;

    RtlInitUnicodeString( &Unicode,
                          CPInfoExW.CodePageName );
    
    Ansi.MaximumLength = MAX_PATH;
    Ansi.Buffer = lpCPInfoEx->CodePageName;

    NTSTATUS status = LdkAnsiStringToUnicodeString( &Unicode,
                                                    &Ansi,
                                                    FALSE ) ;
    if (! NT_SUCCESS(status)) {
        BaseSetLastNTError( status );
        return FALSE;
    }

    RtlMoveMemory( lpCPInfoEx,
                   &CPInfoExW,
                   FIELD_OFFSET(CPINFOEXW, CodePageName) );

	return bSuccess;
}

#define RC_CODE_PAGE_NAME         3

int
GetStringTableEntry (
    _In_ UINT ResourceID,
    _In_ LANGID UILangId,
    _Out_ LPWSTR pBuffer,
    _In_ int cchBuffer,
    _In_ int WhichString
    )
{
    KdBreakPoint();
    UNREFERENCED_PARAMETER( ResourceID );
    UNREFERENCED_PARAMETER( UILangId );
    UNREFERENCED_PARAMETER( pBuffer );
    UNREFERENCED_PARAMETER( ResourceID );
    UNREFERENCED_PARAMETER( cchBuffer );
    UNREFERENCED_PARAMETER( WhichString );
    return 0;
}

WINBASEAPI
BOOL
WINAPI
GetCPInfoExW (
    _In_ UINT CodePage,
    _In_ DWORD dwFlags,
    _Out_ LPCPINFOEXW lpCPInfoEx
    )
{
    UNREFERENCED_PARAMETER( dwFlags );

    if (CodePage >= NLS_CP_ALGORITHM_RANGE) {
        if (UTFCPInfo( CodePage,
                       (LPCPINFO)lpCPInfoEx,
                       TRUE )) {
            return GetStringTableEntry( CodePage,
                                        0,
                                        lpCPInfoEx->CodePageName,
                                        MAX_PATH,
                                        RC_CODE_PAGE_NAME );
        }
        return FALSE;
    }

    KdBreakPoint();
    return FALSE;
}

#define NLS_VALID_LOCALE_MASK          0x000fffff
#define IS_INVALID_LOCALE(Locale)      ( Locale & ~NLS_VALID_LOCALE_MASK )

WINBASEAPI
BOOL
WINAPI
IsValidLocale (
    _In_ LCID Locale,
    _In_ DWORD dwFlags
    )
{
    UNREFERENCED_PARAMETER(dwFlags);
    return IS_INVALID_LOCALE(Locale);
}

PCWSTR
LdkpGetLocaleName (
    _In_ LCID Locale
    )
{
    switch (Locale) {
    case 0x0001: return L"ar";
    case 0x0002: return L"bg";
    case 0x0003: return L"ca";
    case 0x0004: return L"zh-Hans";
    case 0x0005: return L"cs";
    case 0x0006: return L"da";
    case 0x0007: return L"de";
    case 0x0008: return L"el";
    case 0x0009: return L"en";
    case 0x000A: return L"es";
    case 0x000B: return L"fi";
    case 0x000C: return L"fr";
    case 0x000D: return L"he";
    case 0x000E: return L"hu";
    case 0x000F: return L"is";
    case 0x0010: return L"it";
    case 0x0011: return L"ja";
    case 0x0012: return L"ko";
    case 0x0013: return L"nl";
    case 0x0014: return L"no";
    case 0x0015: return L"pl";
    case 0x0016: return L"pt";
    case 0x0017: return L"rm";
    case 0x0018: return L"ro";
    case 0x0019: return L"ru";
    case 0x001A: return L"hr";
    case 0x001B: return L"sk";
    case 0x001C: return L"sq";
    case 0x001D: return L"sv";
    case 0x001E: return L"th";
    case 0x001F: return L"tr";
    case 0x0020: return L"ur";
    case 0x0021: return L"id";
    case 0x0022: return L"uk";
    case 0x0023: return L"be";
    case 0x0024: return L"sl";
    case 0x0025: return L"et";
    case 0x0026: return L"lv";
    case 0x0027: return L"lt";
    case 0x0028: return L"tg";
    case 0x0029: return L"fa";
    case 0x002A: return L"vi";
    case 0x002B: return L"hy";
    case 0x002C: return L"az";
    case 0x002D: return L"eu";
    case 0x002E: return L"hsb";
    case 0x002F: return L"mk";
    case 0x0030: return L"st";
    case 0x0031: return L"ts";
    case 0x0032: return L"tn";
    case 0x0033: return L"ve";
    case 0x0034: return L"xh";
    case 0x0035: return L"zu";
    case 0x0036: return L"af";
    case 0x0037: return L"ka";
    case 0x0038: return L"fo";
    case 0x0039: return L"hi";
    case 0x003A: return L"mt";
    case 0x003B: return L"se";
    case 0x003C: return L"ga";
    case 0x003E: return L"ms";
    case 0x003F: return L"kk";
    case 0x0040: return L"ky";
    case 0x0041: return L"sw";
    case 0x0042: return L"tk";
    case 0x0043: return L"uz";
    case 0x0044: return L"tt";
    case 0x0045: return L"bn";
    case 0x0046: return L"pa";
    case 0x0047: return L"gu";
    case 0x0048: return L"or";
    case 0x0049: return L"ta";
    case 0x004A: return L"te";
    case 0x004B: return L"kn";
    case 0x004C: return L"ml";
    case 0x004D: return L"as";
    case 0x004E: return L"mr";
    case 0x004F: return L"sa";
    case 0x0050: return L"mn";
    case 0x0051: return L"bo";
    case 0x0052: return L"cy";
    case 0x0053: return L"km";
    case 0x0054: return L"lo";
    case 0x0055: return L"my";
    case 0x0056: return L"gl";
    case 0x0057: return L"kok";
    case 0x0059: return L"sd";
    case 0x005A: return L"syr";
    case 0x005B: return L"si";
    case 0x005C: return L"chr";
    case 0x005D: return L"iu";
    case 0x005E: return L"am";
    case 0x005F: return L"tzm";
    case 0x0060: return L"ks";
    case 0x0061: return L"ne";
    case 0x0062: return L"fy";
    case 0x0063: return L"ps";
    case 0x0064: return L"fil";
    case 0x0065: return L"dv";
    case 0x0067: return L"ff";
    case 0x0068: return L"ha";
    case 0x006A: return L"yo";
    case 0x006B: return L"quz";
    case 0x006C: return L"nso";
    case 0x006D: return L"ba";
    case 0x006E: return L"lb";
    case 0x006F: return L"kl";
    case 0x0070: return L"ig";
    case 0x0072: return L"om";
    case 0x0073: return L"ti";
    case 0x0074: return L"gn";
    case 0x0075: return L"haw";
    case 0x0077: return L"so";
    case 0x0078: return L"ii";
    case 0x007A: return L"arn";
    case 0x007C: return L"moh";
    case 0x007E: return L"br";
    case 0x0080: return L"ug";
    case 0x0081: return L"mi";
    case 0x0082: return L"oc";
    case 0x0083: return L"co";
    case 0x0084: return L"gsw";
    case 0x0085: return L"sah";
    case 0x0086: return L"quc";
    case 0x0087: return L"rw";
    case 0x0088: return L"wo";
    case 0x008C: return L"prs";
    case 0x0091: return L"gd";
    case 0x0092: return L"ku";
    case 0x0401: return L"ar-SA";
    case 0x0402: return L"bg-BG";
    case 0x0403: return L"ca-ES";
    case 0x0404: return L"zh-TW";
    case 0x0405: return L"cs-CZ";
    case 0x0406: return L"da-DK";
    case 0x0407: return L"de-DE";
    case 0x0408: return L"el-GR";
    case 0x0409: return L"en-US";
    case 0x040A: return L"es-ES_tradnl";
    case 0x040B: return L"fi-FI";
    case 0x040C: return L"fr-FR";
    case 0x040D: return L"he-IL";
    case 0x040E: return L"hu-HU";
    case 0x040F: return L"is-IS";
    case 0x0410: return L"it-IT";
    case 0x0411: return L"ja-JP";
    case 0x0412: return L"ko-KR";
    case 0x0413: return L"nl-NL";
    case 0x0414: return L"nb-NO";
    case 0x0415: return L"pl-PL";
    case 0x0416: return L"pt-BR";
    case 0x0417: return L"rm-CH";
    case 0x0418: return L"ro-RO";
    case 0x0419: return L"ru-RU";
    case 0x041A: return L"hr-HR";
    case 0x041B: return L"sk-SK";
    case 0x041C: return L"sq-AL";
    case 0x041D: return L"sv-SE";
    case 0x041E: return L"th-TH";
    case 0x041F: return L"tr-TR";
    case 0x0420: return L"ur-PK";
    case 0x0421: return L"id-ID";
    case 0x0422: return L"uk-UA";
    case 0x0423: return L"be-BY";
    case 0x0424: return L"sl-SI";
    case 0x0425: return L"et-EE";
    case 0x0426: return L"lv-LV";
    case 0x0427: return L"lt-LT";
    case 0x0428: return L"tg-Cyrl-TJ";
    case 0x0429: return L"fa-IR";
    case 0x042A: return L"vi-VN";
    case 0x042B: return L"hy-AM";
    case 0x042C: return L"az-Latn-AZ";
    case 0x042D: return L"eu-ES";
    case 0x042E: return L"hsb-DE";
    case 0x042F: return L"mk-MK";
    case 0x0430: return L"st-ZA";
    case 0x0431: return L"ts-ZA";
    case 0x0432: return L"tn-ZA";
    case 0x0433: return L"ve-ZA";
    case 0x0434: return L"xh-ZA";
    case 0x0435: return L"zu-ZA";
    case 0x0436: return L"af-ZA";
    case 0x0437: return L"ka-GE";
    case 0x0438: return L"fo-FO";
    case 0x0439: return L"hi-IN";
    case 0x043A: return L"mt-MT";
    case 0x043B: return L"se-NO";
    case 0x043D: return L"yi-001";
    case 0x043E: return L"ms-MY";
    case 0x043F: return L"kk-KZ";
    case 0x0440: return L"ky-KG";
    case 0x0441: return L"sw-KE";
    case 0x0442: return L"tk-TM";
    case 0x0443: return L"uz-Latn-UZ";
    case 0x0444: return L"tt-RU";
    case 0x0445: return L"bn-IN";
    case 0x0446: return L"pa-IN";
    case 0x0447: return L"gu-IN";
    case 0x0448: return L"or-IN";
    case 0x0449: return L"ta-IN";
    case 0x044A: return L"te-IN";
    case 0x044B: return L"kn-IN";
    case 0x044C: return L"ml-IN";
    case 0x044D: return L"as-IN";
    case 0x044E: return L"mr-IN";
    case 0x044F: return L"sa-IN";
    case 0x0450: return L"mn-MN";
    case 0x0451: return L"bo-CN";
    case 0x0452: return L"cy-GB";
    case 0x0453: return L"km-KH";
    case 0x0454: return L"lo-LA";
    case 0x0455: return L"my-MM";
    case 0x0456: return L"gl-ES";
    case 0x0457: return L"kok-IN";
    case 0x045A: return L"syr-SY";
    case 0x045B: return L"si-LK";
    case 0x045C: return L"chr-Cher-US";
    case 0x045d: return L"iu-Cans-CA";
    case 0x045E: return L"am-ET";
    case 0x045F: return L"tzm-Arab-MA";
    case 0x0460: return L"ks-Arab";
    case 0x0461: return L"ne-NP";
    case 0x0462: return L"fy-NL";
    case 0x0463: return L"ps-AF";
    case 0x0464: return L"fil-PH";
    case 0x0465: return L"dv-MV";
    // case 0x0467: return L"ff-NG";
    case 0x0467: return L"ff-Latn-NG";
    case 0x0468: return L"ha-Latn-NG";
    case 0x046A: return L"yo-NG";
    case 0x046B: return L"quz-BO";
    case 0x046C: return L"nso-ZA";
    case 0x046D: return L"ba-RU";
    case 0x046E: return L"lb-LU";
    case 0x046F: return L"kl-GL";
    case 0x0470: return L"ig-NG";
    case 0x0471: return L"kr-Latn-NG";
    case 0x0472: return L"om-ET";
    case 0x0473: return L"ti-ET";
    case 0x0474: return L"gn-PY";
    case 0x0475: return L"haw-US";
    case 0x0476: return L"la-VA";
    case 0x0477: return L"so-SO";
    case 0x0478: return L"ii-CN";
    case 0x047A: return L"arn-CL";
    case 0x047C: return L"moh-CA";
    case 0x047E: return L"br-FR";
    case 0x0480: return L"ug-CN";
    case 0x0481: return L"mi-NZ";
    case 0x0482: return L"oc-FR";
    case 0x0483: return L"co-FR";
    case 0x0484: return L"gsw-FR";
    case 0x0485: return L"sah-RU";
    case 0x0486: return L"quc-Latn-GT";
    case 0x0487: return L"rw-RW";
    case 0x0488: return L"wo-SN";
    case 0x048C: return L"prs-AF";
    case 0x0491: return L"gd-GB";
    case 0x0492: return L"ku-Arab-IQ";
    case 0x0501: return L"qps-ploc";
    case 0x05FE: return L"qps-ploca";
    case 0x0801: return L"ar-IQ";
    case 0x0803: return L"ca-ES-valencia";
    case 0x0804: return L"zh-CN";
    case 0x0807: return L"de-CH";
    case 0x0809: return L"en-GB";
    case 0x080A: return L"es-MX";
    case 0x080C: return L"fr-BE";
    case 0x0810: return L"it-CH";
    case 0x0813: return L"nl-BE";
    case 0x0814: return L"nn-NO";
    case 0x0816: return L"pt-PT";
    case 0x0818: return L"ro-MD";
    case 0x0819: return L"ru-MD";
    case 0x081A: return L"sr-Latn-CS";
    case 0x081D: return L"sv-FI";
    case 0x0820: return L"ur-IN";
    case 0x082C: return L"az-Cyrl-AZ";
    case 0x082E: return L"dsb-DE";
    case 0x0832: return L"tn-BW";
    case 0x083B: return L"se-SE";
    case 0x083C: return L"ga-IE";
    case 0x083E: return L"ms-BN";
    case 0x0843: return L"uz-Cyrl-UZ";
    case 0x0845: return L"bn-BD";
    case 0x0846: return L"pa-Arab-PK";
    case 0x0849: return L"ta-LK";
    case 0x0850: return L"mn-Mong-CN";
    case 0x0859: return L"sd-Arab-PK";
    case 0x085D: return L"iu-Latn-CA";
    case 0x085F: return L"tzm-Latn-DZ";
    case 0x0860: return L"ks-Deva-IN";
    case 0x0861: return L"ne-IN";
    case 0x0867: return L"ff-Latn-SN";
    case 0x086B: return L"quz-EC";
    case 0x0873: return L"ti-ER";
    case 0x09FF: return L"qps-plocm";
    case 0x0c01: return L"ar-EG";
    case 0x0C04: return L"zh-HK";
    case 0x0C07: return L"de-AT";
    case 0x0C09: return L"en-AU";
    case 0x0c0A: return L"es-ES";
    case 0x0c0C: return L"fr-CA";
    case 0x0C1A: return L"sr-Cyrl-CS";
    case 0x0C3B: return L"se-FI";
    case 0x0C50: return L"mn-Mong-MN";
    case 0x0C51: return L"dz-BT";
    case 0x0C6B: return L"quz-PE";
    case 0x1000: return L"aa";
    // case 0x1000: return L"aa-DJ";
    // case 0x1000: return L"aa-ER";
    // case 0x1000: return L"aa-ET";
    // case 0x1000: return L"af-NA";
    // case 0x1000: return L"agq";
    // case 0x1000: return L"agq-CM";
    // case 0x1000: return L"ak";
    // case 0x1000: return L"ak-GH";
    // case 0x1000: return L"sq-MK";
    // case 0x1000: return L"gsw-LI";
    // case 0x1000: return L"gsw-CH";
    // case 0x1000: return L"ar-TD";
    // case 0x1000: return L"ar-KM";
    // case 0x1000: return L"ar-DJ";
    // case 0x1000: return L"ar-ER";
    // case 0x1000: return L"ar-IL";
    // case 0x1000: return L"ar-MR";
    // case 0x1000: return L"ar-PS";
    // case 0x1000: return L"ar-SO";
    // case 0x1000: return L"ar-SS";
    // case 0x1000: return L"ar-SD";
    // case 0x1000: return L"ar-001";
    // case 0x1000: return L"ast";
    // case 0x1000: return L"ast-ES";
    // case 0x1000: return L"asa";
    // case 0x1000: return L"asa-TZ";
    // case 0x1000: return L"ksf";
    // case 0x1000: return L"ksf-CM";
    // case 0x1000: return L"bm";
    // case 0x1000: return L"bm-Latn-ML";
    // case 0x1000: return L"bas";
    // case 0x1000: return L"bas-CM";
    // case 0x1000: return L"bem";
    // case 0x1000: return L"bem-ZM";
    // case 0x1000: return L"bez";
    // case 0x1000: return L"bez-TZ";
    // case 0x1000: return L"byn";
    // case 0x1000: return L"byn-ER";
    // case 0x1000: return L"brx";
    // case 0x1000: return L"brx-IN";
    // case 0x1000: return L"ca-AD";
    // case 0x1000: return L"ca-FR";
    // case 0x1000: return L"ca-IT";
    // case 0x1000: return L"ceb";
    // case 0x1000: return L"ceb-Latn";
    // case 0x1000: return L"ceb-Latn-PH";
    // case 0x1000: return L"tzm-Latn-MA";
    // case 0x1000: return L"ccp";
    // case 0x1000: return L"ccp-Cakm";
    // case 0x1000: return L"ccp-Cakm-BD";
    // case 0x1000: return L"ccp-Cakm-IN";
    // case 0x1000: return L"ce-RU";
    // case 0x1000: return L"cgg";
    // case 0x1000: return L"cgg-UG";
    // case 0x1000: return L"cu-RU";
    // case 0x1000: return L"swc";
    // case 0x1000: return L"swc-CD";
    // case 0x1000: return L"kw";
    // case 0x1000: return L"kw-GB";
    // case 0x1000: return L"da-GL";
    // case 0x1000: return L"dua";
    // case 0x1000: return L"dua-CM";
    // case 0x1000: return L"nl-AW";
    // case 0x1000: return L"nl-BQ";
    // case 0x1000: return L"nl-CW";
    // case 0x1000: return L"nl-SX";
    // case 0x1000: return L"nl-SR";
    // case 0x1000: return L"dz";
    // case 0x1000: return L"ebu";
    // case 0x1000: return L"ebu-KE";
    // case 0x1000: return L"en-AS";
    // case 0x1000: return L"en-AI";
    // case 0x1000: return L"en-AG";
    // case 0x1000: return L"en-AT";
    // case 0x1000: return L"en-BS";
    // case 0x1000: return L"en-BB";
    // case 0x1000: return L"en-BE";
    // case 0x1000: return L"en-BM";
    // case 0x1000: return L"en-BW";
    // case 0x1000: return L"en-IO";
    // case 0x1000: return L"en-VG";
    // case 0x1000: return L"en-BI";
    // case 0x1000: return L"en-CM";
    // case 0x1000: return L"en-KY";
    // case 0x1000: return L"en-CX";
    // case 0x1000: return L"en-CC";
    // case 0x1000: return L"en-CK";
    // case 0x1000: return L"en-CY";
    // case 0x1000: return L"en-DK";
    // case 0x1000: return L"en-DM";
    // case 0x1000: return L"en-ER";
    // case 0x1000: return L"en-150";
    // case 0x1000: return L"en-FK";
    // case 0x1000: return L"en-FI";
    // case 0x1000: return L"en-FJ";
    // case 0x1000: return L"en-GM";
    // case 0x1000: return L"en-DE";
    // case 0x1000: return L"en-GH";
    // case 0x1000: return L"en-GI";
    // case 0x1000: return L"en-GD";
    // case 0x1000: return L"en-GU";
    // case 0x1000: return L"en-GG";
    // case 0x1000: return L"en-GY";
    // case 0x1000: return L"en-IM";
    // case 0x1000: return L"en-IL";
    // case 0x1000: return L"en-JE";
    // case 0x1000: return L"en-KE";
    // case 0x1000: return L"en-KI";
    // case 0x1000: return L"en-LS";
    // case 0x1000: return L"en-LR";
    // case 0x1000: return L"en-MO";
    // case 0x1000: return L"en-MG";
    // case 0x1000: return L"en-MW";
    // case 0x1000: return L"en-MT";
    // case 0x1000: return L"en-MH";
    // case 0x1000: return L"en-MU";
    // case 0x1000: return L"en-FM";
    // case 0x1000: return L"en-MS";
    // case 0x1000: return L"en-NA";
    // case 0x1000: return L"en-NR";
    // case 0x1000: return L"en-NL";
    // case 0x1000: return L"en-NG";
    // case 0x1000: return L"en-NU";
    // case 0x1000: return L"en-NF";
    // case 0x1000: return L"en-MP";
    // case 0x1000: return L"en-PK";
    // case 0x1000: return L"en-PW";
    // case 0x1000: return L"en-PG";
    // case 0x1000: return L"en-PN";
    // case 0x1000: return L"en-PR";
    // case 0x1000: return L"en-RW";
    // case 0x1000: return L"en-KN";
    // case 0x1000: return L"en-LC";
    // case 0x1000: return L"en-VC";
    // case 0x1000: return L"en-WS";
    // case 0x1000: return L"en-SC";
    // case 0x1000: return L"en-SL";
    // case 0x1000: return L"en-SX";
    // case 0x1000: return L"en-SI";
    // case 0x1000: return L"en-SB";
    // case 0x1000: return L"en-SS";
    // case 0x1000: return L"en-SH";
    // case 0x1000: return L"en-SD";
    // case 0x1000: return L"en-SZ";
    // case 0x1000: return L"en-SE";
    // case 0x1000: return L"en-CH";
    // case 0x1000: return L"en-TZ";
    // case 0x1000: return L"en-TK";
    // case 0x1000: return L"en-TO";
    // case 0x1000: return L"en-TC";
    // case 0x1000: return L"en-TV";
    // case 0x1000: return L"en-UG";
    // case 0x1000: return L"en-UM";
    // case 0x1000: return L"en-VI";
    // case 0x1000: return L"en-VU";
    // case 0x1000: return L"en-001";
    // case 0x1000: return L"en-ZM";
    // case 0x1000: return L"eo";
    // case 0x1000: return L"eo-001";
    // case 0x1000: return L"ee";
    // case 0x1000: return L"ee-GH";
    // case 0x1000: return L"ee-TG";
    // case 0x1000: return L"ewo";
    // case 0x1000: return L"ewo-CM";
    // case 0x1000: return L"fo-DK";
    // case 0x1000: return L"fr-DZ";
    // case 0x1000: return L"fr-BJ";
    // case 0x1000: return L"fr-BF";
    // case 0x1000: return L"fr-BI";
    // case 0x1000: return L"fr-CF";
    // case 0x1000: return L"fr-TD";
    // case 0x1000: return L"fr-KM";
    // case 0x1000: return L"fr-CG";
    // case 0x1000: return L"fr-DJ";
    // case 0x1000: return L"fr-GQ";
    // case 0x1000: return L"fr-GF";
    // case 0x1000: return L"fr-PF";
    // case 0x1000: return L"fr-GA";
    // case 0x1000: return L"fr-GP";
    // case 0x1000: return L"fr-GN";
    // case 0x1000: return L"fr-MG";
    // case 0x1000: return L"fr-MQ";
    // case 0x1000: return L"fr-MR";
    // case 0x1000: return L"fr-MU";
    // case 0x1000: return L"fr-YT";
    // case 0x1000: return L"fr-NC";
    // case 0x1000: return L"fr-NE";
    // case 0x1000: return L"fr-RW";
    // case 0x1000: return L"fr-BL";
    // case 0x1000: return L"fr-MF";
    // case 0x1000: return L"fr-PM";
    // case 0x1000: return L"fr-SC";
    // case 0x1000: return L"fr-SY";
    // case 0x1000: return L"fr-TG";
    // case 0x1000: return L"fr-TN";
    // case 0x1000: return L"fr-VU";
    // case 0x1000: return L"fr-WF";
    // case 0x1000: return L"fur";
    // case 0x1000: return L"fur-IT";
    // case 0x1000: return L"ff-Latn-BF";
    // case 0x1000: return L"ff-CM";
    // case 0x1000: return L"ff-Latn-CM";
    // case 0x1000: return L"ff-Latn-GM";
    // case 0x1000: return L"ff-Latn-GH";
    // case 0x1000: return L"ff-GN";
    // case 0x1000: return L"ff-Latn-GN";
    // case 0x1000: return L"ff-Latn-GW";
    // case 0x1000: return L"ff-Latn-LR";
    // case 0x1000: return L"ff-MR";
    // case 0x1000: return L"ff-Latn-MR";
    // case 0x1000: return L"ff-Latn-NE";
    // case 0x1000: return L"ff-Latn-SL";
    // case 0x1000: return L"lg";
    // case 0x1000: return L"lg-UG";
    // case 0x1000: return L"de-BE";
    // case 0x1000: return L"de-IT";
    // case 0x1000: return L"el-CY";
    // case 0x1000: return L"guz";
    // case 0x1000: return L"guz-KE";
    // case 0x1000: return L"ha-Latn-GH";
    // case 0x1000: return L"ha-Latn-NE";
    // case 0x1000: return L"ia";
    // case 0x1000: return L"ia-FR";
    // case 0x1000: return L"ia-001";
    // case 0x1000: return L"it-SM";
    // case 0x1000: return L"it-VA";
    // case 0x1000: return L"jv";
    // case 0x1000: return L"jv-Latn";
    // case 0x1000: return L"jv-Latn-ID";
    // case 0x1000: return L"dyo";
    // case 0x1000: return L"dyo-SN";
    // case 0x1000: return L"kea";
    // case 0x1000: return L"kea-CV";
    // case 0x1000: return L"kab";
    // case 0x1000: return L"kab-DZ";
    // case 0x1000: return L"kkj";
    // case 0x1000: return L"kkj-CM";
    // case 0x1000: return L"kln";
    // case 0x1000: return L"kln-KE";
    // case 0x1000: return L"kam";
    // case 0x1000: return L"kam-KE";
    // case 0x1000: return L"ks-Arab-IN";
    // case 0x1000: return L"ki";
    // case 0x1000: return L"ki-KE";
    // case 0x1000: return L"sw-TZ";
    // case 0x1000: return L"sw-UG";
    // case 0x1000: return L"ko-KP";
    // case 0x1000: return L"khq";
    // case 0x1000: return L"khq-ML";
    // case 0x1000: return L"ses";
    // case 0x1000: return L"ses-ML";
    // case 0x1000: return L"nmg";
    // case 0x1000: return L"nmg-CM";
    // case 0x1000: return L"ku-Arab-IR";
    // case 0x1000: return L"lkt";
    // case 0x1000: return L"lkt-US";
    // case 0x1000: return L"lag";
    // case 0x1000: return L"lag-TZ";
    // case 0x1000: return L"ln";
    // case 0x1000: return L"ln-AO";
    // case 0x1000: return L"ln-CF";
    // case 0x1000: return L"ln-CG";
    // case 0x1000: return L"ln-CD";
    // case 0x1000: return L"nds";
    // case 0x1000: return L"nds-DE";
    // case 0x1000: return L"nds-NL";
    // case 0x1000: return L"lu";
    // case 0x1000: return L"lu-CD";
    // case 0x1000: return L"luo";
    // case 0x1000: return L"luo-KE";
    // case 0x1000: return L"luy";
    // case 0x1000: return L"luy-KE";
    // case 0x1000: return L"jmc";
    // case 0x1000: return L"jmc-TZ";
    // case 0x1000: return L"mgh";
    // case 0x1000: return L"mgh-MZ";
    // case 0x1000: return L"kde";
    // case 0x1000: return L"kde-TZ";
    // case 0x1000: return L"mg";
    // case 0x1000: return L"mg-MG";
    // case 0x1000: return L"gv";
    // case 0x1000: return L"gv-IM";
    // case 0x1000: return L"mas";
    // case 0x1000: return L"mas-KE";
    // case 0x1000: return L"mas-TZ";
    // case 0x1000: return L"mzn-IR";
    // case 0x1000: return L"mer";
    // case 0x1000: return L"mer-KE";
    // case 0x1000: return L"mgo";
    // case 0x1000: return L"mgo-CM";
    // case 0x1000: return L"mfe";
    // case 0x1000: return L"mfe-MU";
    // case 0x1000: return L"mua";
    // case 0x1000: return L"mua-CM";
    // case 0x1000: return L"nqo";
    // case 0x1000: return L"nqo-GN";
    // case 0x1000: return L"naq";
    // case 0x1000: return L"naq-NA";
    // case 0x1000: return L"nnh";
    // case 0x1000: return L"nnh-CM";
    // case 0x1000: return L"jgo";
    // case 0x1000: return L"jgo-CM";
    // case 0x1000: return L"lrc-IQ";
    // case 0x1000: return L"lrc-IR";
    // case 0x1000: return L"nd";
    // case 0x1000: return L"nd-ZW";
    // case 0x1000: return L"nb-SJ";
    // case 0x1000: return L"nus";
    // case 0x1000: return L"nus-SD";
    // case 0x1000: return L"nus-SS";
    // case 0x1000: return L"nyn";
    // case 0x1000: return L"nyn-UG";
    // case 0x1000: return L"om-KE";
    // case 0x1000: return L"os";
    // case 0x1000: return L"os-GE";
    // case 0x1000: return L"os-RU";
    // case 0x1000: return L"ps-PK";
    // case 0x1000: return L"fa-AF";
    // case 0x1000: return L"pt-AO";
    // case 0x1000: return L"pt-CV";
    // case 0x1000: return L"pt-GQ";
    // case 0x1000: return L"pt-GW";
    // case 0x1000: return L"pt-LU";
    // case 0x1000: return L"pt-MO";
    // case 0x1000: return L"pt-MZ";
    // case 0x1000: return L"pt-ST";
    // case 0x1000: return L"pt-CH";
    // case 0x1000: return L"pt-TL";
    // case 0x1000: return L"prg-001";
    // case 0x1000: return L"ksh";
    // case 0x1000: return L"ksh-DE";
    // case 0x1000: return L"rof";
    // case 0x1000: return L"rof-TZ";
    // case 0x1000: return L"rn";
    // case 0x1000: return L"rn-BI";
    // case 0x1000: return L"ru-BY";
    // case 0x1000: return L"ru-KZ";
    // case 0x1000: return L"ru-KG";
    // case 0x1000: return L"ru-UA";
    // case 0x1000: return L"rwk";
    // case 0x1000: return L"rwk-TZ";
    // case 0x1000: return L"ssy";
    // case 0x1000: return L"ssy-ER";
    // case 0x1000: return L"saq";
    // case 0x1000: return L"saq-KE";
    // case 0x1000: return L"sg";
    // case 0x1000: return L"sg-CF";
    // case 0x1000: return L"sbp";
    // case 0x1000: return L"sbp-TZ";
    // case 0x1000: return L"seh";
    // case 0x1000: return L"seh-MZ";
    // case 0x1000: return L"ksb";
    // case 0x1000: return L"ksb-TZ";
    // case 0x1000: return L"sn";
    // case 0x1000: return L"sn-Latn";
    // case 0x1000: return L"sn-Latn-ZW";
    // case 0x1000: return L"xog";
    // case 0x1000: return L"xog-UG";
    // case 0x1000: return L"so-DJ";
    // case 0x1000: return L"so-ET";
    // case 0x1000: return L"so-KE";
    // case 0x1000: return L"nr";
    // case 0x1000: return L"nr-ZA";
    // case 0x1000: return L"st-LS";
    // case 0x1000: return L"es-BZ";
    // case 0x1000: return L"es-BR";
    // case 0x1000: return L"es-GQ";
    // case 0x1000: return L"es-PH";
    // case 0x1000: return L"zgh";
    // case 0x1000: return L"zgh-Tfng-MA";
    // case 0x1000: return L"zgh-Tfng";
    // case 0x1000: return L"ss";
    // case 0x1000: return L"ss-ZA";
    // case 0x1000: return L"ss-SZ";
    // case 0x1000: return L"sv-AX";
    // case 0x1000: return L"shi";
    // case 0x1000: return L"shi-Tfng";
    // case 0x1000: return L"shi-Tfng-MA";
    // case 0x1000: return L"shi-Latn";
    // case 0x1000: return L"shi-Latn-MA";
    // case 0x1000: return L"dav";
    // case 0x1000: return L"dav-KE";
    // case 0x1000: return L"ta-MY";
    // case 0x1000: return L"ta-SG";
    // case 0x1000: return L"twq";
    // case 0x1000: return L"twq-NE";
    // case 0x1000: return L"teo";
    // case 0x1000: return L"teo-KE";
    // case 0x1000: return L"teo-UG";
    // case 0x1000: return L"bo-IN";
    // case 0x1000: return L"tig";
    // case 0x1000: return L"tig-ER";
    // case 0x1000: return L"to";
    // case 0x1000: return L"to-TO";
    // case 0x1000: return L"tr-CY";
    // case 0x1000: return L"uz-Arab";
    // case 0x1000: return L"uz-Arab-AF";
    // case 0x1000: return L"vai";
    // case 0x1000: return L"vai-Vaii";
    // case 0x1000: return L"vai-Vaii-LR";
    // case 0x1000: return L"vai-Latn-LR";
    // case 0x1000: return L"vai-Latn";
    // case 0x1000: return L"vo";
    // case 0x1000: return L"vo-001";
    // case 0x1000: return L"vun";
    // case 0x1000: return L"vun-TZ";
    // case 0x1000: return L"wae";
    // case 0x1000: return L"wae-CH";
    // case 0x1000: return L"wal";
    // case 0x1000: return L"wal-ET";
    // case 0x1000: return L"yav";
    // case 0x1000: return L"yav-CM";
    // case 0x1000: return L"yo-BJ";
    // case 0x1000: return L"dje";
    // case 0x1000: return L"dje-NE";
    case 0x1001: return L"ar-LY";
    case 0x1004: return L"zh-SG";
    case 0x1007: return L"de-LU";
    case 0x1009: return L"en-CA";
    case 0x100A: return L"es-GT";
    case 0x100C: return L"fr-CH";
    case 0x101A: return L"hr-BA";
    case 0x103B: return L"smj-NO";
    case 0x1401: return L"ar-DZ";
    case 0x1404: return L"zh-MO";
    case 0x1407: return L"de-LI";
    case 0x1409: return L"en-NZ";
    case 0x140A: return L"es-CR";
    case 0x140C: return L"fr-LU";
    case 0x141A: return L"bs-Latn-BA";
    case 0x143B: return L"smj-SE";
    case 0x1801: return L"ar-MA";
    case 0x1809: return L"en-IE";
    case 0x180A: return L"es-PA";
    case 0x180C: return L"fr-MC";
    case 0x181A: return L"sr-Latn-BA";
    case 0x183B: return L"sma-NO";
    case 0x1C01: return L"ar-TN";
    case 0x1C09: return L"en-ZA";
    case 0x1c0A: return L"es-DO";
    case 0x1C0C: return L"fr-029";
    case 0x1C1A: return L"sr-Cyrl-BA";
    case 0x1C3B: return L"sma-SE";
    case 0x2001: return L"ar-OM";
    case 0x2009: return L"en-JM";
    case 0x200A: return L"es-VE";
    case 0x200C: return L"fr-RE";
    case 0x201A: return L"bs-Cyrl-BA";
    case 0x203B: return L"sms-FI";
    case 0x2401: return L"ar-YE";
    case 0x2409: return L"en-029";
    case 0x240A: return L"es-CO";
    case 0x240C: return L"fr-CD";
    case 0x241A: return L"sr-Latn-RS";
    case 0x243B: return L"smn-FI";
    case 0x2801: return L"ar-SY";
    case 0x2809: return L"en-BZ";
    case 0x280A: return L"es-PE";
    case 0x280C: return L"fr-SN";
    case 0x281A: return L"sr-Cyrl-RS";
    case 0x2C01: return L"ar-JO";
    case 0x2c09: return L"en-TT";
    case 0x2C0A: return L"es-AR";
    case 0x2c0C: return L"fr-CM";
    case 0x2c1A: return L"sr-Latn-ME";
    case 0x3001: return L"ar-LB";
    case 0x3009: return L"en-ZW";
    case 0x300A: return L"es-EC";
    case 0x300C: return L"fr-CI";
    case 0x301A: return L"sr-Cyrl-ME";
    case 0x3401: return L"ar-KW";
    case 0x3409: return L"en-PH";
    case 0x340A: return L"es-CL";
    case 0x340C: return L"fr-ML";
    case 0x3801: return L"ar-AE";
    case 0x380A: return L"es-UY";
    case 0x380C: return L"fr-MA";
    case 0x3C01: return L"ar-BH";
    case 0x3C09: return L"en-HK";
    case 0x3C0A: return L"es-PY";
    case 0x3c0C: return L"fr-HT";
    case 0x4001: return L"ar-QA";
    case 0x4009: return L"en-IN";
    case 0x400A: return L"es-BO";
    case 0x4409: return L"en-MY";
    case 0x440A: return L"es-SV";
    case 0x4809: return L"en-SG";
    case 0x480A: return L"es-HN";
    case 0x4C09: return L"en-AE";
    case 0x4C0A: return L"es-NI";
    case 0x500A: return L"es-PR";
    case 0x540A: return L"es-US";
    case 0x580A: return L"es-419";
    case 0x5c0A: return L"es-CU";
    case 0x641A: return L"bs-Cyrl";
    case 0x681A: return L"bs-Latn";
    case 0x6C1A: return L"sr-Cyrl";
    case 0x701A: return L"sr-Latn";
    case 0x703B: return L"smn";
    case 0x742C: return L"az-Cyrl";
    case 0x743B: return L"sms";
    case 0x7804: return L"zh";
    case 0x7814: return L"nn";
    case 0x781A: return L"bs";
    case 0x782C: return L"az-Latn";
    case 0x783B: return L"sma";
    case 0x7843: return L"uz-Cyrl";
    case 0x7850: return L"mn-Cyrl";
    case 0x785D: return L"iu-Cans";
    case 0x7C04: return L"zh-Hant";
    case 0x7C14: return L"nb";
    case 0x7C1A: return L"sr";
    case 0x7C28: return L"tg-Cyrl";
    case 0x7C2E: return L"dsb";
    case 0x7C3B: return L"smj";
    case 0x7C43: return L"uz-Latn";
    case 0x7C46: return L"pa-Arab";
    case 0x7C50: return L"mn-Mong";
    case 0x7C59: return L"sd-Arab";
    case 0x7c5C: return L"chr-Cher";
    case 0x7C5D: return L"iu-Latn";
    case 0x7C5F: return L"tzm-Latn";
    case 0x7C67: return L"ff-Latn";
    case 0x7C68: return L"ha-Latn";
    case 0x7c92: return L"ku-Arab";
    }
    return NULL;
}

LCID
LdkpGetLCID (
    _In_ PCWSTR LocaleName
    )
{
    if (wcsicmp(LocaleName, L"ar") == 0) return 0x0001;
    if (wcsicmp(LocaleName, L"bg") == 0) return 0x0002;
    if (wcsicmp(LocaleName, L"ca") == 0) return 0x0003;
    if (wcsicmp(LocaleName, L"zh-Hans") == 0) return 0x0004;
    if (wcsicmp(LocaleName, L"cs") == 0) return 0x0005;
    if (wcsicmp(LocaleName, L"da") == 0) return 0x0006;
    if (wcsicmp(LocaleName, L"de") == 0) return 0x0007;
    if (wcsicmp(LocaleName, L"el") == 0) return 0x0008;
    if (wcsicmp(LocaleName, L"en") == 0) return 0x0009;
    if (wcsicmp(LocaleName, L"es") == 0) return 0x000A;
    if (wcsicmp(LocaleName, L"fi") == 0) return 0x000B;
    if (wcsicmp(LocaleName, L"fr") == 0) return 0x000C;
    if (wcsicmp(LocaleName, L"he") == 0) return 0x000D;
    if (wcsicmp(LocaleName, L"hu") == 0) return 0x000E;
    if (wcsicmp(LocaleName, L"is") == 0) return 0x000F;
    if (wcsicmp(LocaleName, L"it") == 0) return 0x0010;
    if (wcsicmp(LocaleName, L"ja") == 0) return 0x0011;
    if (wcsicmp(LocaleName, L"ko") == 0) return 0x0012;
    if (wcsicmp(LocaleName, L"nl") == 0) return 0x0013;
    if (wcsicmp(LocaleName, L"no") == 0) return 0x0014;
    if (wcsicmp(LocaleName, L"pl") == 0) return 0x0015;
    if (wcsicmp(LocaleName, L"pt") == 0) return 0x0016;
    if (wcsicmp(LocaleName, L"rm") == 0) return 0x0017;
    if (wcsicmp(LocaleName, L"ro") == 0) return 0x0018;
    if (wcsicmp(LocaleName, L"ru") == 0) return 0x0019;
    if (wcsicmp(LocaleName, L"hr") == 0) return 0x001A;
    if (wcsicmp(LocaleName, L"sk") == 0) return 0x001B;
    if (wcsicmp(LocaleName, L"sq") == 0) return 0x001C;
    if (wcsicmp(LocaleName, L"sv") == 0) return 0x001D;
    if (wcsicmp(LocaleName, L"th") == 0) return 0x001E;
    if (wcsicmp(LocaleName, L"tr") == 0) return 0x001F;
    if (wcsicmp(LocaleName, L"ur") == 0) return 0x0020;
    if (wcsicmp(LocaleName, L"id") == 0) return 0x0021;
    if (wcsicmp(LocaleName, L"uk") == 0) return 0x0022;
    if (wcsicmp(LocaleName, L"be") == 0) return 0x0023;
    if (wcsicmp(LocaleName, L"sl") == 0) return 0x0024;
    if (wcsicmp(LocaleName, L"et") == 0) return 0x0025;
    if (wcsicmp(LocaleName, L"lv") == 0) return 0x0026;
    if (wcsicmp(LocaleName, L"lt") == 0) return 0x0027;
    if (wcsicmp(LocaleName, L"tg") == 0) return 0x0028;
    if (wcsicmp(LocaleName, L"fa") == 0) return 0x0029;
    if (wcsicmp(LocaleName, L"vi") == 0) return 0x002A;
    if (wcsicmp(LocaleName, L"hy") == 0) return 0x002B;
    if (wcsicmp(LocaleName, L"az") == 0) return 0x002C;
    if (wcsicmp(LocaleName, L"eu") == 0) return 0x002D;
    if (wcsicmp(LocaleName, L"hsb") == 0) return 0x002E;
    if (wcsicmp(LocaleName, L"mk") == 0) return 0x002F;
    if (wcsicmp(LocaleName, L"st") == 0) return 0x0030;
    if (wcsicmp(LocaleName, L"ts") == 0) return 0x0031;
    if (wcsicmp(LocaleName, L"tn") == 0) return 0x0032;
    if (wcsicmp(LocaleName, L"ve") == 0) return 0x0033;
    if (wcsicmp(LocaleName, L"xh") == 0) return 0x0034;
    if (wcsicmp(LocaleName, L"zu") == 0) return 0x0035;
    if (wcsicmp(LocaleName, L"af") == 0) return 0x0036;
    if (wcsicmp(LocaleName, L"ka") == 0) return 0x0037;
    if (wcsicmp(LocaleName, L"fo") == 0) return 0x0038;
    if (wcsicmp(LocaleName, L"hi") == 0) return 0x0039;
    if (wcsicmp(LocaleName, L"mt") == 0) return 0x003A;
    if (wcsicmp(LocaleName, L"se") == 0) return 0x003B;
    if (wcsicmp(LocaleName, L"ga") == 0) return 0x003C;
    if (wcsicmp(LocaleName, L"ms") == 0) return 0x003E;
    if (wcsicmp(LocaleName, L"kk") == 0) return 0x003F;
    if (wcsicmp(LocaleName, L"ky") == 0) return 0x0040;
    if (wcsicmp(LocaleName, L"sw") == 0) return 0x0041;
    if (wcsicmp(LocaleName, L"tk") == 0) return 0x0042;
    if (wcsicmp(LocaleName, L"uz") == 0) return 0x0043;
    if (wcsicmp(LocaleName, L"tt") == 0) return 0x0044;
    if (wcsicmp(LocaleName, L"bn") == 0) return 0x0045;
    if (wcsicmp(LocaleName, L"pa") == 0) return 0x0046;
    if (wcsicmp(LocaleName, L"gu") == 0) return 0x0047;
    if (wcsicmp(LocaleName, L"or") == 0) return 0x0048;
    if (wcsicmp(LocaleName, L"ta") == 0) return 0x0049;
    if (wcsicmp(LocaleName, L"te") == 0) return 0x004A;
    if (wcsicmp(LocaleName, L"kn") == 0) return 0x004B;
    if (wcsicmp(LocaleName, L"ml") == 0) return 0x004C;
    if (wcsicmp(LocaleName, L"as") == 0) return 0x004D;
    if (wcsicmp(LocaleName, L"mr") == 0) return 0x004E;
    if (wcsicmp(LocaleName, L"sa") == 0) return 0x004F;
    if (wcsicmp(LocaleName, L"mn") == 0) return 0x0050;
    if (wcsicmp(LocaleName, L"bo") == 0) return 0x0051;
    if (wcsicmp(LocaleName, L"cy") == 0) return 0x0052;
    if (wcsicmp(LocaleName, L"km") == 0) return 0x0053;
    if (wcsicmp(LocaleName, L"lo") == 0) return 0x0054;
    if (wcsicmp(LocaleName, L"my") == 0) return 0x0055;
    if (wcsicmp(LocaleName, L"gl") == 0) return 0x0056;
    if (wcsicmp(LocaleName, L"kok") == 0) return 0x0057;
    if (wcsicmp(LocaleName, L"sd") == 0) return 0x0059;
    if (wcsicmp(LocaleName, L"syr") == 0) return 0x005A;
    if (wcsicmp(LocaleName, L"si") == 0) return 0x005B;
    if (wcsicmp(LocaleName, L"chr") == 0) return 0x005C;
    if (wcsicmp(LocaleName, L"iu") == 0) return 0x005D;
    if (wcsicmp(LocaleName, L"am") == 0) return 0x005E;
    if (wcsicmp(LocaleName, L"tzm") == 0) return 0x005F;
    if (wcsicmp(LocaleName, L"ks") == 0) return 0x0060;
    if (wcsicmp(LocaleName, L"ne") == 0) return 0x0061;
    if (wcsicmp(LocaleName, L"fy") == 0) return 0x0062;
    if (wcsicmp(LocaleName, L"ps") == 0) return 0x0063;
    if (wcsicmp(LocaleName, L"fil") == 0) return 0x0064;
    if (wcsicmp(LocaleName, L"dv") == 0) return 0x0065;
    if (wcsicmp(LocaleName, L"ff") == 0) return 0x0067;
    if (wcsicmp(LocaleName, L"ha") == 0) return 0x0068;
    if (wcsicmp(LocaleName, L"yo") == 0) return 0x006A;
    if (wcsicmp(LocaleName, L"quz") == 0) return 0x006B;
    if (wcsicmp(LocaleName, L"nso") == 0) return 0x006C;
    if (wcsicmp(LocaleName, L"ba") == 0) return 0x006D;
    if (wcsicmp(LocaleName, L"lb") == 0) return 0x006E;
    if (wcsicmp(LocaleName, L"kl") == 0) return 0x006F;
    if (wcsicmp(LocaleName, L"ig") == 0) return 0x0070;
    if (wcsicmp(LocaleName, L"om") == 0) return 0x0072;
    if (wcsicmp(LocaleName, L"ti") == 0) return 0x0073;
    if (wcsicmp(LocaleName, L"gn") == 0) return 0x0074;
    if (wcsicmp(LocaleName, L"haw") == 0) return 0x0075;
    if (wcsicmp(LocaleName, L"so") == 0) return 0x0077;
    if (wcsicmp(LocaleName, L"ii") == 0) return 0x0078;
    if (wcsicmp(LocaleName, L"arn") == 0) return 0x007A;
    if (wcsicmp(LocaleName, L"moh") == 0) return 0x007C;
    if (wcsicmp(LocaleName, L"br") == 0) return 0x007E;
    if (wcsicmp(LocaleName, L"ug") == 0) return 0x0080;
    if (wcsicmp(LocaleName, L"mi") == 0) return 0x0081;
    if (wcsicmp(LocaleName, L"oc") == 0) return 0x0082;
    if (wcsicmp(LocaleName, L"co") == 0) return 0x0083;
    if (wcsicmp(LocaleName, L"gsw") == 0) return 0x0084;
    if (wcsicmp(LocaleName, L"sah") == 0) return 0x0085;
    if (wcsicmp(LocaleName, L"quc") == 0) return 0x0086;
    if (wcsicmp(LocaleName, L"rw") == 0) return 0x0087;
    if (wcsicmp(LocaleName, L"wo") == 0) return 0x0088;
    if (wcsicmp(LocaleName, L"prs") == 0) return 0x008C;
    if (wcsicmp(LocaleName, L"gd") == 0) return 0x0091;
    if (wcsicmp(LocaleName, L"ku") == 0) return 0x0092;
    if (wcsicmp(LocaleName, L"ar-SA") == 0) return 0x0401;
    if (wcsicmp(LocaleName, L"bg-BG") == 0) return 0x0402;
    if (wcsicmp(LocaleName, L"ca-ES") == 0) return 0x0403;
    if (wcsicmp(LocaleName, L"zh-TW") == 0) return 0x0404;
    if (wcsicmp(LocaleName, L"cs-CZ") == 0) return 0x0405;
    if (wcsicmp(LocaleName, L"da-DK") == 0) return 0x0406;
    if (wcsicmp(LocaleName, L"de-DE") == 0) return 0x0407;
    if (wcsicmp(LocaleName, L"el-GR") == 0) return 0x0408;
    if (wcsicmp(LocaleName, L"en-US") == 0) return 0x0409;
    if (wcsicmp(LocaleName, L"es-ES_tradnl") == 0) return 0x040A;
    if (wcsicmp(LocaleName, L"fi-FI") == 0) return 0x040B;
    if (wcsicmp(LocaleName, L"fr-FR") == 0) return 0x040C;
    if (wcsicmp(LocaleName, L"he-IL") == 0) return 0x040D;
    if (wcsicmp(LocaleName, L"hu-HU") == 0) return 0x040E;
    if (wcsicmp(LocaleName, L"is-IS") == 0) return 0x040F;
    if (wcsicmp(LocaleName, L"it-IT") == 0) return 0x0410;
    if (wcsicmp(LocaleName, L"ja-JP") == 0) return 0x0411;
    if (wcsicmp(LocaleName, L"ko-KR") == 0) return 0x0412;
    if (wcsicmp(LocaleName, L"nl-NL") == 0) return 0x0413;
    if (wcsicmp(LocaleName, L"nb-NO") == 0) return 0x0414;
    if (wcsicmp(LocaleName, L"pl-PL") == 0) return 0x0415;
    if (wcsicmp(LocaleName, L"pt-BR") == 0) return 0x0416;
    if (wcsicmp(LocaleName, L"rm-CH") == 0) return 0x0417;
    if (wcsicmp(LocaleName, L"ro-RO") == 0) return 0x0418;
    if (wcsicmp(LocaleName, L"ru-RU") == 0) return 0x0419;
    if (wcsicmp(LocaleName, L"hr-HR") == 0) return 0x041A;
    if (wcsicmp(LocaleName, L"sk-SK") == 0) return 0x041B;
    if (wcsicmp(LocaleName, L"sq-AL") == 0) return 0x041C;
    if (wcsicmp(LocaleName, L"sv-SE") == 0) return 0x041D;
    if (wcsicmp(LocaleName, L"th-TH") == 0) return 0x041E;
    if (wcsicmp(LocaleName, L"tr-TR") == 0) return 0x041F;
    if (wcsicmp(LocaleName, L"ur-PK") == 0) return 0x0420;
    if (wcsicmp(LocaleName, L"id-ID") == 0) return 0x0421;
    if (wcsicmp(LocaleName, L"uk-UA") == 0) return 0x0422;
    if (wcsicmp(LocaleName, L"be-BY") == 0) return 0x0423;
    if (wcsicmp(LocaleName, L"sl-SI") == 0) return 0x0424;
    if (wcsicmp(LocaleName, L"et-EE") == 0) return 0x0425;
    if (wcsicmp(LocaleName, L"lv-LV") == 0) return 0x0426;
    if (wcsicmp(LocaleName, L"lt-LT") == 0) return 0x0427;
    if (wcsicmp(LocaleName, L"tg-Cyrl-TJ") == 0) return 0x0428;
    if (wcsicmp(LocaleName, L"fa-IR") == 0) return 0x0429;
    if (wcsicmp(LocaleName, L"vi-VN") == 0) return 0x042A;
    if (wcsicmp(LocaleName, L"hy-AM") == 0) return 0x042B;
    if (wcsicmp(LocaleName, L"az-Latn-AZ") == 0) return 0x042C;
    if (wcsicmp(LocaleName, L"eu-ES") == 0) return 0x042D;
    if (wcsicmp(LocaleName, L"hsb-DE") == 0) return 0x042E;
    if (wcsicmp(LocaleName, L"mk-MK") == 0) return 0x042F;
    if (wcsicmp(LocaleName, L"st-ZA") == 0) return 0x0430;
    if (wcsicmp(LocaleName, L"ts-ZA") == 0) return 0x0431;
    if (wcsicmp(LocaleName, L"tn-ZA") == 0) return 0x0432;
    if (wcsicmp(LocaleName, L"ve-ZA") == 0) return 0x0433;
    if (wcsicmp(LocaleName, L"xh-ZA") == 0) return 0x0434;
    if (wcsicmp(LocaleName, L"zu-ZA") == 0) return 0x0435;
    if (wcsicmp(LocaleName, L"af-ZA") == 0) return 0x0436;
    if (wcsicmp(LocaleName, L"ka-GE") == 0) return 0x0437;
    if (wcsicmp(LocaleName, L"fo-FO") == 0) return 0x0438;
    if (wcsicmp(LocaleName, L"hi-IN") == 0) return 0x0439;
    if (wcsicmp(LocaleName, L"mt-MT") == 0) return 0x043A;
    if (wcsicmp(LocaleName, L"se-NO") == 0) return 0x043B;
    if (wcsicmp(LocaleName, L"yi-001") == 0) return 0x043D;
    if (wcsicmp(LocaleName, L"ms-MY") == 0) return 0x043E;
    if (wcsicmp(LocaleName, L"kk-KZ") == 0) return 0x043F;
    if (wcsicmp(LocaleName, L"ky-KG") == 0) return 0x0440;
    if (wcsicmp(LocaleName, L"sw-KE") == 0) return 0x0441;
    if (wcsicmp(LocaleName, L"tk-TM") == 0) return 0x0442;
    if (wcsicmp(LocaleName, L"uz-Latn-UZ") == 0) return 0x0443;
    if (wcsicmp(LocaleName, L"tt-RU") == 0) return 0x0444;
    if (wcsicmp(LocaleName, L"bn-IN") == 0) return 0x0445;
    if (wcsicmp(LocaleName, L"pa-IN") == 0) return 0x0446;
    if (wcsicmp(LocaleName, L"gu-IN") == 0) return 0x0447;
    if (wcsicmp(LocaleName, L"or-IN") == 0) return 0x0448;
    if (wcsicmp(LocaleName, L"ta-IN") == 0) return 0x0449;
    if (wcsicmp(LocaleName, L"te-IN") == 0) return 0x044A;
    if (wcsicmp(LocaleName, L"kn-IN") == 0) return 0x044B;
    if (wcsicmp(LocaleName, L"ml-IN") == 0) return 0x044C;
    if (wcsicmp(LocaleName, L"as-IN") == 0) return 0x044D;
    if (wcsicmp(LocaleName, L"mr-IN") == 0) return 0x044E;
    if (wcsicmp(LocaleName, L"sa-IN") == 0) return 0x044F;
    if (wcsicmp(LocaleName, L"mn-MN") == 0) return 0x0450;
    if (wcsicmp(LocaleName, L"bo-CN") == 0) return 0x0451;
    if (wcsicmp(LocaleName, L"cy-GB") == 0) return 0x0452;
    if (wcsicmp(LocaleName, L"km-KH") == 0) return 0x0453;
    if (wcsicmp(LocaleName, L"lo-LA") == 0) return 0x0454;
    if (wcsicmp(LocaleName, L"my-MM") == 0) return 0x0455;
    if (wcsicmp(LocaleName, L"gl-ES") == 0) return 0x0456;
    if (wcsicmp(LocaleName, L"kok-IN") == 0) return 0x0457;
    if (wcsicmp(LocaleName, L"syr-SY") == 0) return 0x045A;
    if (wcsicmp(LocaleName, L"si-LK") == 0) return 0x045B;
    if (wcsicmp(LocaleName, L"chr-Cher-US") == 0) return 0x045C;
    if (wcsicmp(LocaleName, L"iu-Cans-CA") == 0) return 0x045d;
    if (wcsicmp(LocaleName, L"am-ET") == 0) return 0x045E;
    if (wcsicmp(LocaleName, L"tzm-Arab-MA") == 0) return 0x045F;
    if (wcsicmp(LocaleName, L"ks-Arab") == 0) return 0x0460;
    if (wcsicmp(LocaleName, L"ne-NP") == 0) return 0x0461;
    if (wcsicmp(LocaleName, L"fy-NL") == 0) return 0x0462;
    if (wcsicmp(LocaleName, L"ps-AF") == 0) return 0x0463;
    if (wcsicmp(LocaleName, L"fil-PH") == 0) return 0x0464;
    if (wcsicmp(LocaleName, L"dv-MV") == 0) return 0x0465;
    if (wcsicmp(LocaleName, L"ff-NG") == 0) return 0x0467;
    if (wcsicmp(LocaleName, L"ff-Latn-NG") == 0) return 0x0467;
    if (wcsicmp(LocaleName, L"ha-Latn-NG") == 0) return 0x0468;
    if (wcsicmp(LocaleName, L"yo-NG") == 0) return 0x046A;
    if (wcsicmp(LocaleName, L"quz-BO") == 0) return 0x046B;
    if (wcsicmp(LocaleName, L"nso-ZA") == 0) return 0x046C;
    if (wcsicmp(LocaleName, L"ba-RU") == 0) return 0x046D;
    if (wcsicmp(LocaleName, L"lb-LU") == 0) return 0x046E;
    if (wcsicmp(LocaleName, L"kl-GL") == 0) return 0x046F;
    if (wcsicmp(LocaleName, L"ig-NG") == 0) return 0x0470;
    if (wcsicmp(LocaleName, L"kr-Latn-NG") == 0) return 0x0471;
    if (wcsicmp(LocaleName, L"om-ET") == 0) return 0x0472;
    if (wcsicmp(LocaleName, L"ti-ET") == 0) return 0x0473;
    if (wcsicmp(LocaleName, L"gn-PY") == 0) return 0x0474;
    if (wcsicmp(LocaleName, L"haw-US") == 0) return 0x0475;
    if (wcsicmp(LocaleName, L"la-VA") == 0) return 0x0476;
    if (wcsicmp(LocaleName, L"so-SO") == 0) return 0x0477;
    if (wcsicmp(LocaleName, L"ii-CN") == 0) return 0x0478;
    if (wcsicmp(LocaleName, L"arn-CL") == 0) return 0x047A;
    if (wcsicmp(LocaleName, L"moh-CA") == 0) return 0x047C;
    if (wcsicmp(LocaleName, L"br-FR") == 0) return 0x047E;
    if (wcsicmp(LocaleName, L"ug-CN") == 0) return 0x0480;
    if (wcsicmp(LocaleName, L"mi-NZ") == 0) return 0x0481;
    if (wcsicmp(LocaleName, L"oc-FR") == 0) return 0x0482;
    if (wcsicmp(LocaleName, L"co-FR") == 0) return 0x0483;
    if (wcsicmp(LocaleName, L"gsw-FR") == 0) return 0x0484;
    if (wcsicmp(LocaleName, L"sah-RU") == 0) return 0x0485;
    if (wcsicmp(LocaleName, L"quc-Latn-GT") == 0) return 0x0486;
    if (wcsicmp(LocaleName, L"rw-RW") == 0) return 0x0487;
    if (wcsicmp(LocaleName, L"wo-SN") == 0) return 0x0488;
    if (wcsicmp(LocaleName, L"prs-AF") == 0) return 0x048C;
    if (wcsicmp(LocaleName, L"gd-GB") == 0) return 0x0491;
    if (wcsicmp(LocaleName, L"ku-Arab-IQ") == 0) return 0x0492;
    if (wcsicmp(LocaleName, L"qps-ploc") == 0) return 0x0501;
    if (wcsicmp(LocaleName, L"qps-ploca") == 0) return 0x05FE;
    if (wcsicmp(LocaleName, L"ar-IQ") == 0) return 0x0801;
    if (wcsicmp(LocaleName, L"ca-ES-valencia") == 0) return 0x0803;
    if (wcsicmp(LocaleName, L"zh-CN") == 0) return 0x0804;
    if (wcsicmp(LocaleName, L"de-CH") == 0) return 0x0807;
    if (wcsicmp(LocaleName, L"en-GB") == 0) return 0x0809;
    if (wcsicmp(LocaleName, L"es-MX") == 0) return 0x080A;
    if (wcsicmp(LocaleName, L"fr-BE") == 0) return 0x080C;
    if (wcsicmp(LocaleName, L"it-CH") == 0) return 0x0810;
    if (wcsicmp(LocaleName, L"nl-BE") == 0) return 0x0813;
    if (wcsicmp(LocaleName, L"nn-NO") == 0) return 0x0814;
    if (wcsicmp(LocaleName, L"pt-PT") == 0) return 0x0816;
    if (wcsicmp(LocaleName, L"ro-MD") == 0) return 0x0818;
    if (wcsicmp(LocaleName, L"ru-MD") == 0) return 0x0819;
    if (wcsicmp(LocaleName, L"sr-Latn-CS") == 0) return 0x081A;
    if (wcsicmp(LocaleName, L"sv-FI") == 0) return 0x081D;
    if (wcsicmp(LocaleName, L"ur-IN") == 0) return 0x0820;
    if (wcsicmp(LocaleName, L"az-Cyrl-AZ") == 0) return 0x082C;
    if (wcsicmp(LocaleName, L"dsb-DE") == 0) return 0x082E;
    if (wcsicmp(LocaleName, L"tn-BW") == 0) return 0x0832;
    if (wcsicmp(LocaleName, L"se-SE") == 0) return 0x083B;
    if (wcsicmp(LocaleName, L"ga-IE") == 0) return 0x083C;
    if (wcsicmp(LocaleName, L"ms-BN") == 0) return 0x083E;
    if (wcsicmp(LocaleName, L"uz-Cyrl-UZ") == 0) return 0x0843;
    if (wcsicmp(LocaleName, L"bn-BD") == 0) return 0x0845;
    if (wcsicmp(LocaleName, L"pa-Arab-PK") == 0) return 0x0846;
    if (wcsicmp(LocaleName, L"ta-LK") == 0) return 0x0849;
    if (wcsicmp(LocaleName, L"mn-Mong-CN") == 0) return 0x0850;
    if (wcsicmp(LocaleName, L"sd-Arab-PK") == 0) return 0x0859;
    if (wcsicmp(LocaleName, L"iu-Latn-CA") == 0) return 0x085D;
    if (wcsicmp(LocaleName, L"tzm-Latn-DZ") == 0) return 0x085F;
    if (wcsicmp(LocaleName, L"ks-Deva-IN") == 0) return 0x0860;
    if (wcsicmp(LocaleName, L"ne-IN") == 0) return 0x0861;
    if (wcsicmp(LocaleName, L"ff-Latn-SN") == 0) return 0x0867;
    if (wcsicmp(LocaleName, L"quz-EC") == 0) return 0x086B;
    if (wcsicmp(LocaleName, L"ti-ER") == 0) return 0x0873;
    if (wcsicmp(LocaleName, L"qps-plocm") == 0) return 0x09FF;
    if (wcsicmp(LocaleName, L"ar-EG") == 0) return 0x0c01;
    if (wcsicmp(LocaleName, L"zh-HK") == 0) return 0x0C04;
    if (wcsicmp(LocaleName, L"de-AT") == 0) return 0x0C07;
    if (wcsicmp(LocaleName, L"en-AU") == 0) return 0x0C09;
    if (wcsicmp(LocaleName, L"es-ES") == 0) return 0x0c0A;
    if (wcsicmp(LocaleName, L"fr-CA") == 0) return 0x0c0C;
    if (wcsicmp(LocaleName, L"sr-Cyrl-CS") == 0) return 0x0C1A;
    if (wcsicmp(LocaleName, L"se-FI") == 0) return 0x0C3B;
    if (wcsicmp(LocaleName, L"mn-Mong-MN") == 0) return 0x0C50;
    if (wcsicmp(LocaleName, L"dz-BT") == 0) return 0x0C51;
    if (wcsicmp(LocaleName, L"quz-PE") == 0) return 0x0C6B;
    if (wcsicmp(LocaleName, L"aa") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"aa-DJ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"aa-ER") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"aa-ET") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"af-NA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"agq") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"agq-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ak") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ak-GH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sq-MK") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"gsw-LI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"gsw-CH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-TD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-KM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-DJ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-ER") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-IL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-MR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-PS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-SO") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-SS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-SD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-001") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ast") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ast-ES") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"asa") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"asa-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ksf") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ksf-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bm") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bm-Latn-ML") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bas") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bas-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bem") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bem-ZM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bez") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bez-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"byn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"byn-ER") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"brx") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"brx-IN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ca-AD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ca-FR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ca-IT") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ceb") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ceb-Latn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ceb-Latn-PH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"tzm-Latn-MA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ccp") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ccp-Cakm") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ccp-Cakm-BD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ccp-Cakm-IN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ce-RU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"cgg") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"cgg-UG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"cu-RU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"swc") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"swc-CD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kw") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kw-GB") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"da-GL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dua") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dua-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nl-AW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nl-BQ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nl-CW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nl-SX") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nl-SR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dz") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ebu") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ebu-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-AS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-AI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-AG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-AT") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-BS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-BB") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-BE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-BM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-BW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-IO") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-VG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-BI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-KY") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-CX") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-CC") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-CK") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-CY") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-DK") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-DM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-ER") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-150") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-FK") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-FI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-FJ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-GM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-DE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-GH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-GI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-GD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-GU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-GG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-GY") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-IM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-IL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-JE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-KI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-LS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-LR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-MO") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-MG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-MW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-MT") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-MH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-MU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-FM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-MS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-NA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-NR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-NL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-NG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-NU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-NF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-MP") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-PK") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-PW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-PG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-PN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-PR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-RW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-KN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-LC") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-VC") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-WS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SC") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SX") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SB") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-SE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-CH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-TK") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-TO") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-TC") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-TV") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-UG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-UM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-VI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-VU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-001") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"en-ZM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"eo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"eo-001") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ee") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ee-GH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ee-TG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ewo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ewo-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fo-DK") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-DZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-BJ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-BF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-BI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-CF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-TD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-KM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-CG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-DJ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-GQ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-GF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-PF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-GA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-GP") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-GN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-MG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-MQ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-MR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-MU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-YT") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-NC") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-NE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-RW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-BL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-MF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-PM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-SC") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-SY") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-TG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-TN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-VU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fr-WF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fur") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fur-IT") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-BF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-GM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-GH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-GN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-GN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-GW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-LR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-MR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-MR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-NE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ff-Latn-SL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lg") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lg-UG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"de-BE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"de-IT") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"el-CY") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"guz") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"guz-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ha-Latn-GH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ha-Latn-NE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ia") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ia-FR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ia-001") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"it-SM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"it-VA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"jv") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"jv-Latn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"jv-Latn-ID") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dyo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dyo-SN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kea") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kea-CV") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kab") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kab-DZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kkj") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kkj-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kln") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kln-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kam") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kam-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ks-Arab-IN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ki") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ki-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sw-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sw-UG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ko-KP") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"khq") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"khq-ML") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ses") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ses-ML") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nmg") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nmg-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ku-Arab-IR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lkt") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lkt-US") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lag") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lag-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ln") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ln-AO") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ln-CF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ln-CG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ln-CD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nds") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nds-DE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nds-NL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lu") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lu-CD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"luo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"luo-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"luy") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"luy-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"jmc") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"jmc-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mgh") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mgh-MZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kde") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"kde-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mg") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mg-MG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"gv") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"gv-IM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mas") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mas-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mas-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mzn-IR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mer") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mer-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mgo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mgo-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mfe") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mfe-MU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mua") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"mua-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nqo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nqo-GN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"naq") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"naq-NA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nnh") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nnh-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"jgo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"jgo-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lrc-IQ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"lrc-IR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nd") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nd-ZW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nb-SJ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nus") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nus-SD") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nus-SS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nyn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nyn-UG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"om-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"os") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"os-GE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"os-RU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ps-PK") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"fa-AF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-AO") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-CV") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-GQ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-GW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-LU") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-MO") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-MZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-ST") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-CH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"pt-TL") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"prg-001") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ksh") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ksh-DE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"rof") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"rof-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"rn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"rn-BI") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ru-BY") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ru-KZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ru-KG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ru-UA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"rwk") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"rwk-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ssy") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ssy-ER") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"saq") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"saq-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sg") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sg-CF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sbp") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sbp-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"seh") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"seh-MZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ksb") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ksb-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sn-Latn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sn-Latn-ZW") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"xog") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"xog-UG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"so-DJ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"so-ET") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"so-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nr") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"nr-ZA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"st-LS") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"es-BZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"es-BR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"es-GQ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"es-PH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"zgh") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"zgh-Tfng-MA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"zgh-Tfng") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ss") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ss-ZA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ss-SZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"sv-AX") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"shi") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"shi-Tfng") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"shi-Tfng-MA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"shi-Latn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"shi-Latn-MA") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dav") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dav-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ta-MY") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ta-SG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"twq") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"twq-NE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"teo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"teo-KE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"teo-UG") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"bo-IN") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"tig") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"tig-ER") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"to") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"to-TO") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"tr-CY") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"uz-Arab") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"uz-Arab-AF") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vai") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vai-Vaii") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vai-Vaii-LR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vai-Latn-LR") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vai-Latn") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vo") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vo-001") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vun") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"vun-TZ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"wae") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"wae-CH") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"wal") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"wal-ET") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"yav") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"yav-CM") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"yo-BJ") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dje") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"dje-NE") == 0) return 0x1000;
    if (wcsicmp(LocaleName, L"ar-LY") == 0) return 0x1001;
    if (wcsicmp(LocaleName, L"zh-SG") == 0) return 0x1004;
    if (wcsicmp(LocaleName, L"de-LU") == 0) return 0x1007;
    if (wcsicmp(LocaleName, L"en-CA") == 0) return 0x1009;
    if (wcsicmp(LocaleName, L"es-GT") == 0) return 0x100A;
    if (wcsicmp(LocaleName, L"fr-CH") == 0) return 0x100C;
    if (wcsicmp(LocaleName, L"hr-BA") == 0) return 0x101A;
    if (wcsicmp(LocaleName, L"smj-NO") == 0) return 0x103B;
    if (wcsicmp(LocaleName, L"ar-DZ") == 0) return 0x1401;
    if (wcsicmp(LocaleName, L"zh-MO") == 0) return 0x1404;
    if (wcsicmp(LocaleName, L"de-LI") == 0) return 0x1407;
    if (wcsicmp(LocaleName, L"en-NZ") == 0) return 0x1409;
    if (wcsicmp(LocaleName, L"es-CR") == 0) return 0x140A;
    if (wcsicmp(LocaleName, L"fr-LU") == 0) return 0x140C;
    if (wcsicmp(LocaleName, L"bs-Latn-BA") == 0) return 0x141A;
    if (wcsicmp(LocaleName, L"smj-SE") == 0) return 0x143B;
    if (wcsicmp(LocaleName, L"ar-MA") == 0) return 0x1801;
    if (wcsicmp(LocaleName, L"en-IE") == 0) return 0x1809;
    if (wcsicmp(LocaleName, L"es-PA") == 0) return 0x180A;
    if (wcsicmp(LocaleName, L"fr-MC") == 0) return 0x180C;
    if (wcsicmp(LocaleName, L"sr-Latn-BA") == 0) return 0x181A;
    if (wcsicmp(LocaleName, L"sma-NO") == 0) return 0x183B;
    if (wcsicmp(LocaleName, L"ar-TN") == 0) return 0x1C01;
    if (wcsicmp(LocaleName, L"en-ZA") == 0) return 0x1C09;
    if (wcsicmp(LocaleName, L"es-DO") == 0) return 0x1c0A;
    if (wcsicmp(LocaleName, L"fr-029") == 0) return 0x1C0C;
    if (wcsicmp(LocaleName, L"sr-Cyrl-BA") == 0) return 0x1C1A;
    if (wcsicmp(LocaleName, L"sma-SE") == 0) return 0x1C3B;
    if (wcsicmp(LocaleName, L"ar-OM") == 0) return 0x2001;
    if (wcsicmp(LocaleName, L"en-JM") == 0) return 0x2009;
    if (wcsicmp(LocaleName, L"es-VE") == 0) return 0x200A;
    if (wcsicmp(LocaleName, L"fr-RE") == 0) return 0x200C;
    if (wcsicmp(LocaleName, L"bs-Cyrl-BA") == 0) return 0x201A;
    if (wcsicmp(LocaleName, L"sms-FI") == 0) return 0x203B;
    if (wcsicmp(LocaleName, L"ar-YE") == 0) return 0x2401;
    if (wcsicmp(LocaleName, L"en-029") == 0) return 0x2409;
    if (wcsicmp(LocaleName, L"es-CO") == 0) return 0x240A;
    if (wcsicmp(LocaleName, L"fr-CD") == 0) return 0x240C;
    if (wcsicmp(LocaleName, L"sr-Latn-RS") == 0) return 0x241A;
    if (wcsicmp(LocaleName, L"smn-FI") == 0) return 0x243B;
    if (wcsicmp(LocaleName, L"ar-SY") == 0) return 0x2801;
    if (wcsicmp(LocaleName, L"en-BZ") == 0) return 0x2809;
    if (wcsicmp(LocaleName, L"es-PE") == 0) return 0x280A;
    if (wcsicmp(LocaleName, L"fr-SN") == 0) return 0x280C;
    if (wcsicmp(LocaleName, L"sr-Cyrl-RS") == 0) return 0x281A;
    if (wcsicmp(LocaleName, L"ar-JO") == 0) return 0x2C01;
    if (wcsicmp(LocaleName, L"en-TT") == 0) return 0x2c09;
    if (wcsicmp(LocaleName, L"es-AR") == 0) return 0x2C0A;
    if (wcsicmp(LocaleName, L"fr-CM") == 0) return 0x2c0C;
    if (wcsicmp(LocaleName, L"sr-Latn-ME") == 0) return 0x2c1A;
    if (wcsicmp(LocaleName, L"ar-LB") == 0) return 0x3001;
    if (wcsicmp(LocaleName, L"en-ZW") == 0) return 0x3009;
    if (wcsicmp(LocaleName, L"es-EC") == 0) return 0x300A;
    if (wcsicmp(LocaleName, L"fr-CI") == 0) return 0x300C;
    if (wcsicmp(LocaleName, L"sr-Cyrl-ME") == 0) return 0x301A;
    if (wcsicmp(LocaleName, L"ar-KW") == 0) return 0x3401;
    if (wcsicmp(LocaleName, L"en-PH") == 0) return 0x3409;
    if (wcsicmp(LocaleName, L"es-CL") == 0) return 0x340A;
    if (wcsicmp(LocaleName, L"fr-ML") == 0) return 0x340C;
    if (wcsicmp(LocaleName, L"ar-AE") == 0) return 0x3801;
    if (wcsicmp(LocaleName, L"es-UY") == 0) return 0x380A;
    if (wcsicmp(LocaleName, L"fr-MA") == 0) return 0x380C;
    if (wcsicmp(LocaleName, L"ar-BH") == 0) return 0x3C01;
    if (wcsicmp(LocaleName, L"en-HK") == 0) return 0x3C09;
    if (wcsicmp(LocaleName, L"es-PY") == 0) return 0x3C0A;
    if (wcsicmp(LocaleName, L"fr-HT") == 0) return 0x3c0C;
    if (wcsicmp(LocaleName, L"ar-QA") == 0) return 0x4001;
    if (wcsicmp(LocaleName, L"en-IN") == 0) return 0x4009;
    if (wcsicmp(LocaleName, L"es-BO") == 0) return 0x400A;
    if (wcsicmp(LocaleName, L"en-MY") == 0) return 0x4409;
    if (wcsicmp(LocaleName, L"es-SV") == 0) return 0x440A;
    if (wcsicmp(LocaleName, L"en-SG") == 0) return 0x4809;
    if (wcsicmp(LocaleName, L"es-HN") == 0) return 0x480A;
    if (wcsicmp(LocaleName, L"en-AE") == 0) return 0x4C09;
    if (wcsicmp(LocaleName, L"es-NI") == 0) return 0x4C0A;
    if (wcsicmp(LocaleName, L"es-PR") == 0) return 0x500A;
    if (wcsicmp(LocaleName, L"es-US") == 0) return 0x540A;
    if (wcsicmp(LocaleName, L"es-419") == 0) return 0x580A;
    if (wcsicmp(LocaleName, L"es-CU") == 0) return 0x5c0A;
    if (wcsicmp(LocaleName, L"bs-Cyrl") == 0) return 0x641A;
    if (wcsicmp(LocaleName, L"bs-Latn") == 0) return 0x681A;
    if (wcsicmp(LocaleName, L"sr-Cyrl") == 0) return 0x6C1A;
    if (wcsicmp(LocaleName, L"sr-Latn") == 0) return 0x701A;
    if (wcsicmp(LocaleName, L"smn") == 0) return 0x703B;
    if (wcsicmp(LocaleName, L"az-Cyrl") == 0) return 0x742C;
    if (wcsicmp(LocaleName, L"sms") == 0) return 0x743B;
    if (wcsicmp(LocaleName, L"zh") == 0) return 0x7804;
    if (wcsicmp(LocaleName, L"nn") == 0) return 0x7814;
    if (wcsicmp(LocaleName, L"bs") == 0) return 0x781A;
    if (wcsicmp(LocaleName, L"az-Latn") == 0) return 0x782C;
    if (wcsicmp(LocaleName, L"sma") == 0) return 0x783B;
    if (wcsicmp(LocaleName, L"uz-Cyrl") == 0) return 0x7843;
    if (wcsicmp(LocaleName, L"mn-Cyrl") == 0) return 0x7850;
    if (wcsicmp(LocaleName, L"iu-Cans") == 0) return 0x785D;
    if (wcsicmp(LocaleName, L"zh-Hant") == 0) return 0x7C04;
    if (wcsicmp(LocaleName, L"nb") == 0) return 0x7C14;
    if (wcsicmp(LocaleName, L"sr") == 0) return 0x7C1A;
    if (wcsicmp(LocaleName, L"tg-Cyrl") == 0) return 0x7C28;
    if (wcsicmp(LocaleName, L"dsb") == 0) return 0x7C2E;
    if (wcsicmp(LocaleName, L"smj") == 0) return 0x7C3B;
    if (wcsicmp(LocaleName, L"uz-Latn") == 0) return 0x7C43;
    if (wcsicmp(LocaleName, L"pa-Arab") == 0) return 0x7C46;
    if (wcsicmp(LocaleName, L"mn-Mong") == 0) return 0x7C50;
    if (wcsicmp(LocaleName, L"sd-Arab") == 0) return 0x7C59;
    if (wcsicmp(LocaleName, L"chr-Cher") == 0) return 0x7c5C;
    if (wcsicmp(LocaleName, L"iu-Latn") == 0) return 0x7C5D;
    if (wcsicmp(LocaleName, L"tzm-Latn") == 0) return 0x7C5F;
    if (wcsicmp(LocaleName, L"ff-Latn") == 0) return 0x7C67;
    if (wcsicmp(LocaleName, L"ha-Latn") == 0) return 0x7C68;
    if (wcsicmp(LocaleName, L"ku-Arab") == 0) return 0x7c92;
    return 0;
}

WINBASEAPI
int
WINAPI
LCIDToLocaleName (
    _In_ LCID Locale,
    _Out_writes_opt_(cchName) LPWSTR lpName,
    _In_ int cchName,
    _In_ DWORD dwFlags
    )
{
    UNREFERENCED_PARAMETER(dwFlags);

    if (! IsValidLocale(Locale, 0)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    PCWSTR name = LdkpGetLocaleName(Locale);
    size_t len = wcslen(name) + 1;

    if (cchName < len) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    wcscpy_s(lpName, cchName, name);
    return (int)len;
}

WINBASEAPI
LCID
WINAPI
LocaleNameToLCID (
    _In_ LPCWSTR lpName,
    _In_ DWORD dwFlags
    )
{
    UNREFERENCED_PARAMETER(dwFlags);

    if (lpName == LOCALE_NAME_USER_DEFAULT) {
        LCID lcid;
        ZwQueryDefaultLocale( TRUE,
                              &lcid );
        return lcid;
    } else if (wcsicmp(lpName, LOCALE_NAME_SYSTEM_DEFAULT) == 0) {
        LCID lcid;
        ZwQueryDefaultLocale( FALSE,
                              &lcid );
        return lcid;
    } else if (wcslen(lpName) == 0) { // LOCALE_NAME_INVARIANT
        return 0x0409;
    } else {
        return LdkpGetLCID(lpName);
    }
}