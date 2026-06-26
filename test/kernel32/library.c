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

BOOLEAN
LibraryTest (
    VOID
    )
{
    PAGED_CODE();

    typedef LONG(__stdcall* TEST_FN)(LONG);

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

    if (!FreeLibrary( hModule )) {
       fprintf(stderr,
               "[Failed] FreeLibrary(Test.dll) ErrorCode = %lu\n",
               GetLastError());
       return FALSE;
    }

    printf("[Success] Library Test\n\n");
    return TRUE;
}
