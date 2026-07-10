#include "winbase.h"
#include "../ntdll/ntdll.h"
#include "../peb.h"



NTSTATUS
LdkpInitializeNls (
	VOID
	);

PCWSTR
LdkpGetLocaleName (
    _In_ LCID Locale
    );

VOID
LdkpTerminateNls (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeNls)
#pragma alloc_text(PAGE, GetDateFormatEx)
#pragma alloc_text(PAGE, GetTimeFormatEx)
#endif



LCID LdkpSystemLocale;

#ifndef LOCALE_INVARIANT
#define LOCALE_INVARIANT 0x007f
#endif

extern USHORT *NlsAnsiCodePage;
extern USHORT *NlsOemCodePage;
extern PUSHORT *NlsLeadByteInfo;

#define NLS_CP_ALGORITHM_RANGE    60000

typedef struct _LDK_CODE_PAGE_ENTRY {
    LIST_ENTRY Links;
    UINT CodePage;
    PVOID Data;
    ULONG DataSize;
    CPTABLEINFO TableInfo;
} LDK_CODE_PAGE_ENTRY, *PLDK_CODE_PAGE_ENTRY;

LIST_ENTRY LdkpCodePageListHead;
ERESOURCE LdkpCodePageListResource;
BOOLEAN LdkpCodePageListInitialized;

static
VOID
LdkpAcquireCodePageListExclusive (
    VOID
    )
{
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &LdkpCodePageListResource,
                                    TRUE );
}

static
VOID
LdkpAcquireCodePageListShared (
    VOID
    )
{
    KeEnterCriticalRegion();
    ExAcquireResourceSharedLite( &LdkpCodePageListResource,
                                 TRUE );
}

static
VOID
LdkpReleaseCodePageList (
    VOID
    )
{
    ExReleaseResourceLite( &LdkpCodePageListResource );
    KeLeaveCriticalRegion();
}

static
PLDK_CODE_PAGE_ENTRY
LdkpFindCodePageEntryLocked (
    _In_ UINT CodePage
    )
{
    for (PLIST_ENTRY Link = LdkpCodePageListHead.Flink;
         Link != &LdkpCodePageListHead;
         Link = Link->Flink) {
        PLDK_CODE_PAGE_ENTRY Entry = CONTAINING_RECORD( Link,
                                                        LDK_CODE_PAGE_ENTRY,
                                                        Links );
        if (Entry->CodePage == CodePage) {
            return Entry;
        }
    }

    return NULL;
}

static
BOOLEAN
LdkpAppendString (
    _Inout_ PWSTR *Cursor,
    _Inout_ PSIZE_T Remaining,
    _In_ PCWSTR String
    )
{
    while (*String) {
        if (*Remaining <= 1) {
            return FALSE;
        }

        **Cursor = *String;
        (*Cursor)++;
        (*Remaining)--;
        String++;
    }

    **Cursor = UNICODE_NULL;
    return TRUE;
}

static
BOOLEAN
LdkpAppendNumber (
    _Inout_ PWSTR *Cursor,
    _Inout_ PSIZE_T Remaining,
    _In_ UINT Number
    )
{
    WCHAR Digits[10];
    ULONG Count = 0;

    do {
        Digits[Count++] = (WCHAR)(L'0' + (Number % 10));
        Number /= 10;
    } while (Number != 0);

    if (*Remaining <= Count) {
        return FALSE;
    }

    while (Count != 0) {
        **Cursor = Digits[--Count];
        (*Cursor)++;
        (*Remaining)--;
    }

    **Cursor = UNICODE_NULL;
    return TRUE;
}

static
BOOLEAN
LdkpAppendPaddedNumber (
    _Inout_ PWSTR *Cursor,
    _Inout_ PSIZE_T Remaining,
    _In_ UINT Number,
    _In_ ULONG Width
    )
{
    WCHAR Digits[10];
    ULONG Count = 0;
    ULONG Index;

    do {
        Digits[Count++] = (WCHAR)(L'0' + (Number % 10));
        Number /= 10;
    } while (Number != 0);

    if (Width < Count) {
        Width = Count;
    }

    if (*Remaining <= Width) {
        return FALSE;
    }

    for (Index = Count; Index < Width; Index++) {
        **Cursor = L'0';
        (*Cursor)++;
        (*Remaining)--;
    }

    while (Count != 0) {
        **Cursor = Digits[--Count];
        (*Cursor)++;
        (*Remaining)--;
    }

    **Cursor = UNICODE_NULL;
    return TRUE;
}

static
BOOLEAN
LdkpBuildCodePageFileName (
    _In_ UINT CodePage,
    _Out_writes_(PathCch) PWSTR Path,
    _In_ SIZE_T PathCch
    )
{
    PWSTR Cursor = Path;
    SIZE_T Remaining = PathCch;

    *Path = UNICODE_NULL;

    return LdkpAppendString( &Cursor,
                             &Remaining,
                             L"\\SystemRoot\\System32\\C_" ) &&
           LdkpAppendNumber( &Cursor,
                             &Remaining,
                             CodePage ) &&
           LdkpAppendString( &Cursor,
                             &Remaining,
                             L".NLS" );
}

static
BOOLEAN
LdkpIsMissingCodePageFileStatus (
    _In_ NTSTATUS Status
    )
{
    return Status == STATUS_OBJECT_NAME_NOT_FOUND ||
           Status == STATUS_OBJECT_PATH_NOT_FOUND ||
           Status == STATUS_NO_SUCH_FILE;
}

static
NTSTATUS
LdkpReadCodePageFile (
    _In_ UINT CodePage,
    _Outptr_result_bytebuffer_(*DataSize) PVOID *Data,
    _Out_ PULONG DataSize
    )
{
    WCHAR PathBuffer[64];
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    FILE_STANDARD_INFORMATION StandardInfo;
    LARGE_INTEGER ByteOffset;
    HANDLE FileHandle = NULL;
    PVOID Buffer = NULL;
    NTSTATUS Status;

    PAGED_CODE();

    *Data = NULL;
    *DataSize = 0;

    if (! LdkpBuildCodePageFileName( CodePage,
                                     PathBuffer,
                                     RTL_NUMBER_OF(PathBuffer) )) {
        return STATUS_INVALID_PARAMETER;
    }

    RtlInitUnicodeString( &FileName,
                          PathBuffer );
    InitializeObjectAttributes( &ObjectAttributes,
                                &FileName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    Status = ZwOpenFile( &FileHandle,
                         FILE_READ_DATA | FILE_READ_ATTRIBUTES | SYNCHRONIZE,
                         &ObjectAttributes,
                         &IoStatus,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    try {
        Status = ZwQueryInformationFile( FileHandle,
                                         &IoStatus,
                                         &StandardInfo,
                                         sizeof(StandardInfo),
                                         FileStandardInformation );
        if (! NT_SUCCESS(Status)) {
            leave;
        }

        if (StandardInfo.EndOfFile.HighPart != 0 ||
            StandardInfo.EndOfFile.LowPart < sizeof(USHORT)) {
            Status = STATUS_INVALID_PARAMETER;
            leave;
        }

        Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                  0,
                                  StandardInfo.EndOfFile.LowPart );
        if (Buffer == NULL) {
            Status = STATUS_NO_MEMORY;
            leave;
        }

        ByteOffset.QuadPart = 0;
        Status = ZwReadFile( FileHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatus,
                             Buffer,
                             StandardInfo.EndOfFile.LowPart,
                             &ByteOffset,
                             NULL );
        if (! NT_SUCCESS(Status)) {
            leave;
        }

        *Data = Buffer;
        *DataSize = StandardInfo.EndOfFile.LowPart;
        Buffer = NULL;
    } finally {
        if (Buffer != NULL) {
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         Buffer );
        }

        ZwClose( FileHandle );
    }

    return Status;
}

static
VOID
LdkpFreeCodePageEntry (
    _In_ PLDK_CODE_PAGE_ENTRY Entry
    )
{
    if (Entry->Data != NULL) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     Entry->Data );
    }

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 Entry );
}

static
NTSTATUS
LdkpLoadCodePageEntry (
    _In_ UINT CodePage,
    _Outptr_ PLDK_CODE_PAGE_ENTRY *CodePageEntry
    )
{
    PLDK_CODE_PAGE_ENTRY Entry;
    PVOID Data;
    ULONG DataSize;
    NTSTATUS Status;

    PAGED_CODE();

    *CodePageEntry = NULL;

    Status = LdkpReadCodePageFile( CodePage,
                                   &Data,
                                   &DataSize );
    if (! NT_SUCCESS(Status)) {
        if (LdkpIsMissingCodePageFileStatus( Status )) {
            return STATUS_INVALID_PARAMETER;
        }
        return Status;
    }

    Entry = RtlAllocateHeap( RtlProcessHeap(),
                             HEAP_ZERO_MEMORY,
                             sizeof(*Entry) );
    if (Entry == NULL) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     Data );
        return STATUS_NO_MEMORY;
    }

    Entry->CodePage = CodePage;
    Entry->Data = Data;
    Entry->DataSize = DataSize;

    RtlInitCodePageTable( (PUSHORT)Data,
                          &Entry->TableInfo );
    if (Entry->TableInfo.CodePage != CodePage) {
        LdkpFreeCodePageEntry( Entry );
        return STATUS_INVALID_PARAMETER;
    }

    *CodePageEntry = Entry;
    return STATUS_SUCCESS;
}

static
UINT
LdkpGetMacCodePage (
    VOID
    )
{
    LANGID LangId = LANGIDFROMLCID( LdkpSystemLocale );

    switch (PRIMARYLANGID(LangId)) {
    case LANG_ARABIC:
        return 10004;

    case LANG_CHINESE:
        return SUBLANGID(LangId) == SUBLANG_CHINESE_SIMPLIFIED ? 10008 : 10002;

    case LANG_GREEK:
        return 10006;

    case LANG_HEBREW:
        return 10005;

    case LANG_JAPANESE:
        return 10001;

    case LANG_KOREAN:
        return 10003;

    case LANG_RUSSIAN:
        return 10007;

    case LANG_THAI:
        return 10021;

    default:
        return 10000;
    }
}

UINT
LdkpResolveCodePage (
    _In_ UINT CodePage
    )
{
    switch (CodePage) {
    case CP_ACP:
    case CP_THREAD_ACP:
        return *NlsAnsiCodePage;

    case CP_OEMCP:
        return *NlsOemCodePage;

    case CP_MACCP:
        return LdkpGetMacCodePage();

    default:
        return CodePage;
    }
}

NTSTATUS
LdkpGetCodePageTable (
    _In_ UINT CodePage,
    _Outptr_ PCPTABLEINFO *CodePageTable
    )
{
    PLDK_CODE_PAGE_ENTRY Entry;
    PLDK_CODE_PAGE_ENTRY ExistingEntry;
    NTSTATUS Status;

    PAGED_CODE();

    *CodePageTable = NULL;

    if (! LdkpCodePageListInitialized || CodePage >= NLS_CP_ALGORITHM_RANGE) {
        return STATUS_INVALID_PARAMETER;
    }

    CodePage = LdkpResolveCodePage( CodePage );
    if (CodePage >= NLS_CP_ALGORITHM_RANGE) {
        return STATUS_INVALID_PARAMETER;
    }

    LdkpAcquireCodePageListShared();
    Entry = LdkpFindCodePageEntryLocked( CodePage );
    if (Entry != NULL) {
        *CodePageTable = &Entry->TableInfo;
        LdkpReleaseCodePageList();
        return STATUS_SUCCESS;
    }
    LdkpReleaseCodePageList();

    Status = LdkpLoadCodePageEntry( CodePage,
                                    &Entry );
    if (! NT_SUCCESS(Status)) {
        return Status;
    }

    LdkpAcquireCodePageListExclusive();
    ExistingEntry = LdkpFindCodePageEntryLocked( CodePage );
    if (ExistingEntry != NULL) {
        *CodePageTable = &ExistingEntry->TableInfo;
        LdkpReleaseCodePageList();
        LdkpFreeCodePageEntry( Entry );
        return STATUS_SUCCESS;
    }

    InsertTailList( &LdkpCodePageListHead,
                    &Entry->Links );
    *CodePageTable = &Entry->TableInfo;
    LdkpReleaseCodePageList();
    return STATUS_SUCCESS;
}

VOID
LdkpFillCodePageInfo (
    _In_ PCPTABLEINFO CodePageTable,
    _Out_ LPCPINFO lpCPInfo,
    _In_ BOOL fExVer
    )
{
    lpCPInfo->MaxCharSize = CodePageTable->MaximumCharacterSize;
    if (HIBYTE(CodePageTable->DefaultChar) != 0) {
        lpCPInfo->DefaultChar[0] = HIBYTE(CodePageTable->DefaultChar);
        lpCPInfo->DefaultChar[1] = LOBYTE(CodePageTable->DefaultChar);
    } else {
        lpCPInfo->DefaultChar[0] = LOBYTE(CodePageTable->DefaultChar);
        lpCPInfo->DefaultChar[1] = 0;
    }

    RtlCopyMemory( lpCPInfo->LeadByte,
                   CodePageTable->LeadByte,
                   RTL_NUMBER_OF(lpCPInfo->LeadByte) );

    if (fExVer) {
        LPCPINFOEXW lpCPInfoEx = (LPCPINFOEXW)lpCPInfo;
        lpCPInfoEx->UnicodeDefaultChar = CodePageTable->UniDefaultChar;
        lpCPInfoEx->CodePage = CodePageTable->CodePage;
    }
}

VOID
LdkpFormatCodePageName (
    _In_ UINT CodePage,
    _Out_writes_(cchBuffer) LPWSTR Buffer,
    _In_ int cchBuffer
    )
{
    PWSTR Cursor = Buffer;
    SIZE_T Remaining = (SIZE_T)cchBuffer;

    *Buffer = UNICODE_NULL;

    if (CodePage == CP_UTF7) {
        LdkpAppendString( &Cursor,
                          &Remaining,
                          L"65000 (UTF-7)" );
        return;
    }

    if (CodePage == CP_UTF8) {
        LdkpAppendString( &Cursor,
                          &Remaining,
                          L"65001 (UTF-8)" );
        return;
    }

    LdkpAppendNumber( &Cursor,
                      &Remaining,
                      CodePage );
    LdkpAppendString( &Cursor,
                      &Remaining,
                      L"   (Code Page " );
    LdkpAppendNumber( &Cursor,
                      &Remaining,
                      CodePage );
    LdkpAppendString( &Cursor,
                      &Remaining,
                      L")" );
}

NTSTATUS
LdkpInitializeNls (
	VOID
	)
{
	NTSTATUS Status;

    PAGED_CODE();

    InitializeListHead( &LdkpCodePageListHead );
    Status = ExInitializeResourceLite( &LdkpCodePageListResource );
    if (! NT_SUCCESS(Status)) {
        return Status;
    }
    LdkpCodePageListInitialized = TRUE;

	Status = ZwQueryDefaultLocale( FALSE,
                                   &LdkpSystemLocale );
    if (! NT_SUCCESS(Status)) {
        LdkpTerminateNls();
    }
	return Status;
}

VOID
LdkpTerminateNls (
    VOID
    )
{
    PAGED_CODE();

    if (! LdkpCodePageListInitialized) {
        return;
    }

    LdkpAcquireCodePageListExclusive();
    while (! IsListEmpty( &LdkpCodePageListHead )) {
        PLIST_ENTRY Link = RemoveHeadList( &LdkpCodePageListHead );
        PLDK_CODE_PAGE_ENTRY Entry = CONTAINING_RECORD( Link,
                                                        LDK_CODE_PAGE_ENTRY,
                                                        Links );
        LdkpFreeCodePageEntry( Entry );
    }
    LdkpReleaseCodePageList();

    ExDeleteResourceLite( &LdkpCodePageListResource );
    LdkpCodePageListInitialized = FALSE;
}

WINBASEAPI
BOOL
WINAPI
IsValidCodePage (
    _In_ UINT CodePage
    )
{
    PCPTABLEINFO CodePageTable;

    if (CodePage == CP_ACP ||
        CodePage == CP_OEMCP ||
        CodePage == CP_MACCP ||
        CodePage == CP_THREAD_ACP) {
        return FALSE;
    }

    if (CodePage == CP_UTF7 || CodePage == CP_UTF8) {
        return TRUE;
    }

    return NT_SUCCESS(LdkpGetCodePageTable( CodePage,
                                            &CodePageTable ));
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

WINBASEAPI
BOOL
WINAPI
GetCPInfo (
    _In_ UINT CodePage,
    _Out_ LPCPINFO lpCPInfo
    )
{
    PCPTABLEINFO CodePageTable;
    NTSTATUS Status;

    if (lpCPInfo == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    CodePage = LdkpResolveCodePage( CodePage );

    if (CodePage >= NLS_CP_ALGORITHM_RANGE) {
        return UTFCPInfo( CodePage,
                          lpCPInfo,
                          FALSE );
    }

    Status = LdkpGetCodePageTable( CodePage,
                                   &CodePageTable );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    LdkpFillCodePageInfo( CodePageTable,
                          lpCPInfo,
                          FALSE );
    return TRUE;
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

    if (lpCPInfoEx == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

	bSuccess = GetCPInfoExW( CodePage,
                             dwFlags,
                             &CPInfoExW );
    if (! bSuccess) {
        return FALSE;
    }

    UNICODE_STRING Unicode;
	ANSI_STRING Ansi;

    RtlMoveMemory( lpCPInfoEx,
                   &CPInfoExW,
                   FIELD_OFFSET(CPINFOEXW, CodePageName) );

    RtlInitUnicodeString( &Unicode,
                          CPInfoExW.CodePageName );

    Ansi.Length = 0;
    Ansi.MaximumLength = sizeof(lpCPInfoEx->CodePageName);
    Ansi.Buffer = lpCPInfoEx->CodePageName;

    NTSTATUS Status = LdkUnicodeStringToAnsiString( &Ansi,
                                                    &Unicode,
                                                    FALSE );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

	return TRUE;
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
    UNREFERENCED_PARAMETER( UILangId );
    UNREFERENCED_PARAMETER( WhichString );

    WCHAR Name[MAX_PATH];

    LdkpFormatCodePageName( ResourceID,
                            Name,
                            RTL_NUMBER_OF(Name) );

    int Length = (int)wcslen( Name ) + 1;
    if (cchBuffer < Length) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    wcscpy_s( pBuffer,
              cchBuffer,
              Name );
    return Length;
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
    PCPTABLEINFO CodePageTable;
    NTSTATUS Status;

    if (lpCPInfoEx == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    CodePage = LdkpResolveCodePage( CodePage );

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

    Status = LdkpGetCodePageTable( CodePage,
                                   &CodePageTable );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    LdkpFillCodePageInfo( CodePageTable,
                          (LPCPINFO)lpCPInfoEx,
                          TRUE );
    return GetStringTableEntry( CodePageTable->CodePage,
                                0,
                                lpCPInfoEx->CodePageName,
                                MAX_PATH,
                                RC_CODE_PAGE_NAME );
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

    if (IS_INVALID_LOCALE(Locale)) {
        return FALSE;
    }

    return LdkpGetLocaleName( Locale ) != NULL;
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
    case LOCALE_INVARIANT: return L"";
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
    if (_wcsicmp(LocaleName, L"ar") == 0) return 0x0001;
    if (_wcsicmp(LocaleName, L"bg") == 0) return 0x0002;
    if (_wcsicmp(LocaleName, L"ca") == 0) return 0x0003;
    if (_wcsicmp(LocaleName, L"zh-Hans") == 0) return 0x0004;
    if (_wcsicmp(LocaleName, L"cs") == 0) return 0x0005;
    if (_wcsicmp(LocaleName, L"da") == 0) return 0x0006;
    if (_wcsicmp(LocaleName, L"de") == 0) return 0x0007;
    if (_wcsicmp(LocaleName, L"el") == 0) return 0x0008;
    if (_wcsicmp(LocaleName, L"en") == 0) return 0x0009;
    if (_wcsicmp(LocaleName, L"es") == 0) return 0x000A;
    if (_wcsicmp(LocaleName, L"fi") == 0) return 0x000B;
    if (_wcsicmp(LocaleName, L"fr") == 0) return 0x000C;
    if (_wcsicmp(LocaleName, L"he") == 0) return 0x000D;
    if (_wcsicmp(LocaleName, L"hu") == 0) return 0x000E;
    if (_wcsicmp(LocaleName, L"is") == 0) return 0x000F;
    if (_wcsicmp(LocaleName, L"it") == 0) return 0x0010;
    if (_wcsicmp(LocaleName, L"ja") == 0) return 0x0011;
    if (_wcsicmp(LocaleName, L"ko") == 0) return 0x0012;
    if (_wcsicmp(LocaleName, L"nl") == 0) return 0x0013;
    if (_wcsicmp(LocaleName, L"no") == 0) return 0x0014;
    if (_wcsicmp(LocaleName, L"pl") == 0) return 0x0015;
    if (_wcsicmp(LocaleName, L"pt") == 0) return 0x0016;
    if (_wcsicmp(LocaleName, L"rm") == 0) return 0x0017;
    if (_wcsicmp(LocaleName, L"ro") == 0) return 0x0018;
    if (_wcsicmp(LocaleName, L"ru") == 0) return 0x0019;
    if (_wcsicmp(LocaleName, L"hr") == 0) return 0x001A;
    if (_wcsicmp(LocaleName, L"sk") == 0) return 0x001B;
    if (_wcsicmp(LocaleName, L"sq") == 0) return 0x001C;
    if (_wcsicmp(LocaleName, L"sv") == 0) return 0x001D;
    if (_wcsicmp(LocaleName, L"th") == 0) return 0x001E;
    if (_wcsicmp(LocaleName, L"tr") == 0) return 0x001F;
    if (_wcsicmp(LocaleName, L"ur") == 0) return 0x0020;
    if (_wcsicmp(LocaleName, L"id") == 0) return 0x0021;
    if (_wcsicmp(LocaleName, L"uk") == 0) return 0x0022;
    if (_wcsicmp(LocaleName, L"be") == 0) return 0x0023;
    if (_wcsicmp(LocaleName, L"sl") == 0) return 0x0024;
    if (_wcsicmp(LocaleName, L"et") == 0) return 0x0025;
    if (_wcsicmp(LocaleName, L"lv") == 0) return 0x0026;
    if (_wcsicmp(LocaleName, L"lt") == 0) return 0x0027;
    if (_wcsicmp(LocaleName, L"tg") == 0) return 0x0028;
    if (_wcsicmp(LocaleName, L"fa") == 0) return 0x0029;
    if (_wcsicmp(LocaleName, L"vi") == 0) return 0x002A;
    if (_wcsicmp(LocaleName, L"hy") == 0) return 0x002B;
    if (_wcsicmp(LocaleName, L"az") == 0) return 0x002C;
    if (_wcsicmp(LocaleName, L"eu") == 0) return 0x002D;
    if (_wcsicmp(LocaleName, L"hsb") == 0) return 0x002E;
    if (_wcsicmp(LocaleName, L"mk") == 0) return 0x002F;
    if (_wcsicmp(LocaleName, L"st") == 0) return 0x0030;
    if (_wcsicmp(LocaleName, L"ts") == 0) return 0x0031;
    if (_wcsicmp(LocaleName, L"tn") == 0) return 0x0032;
    if (_wcsicmp(LocaleName, L"ve") == 0) return 0x0033;
    if (_wcsicmp(LocaleName, L"xh") == 0) return 0x0034;
    if (_wcsicmp(LocaleName, L"zu") == 0) return 0x0035;
    if (_wcsicmp(LocaleName, L"af") == 0) return 0x0036;
    if (_wcsicmp(LocaleName, L"ka") == 0) return 0x0037;
    if (_wcsicmp(LocaleName, L"fo") == 0) return 0x0038;
    if (_wcsicmp(LocaleName, L"hi") == 0) return 0x0039;
    if (_wcsicmp(LocaleName, L"mt") == 0) return 0x003A;
    if (_wcsicmp(LocaleName, L"se") == 0) return 0x003B;
    if (_wcsicmp(LocaleName, L"ga") == 0) return 0x003C;
    if (_wcsicmp(LocaleName, L"ms") == 0) return 0x003E;
    if (_wcsicmp(LocaleName, L"kk") == 0) return 0x003F;
    if (_wcsicmp(LocaleName, L"ky") == 0) return 0x0040;
    if (_wcsicmp(LocaleName, L"sw") == 0) return 0x0041;
    if (_wcsicmp(LocaleName, L"tk") == 0) return 0x0042;
    if (_wcsicmp(LocaleName, L"uz") == 0) return 0x0043;
    if (_wcsicmp(LocaleName, L"tt") == 0) return 0x0044;
    if (_wcsicmp(LocaleName, L"bn") == 0) return 0x0045;
    if (_wcsicmp(LocaleName, L"pa") == 0) return 0x0046;
    if (_wcsicmp(LocaleName, L"gu") == 0) return 0x0047;
    if (_wcsicmp(LocaleName, L"or") == 0) return 0x0048;
    if (_wcsicmp(LocaleName, L"ta") == 0) return 0x0049;
    if (_wcsicmp(LocaleName, L"te") == 0) return 0x004A;
    if (_wcsicmp(LocaleName, L"kn") == 0) return 0x004B;
    if (_wcsicmp(LocaleName, L"ml") == 0) return 0x004C;
    if (_wcsicmp(LocaleName, L"as") == 0) return 0x004D;
    if (_wcsicmp(LocaleName, L"mr") == 0) return 0x004E;
    if (_wcsicmp(LocaleName, L"sa") == 0) return 0x004F;
    if (_wcsicmp(LocaleName, L"mn") == 0) return 0x0050;
    if (_wcsicmp(LocaleName, L"bo") == 0) return 0x0051;
    if (_wcsicmp(LocaleName, L"cy") == 0) return 0x0052;
    if (_wcsicmp(LocaleName, L"km") == 0) return 0x0053;
    if (_wcsicmp(LocaleName, L"lo") == 0) return 0x0054;
    if (_wcsicmp(LocaleName, L"my") == 0) return 0x0055;
    if (_wcsicmp(LocaleName, L"gl") == 0) return 0x0056;
    if (_wcsicmp(LocaleName, L"kok") == 0) return 0x0057;
    if (_wcsicmp(LocaleName, L"sd") == 0) return 0x0059;
    if (_wcsicmp(LocaleName, L"syr") == 0) return 0x005A;
    if (_wcsicmp(LocaleName, L"si") == 0) return 0x005B;
    if (_wcsicmp(LocaleName, L"chr") == 0) return 0x005C;
    if (_wcsicmp(LocaleName, L"iu") == 0) return 0x005D;
    if (_wcsicmp(LocaleName, L"am") == 0) return 0x005E;
    if (_wcsicmp(LocaleName, L"tzm") == 0) return 0x005F;
    if (_wcsicmp(LocaleName, L"ks") == 0) return 0x0060;
    if (_wcsicmp(LocaleName, L"ne") == 0) return 0x0061;
    if (_wcsicmp(LocaleName, L"fy") == 0) return 0x0062;
    if (_wcsicmp(LocaleName, L"ps") == 0) return 0x0063;
    if (_wcsicmp(LocaleName, L"fil") == 0) return 0x0064;
    if (_wcsicmp(LocaleName, L"dv") == 0) return 0x0065;
    if (_wcsicmp(LocaleName, L"ff") == 0) return 0x0067;
    if (_wcsicmp(LocaleName, L"ha") == 0) return 0x0068;
    if (_wcsicmp(LocaleName, L"yo") == 0) return 0x006A;
    if (_wcsicmp(LocaleName, L"quz") == 0) return 0x006B;
    if (_wcsicmp(LocaleName, L"nso") == 0) return 0x006C;
    if (_wcsicmp(LocaleName, L"ba") == 0) return 0x006D;
    if (_wcsicmp(LocaleName, L"lb") == 0) return 0x006E;
    if (_wcsicmp(LocaleName, L"kl") == 0) return 0x006F;
    if (_wcsicmp(LocaleName, L"ig") == 0) return 0x0070;
    if (_wcsicmp(LocaleName, L"om") == 0) return 0x0072;
    if (_wcsicmp(LocaleName, L"ti") == 0) return 0x0073;
    if (_wcsicmp(LocaleName, L"gn") == 0) return 0x0074;
    if (_wcsicmp(LocaleName, L"haw") == 0) return 0x0075;
    if (_wcsicmp(LocaleName, L"so") == 0) return 0x0077;
    if (_wcsicmp(LocaleName, L"ii") == 0) return 0x0078;
    if (_wcsicmp(LocaleName, L"arn") == 0) return 0x007A;
    if (_wcsicmp(LocaleName, L"moh") == 0) return 0x007C;
    if (_wcsicmp(LocaleName, L"br") == 0) return 0x007E;
    if (_wcsicmp(LocaleName, L"ug") == 0) return 0x0080;
    if (_wcsicmp(LocaleName, L"mi") == 0) return 0x0081;
    if (_wcsicmp(LocaleName, L"oc") == 0) return 0x0082;
    if (_wcsicmp(LocaleName, L"co") == 0) return 0x0083;
    if (_wcsicmp(LocaleName, L"gsw") == 0) return 0x0084;
    if (_wcsicmp(LocaleName, L"sah") == 0) return 0x0085;
    if (_wcsicmp(LocaleName, L"quc") == 0) return 0x0086;
    if (_wcsicmp(LocaleName, L"rw") == 0) return 0x0087;
    if (_wcsicmp(LocaleName, L"wo") == 0) return 0x0088;
    if (_wcsicmp(LocaleName, L"prs") == 0) return 0x008C;
    if (_wcsicmp(LocaleName, L"gd") == 0) return 0x0091;
    if (_wcsicmp(LocaleName, L"ku") == 0) return 0x0092;
    if (_wcsicmp(LocaleName, L"ar-SA") == 0) return 0x0401;
    if (_wcsicmp(LocaleName, L"bg-BG") == 0) return 0x0402;
    if (_wcsicmp(LocaleName, L"ca-ES") == 0) return 0x0403;
    if (_wcsicmp(LocaleName, L"zh-TW") == 0) return 0x0404;
    if (_wcsicmp(LocaleName, L"cs-CZ") == 0) return 0x0405;
    if (_wcsicmp(LocaleName, L"da-DK") == 0) return 0x0406;
    if (_wcsicmp(LocaleName, L"de-DE") == 0) return 0x0407;
    if (_wcsicmp(LocaleName, L"el-GR") == 0) return 0x0408;
    if (_wcsicmp(LocaleName, L"en-US") == 0) return 0x0409;
    if (_wcsicmp(LocaleName, L"es-ES_tradnl") == 0) return 0x040A;
    if (_wcsicmp(LocaleName, L"fi-FI") == 0) return 0x040B;
    if (_wcsicmp(LocaleName, L"fr-FR") == 0) return 0x040C;
    if (_wcsicmp(LocaleName, L"he-IL") == 0) return 0x040D;
    if (_wcsicmp(LocaleName, L"hu-HU") == 0) return 0x040E;
    if (_wcsicmp(LocaleName, L"is-IS") == 0) return 0x040F;
    if (_wcsicmp(LocaleName, L"it-IT") == 0) return 0x0410;
    if (_wcsicmp(LocaleName, L"ja-JP") == 0) return 0x0411;
    if (_wcsicmp(LocaleName, L"ko-KR") == 0) return 0x0412;
    if (_wcsicmp(LocaleName, L"nl-NL") == 0) return 0x0413;
    if (_wcsicmp(LocaleName, L"nb-NO") == 0) return 0x0414;
    if (_wcsicmp(LocaleName, L"pl-PL") == 0) return 0x0415;
    if (_wcsicmp(LocaleName, L"pt-BR") == 0) return 0x0416;
    if (_wcsicmp(LocaleName, L"rm-CH") == 0) return 0x0417;
    if (_wcsicmp(LocaleName, L"ro-RO") == 0) return 0x0418;
    if (_wcsicmp(LocaleName, L"ru-RU") == 0) return 0x0419;
    if (_wcsicmp(LocaleName, L"hr-HR") == 0) return 0x041A;
    if (_wcsicmp(LocaleName, L"sk-SK") == 0) return 0x041B;
    if (_wcsicmp(LocaleName, L"sq-AL") == 0) return 0x041C;
    if (_wcsicmp(LocaleName, L"sv-SE") == 0) return 0x041D;
    if (_wcsicmp(LocaleName, L"th-TH") == 0) return 0x041E;
    if (_wcsicmp(LocaleName, L"tr-TR") == 0) return 0x041F;
    if (_wcsicmp(LocaleName, L"ur-PK") == 0) return 0x0420;
    if (_wcsicmp(LocaleName, L"id-ID") == 0) return 0x0421;
    if (_wcsicmp(LocaleName, L"uk-UA") == 0) return 0x0422;
    if (_wcsicmp(LocaleName, L"be-BY") == 0) return 0x0423;
    if (_wcsicmp(LocaleName, L"sl-SI") == 0) return 0x0424;
    if (_wcsicmp(LocaleName, L"et-EE") == 0) return 0x0425;
    if (_wcsicmp(LocaleName, L"lv-LV") == 0) return 0x0426;
    if (_wcsicmp(LocaleName, L"lt-LT") == 0) return 0x0427;
    if (_wcsicmp(LocaleName, L"tg-Cyrl-TJ") == 0) return 0x0428;
    if (_wcsicmp(LocaleName, L"fa-IR") == 0) return 0x0429;
    if (_wcsicmp(LocaleName, L"vi-VN") == 0) return 0x042A;
    if (_wcsicmp(LocaleName, L"hy-AM") == 0) return 0x042B;
    if (_wcsicmp(LocaleName, L"az-Latn-AZ") == 0) return 0x042C;
    if (_wcsicmp(LocaleName, L"eu-ES") == 0) return 0x042D;
    if (_wcsicmp(LocaleName, L"hsb-DE") == 0) return 0x042E;
    if (_wcsicmp(LocaleName, L"mk-MK") == 0) return 0x042F;
    if (_wcsicmp(LocaleName, L"st-ZA") == 0) return 0x0430;
    if (_wcsicmp(LocaleName, L"ts-ZA") == 0) return 0x0431;
    if (_wcsicmp(LocaleName, L"tn-ZA") == 0) return 0x0432;
    if (_wcsicmp(LocaleName, L"ve-ZA") == 0) return 0x0433;
    if (_wcsicmp(LocaleName, L"xh-ZA") == 0) return 0x0434;
    if (_wcsicmp(LocaleName, L"zu-ZA") == 0) return 0x0435;
    if (_wcsicmp(LocaleName, L"af-ZA") == 0) return 0x0436;
    if (_wcsicmp(LocaleName, L"ka-GE") == 0) return 0x0437;
    if (_wcsicmp(LocaleName, L"fo-FO") == 0) return 0x0438;
    if (_wcsicmp(LocaleName, L"hi-IN") == 0) return 0x0439;
    if (_wcsicmp(LocaleName, L"mt-MT") == 0) return 0x043A;
    if (_wcsicmp(LocaleName, L"se-NO") == 0) return 0x043B;
    if (_wcsicmp(LocaleName, L"yi-001") == 0) return 0x043D;
    if (_wcsicmp(LocaleName, L"ms-MY") == 0) return 0x043E;
    if (_wcsicmp(LocaleName, L"kk-KZ") == 0) return 0x043F;
    if (_wcsicmp(LocaleName, L"ky-KG") == 0) return 0x0440;
    if (_wcsicmp(LocaleName, L"sw-KE") == 0) return 0x0441;
    if (_wcsicmp(LocaleName, L"tk-TM") == 0) return 0x0442;
    if (_wcsicmp(LocaleName, L"uz-Latn-UZ") == 0) return 0x0443;
    if (_wcsicmp(LocaleName, L"tt-RU") == 0) return 0x0444;
    if (_wcsicmp(LocaleName, L"bn-IN") == 0) return 0x0445;
    if (_wcsicmp(LocaleName, L"pa-IN") == 0) return 0x0446;
    if (_wcsicmp(LocaleName, L"gu-IN") == 0) return 0x0447;
    if (_wcsicmp(LocaleName, L"or-IN") == 0) return 0x0448;
    if (_wcsicmp(LocaleName, L"ta-IN") == 0) return 0x0449;
    if (_wcsicmp(LocaleName, L"te-IN") == 0) return 0x044A;
    if (_wcsicmp(LocaleName, L"kn-IN") == 0) return 0x044B;
    if (_wcsicmp(LocaleName, L"ml-IN") == 0) return 0x044C;
    if (_wcsicmp(LocaleName, L"as-IN") == 0) return 0x044D;
    if (_wcsicmp(LocaleName, L"mr-IN") == 0) return 0x044E;
    if (_wcsicmp(LocaleName, L"sa-IN") == 0) return 0x044F;
    if (_wcsicmp(LocaleName, L"mn-MN") == 0) return 0x0450;
    if (_wcsicmp(LocaleName, L"bo-CN") == 0) return 0x0451;
    if (_wcsicmp(LocaleName, L"cy-GB") == 0) return 0x0452;
    if (_wcsicmp(LocaleName, L"km-KH") == 0) return 0x0453;
    if (_wcsicmp(LocaleName, L"lo-LA") == 0) return 0x0454;
    if (_wcsicmp(LocaleName, L"my-MM") == 0) return 0x0455;
    if (_wcsicmp(LocaleName, L"gl-ES") == 0) return 0x0456;
    if (_wcsicmp(LocaleName, L"kok-IN") == 0) return 0x0457;
    if (_wcsicmp(LocaleName, L"syr-SY") == 0) return 0x045A;
    if (_wcsicmp(LocaleName, L"si-LK") == 0) return 0x045B;
    if (_wcsicmp(LocaleName, L"chr-Cher-US") == 0) return 0x045C;
    if (_wcsicmp(LocaleName, L"iu-Cans-CA") == 0) return 0x045d;
    if (_wcsicmp(LocaleName, L"am-ET") == 0) return 0x045E;
    if (_wcsicmp(LocaleName, L"tzm-Arab-MA") == 0) return 0x045F;
    if (_wcsicmp(LocaleName, L"ks-Arab") == 0) return 0x0460;
    if (_wcsicmp(LocaleName, L"ne-NP") == 0) return 0x0461;
    if (_wcsicmp(LocaleName, L"fy-NL") == 0) return 0x0462;
    if (_wcsicmp(LocaleName, L"ps-AF") == 0) return 0x0463;
    if (_wcsicmp(LocaleName, L"fil-PH") == 0) return 0x0464;
    if (_wcsicmp(LocaleName, L"dv-MV") == 0) return 0x0465;
    if (_wcsicmp(LocaleName, L"ff-NG") == 0) return 0x0467;
    if (_wcsicmp(LocaleName, L"ff-Latn-NG") == 0) return 0x0467;
    if (_wcsicmp(LocaleName, L"ha-Latn-NG") == 0) return 0x0468;
    if (_wcsicmp(LocaleName, L"yo-NG") == 0) return 0x046A;
    if (_wcsicmp(LocaleName, L"quz-BO") == 0) return 0x046B;
    if (_wcsicmp(LocaleName, L"nso-ZA") == 0) return 0x046C;
    if (_wcsicmp(LocaleName, L"ba-RU") == 0) return 0x046D;
    if (_wcsicmp(LocaleName, L"lb-LU") == 0) return 0x046E;
    if (_wcsicmp(LocaleName, L"kl-GL") == 0) return 0x046F;
    if (_wcsicmp(LocaleName, L"ig-NG") == 0) return 0x0470;
    if (_wcsicmp(LocaleName, L"kr-Latn-NG") == 0) return 0x0471;
    if (_wcsicmp(LocaleName, L"om-ET") == 0) return 0x0472;
    if (_wcsicmp(LocaleName, L"ti-ET") == 0) return 0x0473;
    if (_wcsicmp(LocaleName, L"gn-PY") == 0) return 0x0474;
    if (_wcsicmp(LocaleName, L"haw-US") == 0) return 0x0475;
    if (_wcsicmp(LocaleName, L"la-VA") == 0) return 0x0476;
    if (_wcsicmp(LocaleName, L"so-SO") == 0) return 0x0477;
    if (_wcsicmp(LocaleName, L"ii-CN") == 0) return 0x0478;
    if (_wcsicmp(LocaleName, L"arn-CL") == 0) return 0x047A;
    if (_wcsicmp(LocaleName, L"moh-CA") == 0) return 0x047C;
    if (_wcsicmp(LocaleName, L"br-FR") == 0) return 0x047E;
    if (_wcsicmp(LocaleName, L"ug-CN") == 0) return 0x0480;
    if (_wcsicmp(LocaleName, L"mi-NZ") == 0) return 0x0481;
    if (_wcsicmp(LocaleName, L"oc-FR") == 0) return 0x0482;
    if (_wcsicmp(LocaleName, L"co-FR") == 0) return 0x0483;
    if (_wcsicmp(LocaleName, L"gsw-FR") == 0) return 0x0484;
    if (_wcsicmp(LocaleName, L"sah-RU") == 0) return 0x0485;
    if (_wcsicmp(LocaleName, L"quc-Latn-GT") == 0) return 0x0486;
    if (_wcsicmp(LocaleName, L"rw-RW") == 0) return 0x0487;
    if (_wcsicmp(LocaleName, L"wo-SN") == 0) return 0x0488;
    if (_wcsicmp(LocaleName, L"prs-AF") == 0) return 0x048C;
    if (_wcsicmp(LocaleName, L"gd-GB") == 0) return 0x0491;
    if (_wcsicmp(LocaleName, L"ku-Arab-IQ") == 0) return 0x0492;
    if (_wcsicmp(LocaleName, L"qps-ploc") == 0) return 0x0501;
    if (_wcsicmp(LocaleName, L"qps-ploca") == 0) return 0x05FE;
    if (_wcsicmp(LocaleName, L"ar-IQ") == 0) return 0x0801;
    if (_wcsicmp(LocaleName, L"ca-ES-valencia") == 0) return 0x0803;
    if (_wcsicmp(LocaleName, L"zh-CN") == 0) return 0x0804;
    if (_wcsicmp(LocaleName, L"de-CH") == 0) return 0x0807;
    if (_wcsicmp(LocaleName, L"en-GB") == 0) return 0x0809;
    if (_wcsicmp(LocaleName, L"es-MX") == 0) return 0x080A;
    if (_wcsicmp(LocaleName, L"fr-BE") == 0) return 0x080C;
    if (_wcsicmp(LocaleName, L"it-CH") == 0) return 0x0810;
    if (_wcsicmp(LocaleName, L"nl-BE") == 0) return 0x0813;
    if (_wcsicmp(LocaleName, L"nn-NO") == 0) return 0x0814;
    if (_wcsicmp(LocaleName, L"pt-PT") == 0) return 0x0816;
    if (_wcsicmp(LocaleName, L"ro-MD") == 0) return 0x0818;
    if (_wcsicmp(LocaleName, L"ru-MD") == 0) return 0x0819;
    if (_wcsicmp(LocaleName, L"sr-Latn-CS") == 0) return 0x081A;
    if (_wcsicmp(LocaleName, L"sv-FI") == 0) return 0x081D;
    if (_wcsicmp(LocaleName, L"ur-IN") == 0) return 0x0820;
    if (_wcsicmp(LocaleName, L"az-Cyrl-AZ") == 0) return 0x082C;
    if (_wcsicmp(LocaleName, L"dsb-DE") == 0) return 0x082E;
    if (_wcsicmp(LocaleName, L"tn-BW") == 0) return 0x0832;
    if (_wcsicmp(LocaleName, L"se-SE") == 0) return 0x083B;
    if (_wcsicmp(LocaleName, L"ga-IE") == 0) return 0x083C;
    if (_wcsicmp(LocaleName, L"ms-BN") == 0) return 0x083E;
    if (_wcsicmp(LocaleName, L"uz-Cyrl-UZ") == 0) return 0x0843;
    if (_wcsicmp(LocaleName, L"bn-BD") == 0) return 0x0845;
    if (_wcsicmp(LocaleName, L"pa-Arab-PK") == 0) return 0x0846;
    if (_wcsicmp(LocaleName, L"ta-LK") == 0) return 0x0849;
    if (_wcsicmp(LocaleName, L"mn-Mong-CN") == 0) return 0x0850;
    if (_wcsicmp(LocaleName, L"sd-Arab-PK") == 0) return 0x0859;
    if (_wcsicmp(LocaleName, L"iu-Latn-CA") == 0) return 0x085D;
    if (_wcsicmp(LocaleName, L"tzm-Latn-DZ") == 0) return 0x085F;
    if (_wcsicmp(LocaleName, L"ks-Deva-IN") == 0) return 0x0860;
    if (_wcsicmp(LocaleName, L"ne-IN") == 0) return 0x0861;
    if (_wcsicmp(LocaleName, L"ff-Latn-SN") == 0) return 0x0867;
    if (_wcsicmp(LocaleName, L"quz-EC") == 0) return 0x086B;
    if (_wcsicmp(LocaleName, L"ti-ER") == 0) return 0x0873;
    if (_wcsicmp(LocaleName, L"qps-plocm") == 0) return 0x09FF;
    if (_wcsicmp(LocaleName, L"ar-EG") == 0) return 0x0c01;
    if (_wcsicmp(LocaleName, L"zh-HK") == 0) return 0x0C04;
    if (_wcsicmp(LocaleName, L"de-AT") == 0) return 0x0C07;
    if (_wcsicmp(LocaleName, L"en-AU") == 0) return 0x0C09;
    if (_wcsicmp(LocaleName, L"es-ES") == 0) return 0x0c0A;
    if (_wcsicmp(LocaleName, L"fr-CA") == 0) return 0x0c0C;
    if (_wcsicmp(LocaleName, L"sr-Cyrl-CS") == 0) return 0x0C1A;
    if (_wcsicmp(LocaleName, L"se-FI") == 0) return 0x0C3B;
    if (_wcsicmp(LocaleName, L"mn-Mong-MN") == 0) return 0x0C50;
    if (_wcsicmp(LocaleName, L"dz-BT") == 0) return 0x0C51;
    if (_wcsicmp(LocaleName, L"quz-PE") == 0) return 0x0C6B;
    if (_wcsicmp(LocaleName, L"aa") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"aa-DJ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"aa-ER") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"aa-ET") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"af-NA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"agq") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"agq-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ak") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ak-GH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sq-MK") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"gsw-LI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"gsw-CH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-TD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-KM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-DJ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-ER") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-IL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-MR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-PS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-SO") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-SS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-SD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-001") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ast") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ast-ES") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"asa") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"asa-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ksf") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ksf-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bm") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bm-Latn-ML") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bas") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bas-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bem") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bem-ZM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bez") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bez-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"byn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"byn-ER") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"brx") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"brx-IN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ca-AD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ca-FR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ca-IT") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ceb") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ceb-Latn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ceb-Latn-PH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"tzm-Latn-MA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ccp") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ccp-Cakm") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ccp-Cakm-BD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ccp-Cakm-IN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ce-RU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"cgg") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"cgg-UG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"cu-RU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"swc") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"swc-CD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kw") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kw-GB") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"da-GL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dua") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dua-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nl-AW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nl-BQ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nl-CW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nl-SX") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nl-SR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dz") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ebu") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ebu-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-AS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-AI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-AG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-AT") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-BS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-BB") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-BE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-BM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-BW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-IO") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-VG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-BI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-KY") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-CX") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-CC") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-CK") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-CY") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-DK") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-DM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-ER") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-150") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-FK") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-FI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-FJ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-GM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-DE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-GH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-GI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-GD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-GU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-GG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-GY") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-IM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-IL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-JE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-KI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-LS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-LR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-MO") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-MG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-MW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-MT") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-MH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-MU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-FM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-MS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-NA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-NR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-NL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-NG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-NU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-NF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-MP") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-PK") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-PW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-PG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-PN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-PR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-RW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-KN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-LC") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-VC") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-WS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SC") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SX") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SB") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-SE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-CH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-TK") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-TO") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-TC") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-TV") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-UG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-UM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-VI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-VU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-001") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"en-ZM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"eo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"eo-001") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ee") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ee-GH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ee-TG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ewo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ewo-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fo-DK") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-DZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-BJ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-BF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-BI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-CF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-TD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-KM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-CG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-DJ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-GQ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-GF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-PF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-GA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-GP") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-GN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-MG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-MQ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-MR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-MU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-YT") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-NC") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-NE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-RW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-BL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-MF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-PM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-SC") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-SY") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-TG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-TN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-VU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fr-WF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fur") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fur-IT") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-BF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-GM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-GH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-GN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-GN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-GW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-LR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-MR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-MR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-NE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ff-Latn-SL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lg") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lg-UG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"de-BE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"de-IT") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"el-CY") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"guz") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"guz-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ha-Latn-GH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ha-Latn-NE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ia") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ia-FR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ia-001") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"it-SM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"it-VA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"jv") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"jv-Latn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"jv-Latn-ID") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dyo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dyo-SN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kea") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kea-CV") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kab") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kab-DZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kkj") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kkj-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kln") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kln-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kam") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kam-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ks-Arab-IN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ki") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ki-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sw-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sw-UG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ko-KP") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"khq") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"khq-ML") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ses") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ses-ML") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nmg") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nmg-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ku-Arab-IR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lkt") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lkt-US") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lag") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lag-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ln") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ln-AO") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ln-CF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ln-CG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ln-CD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nds") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nds-DE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nds-NL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lu") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lu-CD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"luo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"luo-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"luy") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"luy-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"jmc") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"jmc-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mgh") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mgh-MZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kde") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"kde-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mg") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mg-MG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"gv") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"gv-IM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mas") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mas-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mas-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mzn-IR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mer") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mer-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mgo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mgo-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mfe") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mfe-MU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mua") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"mua-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nqo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nqo-GN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"naq") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"naq-NA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nnh") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nnh-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"jgo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"jgo-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lrc-IQ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"lrc-IR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nd") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nd-ZW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nb-SJ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nus") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nus-SD") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nus-SS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nyn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nyn-UG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"om-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"os") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"os-GE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"os-RU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ps-PK") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"fa-AF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-AO") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-CV") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-GQ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-GW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-LU") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-MO") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-MZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-ST") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-CH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"pt-TL") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"prg-001") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ksh") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ksh-DE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"rof") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"rof-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"rn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"rn-BI") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ru-BY") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ru-KZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ru-KG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ru-UA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"rwk") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"rwk-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ssy") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ssy-ER") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"saq") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"saq-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sg") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sg-CF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sbp") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sbp-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"seh") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"seh-MZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ksb") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ksb-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sn-Latn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sn-Latn-ZW") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"xog") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"xog-UG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"so-DJ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"so-ET") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"so-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nr") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"nr-ZA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"st-LS") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"es-BZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"es-BR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"es-GQ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"es-PH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"zgh") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"zgh-Tfng-MA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"zgh-Tfng") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ss") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ss-ZA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ss-SZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"sv-AX") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"shi") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"shi-Tfng") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"shi-Tfng-MA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"shi-Latn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"shi-Latn-MA") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dav") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dav-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ta-MY") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ta-SG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"twq") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"twq-NE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"teo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"teo-KE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"teo-UG") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"bo-IN") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"tig") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"tig-ER") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"to") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"to-TO") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"tr-CY") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"uz-Arab") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"uz-Arab-AF") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vai") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vai-Vaii") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vai-Vaii-LR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vai-Latn-LR") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vai-Latn") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vo") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vo-001") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vun") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"vun-TZ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"wae") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"wae-CH") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"wal") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"wal-ET") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"yav") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"yav-CM") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"yo-BJ") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dje") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"dje-NE") == 0) return 0x1000;
    if (_wcsicmp(LocaleName, L"ar-LY") == 0) return 0x1001;
    if (_wcsicmp(LocaleName, L"zh-SG") == 0) return 0x1004;
    if (_wcsicmp(LocaleName, L"de-LU") == 0) return 0x1007;
    if (_wcsicmp(LocaleName, L"en-CA") == 0) return 0x1009;
    if (_wcsicmp(LocaleName, L"es-GT") == 0) return 0x100A;
    if (_wcsicmp(LocaleName, L"fr-CH") == 0) return 0x100C;
    if (_wcsicmp(LocaleName, L"hr-BA") == 0) return 0x101A;
    if (_wcsicmp(LocaleName, L"smj-NO") == 0) return 0x103B;
    if (_wcsicmp(LocaleName, L"ar-DZ") == 0) return 0x1401;
    if (_wcsicmp(LocaleName, L"zh-MO") == 0) return 0x1404;
    if (_wcsicmp(LocaleName, L"de-LI") == 0) return 0x1407;
    if (_wcsicmp(LocaleName, L"en-NZ") == 0) return 0x1409;
    if (_wcsicmp(LocaleName, L"es-CR") == 0) return 0x140A;
    if (_wcsicmp(LocaleName, L"fr-LU") == 0) return 0x140C;
    if (_wcsicmp(LocaleName, L"bs-Latn-BA") == 0) return 0x141A;
    if (_wcsicmp(LocaleName, L"smj-SE") == 0) return 0x143B;
    if (_wcsicmp(LocaleName, L"ar-MA") == 0) return 0x1801;
    if (_wcsicmp(LocaleName, L"en-IE") == 0) return 0x1809;
    if (_wcsicmp(LocaleName, L"es-PA") == 0) return 0x180A;
    if (_wcsicmp(LocaleName, L"fr-MC") == 0) return 0x180C;
    if (_wcsicmp(LocaleName, L"sr-Latn-BA") == 0) return 0x181A;
    if (_wcsicmp(LocaleName, L"sma-NO") == 0) return 0x183B;
    if (_wcsicmp(LocaleName, L"ar-TN") == 0) return 0x1C01;
    if (_wcsicmp(LocaleName, L"en-ZA") == 0) return 0x1C09;
    if (_wcsicmp(LocaleName, L"es-DO") == 0) return 0x1c0A;
    if (_wcsicmp(LocaleName, L"fr-029") == 0) return 0x1C0C;
    if (_wcsicmp(LocaleName, L"sr-Cyrl-BA") == 0) return 0x1C1A;
    if (_wcsicmp(LocaleName, L"sma-SE") == 0) return 0x1C3B;
    if (_wcsicmp(LocaleName, L"ar-OM") == 0) return 0x2001;
    if (_wcsicmp(LocaleName, L"en-JM") == 0) return 0x2009;
    if (_wcsicmp(LocaleName, L"es-VE") == 0) return 0x200A;
    if (_wcsicmp(LocaleName, L"fr-RE") == 0) return 0x200C;
    if (_wcsicmp(LocaleName, L"bs-Cyrl-BA") == 0) return 0x201A;
    if (_wcsicmp(LocaleName, L"sms-FI") == 0) return 0x203B;
    if (_wcsicmp(LocaleName, L"ar-YE") == 0) return 0x2401;
    if (_wcsicmp(LocaleName, L"en-029") == 0) return 0x2409;
    if (_wcsicmp(LocaleName, L"es-CO") == 0) return 0x240A;
    if (_wcsicmp(LocaleName, L"fr-CD") == 0) return 0x240C;
    if (_wcsicmp(LocaleName, L"sr-Latn-RS") == 0) return 0x241A;
    if (_wcsicmp(LocaleName, L"smn-FI") == 0) return 0x243B;
    if (_wcsicmp(LocaleName, L"ar-SY") == 0) return 0x2801;
    if (_wcsicmp(LocaleName, L"en-BZ") == 0) return 0x2809;
    if (_wcsicmp(LocaleName, L"es-PE") == 0) return 0x280A;
    if (_wcsicmp(LocaleName, L"fr-SN") == 0) return 0x280C;
    if (_wcsicmp(LocaleName, L"sr-Cyrl-RS") == 0) return 0x281A;
    if (_wcsicmp(LocaleName, L"ar-JO") == 0) return 0x2C01;
    if (_wcsicmp(LocaleName, L"en-TT") == 0) return 0x2c09;
    if (_wcsicmp(LocaleName, L"es-AR") == 0) return 0x2C0A;
    if (_wcsicmp(LocaleName, L"fr-CM") == 0) return 0x2c0C;
    if (_wcsicmp(LocaleName, L"sr-Latn-ME") == 0) return 0x2c1A;
    if (_wcsicmp(LocaleName, L"ar-LB") == 0) return 0x3001;
    if (_wcsicmp(LocaleName, L"en-ZW") == 0) return 0x3009;
    if (_wcsicmp(LocaleName, L"es-EC") == 0) return 0x300A;
    if (_wcsicmp(LocaleName, L"fr-CI") == 0) return 0x300C;
    if (_wcsicmp(LocaleName, L"sr-Cyrl-ME") == 0) return 0x301A;
    if (_wcsicmp(LocaleName, L"ar-KW") == 0) return 0x3401;
    if (_wcsicmp(LocaleName, L"en-PH") == 0) return 0x3409;
    if (_wcsicmp(LocaleName, L"es-CL") == 0) return 0x340A;
    if (_wcsicmp(LocaleName, L"fr-ML") == 0) return 0x340C;
    if (_wcsicmp(LocaleName, L"ar-AE") == 0) return 0x3801;
    if (_wcsicmp(LocaleName, L"es-UY") == 0) return 0x380A;
    if (_wcsicmp(LocaleName, L"fr-MA") == 0) return 0x380C;
    if (_wcsicmp(LocaleName, L"ar-BH") == 0) return 0x3C01;
    if (_wcsicmp(LocaleName, L"en-HK") == 0) return 0x3C09;
    if (_wcsicmp(LocaleName, L"es-PY") == 0) return 0x3C0A;
    if (_wcsicmp(LocaleName, L"fr-HT") == 0) return 0x3c0C;
    if (_wcsicmp(LocaleName, L"ar-QA") == 0) return 0x4001;
    if (_wcsicmp(LocaleName, L"en-IN") == 0) return 0x4009;
    if (_wcsicmp(LocaleName, L"es-BO") == 0) return 0x400A;
    if (_wcsicmp(LocaleName, L"en-MY") == 0) return 0x4409;
    if (_wcsicmp(LocaleName, L"es-SV") == 0) return 0x440A;
    if (_wcsicmp(LocaleName, L"en-SG") == 0) return 0x4809;
    if (_wcsicmp(LocaleName, L"es-HN") == 0) return 0x480A;
    if (_wcsicmp(LocaleName, L"en-AE") == 0) return 0x4C09;
    if (_wcsicmp(LocaleName, L"es-NI") == 0) return 0x4C0A;
    if (_wcsicmp(LocaleName, L"es-PR") == 0) return 0x500A;
    if (_wcsicmp(LocaleName, L"es-US") == 0) return 0x540A;
    if (_wcsicmp(LocaleName, L"es-419") == 0) return 0x580A;
    if (_wcsicmp(LocaleName, L"es-CU") == 0) return 0x5c0A;
    if (_wcsicmp(LocaleName, L"bs-Cyrl") == 0) return 0x641A;
    if (_wcsicmp(LocaleName, L"bs-Latn") == 0) return 0x681A;
    if (_wcsicmp(LocaleName, L"sr-Cyrl") == 0) return 0x6C1A;
    if (_wcsicmp(LocaleName, L"sr-Latn") == 0) return 0x701A;
    if (_wcsicmp(LocaleName, L"smn") == 0) return 0x703B;
    if (_wcsicmp(LocaleName, L"az-Cyrl") == 0) return 0x742C;
    if (_wcsicmp(LocaleName, L"sms") == 0) return 0x743B;
    if (_wcsicmp(LocaleName, L"zh") == 0) return 0x7804;
    if (_wcsicmp(LocaleName, L"nn") == 0) return 0x7814;
    if (_wcsicmp(LocaleName, L"bs") == 0) return 0x781A;
    if (_wcsicmp(LocaleName, L"az-Latn") == 0) return 0x782C;
    if (_wcsicmp(LocaleName, L"sma") == 0) return 0x783B;
    if (_wcsicmp(LocaleName, L"uz-Cyrl") == 0) return 0x7843;
    if (_wcsicmp(LocaleName, L"mn-Cyrl") == 0) return 0x7850;
    if (_wcsicmp(LocaleName, L"iu-Cans") == 0) return 0x785D;
    if (_wcsicmp(LocaleName, L"zh-Hant") == 0) return 0x7C04;
    if (_wcsicmp(LocaleName, L"nb") == 0) return 0x7C14;
    if (_wcsicmp(LocaleName, L"sr") == 0) return 0x7C1A;
    if (_wcsicmp(LocaleName, L"tg-Cyrl") == 0) return 0x7C28;
    if (_wcsicmp(LocaleName, L"dsb") == 0) return 0x7C2E;
    if (_wcsicmp(LocaleName, L"smj") == 0) return 0x7C3B;
    if (_wcsicmp(LocaleName, L"uz-Latn") == 0) return 0x7C43;
    if (_wcsicmp(LocaleName, L"pa-Arab") == 0) return 0x7C46;
    if (_wcsicmp(LocaleName, L"mn-Mong") == 0) return 0x7C50;
    if (_wcsicmp(LocaleName, L"sd-Arab") == 0) return 0x7C59;
    if (_wcsicmp(LocaleName, L"chr-Cher") == 0) return 0x7c5C;
    if (_wcsicmp(LocaleName, L"iu-Latn") == 0) return 0x7C5D;
    if (_wcsicmp(LocaleName, L"tzm-Latn") == 0) return 0x7C5F;
    if (_wcsicmp(LocaleName, L"ff-Latn") == 0) return 0x7C67;
    if (_wcsicmp(LocaleName, L"ha-Latn") == 0) return 0x7C68;
    if (_wcsicmp(LocaleName, L"ku-Arab") == 0) return 0x7c92;
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

    if (! IsValidLocale( Locale,
                         0 )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    PCWSTR name = LdkpGetLocaleName( Locale );
    if (name == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    int len = (int)wcslen(name) + 1;

    if (cchName == 0) {
        return len;
    }

    if (lpName == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (cchName < len) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    wcscpy_s( lpName,
              cchName,
              name );
    return len;
}

static
BOOL
LdkpNormalizeLocaleName (
    _In_ LPCWSTR LocaleName,
    _Out_writes_(NormalizedNameCch) PWSTR NormalizedName,
    _In_ SIZE_T NormalizedNameCch
    )
{
    SIZE_T Index = 0;

    if (NormalizedNameCch == 0) {
        return FALSE;
    }

    while (LocaleName[Index] != UNICODE_NULL &&
           LocaleName[Index] != L'.' &&
           LocaleName[Index] != L'@') {
        if (Index + 1 >= NormalizedNameCch) {
            NormalizedName[0] = UNICODE_NULL;
            return FALSE;
        }

        NormalizedName[Index] = LocaleName[Index] == L'_' ? L'-' : LocaleName[Index];
        Index++;
    }

    if (LocaleName[Index] != UNICODE_NULL) {
        NormalizedName[0] = UNICODE_NULL;
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    NormalizedName[Index] = UNICODE_NULL;
    return TRUE;
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
    } else if (_wcsicmp(lpName, LOCALE_NAME_SYSTEM_DEFAULT) == 0) {
        LCID lcid;
        ZwQueryDefaultLocale( FALSE,
                              &lcid );
        return lcid;
    } else if (wcslen(lpName) == 0) { // LOCALE_NAME_INVARIANT
        return LOCALE_INVARIANT;
    } else {
        WCHAR NormalizedName[128];
        LCID lcid = LdkpGetLCID(lpName);
        if (lcid == 0 &&
            LdkpNormalizeLocaleName( lpName,
                                     NormalizedName,
                                     RTL_NUMBER_OF(NormalizedName) )) {
            lcid = LdkpGetLCID(NormalizedName);
        }
        if (lcid == 0) {
            SetLastError( ERROR_INVALID_PARAMETER );
        }
        return lcid;
    }
}

WINBASEAPI
LCID
WINAPI
GetUserDefaultLCID (
    VOID
    )
{
    LCID Lcid;
    NTSTATUS Status;
    Status = ZwQueryDefaultLocale( TRUE,
                                   &Lcid );
    if (NT_SUCCESS(Status)) {
        return Lcid;
    }
    LdkSetLastNTError( Status );
    return 0;
}

WINBASEAPI
int
WINAPI
GetUserDefaultLocaleName (
    _Out_writes_(cchLocaleName) LPWSTR lpLocaleName,
    _In_ int cchLocaleName
    )
{
    LCID Locale = GetUserDefaultLCID();
    if (Locale == 0) {
        return 0;
    }
    return LCIDToLocaleName( Locale,
                             lpLocaleName,
                             cchLocaleName,
                             0 );
}

static
int
LdkpCopyLocaleString (
    _In_ PCWSTR Value,
    _Out_writes_opt_(cchData) LPWSTR lpLCData,
    _In_ int cchData
    )
{
    int Required = (int)wcslen(Value) + 1;

    if (lpLCData == NULL || cchData == 0) {
        return Required;
    }

    if (cchData < Required) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    wcscpy_s( lpLCData,
              cchData,
              Value );
    return Required;
}

static
int
LdkpCopyLocaleNumber (
    _In_ DWORD Value,
    _Out_writes_opt_(cchData) LPWSTR lpLCData,
    _In_ int cchData
    )
{
    int Required = sizeof(DWORD) / sizeof(WCHAR);

    if (lpLCData == NULL || cchData == 0) {
        return Required;
    }

    if (cchData < Required) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    RtlCopyMemory( lpLCData,
                   &Value,
                   sizeof(Value) );
    return Required;
}

static
BOOL
LdkpAnsiStringToWideForNls (
    _In_reads_(cchSource) LPCSTR Source,
    _In_ int cchSource,
    _Outptr_result_z_ PWSTR *WideString,
    _Out_ int *WideCount
    )
{
    int Required;
    int Converted;
    PWSTR Buffer;

    if (Source == NULL || cchSource < -1 || WideString == NULL || WideCount == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *WideString = NULL;
    *WideCount = 0;

    if (cchSource == 0) {
        Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  sizeof(WCHAR) );
        if (Buffer == NULL) {
            SetLastError( ERROR_NOT_ENOUGH_MEMORY );
            return FALSE;
        }

        *WideString = Buffer;
        *WideCount = 0;
        return TRUE;
    }

    Required = MultiByteToWideChar( CP_ACP,
                                    0,
                                    Source,
                                    cchSource,
                                    NULL,
                                    0 );
    if (Required == 0) {
        return FALSE;
    }

    Buffer = RtlAllocateHeap( RtlProcessHeap(),
                              HEAP_ZERO_MEMORY,
                              ((SIZE_T)Required + 1) * sizeof(WCHAR) );
    if (Buffer == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return FALSE;
    }

    Converted = MultiByteToWideChar( CP_ACP,
                                     0,
                                     Source,
                                     cchSource,
                                     Buffer,
                                     Required );
    if (Converted == 0) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     Buffer );
        return FALSE;
    }

    Buffer[Converted] = UNICODE_NULL;
    *WideString = Buffer;
    *WideCount = cchSource == -1 ? -1 : Converted;
    return TRUE;
}

static
int
LdkpWideStringToAnsiForNls (
    _In_reads_(cchSource) LPCWSTR Source,
    _In_ int cchSource,
    _Out_writes_opt_(cchDest) LPSTR Destination,
    _In_ int cchDest
    )
{
    int Required;

    if (cchDest < 0 || (Destination == NULL && cchDest != 0)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    Required = WideCharToMultiByte( CP_ACP,
                                    0,
                                    Source,
                                    cchSource,
                                    NULL,
                                    0,
                                    NULL,
                                    NULL );
    if (Required == 0) {
        return 0;
    }

    if (cchDest == 0) {
        return Required;
    }

    return WideCharToMultiByte( CP_ACP,
                                0,
                                Source,
                                cchSource,
                                Destination,
                                cchDest,
                                NULL,
                                NULL );
}

typedef struct _LDKP_LOCALE_DATA {
    LCID Locale;
    PCWSTR DisplayName;
    PCWSTR LanguageName;
    PCWSTR AbbreviatedLanguageName;
    PCWSTR Iso639LanguageName;
    PCWSTR Iso639LanguageName2;
    PCWSTR CountryName;
    PCWSTR AbbreviatedCountryName;
    PCWSTR Iso3166CountryName;
    PCWSTR Iso3166CountryName2;
    PCWSTR ListSeparator;
    PCWSTR DecimalSeparator;
    PCWSTR ThousandSeparator;
    PCWSTR Grouping;
    PCWSTR MonetaryDecimalSeparator;
    PCWSTR MonetaryThousandSeparator;
    PCWSTR MonetaryGrouping;
    PCWSTR CurrencySymbol;
    PCWSTR InternationalCurrencySymbol;
    PCWSTR ShortDate;
    PCWSTR LongDate;
    PCWSTR TimeFormat;
    PCWSTR ShortTime;
    PCWSTR YearMonth;
    PCWSTR Duration;
    PCWSTR DateSeparator;
    PCWSTR TimeSeparator;
    PCWSTR AmDesignator;
    PCWSTR PmDesignator;
    PCWSTR Scripts;
    PCWSTR Parent;
    DWORD DefaultAnsiCodePage;
    DWORD DefaultOemCodePage;
    DWORD DefaultMacCodePage;
    DWORD DefaultEbcdicCodePage;
    DWORD DialingCode;
    DWORD DefaultCountry;
    DWORD Measure;
    DWORD Digits;
    DWORD CurrencyDigits;
    DWORD InternationalCurrencyDigits;
    DWORD PositiveSymbolPrecedes;
    DWORD NegativeSymbolPrecedes;
    DWORD PositiveSeparatedBySpace;
    DWORD NegativeSeparatedBySpace;
    DWORD PositiveSignPosition;
    DWORD NegativeSignPosition;
    DWORD CurrencyFormat;
    DWORD NegativeCurrencyFormat;
    DWORD FirstDayOfWeek;
    DWORD GeoId;
    DWORD DateOrder;
    DWORD LongDateOrder;
    DWORD TimeFormat24Hour;
} LDKP_LOCALE_DATA, *PLDKP_LOCALE_DATA;

static const LDKP_LOCALE_DATA LdkpLocaleData[] = {
    {
        0x0409, L"English (United States)", L"English", L"ENU", L"en", L"eng",
        L"United States", L"USA", L"US", L"USA", L",", L".", L",", L"3;0",
        L".", L",", L"3;0", L"$", L"USD", L"M/d/yyyy",
        L"dddd, MMMM d, yyyy", L"h:mm:ss tt", L"h:mm tt", L"MMMM yyyy",
        L"h:mm:ss", L"/", L":", L"AM", L"PM", L"Latn;", L"en",
        1252, 437, 10000, 37, 1, 1, 1, 2, 2, 2, 1, 1, 0, 0, 3, 0,
        0, 0, 6, 244, 0, 0, 0
    },
    {
        0x0407, L"German (Germany)", L"German", L"DEU", L"de", L"deu",
        L"Germany", L"DEU", L"DE", L"DEU", L";", L",", L".", L"3;0",
        L",", L".", L"3;0", L"\x20ac", L"EUR", L"dd.MM.yyyy",
        L"dddd, d. MMMM yyyy", L"HH:mm:ss", L"HH:mm", L"MMMM yyyy",
        L"HH:mm:ss", L".", L":", L"", L"", L"Latn;", L"de",
        1252, 850, 10000, 20273, 49, 49, 0, 2, 2, 2, 0, 0, 1, 1, 3,
        1, 3, 8, 0, 94, 1, 1, 1
    },
    {
        0x0809, L"English (United Kingdom)", L"English", L"ENG", L"en", L"eng",
        L"United Kingdom", L"GBR", L"GB", L"GBR", L",", L".", L",", L"3;0",
        L".", L",", L"3;0", L"\x00a3", L"GBP", L"dd/MM/yyyy",
        L"dd MMMM yyyy", L"HH:mm:ss", L"HH:mm", L"MMMM yyyy", L"HH:mm:ss",
        L"/", L":", L"am", L"pm", L"Latn;", L"en",
        1252, 850, 10000, 20285, 44, 44, 0, 2, 2, 2, 1, 1, 0, 0,
        3, 1, 0, 1, 0, 242, 1, 1, 1
    },
    {
        0x0412, L"Korean (Korea)", L"Korean", L"KOR", L"ko", L"kor",
        L"Korea", L"KOR", L"KR", L"KOR", L",", L".", L",", L"3;0",
        L".", L",", L"3;0", L"\x20a9", L"KRW", L"yyyy-MM-dd",
        L"yyyy'\xb144' M'\xc6d4' d'\xc77c' dddd", L"tt h:mm:ss", L"tt h:mm",
        L"yyyy'\xb144' M'\xc6d4'", L"h:mm:ss", L"-", L":",
        L"\xc624\xc804", L"\xc624\xd6c4", L"Hang;Hani;Kore;", L"ko",
        949, 949, 10003, 20833, 82, 82, 0, 2, 0, 0, 1, 1, 0, 0,
        3, 3, 0, 0, 6, 134, 2, 2, 0
    },
    {
        0x0804, L"Chinese (Simplified, China)", L"Chinese", L"CHS", L"zh", L"zho",
        L"China", L"CHN", L"CN", L"CHN", L",", L".", L",", L"3;0",
        L".", L",", L"3;0", L"\x00a5", L"CNY", L"yyyy/M/d",
        L"yyyy'\x5e74'M'\x6708'd'\x65e5'", L"H:mm:ss", L"H:mm",
        L"yyyy'\x5e74'M'\x6708'", L"H:mm:ss", L"/", L":",
        L"\x4e0a\x5348", L"\x4e0b\x5348", L"Hani;Hans;", L"zh-Hans",
        936, 936, 10008, 500, 86, 86, 0, 2, 2, 2, 1, 1, 0, 0,
        3, 3, 0, 0, 0, 45, 2, 2, 1
    },
    {
        0x0411, L"Japanese (Japan)", L"Japanese", L"JPN", L"ja", L"jpn",
        L"Japan", L"JPN", L"JP", L"JPN", L",", L".", L",", L"3;0",
        L".", L",", L"3;0", L"\x00a5", L"JPY", L"yyyy/MM/dd",
        L"yyyy'\x5e74'M'\x6708'd'\x65e5'", L"H:mm:ss", L"H:mm",
        L"yyyy'\x5e74'M'\x6708'", L"H:mm:ss", L"/", L":",
        L"\x5348\x524d", L"\x5348\x5f8c", L"Hani;Hira;Jpan;Kana;", L"ja",
        932, 932, 10001, 20290, 81, 81, 0, 2, 0, 0, 1, 1, 0, 0,
        3, 3, 0, 0, 0, 122, 2, 2, 1
    },
    {
        0x0419, L"Russian (Russia)", L"Russian", L"RUS", L"ru", L"rus",
        L"Russia", L"RUS", L"RU", L"RUS", L";", L",", L"\x00a0", L"3;0",
        L",", L"\x00a0", L"3;0", L"\x20bd", L"RUB", L"dd.MM.yyyy",
        L"d MMMM yyyy '\x0433.'", L"H:mm:ss", L"H:mm",
        L"MMMM yyyy", L"H:mm:ss", L".", L":", L"", L"",
        L"Cyrl;", L"ru", 1251, 866, 10007, 20880, 7, 7, 0, 2, 2, 2,
        0, 0, 1, 1, 3, 3, 3, 8, 0, 203, 1, 1, 1
    },
};

static
const LDKP_LOCALE_DATA *
LdkpGetLocaleData (
    _In_ LCID Locale
    )
{
    LCID BaseLocale = Locale & 0xFFFF;
    ULONG Index;

    for (Index = 0; Index < RTL_NUMBER_OF(LdkpLocaleData); Index++) {
        if (LdkpLocaleData[Index].Locale == BaseLocale) {
            return &LdkpLocaleData[Index];
        }
    }

    return &LdkpLocaleData[0];
}

static
BOOL
LdkpGetLocaleNumber (
    _In_ LCID Locale,
    _In_ LCTYPE LCType,
    _Out_ PDWORD Value
    )
{
    const LDKP_LOCALE_DATA *LocaleData = LdkpGetLocaleData(Locale);

    switch (LCType) {
    case LOCALE_ILANGUAGE:
    case LOCALE_IDEFAULTLANGUAGE:
        *Value = Locale;
        return TRUE;
    case LOCALE_IDEFAULTANSICODEPAGE:
        *Value = LocaleData->DefaultAnsiCodePage;
        return TRUE;
    case LOCALE_IDEFAULTCODEPAGE:
        *Value = LocaleData->DefaultOemCodePage;
        return TRUE;
    case LOCALE_IDEFAULTMACCODEPAGE:
        *Value = LocaleData->DefaultMacCodePage;
        return TRUE;
    case LOCALE_IDEFAULTEBCDICCODEPAGE:
        *Value = LocaleData->DefaultEbcdicCodePage;
        return TRUE;
    case LOCALE_IDIALINGCODE:
        *Value = LocaleData->DialingCode;
        return TRUE;
    case LOCALE_IDEFAULTCOUNTRY:
        *Value = LocaleData->DefaultCountry;
        return TRUE;
    case LOCALE_IMEASURE:
        *Value = LocaleData->Measure;
        return TRUE;
    case LOCALE_IDIGITS:
        *Value = LocaleData->Digits;
        return TRUE;
    case LOCALE_ICURRDIGITS:
        *Value = LocaleData->CurrencyDigits;
        return TRUE;
    case LOCALE_IINTLCURRDIGITS:
        *Value = LocaleData->InternationalCurrencyDigits;
        return TRUE;
    case LOCALE_ILZERO:
    case LOCALE_ICALENDARTYPE:
    case LOCALE_IOPTIONALCALENDAR:
    case LOCALE_IPAPERSIZE:
    case LOCALE_IDIGITSUBSTITUTION:
    case LOCALE_ICENTURY:
        *Value = 1;
        return TRUE;
    case LOCALE_INEGNUMBER:
        *Value = 1;
        return TRUE;
    case LOCALE_IPOSSYMPRECEDES:
        *Value = LocaleData->PositiveSymbolPrecedes;
        return TRUE;
    case LOCALE_INEGSYMPRECEDES:
        *Value = LocaleData->NegativeSymbolPrecedes;
        return TRUE;
    case LOCALE_IPOSSEPBYSPACE:
        *Value = LocaleData->PositiveSeparatedBySpace;
        return TRUE;
    case LOCALE_INEGSEPBYSPACE:
        *Value = LocaleData->NegativeSeparatedBySpace;
        return TRUE;
    case LOCALE_ICURRENCY:
        *Value = LocaleData->CurrencyFormat;
        return TRUE;
    case LOCALE_INEGCURR:
        *Value = LocaleData->NegativeCurrencyFormat;
        return TRUE;
    case LOCALE_IFIRSTWEEKOFYEAR:
    case LOCALE_INEUTRAL:
    case LOCALE_IREADINGLAYOUT:
    case LOCALE_ITIMEMARKPOSN:
    case LOCALE_ITLZERO:
    case LOCALE_IDAYLZERO:
    case LOCALE_IMONLZERO:
    case LOCALE_IPOSITIVEPERCENT:
    case LOCALE_INEGATIVEPERCENT:
        *Value = 0;
        return TRUE;
    case LOCALE_IDATE:
        *Value = LocaleData->DateOrder;
        return TRUE;
    case LOCALE_ILDATE:
        *Value = LocaleData->LongDateOrder;
        return TRUE;
    case LOCALE_ITIME:
        *Value = LocaleData->TimeFormat24Hour;
        return TRUE;
    case LOCALE_IPOSSIGNPOSN:
        *Value = LocaleData->PositiveSignPosition;
        return TRUE;
    case LOCALE_INEGSIGNPOSN:
        *Value = LocaleData->NegativeSignPosition;
        return TRUE;
    case LOCALE_IFIRSTDAYOFWEEK:
        *Value = LocaleData->FirstDayOfWeek;
        return TRUE;
    case LOCALE_IGEOID:
        *Value = LocaleData->GeoId;
        return TRUE;
    }

    return FALSE;
}

static
PCWSTR
LdkpGetNativeDisplayName (
    _In_ const LDKP_LOCALE_DATA *LocaleData
    )
{
    switch (LocaleData->Locale) {
    case 0x0412:
        return L"\xd55c\xad6d\xc5b4(\xb300\xd55c\xbbfc\xad6d)";
    case 0x0804:
        return L"\x4e2d\x6587(\x4e2d\x56fd)";
    case 0x0411:
        return L"\x65e5\x672c\x8a9e (\x65e5\x672c)";
    case 0x0419:
        return L"\x0440\x0443\x0441\x0441\x043a\x0438\x0439 (\x0420\x043e\x0441\x0441\x0438\x044f)";
    }

    return LocaleData->DisplayName;
}

static
PCWSTR
LdkpGetNativeLanguageName (
    _In_ const LDKP_LOCALE_DATA *LocaleData
    )
{
    switch (LocaleData->Locale) {
    case 0x0412:
        return L"\xd55c\xad6d\xc5b4";
    case 0x0804:
        return L"\x4e2d\x6587(\x7b80\x4f53)";
    case 0x0411:
        return L"\x65e5\x672c\x8a9e";
    case 0x0419:
        return L"\x0440\x0443\x0441\x0441\x043a\x0438\x0439";
    }

    return LocaleData->LanguageName;
}

static
PCWSTR
LdkpGetNativeCountryName (
    _In_ const LDKP_LOCALE_DATA *LocaleData
    )
{
    switch (LocaleData->Locale) {
    case 0x0412:
        return L"\xb300\xd55c\xbbfc\xad6d";
    case 0x0804:
        return L"\x4e2d\x56fd";
    case 0x0411:
        return L"\x65e5\x672c";
    case 0x0419:
        return L"\x0420\x043e\x0441\x0441\x0438\x044f";
    }

    return LocaleData->CountryName;
}

static
PCWSTR
LdkpGetLocaleString (
    _In_ LCID Locale,
    _In_ LCTYPE LCType
    )
{
    const LDKP_LOCALE_DATA *LocaleData = LdkpGetLocaleData(Locale);
    static PCWSTR LongDays[] = {
        L"Monday", L"Tuesday", L"Wednesday", L"Thursday", L"Friday", L"Saturday", L"Sunday"
    };
    static PCWSTR ShortDays[] = {
        L"Mon", L"Tue", L"Wed", L"Thu", L"Fri", L"Sat", L"Sun"
    };
    static PCWSTR ShortestDays[] = {
        L"M", L"T", L"W", L"T", L"F", L"S", L"S"
    };
    static PCWSTR LongMonths[] = {
        L"January", L"February", L"March", L"April", L"May", L"June",
        L"July", L"August", L"September", L"October", L"November", L"December"
    };
    static PCWSTR ShortMonths[] = {
        L"Jan", L"Feb", L"Mar", L"Apr", L"May", L"Jun",
        L"Jul", L"Aug", L"Sep", L"Oct", L"Nov", L"Dec"
    };
    static PCWSTR GermanLongDays[] = {
        L"Montag", L"Dienstag", L"Mittwoch", L"Donnerstag", L"Freitag", L"Samstag", L"Sonntag"
    };
    static PCWSTR GermanShortDays[] = {
        L"Mo", L"Di", L"Mi", L"Do", L"Fr", L"Sa", L"So"
    };
    static PCWSTR GermanShortestDays[] = {
        L"M", L"D", L"M", L"D", L"F", L"S", L"S"
    };
    static PCWSTR GermanLongMonths[] = {
        L"Januar", L"Februar", L"M\x00e4rz", L"April", L"Mai", L"Juni",
        L"Juli", L"August", L"September", L"Oktober", L"November", L"Dezember"
    };
    static PCWSTR GermanShortMonths[] = {
        L"Jan", L"Feb", L"M\x00e4r", L"Apr", L"Mai", L"Jun",
        L"Jul", L"Aug", L"Sep", L"Okt", L"Nov", L"Dez"
    };
    static PCWSTR KoreanLongDays[] = {
        L"\xc6d4\xc694\xc77c", L"\xd654\xc694\xc77c", L"\xc218\xc694\xc77c",
        L"\xbaa9\xc694\xc77c", L"\xae08\xc694\xc77c", L"\xd1a0\xc694\xc77c",
        L"\xc77c\xc694\xc77c"
    };
    static PCWSTR KoreanShortDays[] = {
        L"\xc6d4", L"\xd654", L"\xc218", L"\xbaa9", L"\xae08", L"\xd1a0", L"\xc77c"
    };
    static PCWSTR KoreanLongMonths[] = {
        L"1" L"\xc6d4", L"2" L"\xc6d4", L"3" L"\xc6d4", L"4" L"\xc6d4",
        L"5" L"\xc6d4", L"6" L"\xc6d4", L"7" L"\xc6d4", L"8" L"\xc6d4",
        L"9" L"\xc6d4", L"10" L"\xc6d4", L"11" L"\xc6d4", L"12" L"\xc6d4"
    };
    static PCWSTR ChineseLongDays[] = {
        L"\x661f\x671f\x4e00", L"\x661f\x671f\x4e8c", L"\x661f\x671f\x4e09",
        L"\x661f\x671f\x56db", L"\x661f\x671f\x4e94", L"\x661f\x671f\x516d",
        L"\x661f\x671f\x65e5"
    };
    static PCWSTR ChineseShortDays[] = {
        L"\x5468\x4e00", L"\x5468\x4e8c", L"\x5468\x4e09", L"\x5468\x56db",
        L"\x5468\x4e94", L"\x5468\x516d", L"\x5468\x65e5"
    };
    static PCWSTR ChineseShortestDays[] = {
        L"\x4e00", L"\x4e8c", L"\x4e09", L"\x56db", L"\x4e94", L"\x516d", L"\x65e5"
    };
    static PCWSTR ChineseLongMonths[] = {
        L"\x4e00\x6708", L"\x4e8c\x6708", L"\x4e09\x6708", L"\x56db\x6708",
        L"\x4e94\x6708", L"\x516d\x6708", L"\x4e03\x6708", L"\x516b\x6708",
        L"\x4e5d\x6708", L"\x5341\x6708", L"\x5341\x4e00\x6708", L"\x5341\x4e8c\x6708"
    };
    static PCWSTR JapaneseLongDays[] = {
        L"\x6708\x66dc\x65e5", L"\x706b\x66dc\x65e5", L"\x6c34\x66dc\x65e5",
        L"\x6728\x66dc\x65e5", L"\x91d1\x66dc\x65e5", L"\x571f\x66dc\x65e5",
        L"\x65e5\x66dc\x65e5"
    };
    static PCWSTR JapaneseShortDays[] = {
        L"\x6708", L"\x706b", L"\x6c34", L"\x6728", L"\x91d1", L"\x571f", L"\x65e5"
    };
    static PCWSTR JapaneseLongMonths[] = {
        L"1" L"\x6708", L"2" L"\x6708", L"3" L"\x6708", L"4" L"\x6708",
        L"5" L"\x6708", L"6" L"\x6708", L"7" L"\x6708", L"8" L"\x6708",
        L"9" L"\x6708", L"10" L"\x6708", L"11" L"\x6708", L"12" L"\x6708"
    };
    static PCWSTR RussianLongDays[] = {
        L"\x043f\x043e\x043d\x0435\x0434\x0435\x043b\x044c\x043d\x0438\x043a",
        L"\x0432\x0442\x043e\x0440\x043d\x0438\x043a",
        L"\x0441\x0440\x0435\x0434\x0430",
        L"\x0447\x0435\x0442\x0432\x0435\x0440\x0433",
        L"\x043f\x044f\x0442\x043d\x0438\x0446\x0430",
        L"\x0441\x0443\x0431\x0431\x043e\x0442\x0430",
        L"\x0432\x043e\x0441\x043a\x0440\x0435\x0441\x0435\x043d\x044c\x0435"
    };
    static PCWSTR RussianShortDays[] = {
        L"\x043f\x043d", L"\x0432\x0442", L"\x0441\x0440", L"\x0447\x0442",
        L"\x043f\x0442", L"\x0441\x0431", L"\x0432\x0441"
    };
    static PCWSTR RussianLongMonths[] = {
        L"\x042f\x043d\x0432\x0430\x0440\x044c",
        L"\x0424\x0435\x0432\x0440\x0430\x043b\x044c",
        L"\x041c\x0430\x0440\x0442",
        L"\x0410\x043f\x0440\x0435\x043b\x044c",
        L"\x041c\x0430\x0439",
        L"\x0418\x044e\x043d\x044c",
        L"\x0418\x044e\x043b\x044c",
        L"\x0410\x0432\x0433\x0443\x0441\x0442",
        L"\x0421\x0435\x043d\x0442\x044f\x0431\x0440\x044c",
        L"\x041e\x043a\x0442\x044f\x0431\x0440\x044c",
        L"\x041d\x043e\x044f\x0431\x0440\x044c",
        L"\x0414\x0435\x043a\x0430\x0431\x0440\x044c"
    };
    static PCWSTR RussianShortMonths[] = {
        L"\x044f\x043d\x0432.", L"\x0444\x0435\x0432\x0440.", L"\x043c\x0430\x0440.",
        L"\x0430\x043f\x0440.", L"\x043c\x0430\x0439", L"\x0438\x044e\x043d.",
        L"\x0438\x044e\x043b.", L"\x0430\x0432\x0433.", L"\x0441\x0435\x043d\x0442.",
        L"\x043e\x043a\x0442.", L"\x043d\x043e\x044f\x0431.", L"\x0434\x0435\x043a."
    };
    PCWSTR *ActiveLongDays = LongDays;
    PCWSTR *ActiveShortDays = ShortDays;
    PCWSTR *ActiveShortestDays = ShortestDays;
    PCWSTR *ActiveLongMonths = LongMonths;
    PCWSTR *ActiveShortMonths = ShortMonths;

    if (LocaleData->Locale == 0x0407) {
        ActiveLongDays = GermanLongDays;
        ActiveShortDays = GermanShortDays;
        ActiveShortestDays = GermanShortestDays;
        ActiveLongMonths = GermanLongMonths;
        ActiveShortMonths = GermanShortMonths;
    } else if (LocaleData->Locale == 0x0412) {
        ActiveLongDays = KoreanLongDays;
        ActiveShortDays = KoreanShortDays;
        ActiveShortestDays = KoreanShortDays;
        ActiveLongMonths = KoreanLongMonths;
        ActiveShortMonths = KoreanLongMonths;
    } else if (LocaleData->Locale == 0x0804) {
        ActiveLongDays = ChineseLongDays;
        ActiveShortDays = ChineseShortDays;
        ActiveShortestDays = ChineseShortestDays;
        ActiveLongMonths = ChineseLongMonths;
        ActiveShortMonths = ChineseLongMonths;
    } else if (LocaleData->Locale == 0x0411) {
        ActiveLongDays = JapaneseLongDays;
        ActiveShortDays = JapaneseShortDays;
        ActiveShortestDays = JapaneseShortDays;
        ActiveLongMonths = JapaneseLongMonths;
        ActiveShortMonths = JapaneseLongMonths;
    } else if (LocaleData->Locale == 0x0419) {
        ActiveLongDays = RussianLongDays;
        ActiveShortDays = RussianShortDays;
        ActiveShortestDays = RussianShortDays;
        ActiveLongMonths = RussianLongMonths;
        ActiveShortMonths = RussianShortMonths;
    }

    if (LCType >= LOCALE_SDAYNAME1 && LCType <= LOCALE_SDAYNAME7) {
        return ActiveLongDays[LCType - LOCALE_SDAYNAME1];
    }
    if (LCType >= LOCALE_SABBREVDAYNAME1 && LCType <= LOCALE_SABBREVDAYNAME7) {
        return ActiveShortDays[LCType - LOCALE_SABBREVDAYNAME1];
    }
    if (LCType >= LOCALE_SSHORTESTDAYNAME1 && LCType <= LOCALE_SSHORTESTDAYNAME7) {
        return ActiveShortestDays[LCType - LOCALE_SSHORTESTDAYNAME1];
    }
    if (LCType >= LOCALE_SMONTHNAME1 && LCType <= LOCALE_SMONTHNAME12) {
        return ActiveLongMonths[LCType - LOCALE_SMONTHNAME1];
    }
    if (LCType >= LOCALE_SABBREVMONTHNAME1 && LCType <= LOCALE_SABBREVMONTHNAME12) {
        return ActiveShortMonths[LCType - LOCALE_SABBREVMONTHNAME1];
    }

    switch (LCType) {
    case LOCALE_SNAME:
    case LOCALE_SSORTLOCALE:
        return LdkpGetLocaleName(Locale);
    case LOCALE_SLOCALIZEDDISPLAYNAME:
    case LOCALE_SENGLISHDISPLAYNAME:
        return LocaleData->DisplayName;
    case LOCALE_SNATIVEDISPLAYNAME:
        return LdkpGetNativeDisplayName(LocaleData);
    case LOCALE_SLOCALIZEDLANGUAGENAME:
    case LOCALE_SENGLISHLANGUAGENAME:
        return LocaleData->LanguageName;
    case LOCALE_SNATIVELANGUAGENAME:
        return LdkpGetNativeLanguageName(LocaleData);
    case LOCALE_SABBREVLANGNAME:
        return LocaleData->AbbreviatedLanguageName;
    case LOCALE_SISO639LANGNAME:
        return LocaleData->Iso639LanguageName;
    case LOCALE_SISO639LANGNAME2:
        return LocaleData->Iso639LanguageName2;
    case LOCALE_SLOCALIZEDCOUNTRYNAME:
    case LOCALE_SENGLISHCOUNTRYNAME:
        return LocaleData->CountryName;
    case LOCALE_SNATIVECOUNTRYNAME:
        return LdkpGetNativeCountryName(LocaleData);
    case LOCALE_SABBREVCTRYNAME:
        return LocaleData->AbbreviatedCountryName;
    case LOCALE_SISO3166CTRYNAME:
        return LocaleData->Iso3166CountryName;
    case LOCALE_SISO3166CTRYNAME2:
        return LocaleData->Iso3166CountryName2;
    case LOCALE_SLIST:
        return LocaleData->ListSeparator;
    case LOCALE_SDECIMAL:
        return LocaleData->DecimalSeparator;
    case LOCALE_SMONDECIMALSEP:
        return LocaleData->MonetaryDecimalSeparator;
    case LOCALE_STHOUSAND:
        return LocaleData->ThousandSeparator;
    case LOCALE_SMONTHOUSANDSEP:
        return LocaleData->MonetaryThousandSeparator;
    case LOCALE_SGROUPING:
        return LocaleData->Grouping;
    case LOCALE_SMONGROUPING:
        return LocaleData->MonetaryGrouping;
    case LOCALE_SNATIVEDIGITS:
        return L"0123456789";
    case LOCALE_SCURRENCY:
        return LocaleData->CurrencySymbol;
    case LOCALE_SINTLSYMBOL:
        return LocaleData->InternationalCurrencySymbol;
    case LOCALE_SPOSITIVESIGN:
    case LOCALE_SSORTNAME:
        return L"";
    case LOCALE_SNEGATIVESIGN:
        return L"-";
    case LOCALE_SSHORTDATE:
        return LocaleData->ShortDate;
    case LOCALE_SLONGDATE:
        return LocaleData->LongDate;
    case LOCALE_STIMEFORMAT:
        return LocaleData->TimeFormat;
    case LOCALE_SSHORTTIME:
        return LocaleData->ShortTime;
    case LOCALE_SYEARMONTH:
        return LocaleData->YearMonth;
    case LOCALE_SMONTHNAME13:
    case LOCALE_SABBREVMONTHNAME13:
        return L"";
    case LOCALE_SDURATION:
        return LocaleData->Duration;
    case LOCALE_SDATE:
        return LocaleData->DateSeparator;
    case LOCALE_STIME:
        return LocaleData->TimeSeparator;
    case LOCALE_SAM:
        return LocaleData->AmDesignator;
    case LOCALE_SPM:
        return LocaleData->PmDesignator;
    case LOCALE_SNAN:
        return L"NaN";
    case LOCALE_SPOSINFINITY:
        return L"Infinity";
    case LOCALE_SNEGINFINITY:
        return L"-Infinity";
    case LOCALE_SSCRIPTS:
        return LocaleData->Scripts;
    case LOCALE_SPARENT:
    case LOCALE_SCONSOLEFALLBACKNAME:
        return LocaleData->Parent;
    case LOCALE_SPERCENT:
        return L"%";
    case LOCALE_SPERMILLE:
        return L"\x2030";
    case LOCALE_SMONTHDAY:
        if (LocaleData->Locale == 0x0412) {
            return L"M'\xc6d4' d'\xc77c'";
        }
        if (LocaleData->Locale == 0x0804 ||
            LocaleData->Locale == 0x0411) {
            return L"M'\x6708'd'\x65e5'";
        }
        if (LocaleData->Locale == 0x0419) {
            return L"d MMMM";
        }
        return L"MMMM d";
#ifdef LOCALE_SSHORTESTAM
    case LOCALE_SSHORTESTAM:
        return L"A";
#endif
#ifdef LOCALE_SSHORTESTPM
    case LOCALE_SSHORTESTPM:
        return L"P";
#endif
    case LOCALE_SOPENTYPELANGUAGETAG:
        switch (LocaleData->Locale) {
        case 0x0409:
        case 0x0809:
            return L"ENG ";
        case 0x0407:
            return L"DEU ";
        case 0x0412:
            return L"KOR ";
        case 0x0804:
            return L"ZHS ";
        case 0x0411:
            return L"JAN ";
        case 0x0419:
            return L"RUS ";
        }
        return L"latn";
    }

    return NULL;
}

WINBASEAPI
int
WINAPI
GetLocaleInfoW (
    _In_ LCID     Locale,
    _In_ LCTYPE   LCType,
    _Out_writes_opt_(cchData) LPWSTR lpLCData,
    _In_ int      cchData
    )
{
    DWORD Value;
    LCTYPE BaseType = LCType & ~(LOCALE_RETURN_NUMBER | LOCALE_USE_CP_ACP | LOCALE_NOUSEROVERRIDE);

    if (! IsValidLocale( Locale,
                         LCID_SUPPORTED )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (LCType & LOCALE_RETURN_NUMBER) {
        if (! LdkpGetLocaleNumber( Locale,
                                   BaseType,
                                   &Value )) {
            SetLastError( ERROR_INVALID_PARAMETER );
            return 0;
        }

        return LdkpCopyLocaleNumber( Value,
                                     lpLCData,
                                     cchData );
    }

    PCWSTR StringValue = LdkpGetLocaleString( Locale,
                                              BaseType );
    if (StringValue == NULL) {
        if (LdkpGetLocaleNumber( Locale,
                                 BaseType,
                                 &Value )) {
            WCHAR NumberString[16];
            PWSTR Cursor = NumberString;
            SIZE_T Remaining = RTL_NUMBER_OF(NumberString);
            BOOLEAN Appended;

            if (BaseType == LOCALE_IDEFAULTEBCDICCODEPAGE &&
                Value < 100) {
                Appended = LdkpAppendPaddedNumber( &Cursor,
                                                   &Remaining,
                                                   Value,
                                                   3 );
            } else {
                Appended = LdkpAppendNumber( &Cursor,
                                             &Remaining,
                                             Value );
            }

            if (! Appended) {
                SetLastError( ERROR_INSUFFICIENT_BUFFER );
                return 0;
            }

            return LdkpCopyLocaleString( NumberString,
                                         lpLCData,
                                         cchData );
        }

        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    return LdkpCopyLocaleString( StringValue,
                                 lpLCData,
                                 cchData );
}

WINBASEAPI
int
WINAPI
GetLocaleInfoA (
    _In_ LCID Locale,
    _In_ LCTYPE LCType,
    _Out_writes_opt_(cchData) LPSTR lpLCData,
    _In_ int cchData
    )
{
    PWSTR WideBuffer;
    int WideChars;
    int Result;

    if (cchData < 0 || (lpLCData == NULL && cchData != 0)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (LCType & LOCALE_RETURN_NUMBER) {
        DWORD Value = 0;

        Result = GetLocaleInfoW( Locale,
                                 LCType,
                                 (LPWSTR)&Value,
                                 sizeof(Value) / sizeof(WCHAR) );
        if (Result == 0) {
            return 0;
        }

        if (cchData == 0) {
            return sizeof(Value);
        }

        if (cchData < (int)sizeof(Value)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return 0;
        }

        RtlCopyMemory( lpLCData,
                       &Value,
                       sizeof(Value) );
        return sizeof(Value);
    }

    WideChars = GetLocaleInfoW( Locale,
                                LCType,
                                NULL,
                                0 );
    if (WideChars == 0) {
        return 0;
    }

    WideBuffer = RtlAllocateHeap( RtlProcessHeap(),
                                  HEAP_ZERO_MEMORY,
                                  (SIZE_T)WideChars * sizeof(WCHAR) );
    if (WideBuffer == NULL) {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    Result = GetLocaleInfoW( Locale,
                             LCType,
                             WideBuffer,
                             WideChars );
    if (Result != 0) {
        Result = LdkpWideStringToAnsiForNls( WideBuffer,
                                             -1,
                                             lpLCData,
                                             cchData );
    }

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 WideBuffer );
    return Result;
}

WINBASEAPI
int
WINAPI
GetLocaleInfoEx (
    _In_opt_ LPCWSTR lpLocaleName,
    _In_ LCTYPE LCType,
    _Out_writes_to_opt_(cchData, return) LPWSTR lpLCData,
    _In_ int cchData
    )
{
    LCID Locale;

    if (lpLocaleName == LOCALE_NAME_USER_DEFAULT) {
        Locale = GetUserDefaultLCID();
    } else if (_wcsicmp(lpLocaleName, LOCALE_NAME_SYSTEM_DEFAULT) == 0) {
        Locale = LdkpSystemLocale;
        if (Locale == 0) {
            Locale = 0x0409;
        }
    } else if (lpLocaleName[0] == L'\0') {
        Locale = 0x0409;
    } else {
        Locale = LocaleNameToLCID( lpLocaleName,
                                   0 );
    }

    if (Locale == 0) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    return GetLocaleInfoW( Locale,
                           LCType,
                           lpLCData,
                           cchData );
}

static
PCWSTR
LdkpGetSystemTimeDayName (
    _In_ LCID Locale,
    _In_ WORD DayOfWeek,
    _In_ BOOL Abbreviated
    )
{
    LCTYPE BaseType;
    WORD MondayBased;

    MondayBased = DayOfWeek == 0 ? 6 : (WORD)(DayOfWeek - 1);
    BaseType = Abbreviated ? LOCALE_SABBREVDAYNAME1 : LOCALE_SDAYNAME1;
    return LdkpGetLocaleString( Locale,
                                BaseType + MondayBased );
}

static
PCWSTR
LdkpGetSystemTimeMonthName (
    _In_ LCID Locale,
    _In_ WORD Month,
    _In_ BOOL Abbreviated
    )
{
    LCTYPE BaseType;

    if (Month < 1 || Month > 12) {
        return L"";
    }

    BaseType = Abbreviated ? LOCALE_SABBREVMONTHNAME1 : LOCALE_SMONTHNAME1;
    return LdkpGetLocaleString( Locale,
                                BaseType + Month - 1 );
}

static
BOOLEAN
LdkpAppendDateFormatToken (
    _Inout_ PWSTR *Cursor,
    _Inout_ PSIZE_T Remaining,
    _In_ LCID Locale,
    _In_ const SYSTEMTIME *Date,
    _In_ WCHAR Token,
    _In_ ULONG Count
    )
{
    switch (Token) {
    case L'y':
        if (Count <= 2) {
            return LdkpAppendPaddedNumber( Cursor,
                                           Remaining,
                                           Date->wYear % 100,
                                           2 );
        }
        return LdkpAppendPaddedNumber( Cursor,
                                       Remaining,
                                       Date->wYear,
                                       4 );
    case L'M':
        if (Count >= 4) {
            return LdkpAppendString( Cursor,
                                     Remaining,
                                     LdkpGetSystemTimeMonthName( Locale,
                                                                 Date->wMonth,
                                                                 FALSE ) );
        }
        if (Count == 3) {
            return LdkpAppendString( Cursor,
                                     Remaining,
                                     LdkpGetSystemTimeMonthName( Locale,
                                                                 Date->wMonth,
                                                                 TRUE ) );
        }
        return Count == 2 ?
               LdkpAppendPaddedNumber( Cursor,
                                        Remaining,
                                        Date->wMonth,
                                        2 ) :
               LdkpAppendNumber( Cursor,
                                  Remaining,
                                  Date->wMonth );
    case L'd':
        if (Count >= 4) {
            return LdkpAppendString( Cursor,
                                     Remaining,
                                     LdkpGetSystemTimeDayName( Locale,
                                                               Date->wDayOfWeek,
                                                               FALSE ) );
        }
        if (Count == 3) {
            return LdkpAppendString( Cursor,
                                     Remaining,
                                     LdkpGetSystemTimeDayName( Locale,
                                                               Date->wDayOfWeek,
                                                               TRUE ) );
        }
        return Count == 2 ?
               LdkpAppendPaddedNumber( Cursor,
                                        Remaining,
                                        Date->wDay,
                                        2 ) :
               LdkpAppendNumber( Cursor,
                                  Remaining,
                                  Date->wDay );
    default:
        while (Count != 0) {
            if (*Remaining <= 1) {
                return FALSE;
            }

            **Cursor = Token;
            (*Cursor)++;
            (*Remaining)--;
            Count--;
        }
        **Cursor = UNICODE_NULL;
        return TRUE;
    }
}

static
BOOLEAN
LdkpAppendDateFormat (
    _Inout_ PWSTR *Cursor,
    _Inout_ PSIZE_T Remaining,
    _In_ LCID Locale,
    _In_ const SYSTEMTIME *Date,
    _In_ PCWSTR Format
    )
{
    while (*Format != UNICODE_NULL) {
        WCHAR Token = *Format++;
        ULONG Count = 1;

        if (Token == L'\'') {
            while (*Format != UNICODE_NULL && *Format != L'\'') {
                WCHAR Literal[2] = { *Format++, UNICODE_NULL };
                if (! LdkpAppendString( Cursor,
                                        Remaining,
                                        Literal )) {
                    return FALSE;
                }
            }
            if (*Format == L'\'') {
                Format++;
            }
            continue;
        }

        while (*Format == Token) {
            Count++;
            Format++;
        }

        if (! LdkpAppendDateFormatToken( Cursor,
                                         Remaining,
                                         Locale,
                                         Date,
                                         Token,
                                         Count )) {
            return FALSE;
        }
    }

    return TRUE;
}

static
UINT
LdkpGetTwelveHour (
    _In_ WORD Hour
    )
{
    UINT TwelveHour = Hour % 12;

    return TwelveHour == 0 ? 12 : TwelveHour;
}

static
BOOLEAN
LdkpAppendTimeFormatToken (
    _Inout_ PWSTR *Cursor,
    _Inout_ PSIZE_T Remaining,
    _In_ LCID Locale,
    _In_ const SYSTEMTIME *Time,
    _In_ DWORD Flags,
    _In_ WCHAR Token,
    _In_ ULONG Count
    )
{
    switch (Token) {
    case L'H':
        return Count == 2 ?
               LdkpAppendPaddedNumber( Cursor,
                                        Remaining,
                                        Time->wHour,
                                        2 ) :
               LdkpAppendNumber( Cursor,
                                  Remaining,
                                  Time->wHour );
    case L'h':
        if (Flags & TIME_FORCE24HOURFORMAT) {
            return Count == 2 ?
                   LdkpAppendPaddedNumber( Cursor,
                                            Remaining,
                                            Time->wHour,
                                            2 ) :
                   LdkpAppendNumber( Cursor,
                                      Remaining,
                                      Time->wHour );
        }
        return Count == 2 ?
               LdkpAppendPaddedNumber( Cursor,
                                        Remaining,
                                        LdkpGetTwelveHour(Time->wHour),
                                        2 ) :
               LdkpAppendNumber( Cursor,
                                  Remaining,
                                  LdkpGetTwelveHour(Time->wHour) );
    case L'm':
        if (Flags & TIME_NOMINUTESORSECONDS) {
            return TRUE;
        }
        return Count == 2 ?
               LdkpAppendPaddedNumber( Cursor,
                                        Remaining,
                                        Time->wMinute,
                                        2 ) :
               LdkpAppendNumber( Cursor,
                                  Remaining,
                                  Time->wMinute );
    case L's':
        if (Flags & (TIME_NOMINUTESORSECONDS | TIME_NOSECONDS)) {
            return TRUE;
        }
        return Count == 2 ?
               LdkpAppendPaddedNumber( Cursor,
                                        Remaining,
                                        Time->wSecond,
                                        2 ) :
               LdkpAppendNumber( Cursor,
                                  Remaining,
                                  Time->wSecond );
    case L't':
        if (Flags & (TIME_NOTIMEMARKER | TIME_FORCE24HOURFORMAT)) {
            return TRUE;
        }
        return LdkpAppendString( Cursor,
                                 Remaining,
                                 Time->wHour < 12 ?
                                     LdkpGetLocaleString(Locale, LOCALE_SAM) :
                                     LdkpGetLocaleString(Locale, LOCALE_SPM) );
    default:
        while (Count != 0) {
            if (*Remaining <= 1) {
                return FALSE;
            }

            **Cursor = Token;
            (*Cursor)++;
            (*Remaining)--;
            Count--;
        }
        **Cursor = UNICODE_NULL;
        return TRUE;
    }
}

static
BOOLEAN
LdkpAppendTimeFormat (
    _Inout_ PWSTR *Cursor,
    _Inout_ PSIZE_T Remaining,
    _In_ LCID Locale,
    _In_ const SYSTEMTIME *Time,
    _In_ DWORD Flags,
    _In_ PCWSTR Format
    )
{
    while (*Format != UNICODE_NULL) {
        WCHAR Token = *Format++;
        ULONG Count = 1;

        if (Token == L'\'') {
            while (*Format != UNICODE_NULL && *Format != L'\'') {
                WCHAR Literal[2] = { *Format++, UNICODE_NULL };
                if (! LdkpAppendString( Cursor,
                                        Remaining,
                                        Literal )) {
                    return FALSE;
                }
            }
            if (*Format == L'\'') {
                Format++;
            }
            continue;
        }

        while (*Format == Token) {
            Count++;
            Format++;
        }

        if (! LdkpAppendTimeFormatToken( Cursor,
                                         Remaining,
                                         Locale,
                                         Time,
                                         Flags,
                                         Token,
                                         Count )) {
            return FALSE;
        }
    }

    return TRUE;
}

WINBASEAPI
int
WINAPI
GetDateFormatW (
    _In_ LCID Locale,
    _In_ DWORD dwFlags,
    _In_opt_ CONST SYSTEMTIME* lpDate,
    _In_opt_ LPCWSTR lpFormat,
    _Out_writes_opt_(cchDate) LPWSTR lpDateStr,
    _In_ int cchDate
    )
{
    SYSTEMTIME CurrentDate;
    WCHAR Buffer[160];
    PWSTR Cursor = Buffer;
    SIZE_T Remaining = RTL_NUMBER_OF(Buffer);
    PCWSTR Format = lpFormat;

    if (! IsValidLocale( Locale,
                         LCID_SUPPORTED )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (lpDate == NULL) {
        GetLocalTime( &CurrentDate );
        lpDate = &CurrentDate;
    }

    if (Format == NULL) {
        LCTYPE FormatType = LOCALE_SSHORTDATE;

        if (dwFlags & DATE_LONGDATE) {
            FormatType = LOCALE_SLONGDATE;
#ifdef DATE_YEARMONTH
        } else if (dwFlags & DATE_YEARMONTH) {
            FormatType = LOCALE_SYEARMONTH;
#endif
#ifdef DATE_MONTHDAY
        } else if (dwFlags & DATE_MONTHDAY) {
            FormatType = LOCALE_SMONTHDAY;
#endif
        }

        Format = LdkpGetLocaleString( Locale,
                                      FormatType );
    }

    *Buffer = UNICODE_NULL;
    if (! LdkpAppendDateFormat( &Cursor,
                                &Remaining,
                                Locale,
                                lpDate,
                                Format )) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    return LdkpCopyLocaleString( Buffer,
                                 lpDateStr,
                                 cchDate );
}

WINBASEAPI
int
WINAPI
GetTimeFormatW (
    _In_ LCID Locale,
    _In_ DWORD dwFlags,
    _In_opt_ CONST SYSTEMTIME* lpTime,
    _In_opt_ LPCWSTR lpFormat,
    _Out_writes_opt_(cchTime) LPWSTR lpTimeStr,
    _In_ int cchTime
    )
{
    SYSTEMTIME CurrentTime;
    WCHAR Buffer[96];
    PWSTR Cursor = Buffer;
    SIZE_T Remaining = RTL_NUMBER_OF(Buffer);
    PCWSTR Format = lpFormat;

    if (! IsValidLocale( Locale,
                         LCID_SUPPORTED )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (lpTime == NULL) {
        GetLocalTime( &CurrentTime );
        lpTime = &CurrentTime;
    }

    if (Format == NULL) {
        Format = LdkpGetLocaleString( Locale,
                                      (dwFlags & TIME_NOSECONDS) ?
                                          LOCALE_SSHORTTIME :
                                          LOCALE_STIMEFORMAT );
    }

    *Buffer = UNICODE_NULL;
    if (! LdkpAppendTimeFormat( &Cursor,
                                &Remaining,
                                Locale,
                                lpTime,
                                dwFlags,
                                Format )) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    return LdkpCopyLocaleString( Buffer,
                                 lpTimeStr,
                                 cchTime );
}

static
BOOL
LdkpResolveLocaleNameForNls (
    _In_opt_ LPCWSTR LocaleName,
    _Out_ PLCID Locale
    )
{
    if (Locale == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    if (LocaleName == LOCALE_NAME_USER_DEFAULT) {
        *Locale = GetUserDefaultLCID();
    } else if (_wcsicmp(LocaleName, LOCALE_NAME_SYSTEM_DEFAULT) == 0) {
        *Locale = LdkpSystemLocale != 0 ? LdkpSystemLocale : 0x0409;
    } else if (LocaleName[0] == UNICODE_NULL) {
        *Locale = 0x0409;
    } else {
        *Locale = LocaleNameToLCID( LocaleName,
                                    0 );
    }

    if (*Locale == 0 ||
        ! IsValidLocale( *Locale,
                         LCID_SUPPORTED )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return TRUE;
}

WINBASEAPI
int
WINAPI
GetTimeFormatEx (
    _In_opt_ LPCWSTR lpLocaleName,
    _In_ DWORD dwFlags,
    _In_opt_ CONST SYSTEMTIME* lpTime,
    _In_opt_ LPCWSTR lpFormat,
    _Out_writes_opt_(cchTime) LPWSTR lpTimeStr,
    _In_ int cchTime
    )
{
    LCID Locale;

    PAGED_CODE();

    if (! LdkpResolveLocaleNameForNls( lpLocaleName,
                                       &Locale )) {
        return 0;
    }

    return GetTimeFormatW( Locale,
                           dwFlags,
                           lpTime,
                           lpFormat,
                           lpTimeStr,
                           cchTime );
}

WINBASEAPI
int
WINAPI
GetDateFormatEx (
    _In_opt_ LPCWSTR lpLocaleName,
    _In_ DWORD dwFlags,
    _In_opt_ CONST SYSTEMTIME* lpDate,
    _In_opt_ LPCWSTR lpFormat,
    _Out_writes_opt_(cchDate) LPWSTR lpDateStr,
    _In_ int cchDate,
    _In_opt_ LPCWSTR lpCalendar
    )
{
    LCID Locale;

    PAGED_CODE();

    if (lpCalendar != NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (! LdkpResolveLocaleNameForNls( lpLocaleName,
                                       &Locale )) {
        return 0;
    }

    return GetDateFormatW( Locale,
                           dwFlags,
                           lpDate,
                           lpFormat,
                           lpDateStr,
                           cchDate );
}

static
int
LdkpCompareResultFromDifference (
    _In_ int Difference
    )
{
    if (Difference < 0) {
        return CSTR_LESS_THAN;
    }

    if (Difference > 0) {
        return CSTR_GREATER_THAN;
    }

    return CSTR_EQUAL;
}

static
BOOL
LdkpGetNlsCompareLength (
    _In_z_count_(cchString) LPCWSTR String,
    _In_ int cchString,
    _Out_ int *Length
    )
{
    if (String == NULL || cchString < -1 || Length == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *Length = cchString == -1 ? (int)wcslen(String) : cchString;
    return TRUE;
}

static
BOOL
LdkpGetNlsMapLength (
    _In_z_count_(cchString) LPCWSTR String,
    _In_ int cchString,
    _Out_ int *Length
    )
{
    if (String == NULL || cchString < -1 || Length == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    *Length = cchString == -1 ? (int)wcslen(String) + 1 : cchString;
    return TRUE;
}

static
WCHAR
LdkpDowncaseUnicodeCharForNls (
    _In_ WCHAR Ch
    )
{
    if (Ch >= L'A' && Ch <= L'Z') {
        return Ch + (L'a' - L'A');
    }

    if (Ch >= 0x0410 && Ch <= 0x042f) {
        return Ch + 0x20;
    }

    if (Ch == 0x0401) {
        return 0x0451;
    }

    return Ch;
}

static
WCHAR
LdkpFoldUnicodeCharForNls (
    _In_ WCHAR Ch,
    _In_ DWORD Flags
    )
{
    if (Flags & (NORM_IGNORECASE | LINGUISTIC_IGNORECASE | NORM_LINGUISTIC_CASING)) {
        return RtlUpcaseUnicodeChar( Ch );
    }

    return Ch;
}

static
BOOL
LdkpIsIgnorableSymbolForNls (
    _In_ WCHAR Ch,
    _In_ DWORD Flags
    )
{
    if (! (Flags & NORM_IGNORESYMBOLS)) {
        return FALSE;
    }

    if (Ch > 0x7f) {
        return FALSE;
    }

    if ((Ch >= L'0' && Ch <= L'9') ||
        (Ch >= L'A' && Ch <= L'Z') ||
        (Ch >= L'a' && Ch <= L'z')) {
        return FALSE;
    }

    return Ch != L' ' && Ch != L'\t' && Ch != L'\r' && Ch != L'\n';
}

static
BOOL
LdkpValidateCompareFlags (
    _In_ DWORD Flags
    )
{
    const DWORD ValidFlags = NORM_IGNORECASE |
                             LINGUISTIC_IGNORECASE |
                             NORM_LINGUISTIC_CASING |
                             NORM_IGNORESYMBOLS |
                             SORT_STRINGSORT;

    if (Flags & ~ValidFlags) {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    return TRUE;
}

static
int
LdkpCompareStringOrdinalWorker (
    _In_reads_(Length1) LPCWSTR String1,
    _In_ int Length1,
    _In_reads_(Length2) LPCWSTR String2,
    _In_ int Length2,
    _In_ DWORD Flags
    )
{
    int Index1 = 0;
    int Index2 = 0;

    while (Index1 < Length1 || Index2 < Length2) {
        WCHAR Ch1;
        WCHAR Ch2;

        while (Index1 < Length1 &&
               LdkpIsIgnorableSymbolForNls( String1[Index1],
                                             Flags )) {
            Index1++;
        }

        while (Index2 < Length2 &&
               LdkpIsIgnorableSymbolForNls( String2[Index2],
                                             Flags )) {
            Index2++;
        }

        if (Index1 >= Length1 || Index2 >= Length2) {
            return LdkpCompareResultFromDifference( (Index1 < Length1) - (Index2 < Length2) );
        }

        Ch1 = LdkpFoldUnicodeCharForNls( String1[Index1],
                                         Flags );
        Ch2 = LdkpFoldUnicodeCharForNls( String2[Index2],
                                         Flags );
        if (Ch1 != Ch2) {
            return LdkpCompareResultFromDifference( Ch1 < Ch2 ? -1 : 1 );
        }

        Index1++;
        Index2++;
    }

    return CSTR_EQUAL;
}

WINBASEAPI
int
WINAPI
CompareStringOrdinal (
    _In_NLS_string_(cchCount1) LPCWCH lpString1,
    _In_ int cchCount1,
    _In_NLS_string_(cchCount2) LPCWCH lpString2,
    _In_ int cchCount2,
    _In_ BOOL bIgnoreCase
    )
{
    int Length1;
    int Length2;

    if (! LdkpGetNlsCompareLength( lpString1,
                                   cchCount1,
                                   &Length1 ) ||
        ! LdkpGetNlsCompareLength( lpString2,
                                   cchCount2,
                                   &Length2 )) {
        return 0;
    }

    return LdkpCompareStringOrdinalWorker( lpString1,
                                           Length1,
                                           lpString2,
                                           Length2,
                                           bIgnoreCase ? NORM_IGNORECASE : 0 );
}

WINBASEAPI
int
WINAPI
CompareStringEx (
    _In_opt_ LPCWSTR lpLocaleName,
    _In_ DWORD dwCmpFlags,
    _In_NLS_string_(cchCount1) LPCWCH lpString1,
    _In_ int cchCount1,
    _In_NLS_string_(cchCount2) LPCWCH lpString2,
    _In_ int cchCount2,
    _Reserved_ LPNLSVERSIONINFO lpVersionInformation,
    _Reserved_ LPVOID lpReserved,
    _Reserved_ LPARAM lParam
    )
{
    LCID Locale;
    int Length1;
    int Length2;

    UNREFERENCED_PARAMETER(lpVersionInformation);

    if (lpReserved != NULL || lParam != 0) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (! LdkpValidateCompareFlags( dwCmpFlags ) ||
        ! LdkpResolveLocaleNameForNls( lpLocaleName,
                                       &Locale ) ||
        ! LdkpGetNlsCompareLength( lpString1,
                                   cchCount1,
                                   &Length1 ) ||
        ! LdkpGetNlsCompareLength( lpString2,
                                   cchCount2,
                                   &Length2 )) {
        return 0;
    }

    UNREFERENCED_PARAMETER(Locale);

    return LdkpCompareStringOrdinalWorker( lpString1,
                                           Length1,
                                           lpString2,
                                           Length2,
                                           dwCmpFlags );
}

WINBASEAPI
int
WINAPI
CompareStringW (
    _In_ LCID Locale,
    _In_ DWORD dwCmpFlags,
    _In_NLS_string_(cchCount1) LPCWCH lpString1,
    _In_ int cchCount1,
    _In_NLS_string_(cchCount2) LPCWCH lpString2,
    _In_ int cchCount2
    )
{
    WCHAR LocaleName[LOCALE_NAME_MAX_LENGTH];

    if (LCIDToLocaleName( Locale,
                          LocaleName,
                          RTL_NUMBER_OF(LocaleName),
                          0 ) == 0) {
        return 0;
    }

    return CompareStringEx( LocaleName,
                            dwCmpFlags,
                            lpString1,
                            cchCount1,
                            lpString2,
                            cchCount2,
                            NULL,
                            NULL,
                            0 );
}

WINBASEAPI
int
WINAPI
CompareStringA (
    _In_ LCID Locale,
    _In_ DWORD dwCmpFlags,
    _In_reads_(cchCount1) LPCSTR lpString1,
    _In_ int cchCount1,
    _In_reads_(cchCount2) LPCSTR lpString2,
    _In_ int cchCount2
    )
{
    PWSTR WideString1;
    PWSTR WideString2;
    int WideCount1;
    int WideCount2;
    int Result;

    if (! LdkpAnsiStringToWideForNls( lpString1,
                                      cchCount1,
                                      &WideString1,
                                      &WideCount1 )) {
        return 0;
    }

    if (! LdkpAnsiStringToWideForNls( lpString2,
                                      cchCount2,
                                      &WideString2,
                                      &WideCount2 )) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     WideString1 );
        return 0;
    }

    Result = CompareStringW( Locale,
                             dwCmpFlags,
                             WideString1,
                             WideCount1,
                             WideString2,
                             WideCount2 );

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 WideString2 );
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 WideString1 );
    return Result;
}

static
BOOL
LdkpValidateMapFlags (
    _In_ DWORD Flags
    )
{
    const DWORD CaseMask = LCMAP_TITLECASE;
    DWORD CaseFlags = Flags & CaseMask;

    if (Flags & LCMAP_SORTKEY) {
        const DWORD ValidSortKeyFlags = LCMAP_SORTKEY |
                                        NORM_IGNORECASE |
                                        LINGUISTIC_IGNORECASE |
                                        NORM_LINGUISTIC_CASING |
                                        NORM_IGNORESYMBOLS |
                                        SORT_STRINGSORT;

        if (Flags & ~ValidSortKeyFlags) {
            SetLastError( ERROR_INVALID_FLAGS );
            return FALSE;
        }

        return TRUE;
    }

    if (CaseFlags != 0 &&
        CaseFlags != LCMAP_LOWERCASE &&
        CaseFlags != LCMAP_UPPERCASE &&
        CaseFlags != LCMAP_TITLECASE) {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    if (Flags & ~(LCMAP_TITLECASE | LCMAP_BYTEREV | LCMAP_LINGUISTIC_CASING)) {
        SetLastError( ERROR_INVALID_FLAGS );
        return FALSE;
    }

    return TRUE;
}

static
WCHAR
LdkpMapUnicodeCharForNls (
    _In_ WCHAR Ch,
    _In_ DWORD Flags,
    _In_ BOOL WordStart
    )
{
    DWORD CaseFlags = Flags & LCMAP_TITLECASE;

    if (CaseFlags == LCMAP_UPPERCASE) {
        Ch = RtlUpcaseUnicodeChar( Ch );
    } else if (CaseFlags == LCMAP_LOWERCASE) {
        Ch = LdkpDowncaseUnicodeCharForNls( Ch );
    } else if (CaseFlags == LCMAP_TITLECASE) {
        Ch = WordStart ? RtlUpcaseUnicodeChar( Ch ) : LdkpDowncaseUnicodeCharForNls( Ch );
    }

    if (Flags & LCMAP_BYTEREV) {
        Ch = (WCHAR)(((Ch & 0x00ff) << 8) | ((Ch & 0xff00) >> 8));
    }

    return Ch;
}

static
int
LdkpMapSortKeyForNls (
    _In_reads_(SourceLength) LPCWSTR Source,
    _In_ int SourceLength,
    _In_ DWORD Flags,
    _Out_writes_bytes_opt_(DestinationLength) PBYTE Destination,
    _In_ int DestinationLength
    )
{
    int Required = 1;
    int Output = 0;

    for (int Index = 0; Index < SourceLength; Index++) {
        if (! LdkpIsIgnorableSymbolForNls( Source[Index],
                                           Flags )) {
            Required++;
        }
    }

    if (Destination == NULL || DestinationLength == 0) {
        return Required;
    }

    if (DestinationLength < Required) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    for (int Index = 0; Index < SourceLength; Index++) {
        WCHAR Ch;

        if (LdkpIsIgnorableSymbolForNls( Source[Index],
                                         Flags )) {
            continue;
        }

        Ch = LdkpFoldUnicodeCharForNls( Source[Index],
                                        Flags );
        Destination[Output++] = Ch <= 0xff ? (BYTE)Ch : (BYTE)'?';
    }

    Destination[Output++] = 0;
    return Output;
}

WINBASEAPI
int
WINAPI
LCMapStringEx (
    _In_opt_ LPCWSTR lpLocaleName,
    _In_ DWORD dwMapFlags,
    _In_reads_(cchSrc) LPCWSTR lpSrcStr,
    _In_ int cchSrc,
    _Out_writes_opt_(cchDest) LPWSTR lpDestStr,
    _In_ int cchDest,
    _In_opt_ LPNLSVERSIONINFO lpVersionInformation,
    _In_opt_ LPVOID lpReserved,
    _In_opt_ LPARAM sortHandle
    )
{
    LCID Locale;
    int SourceLength;
    BOOL WordStart = TRUE;

    UNREFERENCED_PARAMETER(lpVersionInformation);

    if (lpReserved != NULL || sortHandle != 0 ||
        cchDest < 0 || (lpDestStr == NULL && cchDest != 0)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (! LdkpValidateMapFlags( dwMapFlags ) ||
        ! LdkpResolveLocaleNameForNls( lpLocaleName,
                                       &Locale ) ||
        ! LdkpGetNlsMapLength( lpSrcStr,
                               cchSrc,
                               &SourceLength )) {
        return 0;
    }

    UNREFERENCED_PARAMETER(Locale);

    if (dwMapFlags & LCMAP_SORTKEY) {
        if (cchSrc == -1 && SourceLength > 0) {
            SourceLength--;
        }

        return LdkpMapSortKeyForNls( lpSrcStr,
                                     SourceLength,
                                     dwMapFlags,
                                     (PBYTE)lpDestStr,
                                     cchDest );
    }

    if (lpDestStr == NULL || cchDest == 0) {
        return SourceLength;
    }

    if (cchDest < SourceLength) {
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return 0;
    }

    for (int Index = 0; Index < SourceLength; Index++) {
        WCHAR Source = lpSrcStr[Index];

        lpDestStr[Index] = LdkpMapUnicodeCharForNls( Source,
                                                     dwMapFlags,
                                                     WordStart );
        WordStart = Source == L' ' || Source == L'\t' ||
                    Source == L'\r' || Source == L'\n' ||
                    Source == L'-' || Source == L'_';
    }

    return SourceLength;
}

WINBASEAPI
int
WINAPI
LCMapStringW (
    _In_ LCID Locale,
    _In_ DWORD dwMapFlags,
    _In_reads_(cchSrc) LPCWSTR lpSrcStr,
    _In_ int cchSrc,
    _Out_writes_opt_(cchDest) LPWSTR lpDestStr,
    _In_ int cchDest
    )
{
    WCHAR LocaleName[LOCALE_NAME_MAX_LENGTH];

    if (LCIDToLocaleName( Locale,
                          LocaleName,
                          RTL_NUMBER_OF(LocaleName),
                          0 ) == 0) {
        return 0;
    }

    return LCMapStringEx( LocaleName,
                          dwMapFlags,
                          lpSrcStr,
                          cchSrc,
                          lpDestStr,
                          cchDest,
                          NULL,
                          NULL,
                          0 );
}

WINBASEAPI
int
WINAPI
LCMapStringA (
    _In_ LCID Locale,
    _In_ DWORD dwMapFlags,
    _In_reads_(cchSrc) LPCSTR lpSrcStr,
    _In_ int cchSrc,
    _Out_writes_opt_(cchDest) LPSTR lpDestStr,
    _In_ int cchDest
    )
{
    PWSTR WideSource;
    PWSTR WideDestination;
    int WideSourceCount;
    int WideDestinationCount;
    int Result;

    if (cchDest < 0 || (lpDestStr == NULL && cchDest != 0)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    if (! LdkpAnsiStringToWideForNls( lpSrcStr,
                                      cchSrc,
                                      &WideSource,
                                      &WideSourceCount )) {
        return 0;
    }

    if (dwMapFlags & LCMAP_SORTKEY) {
        Result = LCMapStringW( Locale,
                               dwMapFlags,
                               WideSource,
                               WideSourceCount,
                               (LPWSTR)lpDestStr,
                               cchDest );
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     WideSource );
        return Result;
    }

    WideDestinationCount = LCMapStringW( Locale,
                                         dwMapFlags,
                                         WideSource,
                                         WideSourceCount,
                                         NULL,
                                         0 );
    if (WideDestinationCount == 0) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     WideSource );
        return 0;
    }

    WideDestination = RtlAllocateHeap( RtlProcessHeap(),
                                       HEAP_ZERO_MEMORY,
                                       (SIZE_T)WideDestinationCount * sizeof(WCHAR) );
    if (WideDestination == NULL) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     WideSource );
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return 0;
    }

    Result = LCMapStringW( Locale,
                           dwMapFlags,
                           WideSource,
                           WideSourceCount,
                           WideDestination,
                           WideDestinationCount );
    if (Result != 0) {
        Result = LdkpWideStringToAnsiForNls( WideDestination,
                                             Result,
                                             lpDestStr,
                                             cchDest );
    }

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 WideDestination );
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 WideSource );
    return Result;
}

WINBASEAPI
BOOL
WINAPI
IsValidLocaleName (
    _In_ LPCWSTR lpLocaleName
    )
{
    LCID Locale;

    if (lpLocaleName == NULL) {
        return FALSE;
    }

    return LdkpResolveLocaleNameForNls( lpLocaleName,
                                        &Locale );
}

WINBASEAPI
BOOL
WINAPI
GetStringTypeExW (
    _In_ LCID Locale,
    _In_ DWORD dwInfoType,
    _In_NLS_string_(cchSrc) LPCWCH lpSrcStr,
    _In_ int cchSrc,
    _Out_ LPWORD lpCharType
    )
{
    if (! IsValidLocale( Locale,
                         LCID_SUPPORTED )) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    return GetStringTypeW( dwInfoType,
                           lpSrcStr,
                           cchSrc,
                           lpCharType );
}

WINBASEAPI
BOOL
WINAPI
GetStringTypeA (
    _In_ LCID Locale,
    _In_ DWORD dwInfoType,
    _In_NLS_string_(cchSrc) LPCSTR lpSrcStr,
    _In_ int cchSrc,
    _Out_ LPWORD lpCharType
    )
{
    PWSTR WideSource;
    int WideCount;
    BOOL Result;

    if (! LdkpAnsiStringToWideForNls( lpSrcStr,
                                      cchSrc,
                                      &WideSource,
                                      &WideCount )) {
        return FALSE;
    }

    Result = GetStringTypeExW( Locale,
                               dwInfoType,
                               WideSource,
                               WideCount,
                               lpCharType );

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 WideSource );
    return Result;
}

WINBASEAPI
BOOL
WINAPI
GetStringTypeExA (
    _In_ LCID Locale,
    _In_ DWORD dwInfoType,
    _In_NLS_string_(cchSrc) LPCSTR lpSrcStr,
    _In_ int cchSrc,
    _Out_ LPWORD lpCharType
    )
{
    return GetStringTypeA( Locale,
                           dwInfoType,
                           lpSrcStr,
                           cchSrc,
                           lpCharType );
}

WINBASEAPI
BOOL
WINAPI
EnumSystemLocalesW (
    _In_ LOCALE_ENUMPROCW lpLocaleEnumProc,
    _In_ DWORD dwFlags
    )
{
    static PWSTR LocaleNames[] = {
        L"0409",
        L"0407",
        L"0809",
        L"0412",
        L"0804",
        L"0411",
        L"0419",
    };
    ULONG Index;

    UNREFERENCED_PARAMETER(dwFlags);

    if (lpLocaleEnumProc == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(LocaleNames); Index++) {
        if (! lpLocaleEnumProc( LocaleNames[Index] )) {
            break;
        }
    }

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
EnumSystemLocalesEx (
    _In_ LOCALE_ENUMPROCEX lpLocaleEnumProcEx,
    _In_ DWORD dwFlags,
    _In_ LPARAM lParam,
    _In_opt_ LPVOID lpReserved
    )
{
    static PWSTR LocaleNames[] = {
        L"en-US",
        L"de-DE",
        L"en-GB",
        L"ko-KR",
        L"zh-CN",
        L"ja-JP",
        L"ru-RU",
    };
    ULONG Index;

    UNREFERENCED_PARAMETER(lpReserved);

    if (lpLocaleEnumProcEx == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    for (Index = 0; Index < RTL_NUMBER_OF(LocaleNames); Index++) {
        if (! lpLocaleEnumProcEx( LocaleNames[Index],
                                  dwFlags,
                                  lParam )) {
            break;
        }
    }

    return TRUE;
}
