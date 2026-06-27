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

BOOLEAN
LibraryTest (
    VOID
    )
{
    PAGED_CODE();

    typedef LONG(__stdcall* TEST_FN)(LONG);
    WCHAR DependencyOwnerPath[MAX_PATH];

    printf("Library Test\n");

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

    HMODULE hDataFile = LoadLibraryExW( L"Test.dll",
                                        NULL,
                                        LOAD_LIBRARY_AS_DATAFILE );
    if (!hDataFile) {
       fprintf(stderr,
               "[Failed] LoadLibraryExW(Test.dll, LOAD_LIBRARY_AS_DATAFILE) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    if (GetProcAddress( hDataFile, "TestFunction" ) != NULL) {
       fprintf(stderr,
               "[Failed] GetProcAddress unexpectedly succeeded for datafile handle\n");
       FreeLibrary( hDataFile );
       return FALSE;
    }
    if (!FreeLibrary( hDataFile )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(Test.dll datafile) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }
    printf("[Success] LoadLibraryExW LOAD_LIBRARY_AS_DATAFILE\n");

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

    printf("[Success] Library Test\n\n");
    return TRUE;
}
