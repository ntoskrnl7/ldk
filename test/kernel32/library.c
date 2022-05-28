#if _KERNEL_MODE
#include <Ldk/libloaderapi.h>

BOOLEAN
LibraryTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LibraryTest)
#endif
#else
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

    HMODULE hModule = LoadLibraryW( L"Test.dll" );
    if (!hModule) {
       return FALSE;
    }

    TEST_FN testFn = (TEST_FN)GetProcAddress( hModule, "TestFunction" );
    if (!testFn) {
       FreeLibrary( hModule );
       return FALSE;
    }
    if (testFn(10) != 10) {
       FreeLibrary( hModule );
       return FALSE;
    }
    return FreeLibrary( hModule ) == TRUE;
}