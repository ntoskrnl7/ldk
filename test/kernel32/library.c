#if _KERNEL_MODE
#include <Ldk/Windows.h>
#include "../../src/teb.h"

BOOLEAN
LibraryTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LibraryTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))

#else
#include <windows.h>
#include <libloaderapi.h>
#include <stdio.h>
#define PAGED_CODE()
#endif

typedef LONG(__stdcall* TEST_FN)(LONG);
typedef VOID(__stdcall* TLS_RESET_FN)(VOID);
typedef VOID(__stdcall* TLS_SET_LOG_FN)(LONG *, LONG *, LONG);
typedef LONG(__stdcall* TLS_GET_COUNT_FN)(VOID);
typedef LONG(__stdcall* TLS_GET_EVENT_FN)(LONG);
typedef PVOID(__stdcall* TLS_STATIC_TLS_ACCESSOR_FN)(ULONG);
typedef VOID(__stdcall* TLS_SET_STATIC_TLS_ACCESSOR_FN)(TLS_STATIC_TLS_ACCESSOR_FN);
typedef ULONG(__stdcall* TLS_GET_STATIC_INDEX_FN)(VOID);
typedef LONG(__stdcall* TLS_GET_STATIC_VALUE_FN)(VOID);
typedef VOID(__stdcall* TLS_SET_STATIC_VALUE_FN)(LONG);
typedef VOID(__stdcall* THREAD_NOTIFY_RESET_FN)(VOID);
typedef VOID(__stdcall* THREAD_NOTIFY_SET_LOG_FN)(LONG *, LONG *, LONG);
typedef LONG(__stdcall* THREAD_NOTIFY_GET_COUNT_FN)(VOID);
typedef LONG(__stdcall* THREAD_NOTIFY_GET_EVENT_FN)(LONG);

#define TLS_EVENT_TLS_PROCESS_ATTACH 0x101
#define TLS_EVENT_DLL_PROCESS_ATTACH 0x102
#define TLS_EVENT_TLS_THREAD_ATTACH  0x201
#define TLS_EVENT_TLS_THREAD_DETACH  0x301
#define TLS_EVENT_TLS_PROCESS_DETACH 0x401
#define TLS_EVENT_DLL_PROCESS_DETACH 0x402
#define TLS_TEST_LOG_CAPACITY        16
#define TLS_STATIC_INITIAL_VALUE     7
#define TLS_STATIC_THREAD_VALUE      22
#define TLS_STATIC_MAIN_VALUE        11
#define TLS_STATIC_INVALID_INDEX     0xffffffff

#define THREAD_NOTIFY_EVENT_PROCESS_ATTACH 0x111
#define THREAD_NOTIFY_EVENT_THREAD_ATTACH  0x211
#define THREAD_NOTIFY_EVENT_THREAD_DETACH  0x311
#define THREAD_NOTIFY_EVENT_PROCESS_DETACH 0x411
#define THREAD_NOTIFY_TEST_LOG_CAPACITY    16

#if _KERNEL_MODE
static
PVOID
__stdcall
LdkTestGetStaticTlsBlock (
    _In_ ULONG Index
    )
{
    PLDK_TEB Teb = NtCurrentTeb();

    if (Index >= RTL_NUMBER_OF(Teb->TlsSlots)) {
        return NULL;
    }

    return Teb->TlsSlots[Index];
}
#else
#if defined(_M_AMD64) || defined(_M_ARM64) || defined(_M_ARM64EC)
#define LDK_TEST_TEB_STATIC_TLS_OFFSET 0x58
#else
#define LDK_TEST_TEB_STATIC_TLS_OFFSET 0x2c
#endif

static
PVOID
__stdcall
LdkTestGetStaticTlsBlock (
    _In_ ULONG Index
    )
{
    PVOID *TlsArray;

    if (Index >= 4096) {
        return NULL;
    }

    TlsArray = *(PVOID **)((PBYTE)NtCurrentTeb() + LDK_TEST_TEB_STATIC_TLS_OFFSET);
    if (!TlsArray) {
        return NULL;
    }

    return TlsArray[Index];
}
#endif

typedef struct _TLS_STATIC_THREAD_CONTEXT {
    TLS_GET_STATIC_VALUE_FN GetValue;
    TLS_SET_STATIC_VALUE_FN SetValue;
    LONG Counter;
    LONG Failure;
    LONG InitialValue;
    LONG FinalValue;
} TLS_STATIC_THREAD_CONTEXT, *PTLS_STATIC_THREAD_CONTEXT;

static
DWORD
WINAPI
TlsStaticTestThreadProc (
    _In_opt_ LPVOID Parameter
    )
{
    PTLS_STATIC_THREAD_CONTEXT Context = (PTLS_STATIC_THREAD_CONTEXT)Parameter;

    if (!Context ||
        !Context->GetValue ||
        !Context->SetValue) {
        return 1;
    }

    Context->InitialValue = Context->GetValue();
    if (Context->InitialValue != TLS_STATIC_INITIAL_VALUE) {
        Context->Failure = 1;
        return 1;
    }

    Context->SetValue( TLS_STATIC_THREAD_VALUE );
    Context->FinalValue = Context->GetValue();
    if (Context->FinalValue != TLS_STATIC_THREAD_VALUE) {
        Context->Failure = 2;
        return 1;
    }

    InterlockedIncrement( &Context->Counter );
    return 0;
}

static
DWORD
WINAPI
ThreadNotifyTestThreadProc (
    _In_opt_ LPVOID Parameter
    )
{
    LONG *Counter = (LONG *)Parameter;

    if (Counter) {
        InterlockedIncrement( Counter );
    }

    return 0;
}

static
BOOL
BuildTestDllPath (
    _In_ LPCWSTR DllName,
    _Out_writes_(BufferCch) LPWSTR Buffer,
    _In_ DWORD BufferCch
    )
{
    DWORD Length;
    DWORD DirectoryLength = 0;
    DWORD PrefixLength = 0;
    DWORD DllNameLength = 0;

    Length = GetModuleFileNameW( NULL,
                                 Buffer,
                                 BufferCch );
    if (Length == 0 ||
        Length >= BufferCch) {
        return FALSE;
    }

    if (Buffer[0] == L'\\' &&
        Buffer[1] == L'?' &&
        Buffer[2] == L'?' &&
        Buffer[3] == L'\\') {
        PrefixLength = 4;
    }

    for (DWORD Index = Length; Index != PrefixLength; Index--) {
        if (Buffer[Index - 1] == L'\\' ||
            Buffer[Index - 1] == L'/') {
            DirectoryLength = Index - 1;
            break;
        }
    }

    if (DirectoryLength == 0) {
        return FALSE;
    }

    if (PrefixLength) {
        DirectoryLength -= PrefixLength;
        RtlMoveMemory( Buffer,
                       Buffer + PrefixLength,
                       (DirectoryLength + 1) * sizeof(WCHAR) );
    }

    while (DllName[DllNameLength]) {
        DllNameLength++;
    }

    if (DirectoryLength + 1 + DllNameLength + 1 > BufferCch) {
        return FALSE;
    }

    Buffer[DirectoryLength] = L'\\';
    RtlCopyMemory( Buffer + DirectoryLength + 1,
                   DllName,
                   (DllNameLength + 1) * sizeof(WCHAR) );
    return TRUE;
}

static
BOOL
BuildTestDllDirectory (
    _Out_writes_(BufferCch) LPWSTR Buffer,
    _In_ DWORD BufferCch
    )
{
    if (!BuildTestDllPath( L"Test.dll",
                           Buffer,
                           BufferCch )) {
        return FALSE;
    }

    for (DWORD Index = (DWORD)wcslen(Buffer); Index != 0; Index--) {
        if (Buffer[Index - 1] == L'\\' ||
            Buffer[Index - 1] == L'/') {
            Buffer[Index - 1] = UNICODE_NULL;
            return TRUE;
        }
    }

    return FALSE;
}

static
BOOL
VerifyResourceOnlyLoad (
    _In_ LPCWSTR DllName,
    _In_ DWORD Flags,
    _In_ LPCSTR Label
    )
{
    HMODULE Module;

    Module = LoadLibraryExW( DllName,
                             NULL,
                             Flags );
    if (!Module) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW resource-only %s ErrorCode = %lu\n",
               Label,
               GetLastError());
       return FALSE;
    }

    if (GetProcAddress( Module, "TestFunction" ) != NULL) {
       fprintf(stderr,
               "[Failed] GetProcAddress unexpectedly succeeded for %s handle\n",
               Label);
       FreeLibrary( Module );
       return FALSE;
    }

    if (!FreeLibrary( Module )) {
       fprintf(stderr,
               "[Failed] FreeLibrary resource-only %s ErrorCode = %lu\n",
               Label,
               GetLastError());
       return FALSE;
    }

    printf("[Success] LoadLibraryExW %s\n", Label);
    return TRUE;
}

static
BOOL
CopyWideAsciiForTest (
    _In_ LPCWSTR Source,
    _Out_writes_(BufferCch) LPSTR Buffer,
    _In_ DWORD BufferCch
    )
{
    DWORD Index;

    for (Index = 0; Source[Index] != UNICODE_NULL; Index++) {
        if (Index + 1 >= BufferCch ||
            Source[Index] > 0x7f) {
            return FALSE;
        }
        Buffer[Index] = (CHAR)Source[Index];
    }

    Buffer[Index] = ANSI_NULL;
    return TRUE;
}

BOOLEAN
LibraryTest (
    VOID
    )
{
    PAGED_CODE();

    WCHAR DllDirectory[MAX_PATH];
    WCHAR DllDirectoryProbe[MAX_PATH];
    WCHAR DependencyOwnerPath[MAX_PATH];
    CHAR DllDirectoryA[MAX_PATH];
    CHAR DllDirectoryProbeA[MAX_PATH];
    HMODULE hFailAttach;

    printf("Library Test\n");

    if (!BuildTestDllDirectory( DllDirectory,
                                RTL_NUMBER_OF(DllDirectory) )) {
       fprintf(stderr,
               "[Failed] BuildTestDllDirectory ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    if (!SetDllDirectoryW( NULL )) {
       fprintf(stderr,
               "[Failed] SetDllDirectoryW(NULL) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetDllDirectoryW( 0,
                          NULL ) != 1) {
       fprintf(stderr,
               "[Failed] GetDllDirectoryW(empty query) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    DllDirectoryProbe[0] = L'X';
    if (GetDllDirectoryW( RTL_NUMBER_OF(DllDirectoryProbe),
                          DllDirectoryProbe ) != 0 ||
        DllDirectoryProbe[0] != UNICODE_NULL) {
       fprintf(stderr,
               "[Failed] GetDllDirectoryW(empty buffer) ErrorCode = %lu Value = %ws\n",
               GetLastError(),
               DllDirectoryProbe);
       return FALSE;
    }
    printf("[Success] GetDllDirectoryW empty state\n");

    HMODULE hModule = LoadLibraryW( L"Test.dll" );
    if (!hModule) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(Test.dll) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryW(Test.dll)\n");

    TEST_FN testFn = (TEST_FN)GetProcAddress( hModule, "TestFunction" );
    if (!testFn) {
       fprintf(stderr,
               "[Failed] GetProcAddress(TestFunction) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    if (testFn(10) != 10) {
       fprintf(stderr,
               "[Failed] TestFunction returned unexpected value\n");
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] GetProcAddress(TestFunction)\n");

    HMODULE hUnchanged = NULL;
    if (!GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             L"Test.dll",
                             &hUnchanged ) ||
        hUnchanged != hModule) {
       fprintf(stderr,
               "[Failed] GetModuleHandleExW(Test.dll, UNCHANGED_REFCOUNT) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }

    HMODULE hFromAddress = NULL;
    if (!GetModuleHandleExA( GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                             GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                             (LPCSTR)testFn,
                             &hFromAddress ) ||
        hFromAddress != hModule) {
       fprintf(stderr,
               "[Failed] GetModuleHandleExA(TestFunction, FROM_ADDRESS) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }

    HMODULE hReferenced = NULL;
    if (!GetModuleHandleExW( 0,
                             L"Test.dll",
                             &hReferenced ) ||
        hReferenced != hModule) {
       fprintf(stderr,
               "[Failed] GetModuleHandleExW(Test.dll) refcount ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }

    if (!FreeLibrary( hModule )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(Test.dll original reference) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hReferenced );
       return FALSE;
    }
    hModule = hReferenced;

    if (GetModuleHandleW( L"Test.dll" ) != hModule) {
       fprintf(stderr,
               "[Failed] GetModuleHandleExW(Test.dll) did not keep Test.dll loaded ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] GetModuleHandleEx refcount\n");

    HMODULE hModule2 = LoadLibraryW( L"Test.dll" );
    if (hModule2 != hModule) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(Test.dll) refcount handle mismatch ErrorCode = %lu\n",
               GetLastError());
       if (hModule2) {
          FreeLibrary( hModule2 );
       }
       FreeLibrary( hModule );
       return FALSE;
    }
    if (!FreeLibrary( hModule2 )) {
       fprintf(stderr,
               "[Failed] first FreeLibrary(Test.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    if (GetModuleHandleW( L"Test.dll" ) != hModule) {
       fprintf(stderr,
               "[Failed] Test.dll was unloaded before refcount reached zero ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryW(Test.dll) refcount\n");

    TEST_FN ordinalFn = (TEST_FN)GetProcAddress( hModule, (LPCSTR)(ULONG_PTR)7 );
    if (!ordinalFn) {
       fprintf(stderr,
               "[Failed] GetProcAddress(Test.dll, ordinal 7) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    if (ordinalFn(10) != 17) {
       fprintf(stderr,
               "[Failed] Test ordinal 7 returned unexpected value\n");
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] GetProcAddress(Test.dll, ordinal 7)\n");

    HMODULE hOrdinalImport = LoadLibraryW( L"OrdinalImport.dll" );
    if (!hOrdinalImport) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(OrdinalImport.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] LoadLibraryW(OrdinalImport.dll)\n");

    TEST_FN ordinalImportFn = (TEST_FN)GetProcAddress( hOrdinalImport, "TestOrdinalImportFunction" );
    if (!ordinalImportFn) {
       fprintf(stderr,
               "[Failed] GetProcAddress(TestOrdinalImportFunction) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hOrdinalImport );
       FreeLibrary( hModule );
       return FALSE;
    }
    if (ordinalImportFn(10) != 17) {
       fprintf(stderr,
               "[Failed] TestOrdinalImportFunction returned unexpected value\n");
       FreeLibrary( hOrdinalImport );
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] TestOrdinalImportFunction\n");
    if (!FreeLibrary( hOrdinalImport )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(OrdinalImport.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }

    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll was loaded before dependency-owner test ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }

    HMODULE hDependencyOwner = LoadLibraryW( L"DependencyOwner.dll" );
    if (!hDependencyOwner) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(DependencyOwner.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] LoadLibraryW(DependencyOwner.dll)\n");

    if (GetModuleHandleW( L"AutoDependency.dll" ) == NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll was not retained for DependencyOwner.dll ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDependencyOwner );
       FreeLibrary( hModule );
       return FALSE;
    }

    TEST_FN dependencyOwnerFn = (TEST_FN)GetProcAddress( hDependencyOwner, "DependencyOwnerFunction" );
    if (!dependencyOwnerFn) {
       fprintf(stderr,
               "[Failed] GetProcAddress(DependencyOwnerFunction) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDependencyOwner );
       FreeLibrary( hModule );
       return FALSE;
    }
    if (dependencyOwnerFn(10) != 34) {
       fprintf(stderr,
               "[Failed] DependencyOwnerFunction returned unexpected value\n");
       FreeLibrary( hDependencyOwner );
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] DependencyOwnerFunction\n");

    if (!FreeLibrary( hDependencyOwner )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after DependencyOwner.dll unload ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] DependencyOwner.dll dependency unload\n");

    hFailAttach = LoadLibraryW( L"FailAttach.dll" );
    if (hFailAttach != NULL) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(FailAttach.dll) unexpectedly succeeded\n");
       FreeLibrary( hFailAttach );
       FreeLibrary( hModule );
       return FALSE;
    }
    if (GetModuleHandleW( L"FailAttach.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] FailAttach.dll stayed loaded after DllMain failure ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after FailAttach.dll failure ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] LoadLibraryW(FailAttach.dll) DllMain failure cleanup\n");

    HMODULE hTlsCallback = LoadLibraryW( L"TlsCallback.dll" );
    if (!hTlsCallback) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(TlsCallback.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }

    TLS_RESET_FN tlsResetFn = (TLS_RESET_FN)GetProcAddress( hTlsCallback,
                                                            "TlsCallbackReset" );
    TLS_SET_LOG_FN tlsSetLogFn = (TLS_SET_LOG_FN)GetProcAddress( hTlsCallback,
                                                                 "TlsCallbackSetLog" );
    TLS_GET_COUNT_FN tlsGetCountFn = (TLS_GET_COUNT_FN)GetProcAddress( hTlsCallback,
                                                                       "TlsCallbackGetCount" );
    TLS_GET_EVENT_FN tlsGetEventFn = (TLS_GET_EVENT_FN)GetProcAddress( hTlsCallback,
                                                                       "TlsCallbackGetEvent" );
    TLS_SET_STATIC_TLS_ACCESSOR_FN tlsSetStaticTlsAccessorFn = (TLS_SET_STATIC_TLS_ACCESSOR_FN)GetProcAddress( hTlsCallback,
                                                                                                              "TlsCallbackSetStaticTlsAccessor" );
    TLS_GET_STATIC_INDEX_FN tlsGetStaticIndexFn = (TLS_GET_STATIC_INDEX_FN)GetProcAddress( hTlsCallback,
                                                                                          "TlsCallbackGetStaticIndex" );
    TLS_GET_STATIC_VALUE_FN tlsGetStaticValueFn = (TLS_GET_STATIC_VALUE_FN)GetProcAddress( hTlsCallback,
                                                                                          "TlsCallbackGetStaticValue" );
    TLS_SET_STATIC_VALUE_FN tlsSetStaticValueFn = (TLS_SET_STATIC_VALUE_FN)GetProcAddress( hTlsCallback,
                                                                                          "TlsCallbackSetStaticValue" );
    if (!tlsResetFn ||
        !tlsSetLogFn ||
        !tlsGetCountFn ||
        !tlsGetEventFn ||
        !tlsSetStaticTlsAccessorFn ||
        !tlsGetStaticIndexFn ||
        !tlsGetStaticValueFn ||
        !tlsSetStaticValueFn) {
       fprintf(stderr,
               "[Failed] TlsCallback.dll exports ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hTlsCallback );
       FreeLibrary( hModule );
       return FALSE;
    }

    if (tlsGetCountFn() < 2 ||
        tlsGetEventFn(0) != TLS_EVENT_TLS_PROCESS_ATTACH ||
        tlsGetEventFn(1) != TLS_EVENT_DLL_PROCESS_ATTACH) {
       fprintf(stderr,
               "[Failed] TlsCallback process attach order Count = %ld Event0 = 0x%lx Event1 = 0x%lx\n",
               tlsGetCountFn(),
               tlsGetEventFn(0),
               tlsGetEventFn(1));
       FreeLibrary( hTlsCallback );
       FreeLibrary( hModule );
        return FALSE;
    }

    tlsSetStaticTlsAccessorFn( LdkTestGetStaticTlsBlock );
    if (tlsGetStaticIndexFn() == TLS_STATIC_INVALID_INDEX ||
        tlsGetStaticValueFn() != TLS_STATIC_INITIAL_VALUE) {
       fprintf(stderr,
               "[Failed] TlsCallback static TLS main initial Index = %lu Value = %ld\n",
               tlsGetStaticIndexFn(),
               tlsGetStaticValueFn());
       FreeLibrary( hTlsCallback );
       FreeLibrary( hModule );
       return FALSE;
    }

    tlsSetStaticValueFn( TLS_STATIC_MAIN_VALUE );
    if (tlsGetStaticValueFn() != TLS_STATIC_MAIN_VALUE) {
       fprintf(stderr,
               "[Failed] TlsCallback static TLS main set Value = %ld\n",
               tlsGetStaticValueFn());
       FreeLibrary( hTlsCallback );
       FreeLibrary( hModule );
       return FALSE;
    }

    LONG TlsThreadLogCount = 0;
    LONG TlsThreadLog[TLS_TEST_LOG_CAPACITY] = { 0 };
    tlsSetLogFn( &TlsThreadLogCount,
                 TlsThreadLog,
                 TLS_TEST_LOG_CAPACITY );

    TLS_STATIC_THREAD_CONTEXT TlsStaticContext = { 0 };
    TlsStaticContext.GetValue = tlsGetStaticValueFn;
    TlsStaticContext.SetValue = tlsSetStaticValueFn;
    HANDLE hTlsThread = CreateThread( NULL,
                                      0,
                                      TlsStaticTestThreadProc,
                                      &TlsStaticContext,
                                      0,
                                      NULL );
    if (!hTlsThread) {
       fprintf(stderr,
               "[Failed] TlsCallback CreateThread ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hTlsCallback );
       FreeLibrary( hModule );
       return FALSE;
    }

    DWORD TlsWait = WaitForSingleObject( hTlsThread,
                                         5000 );
    CloseHandle( hTlsThread );
    if (TlsWait != WAIT_OBJECT_0 ||
        TlsStaticContext.Counter != 1 ||
        TlsStaticContext.Failure != 0 ||
        TlsStaticContext.InitialValue != TLS_STATIC_INITIAL_VALUE ||
        TlsStaticContext.FinalValue != TLS_STATIC_THREAD_VALUE ||
        tlsGetStaticValueFn() != TLS_STATIC_MAIN_VALUE ||
        TlsThreadLogCount < 2 ||
        TlsThreadLog[0] != TLS_EVENT_TLS_THREAD_ATTACH ||
        TlsThreadLog[1] != TLS_EVENT_TLS_THREAD_DETACH) {
       fprintf(stderr,
               "[Failed] TlsCallback thread/static TLS Wait = 0x%08lx Counter = %ld Failure = %ld Initial = %ld Final = %ld Main = %ld Count = %ld Event0 = 0x%lx Event1 = 0x%lx\n",
               TlsWait,
               TlsStaticContext.Counter,
               TlsStaticContext.Failure,
               TlsStaticContext.InitialValue,
               TlsStaticContext.FinalValue,
               tlsGetStaticValueFn(),
               TlsThreadLogCount,
               TlsThreadLog[0],
               TlsThreadLog[1]);
       FreeLibrary( hTlsCallback );
       FreeLibrary( hModule );
       return FALSE;
    }

    ULONG TlsStaticIndex = tlsGetStaticIndexFn();
    LONG TlsStaticMainValue = tlsGetStaticValueFn();

    LONG TlsDetachLogCount = 0;
    LONG TlsDetachLog[TLS_TEST_LOG_CAPACITY] = { 0 };
    tlsSetLogFn( &TlsDetachLogCount,
                 TlsDetachLog,
                 TLS_TEST_LOG_CAPACITY );

    if (!FreeLibrary( hTlsCallback )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(TlsCallback.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    hTlsCallback = NULL;

    if (TlsDetachLogCount < 2 ||
        TlsDetachLog[0] != TLS_EVENT_TLS_PROCESS_DETACH ||
        TlsDetachLog[1] != TLS_EVENT_DLL_PROCESS_DETACH) {
       fprintf(stderr,
               "[Failed] TlsCallback process detach order Count = %ld Event0 = 0x%lx Event1 = 0x%lx\n",
               TlsDetachLogCount,
               TlsDetachLog[0],
               TlsDetachLog[1]);
       FreeLibrary( hModule );
        return FALSE;
    }
    printf("[Success] TLS callback notifications and static TLS storage Index = %lu Main = %ld ThreadInitial = %ld ThreadFinal = %ld\n",
           TlsStaticIndex,
           TlsStaticMainValue,
           TlsStaticContext.InitialValue,
           TlsStaticContext.FinalValue);

    HMODULE hThreadNotify = LoadLibraryW( L"ThreadNotify.dll" );
    if (!hThreadNotify) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(ThreadNotify.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }

    THREAD_NOTIFY_RESET_FN threadNotifyResetFn = (THREAD_NOTIFY_RESET_FN)GetProcAddress( hThreadNotify,
                                                                                        "ThreadNotifyReset" );
    THREAD_NOTIFY_SET_LOG_FN threadNotifySetLogFn = (THREAD_NOTIFY_SET_LOG_FN)GetProcAddress( hThreadNotify,
                                                                                              "ThreadNotifySetLog" );
    THREAD_NOTIFY_GET_COUNT_FN threadNotifyGetCountFn = (THREAD_NOTIFY_GET_COUNT_FN)GetProcAddress( hThreadNotify,
                                                                                                    "ThreadNotifyGetCount" );
    THREAD_NOTIFY_GET_EVENT_FN threadNotifyGetEventFn = (THREAD_NOTIFY_GET_EVENT_FN)GetProcAddress( hThreadNotify,
                                                                                                    "ThreadNotifyGetEvent" );
    if (!threadNotifyResetFn ||
        !threadNotifySetLogFn ||
        !threadNotifyGetCountFn ||
        !threadNotifyGetEventFn) {
       fprintf(stderr,
               "[Failed] ThreadNotify.dll exports ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hThreadNotify );
       FreeLibrary( hModule );
       return FALSE;
    }

    if (threadNotifyGetCountFn() < 1 ||
        threadNotifyGetEventFn(0) != THREAD_NOTIFY_EVENT_PROCESS_ATTACH) {
       fprintf(stderr,
               "[Failed] ThreadNotify process attach Count = %ld Event0 = 0x%lx\n",
               threadNotifyGetCountFn(),
               threadNotifyGetEventFn(0));
       FreeLibrary( hThreadNotify );
       FreeLibrary( hModule );
       return FALSE;
    }

    LONG ThreadNotifyLogCount = 0;
    LONG ThreadNotifyLog[THREAD_NOTIFY_TEST_LOG_CAPACITY] = { 0 };
    threadNotifySetLogFn( &ThreadNotifyLogCount,
                          ThreadNotifyLog,
                          THREAD_NOTIFY_TEST_LOG_CAPACITY );

    LONG ThreadNotifyCounter = 0;
    HANDLE hThreadNotifyThread = CreateThread( NULL,
                                               0,
                                               ThreadNotifyTestThreadProc,
                                               &ThreadNotifyCounter,
                                               0,
                                               NULL );
    if (!hThreadNotifyThread) {
       fprintf(stderr,
               "[Failed] ThreadNotify CreateThread ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hThreadNotify );
       FreeLibrary( hModule );
       return FALSE;
    }

    DWORD ThreadNotifyWait = WaitForSingleObject( hThreadNotifyThread,
                                                  5000 );
    CloseHandle( hThreadNotifyThread );
    if (ThreadNotifyWait != WAIT_OBJECT_0 ||
        ThreadNotifyCounter != 1 ||
        ThreadNotifyLogCount < 2 ||
        ThreadNotifyLog[0] != THREAD_NOTIFY_EVENT_THREAD_ATTACH ||
        ThreadNotifyLog[1] != THREAD_NOTIFY_EVENT_THREAD_DETACH) {
       fprintf(stderr,
               "[Failed] ThreadNotify thread events Wait = 0x%08lx Counter = %ld Count = %ld Event0 = 0x%lx Event1 = 0x%lx\n",
               ThreadNotifyWait,
               ThreadNotifyCounter,
               ThreadNotifyLogCount,
               ThreadNotifyLog[0],
               ThreadNotifyLog[1]);
       FreeLibrary( hThreadNotify );
       FreeLibrary( hModule );
       return FALSE;
    }

    if (DisableThreadLibraryCalls( NULL )) {
       fprintf(stderr,
               "[Failed] DisableThreadLibraryCalls(NULL) unexpectedly succeeded\n");
       FreeLibrary( hThreadNotify );
       FreeLibrary( hModule );
       return FALSE;
    }

    if (!DisableThreadLibraryCalls( hThreadNotify )) {
       fprintf(stderr,
               "[Failed] DisableThreadLibraryCalls(ThreadNotify.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hThreadNotify );
       FreeLibrary( hModule );
       return FALSE;
    }

    ThreadNotifyLogCount = 0;
    ThreadNotifyLog[0] = 0;
    ThreadNotifyLog[1] = 0;
    threadNotifySetLogFn( &ThreadNotifyLogCount,
                          ThreadNotifyLog,
                          THREAD_NOTIFY_TEST_LOG_CAPACITY );

    ThreadNotifyCounter = 0;
    hThreadNotifyThread = CreateThread( NULL,
                                        0,
                                        ThreadNotifyTestThreadProc,
                                        &ThreadNotifyCounter,
                                        0,
                                        NULL );
    if (!hThreadNotifyThread) {
       fprintf(stderr,
               "[Failed] ThreadNotify disabled CreateThread ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hThreadNotify );
       FreeLibrary( hModule );
       return FALSE;
    }

    ThreadNotifyWait = WaitForSingleObject( hThreadNotifyThread,
                                            5000 );
    CloseHandle( hThreadNotifyThread );
    if (ThreadNotifyWait != WAIT_OBJECT_0 ||
        ThreadNotifyCounter != 1 ||
        ThreadNotifyLogCount != 0) {
       fprintf(stderr,
               "[Failed] ThreadNotify disabled events Wait = 0x%08lx Counter = %ld Count = %ld Event0 = 0x%lx\n",
               ThreadNotifyWait,
               ThreadNotifyCounter,
               ThreadNotifyLogCount,
               ThreadNotifyLog[0]);
       FreeLibrary( hThreadNotify );
       FreeLibrary( hModule );
       return FALSE;
    }

    LONG ThreadNotifyDetachLogCount = 0;
    LONG ThreadNotifyDetachLog[THREAD_NOTIFY_TEST_LOG_CAPACITY] = { 0 };
    threadNotifySetLogFn( &ThreadNotifyDetachLogCount,
                          ThreadNotifyDetachLog,
                          THREAD_NOTIFY_TEST_LOG_CAPACITY );

    if (!FreeLibrary( hThreadNotify )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(ThreadNotify.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hModule );
       return FALSE;
    }
    hThreadNotify = NULL;

    if (ThreadNotifyDetachLogCount < 1 ||
        ThreadNotifyDetachLog[0] != THREAD_NOTIFY_EVENT_PROCESS_DETACH) {
       fprintf(stderr,
               "[Failed] ThreadNotify process detach Count = %ld Event0 = 0x%lx\n",
               ThreadNotifyDetachLogCount,
               ThreadNotifyDetachLog[0]);
       FreeLibrary( hModule );
       return FALSE;
    }
    printf("[Success] DllMain thread notifications and DisableThreadLibraryCalls\n");

    if (!FreeLibrary( hModule )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(Test.dll) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    hModule = NULL;

    if (LoadLibraryExW( L"DependencyOwner.dll",
                        NULL,
                        LOAD_LIBRARY_SEARCH_SYSTEM32 ) != NULL) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, SEARCH_SYSTEM32) unexpectedly succeeded\n");
       return FALSE;
    }
    printf("[Success] LoadLibraryExW SEARCH_SYSTEM32 isolation\n");

    HMODULE hApplicationDir = LoadLibraryExW( L"DependencyOwner.dll",
                                              NULL,
                                              LOAD_LIBRARY_SEARCH_APPLICATION_DIR );
    if (!hApplicationDir) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, SEARCH_APPLICATION_DIR) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    dependencyOwnerFn = (TEST_FN)GetProcAddress( hApplicationDir, "DependencyOwnerFunction" );
    if (!dependencyOwnerFn ||
        dependencyOwnerFn(10) != 34) {
       fprintf(stderr,
               "[Failed] SEARCH_APPLICATION_DIR DependencyOwnerFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hApplicationDir );
       return FALSE;
    }
    if (!FreeLibrary( hApplicationDir )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll SEARCH_APPLICATION_DIR) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after SEARCH_APPLICATION_DIR owner unload ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW SEARCH_APPLICATION_DIR\n");

    HMODULE hDefaultDirs = LoadLibraryExW( L"DependencyOwner.dll",
                                           NULL,
                                           LOAD_LIBRARY_SEARCH_DEFAULT_DIRS );
    if (!hDefaultDirs) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, SEARCH_DEFAULT_DIRS) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    dependencyOwnerFn = (TEST_FN)GetProcAddress( hDefaultDirs, "DependencyOwnerFunction" );
    if (!dependencyOwnerFn ||
        dependencyOwnerFn(10) != 34) {
       fprintf(stderr,
               "[Failed] SEARCH_DEFAULT_DIRS DependencyOwnerFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDefaultDirs );
       return FALSE;
    }
    if (!FreeLibrary( hDefaultDirs )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll SEARCH_DEFAULT_DIRS) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after SEARCH_DEFAULT_DIRS owner unload ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW SEARCH_DEFAULT_DIRS\n");

    if (LoadLibraryExW( L"DependencyOwner.dll",
                        NULL,
                        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR ) != NULL ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW relative SEARCH_DLL_LOAD_DIR was not rejected ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW relative SEARCH_DLL_LOAD_DIR rejection\n");

    if (LoadLibraryExW( L"DependencyOwner.dll",
                        NULL,
                        LOAD_WITH_ALTERED_SEARCH_PATH | LOAD_LIBRARY_SEARCH_SYSTEM32 ) != NULL ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW altered/search flag mix was not rejected ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW invalid flag mix rejection\n");

    if (LoadLibraryExW( L"DependencyOwner.dll",
                        NULL,
                        LOAD_LIBRARY_SEARCH_USER_DIRS ) != NULL) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, SEARCH_USER_DIRS) unexpectedly succeeded without SetDllDirectory\n");
       return FALSE;
    }

    if (!SetDllDirectoryW( DllDirectory )) {
       fprintf(stderr,
               "[Failed] SetDllDirectoryW(test directory) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    DWORD DllDirectoryLength = (DWORD)wcslen( DllDirectory );
    if (GetDllDirectoryW( 0,
                          NULL ) != DllDirectoryLength + 1) {
       fprintf(stderr,
               "[Failed] GetDllDirectoryW(test directory query) ErrorCode = %lu\n",
               GetLastError());
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    DllDirectoryProbe[0] = L'X';
    if (GetDllDirectoryW( 1,
                          DllDirectoryProbe ) != DllDirectoryLength + 1 ||
        DllDirectoryProbe[0] != UNICODE_NULL) {
       fprintf(stderr,
               "[Failed] GetDllDirectoryW(test directory small buffer) ErrorCode = %lu\n",
               GetLastError());
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    if (GetDllDirectoryW( RTL_NUMBER_OF(DllDirectoryProbe),
                          DllDirectoryProbe ) != DllDirectoryLength ||
        wcscmp( DllDirectoryProbe,
                DllDirectory ) != 0) {
       fprintf(stderr,
               "[Failed] GetDllDirectoryW(test directory) ErrorCode = %lu Value = %ws Expected = %ws\n",
               GetLastError(),
               DllDirectoryProbe,
               DllDirectory);
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    if (!CopyWideAsciiForTest( DllDirectory,
                               DllDirectoryA,
                               RTL_NUMBER_OF(DllDirectoryA) )) {
       fprintf(stderr,
               "[Failed] test DLL directory is not ASCII-compatible for GetDllDirectoryA\n");
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    DWORD DllDirectoryALength = 0;
    while (DllDirectoryA[DllDirectoryALength] != ANSI_NULL) {
       DllDirectoryALength++;
    }
    if (GetDllDirectoryA( 0,
                          NULL ) != DllDirectoryALength + 1) {
       fprintf(stderr,
               "[Failed] GetDllDirectoryA(test directory query) ErrorCode = %lu\n",
               GetLastError());
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    DWORD DllDirectoryAReturn = GetDllDirectoryA( RTL_NUMBER_OF(DllDirectoryProbeA),
                                                  DllDirectoryProbeA );
    DWORD DllDirectoryAActualLength = 0;
    while (DllDirectoryProbeA[DllDirectoryAActualLength] != ANSI_NULL) {
       DllDirectoryAActualLength++;
    }
    if ((DllDirectoryAReturn != DllDirectoryAActualLength &&
         DllDirectoryAReturn + 1 != DllDirectoryAActualLength) ||
        strcmp( DllDirectoryProbeA,
                DllDirectoryA ) != 0) {
       fprintf(stderr,
               "[Failed] GetDllDirectoryA(test directory) Return = %lu ActualLength = %lu ExpectedLength = %lu ErrorCode = %lu Value = %s Expected = %s\n",
               DllDirectoryAReturn,
               DllDirectoryAActualLength,
               DllDirectoryALength,
               GetLastError(),
               DllDirectoryProbeA,
               DllDirectoryA);
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    printf("[Success] GetDllDirectoryA/W test directory\n");

    HMODULE hUserDirs = LoadLibraryExW( L"DependencyOwner.dll",
                                        NULL,
                                        LOAD_LIBRARY_SEARCH_USER_DIRS );
    if (!hUserDirs) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, SEARCH_USER_DIRS) ErrorCode = %lu\n",
               GetLastError());
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    dependencyOwnerFn = (TEST_FN)GetProcAddress( hUserDirs, "DependencyOwnerFunction" );
    if (!dependencyOwnerFn ||
        dependencyOwnerFn(10) != 34) {
       fprintf(stderr,
               "[Failed] SEARCH_USER_DIRS DependencyOwnerFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hUserDirs );
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    if (!FreeLibrary( hUserDirs )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll SEARCH_USER_DIRS) ErrorCode = %lu\n",
               GetLastError());
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after SEARCH_USER_DIRS owner unload ErrorCode = %lu\n",
               GetLastError());
       SetDllDirectoryW( NULL );
       return FALSE;
    }
    if (!SetDllDirectoryW( NULL )) {
       fprintf(stderr,
               "[Failed] SetDllDirectoryW(NULL after SEARCH_USER_DIRS) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW SEARCH_USER_DIRS\n");

    if (AddDllDirectory( L"relative-directory" ) != NULL ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
       fprintf(stderr,
               "[Failed] AddDllDirectory(relative) was not rejected ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    DLL_DIRECTORY_COOKIE UserCookie = AddDllDirectory( DllDirectory );
    if (!UserCookie) {
       fprintf(stderr,
               "[Failed] AddDllDirectory(test directory) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    HMODULE hAddedUserDir = LoadLibraryExW( L"DependencyOwner.dll",
                                            NULL,
                                            LOAD_LIBRARY_SEARCH_USER_DIRS );
    if (!hAddedUserDir) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, AddDllDirectory user dir) ErrorCode = %lu\n",
               GetLastError());
       RemoveDllDirectory( UserCookie );
       return FALSE;
    }
    dependencyOwnerFn = (TEST_FN)GetProcAddress( hAddedUserDir, "DependencyOwnerFunction" );
    if (!dependencyOwnerFn ||
        dependencyOwnerFn(10) != 34) {
       fprintf(stderr,
               "[Failed] AddDllDirectory DependencyOwnerFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hAddedUserDir );
       RemoveDllDirectory( UserCookie );
       return FALSE;
    }
    if (!FreeLibrary( hAddedUserDir )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll AddDllDirectory) ErrorCode = %lu\n",
               GetLastError());
       RemoveDllDirectory( UserCookie );
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after AddDllDirectory owner unload ErrorCode = %lu\n",
               GetLastError());
       RemoveDllDirectory( UserCookie );
       return FALSE;
    }
    if (!RemoveDllDirectory( UserCookie )) {
       fprintf(stderr,
               "[Failed] RemoveDllDirectory(test directory) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (LoadLibraryExW( L"DependencyOwner.dll",
                        NULL,
                        LOAD_LIBRARY_SEARCH_USER_DIRS ) != NULL) {
       fprintf(stderr,
               "[Failed] SEARCH_USER_DIRS succeeded after RemoveDllDirectory\n");
       return FALSE;
    }
    printf("[Success] AddDllDirectory / RemoveDllDirectory\n");

    HMODULE hDontResolve = LoadLibraryExW( L"DependencyOwner.dll",
                                           NULL,
                                           DONT_RESOLVE_DLL_REFERENCES );
    if (!hDontResolve) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, DONT_RESOLVE_DLL_REFERENCES) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] DONT_RESOLVE_DLL_REFERENCES loaded AutoDependency.dll ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDontResolve );
       return FALSE;
    }
    if (!FreeLibrary( hDontResolve )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll DONT_RESOLVE) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW DONT_RESOLVE_DLL_REFERENCES\n");

    if (!BuildTestDllPath( L"DependencyOwner.dll",
                           DependencyOwnerPath,
                           RTL_NUMBER_OF(DependencyOwnerPath) )) {
       fprintf(stderr,
               "[Failed] BuildTestDllPath(DependencyOwner.dll) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    HMODULE hSearchLoadDir = LoadLibraryExW( DependencyOwnerPath,
                                             NULL,
                                             LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
    if (!hSearchLoadDir) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, SEARCH_DLL_LOAD_DIR) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    dependencyOwnerFn = (TEST_FN)GetProcAddress( hSearchLoadDir, "DependencyOwnerFunction" );
    if (!dependencyOwnerFn ||
        dependencyOwnerFn(10) != 34) {
       fprintf(stderr,
               "[Failed] SEARCH_DLL_LOAD_DIR DependencyOwnerFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hSearchLoadDir );
       return FALSE;
    }
    if (!FreeLibrary( hSearchLoadDir )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll SEARCH_DLL_LOAD_DIR) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after SEARCH_DLL_LOAD_DIR owner unload ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW SEARCH_DLL_LOAD_DIR\n");

    HMODULE hAltered = LoadLibraryExW( DependencyOwnerPath,
                                       NULL,
                                       LOAD_WITH_ALTERED_SEARCH_PATH );
    if (!hAltered) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(DependencyOwner.dll, LOAD_WITH_ALTERED_SEARCH_PATH) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    dependencyOwnerFn = (TEST_FN)GetProcAddress( hAltered, "DependencyOwnerFunction" );
    if (!dependencyOwnerFn ||
        dependencyOwnerFn(10) != 34) {
       fprintf(stderr,
               "[Failed] LOAD_WITH_ALTERED_SEARCH_PATH DependencyOwnerFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hAltered );
       return FALSE;
    }
    if (!FreeLibrary( hAltered )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll ALTERED_SEARCH_PATH) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after ALTERED_SEARCH_PATH owner unload ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW LOAD_WITH_ALTERED_SEARCH_PATH\n");

    for (DWORD Loop = 0; Loop < 16; Loop++) {
       HMODULE hStress = LoadLibraryExW( DependencyOwnerPath,
                                         NULL,
                                         LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR );
       if (!hStress) {
          fprintf(stderr,
                  "[Failed] Dependency graph stress LoadLibraryExW loop %lu ErrorCode = %lu\n",
                  Loop,
                  GetLastError());
          return FALSE;
       }
       dependencyOwnerFn = (TEST_FN)GetProcAddress( hStress, "DependencyOwnerFunction" );
       if (!dependencyOwnerFn ||
           dependencyOwnerFn((LONG)Loop) != (LONG)Loop + 24) {
          fprintf(stderr,
                  "[Failed] Dependency graph stress DependencyOwnerFunction loop %lu ErrorCode = %lu\n",
                  Loop,
                  GetLastError());
          FreeLibrary( hStress );
          return FALSE;
       }
       if (!FreeLibrary( hStress )) {
          fprintf(stderr,
                  "[Failed] Dependency graph stress FreeLibrary loop %lu ErrorCode = %lu\n",
                  Loop,
                  GetLastError());
          return FALSE;
       }
       if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
          fprintf(stderr,
                  "[Failed] AutoDependency.dll stayed loaded after dependency stress loop %lu ErrorCode = %lu\n",
                  Loop,
                  GetLastError());
          return FALSE;
       }
    }
    printf("[Success] Dependency graph load/unload stress\n");

    if (GetModuleHandleW( L"DelayDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] DelayDependency.dll was loaded before delay-load test ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    HMODULE hDelayOwner = LoadLibraryW( L"DelayOwner.dll" );
    if (!hDelayOwner) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(DelayOwner.dll) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    if (GetModuleHandleW( L"DelayDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] DelayDependency.dll was loaded before first delay call ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDelayOwner );
       return FALSE;
    }

    TEST_FN delayProbeFn = (TEST_FN)GetProcAddress( hDelayOwner, "DelayOwnerProbeFunction" );
    if (!delayProbeFn ||
        delayProbeFn(10) != 12) {
       fprintf(stderr,
               "[Failed] DelayOwnerProbeFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDelayOwner );
       return FALSE;
    }

    TEST_FN delayCallFn = (TEST_FN)GetProcAddress( hDelayOwner, "DelayOwnerCallFunction" );
    if (!delayCallFn ||
        delayCallFn(10) != 55) {
       fprintf(stderr,
               "[Failed] DelayOwnerCallFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDelayOwner );
       return FALSE;
    }

    HMODULE hDelayDependency = GetModuleHandleW( L"DelayDependency.dll" );
    if (!hDelayDependency) {
       fprintf(stderr,
               "[Failed] DelayDependency.dll was not loaded by first delay call ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDelayOwner );
       return FALSE;
    }

    if (!FreeLibrary( hDelayOwner )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DelayOwner.dll) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDelayDependency );
       return FALSE;
    }

    hDelayDependency = GetModuleHandleW( L"DelayDependency.dll" );
    if (hDelayDependency &&
        !FreeLibrary( hDelayDependency )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DelayDependency.dll) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] Delay-load first-call resolution\n");

    if (!VerifyResourceOnlyLoad( L"Test.dll",
                                 LOAD_LIBRARY_AS_DATAFILE,
                                 "LOAD_LIBRARY_AS_DATAFILE" )) {
       return FALSE;
    }

    if (!VerifyResourceOnlyLoad( L"Test.dll",
                                 LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE,
                                 "LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE" )) {
       return FALSE;
    }

    if (!VerifyResourceOnlyLoad( L"Test.dll",
                                 LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE,
                                 "LOAD_LIBRARY_AS_DATAFILE | LOAD_LIBRARY_AS_IMAGE_RESOURCE" )) {
       return FALSE;
    }

    if (FreeLibrary( NULL )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(NULL) unexpectedly succeeded\n");
       return FALSE;
    }
    if (FreeLibrary( (HMODULE)INVALID_HANDLE_VALUE )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(invalid module) unexpectedly succeeded\n");
       return FALSE;
    }
    printf("[Success] FreeLibrary invalid handles\n");

    HMODULE hPinnedLoad = LoadLibraryW( L"Test.dll" );
    if (!hPinnedLoad) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(Test.dll pin target) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    HMODULE hPinned = NULL;
    if (!GetModuleHandleExW( GET_MODULE_HANDLE_EX_FLAG_PIN,
                             L"Test.dll",
                             &hPinned ) ||
        hPinned != hPinnedLoad) {
       fprintf(stderr,
               "[Failed] GetModuleHandleExW(Test.dll, PIN) ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hPinnedLoad );
       return FALSE;
    }

    if (!FreeLibrary( hPinnedLoad )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(Test.dll pinned load reference) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (!FreeLibrary( hPinned )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(Test.dll pinned handle) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetModuleHandleW( L"Test.dll" ) != hPinned) {
       fprintf(stderr,
               "[Failed] pinned Test.dll was unloaded ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] GetModuleHandleEx PIN\n");

    if (SetDefaultDllDirectories( 0 ) ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
       fprintf(stderr,
               "[Failed] SetDefaultDllDirectories(0) was not rejected ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    DLL_DIRECTORY_COOKIE DefaultCookie = AddDllDirectory( DllDirectory );
    if (!DefaultCookie) {
       fprintf(stderr,
               "[Failed] AddDllDirectory(default search directory) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (!SetDefaultDllDirectories( LOAD_LIBRARY_SEARCH_USER_DIRS )) {
       fprintf(stderr,
               "[Failed] SetDefaultDllDirectories(USER_DIRS) ErrorCode = %lu\n",
               GetLastError());
       RemoveDllDirectory( DefaultCookie );
       return FALSE;
    }

    HMODULE hDefaultSearch = LoadLibraryW( L"DependencyOwner.dll" );
    if (!hDefaultSearch) {
       fprintf(stderr,
               "[Failed] LoadLibraryW(DependencyOwner.dll) with default user dirs ErrorCode = %lu\n",
               GetLastError());
       RemoveDllDirectory( DefaultCookie );
       return FALSE;
    }
    dependencyOwnerFn = (TEST_FN)GetProcAddress( hDefaultSearch, "DependencyOwnerFunction" );
    if (!dependencyOwnerFn ||
        dependencyOwnerFn(10) != 34) {
       fprintf(stderr,
               "[Failed] default user dirs DependencyOwnerFunction ErrorCode = %lu\n",
               GetLastError());
       FreeLibrary( hDefaultSearch );
       RemoveDllDirectory( DefaultCookie );
       return FALSE;
    }
    if (!FreeLibrary( hDefaultSearch )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(DependencyOwner.dll default user dirs) ErrorCode = %lu\n",
               GetLastError());
       RemoveDllDirectory( DefaultCookie );
       return FALSE;
    }
    if (!RemoveDllDirectory( DefaultCookie )) {
       fprintf(stderr,
               "[Failed] RemoveDllDirectory(default search directory) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetModuleHandleW( L"AutoDependency.dll" ) != NULL) {
       fprintf(stderr,
               "[Failed] AutoDependency.dll stayed loaded after default user dirs owner unload ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] SetDefaultDllDirectories USER_DIRS\n");

    printf("[Success] Library Test\n\n");
    return TRUE;
}
