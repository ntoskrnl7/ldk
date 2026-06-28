#if _KERNEL_MODE
#include <Ldk/Windows.h>

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
