#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
LibraryTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LibraryTest)
#endif
#else
#include <windows.h>
#include <stdio.h>

#define DbgPrint        printf
#define PAGED_CODE()
#endif

BOOLEAN
LibraryTest (
    VOID
    )
{
    PAGED_CODE();

    // //
    // // TestFunction함수를 가진 Test.dll를 만들어주세요
    // //

    // typedef LONG(__stdcall* TEST_FN)(LONG);

    // HMODULE hModule = LoadLibraryW(L"Test.dll");
    // if (!hModule) {
    //    return FALSE;
    // }

    // TEST_FN testFn = (TEST_FN)GetProcAddress(hModule, "TestFunction");
    // if (!testFn) {
    //    FreeLibrary(hModule);
    //    return FALSE;
    // }
    // if (testFn(10) != 10) {
    //    FreeLibrary(hModule);
    //    return FALSE;
    // }
    // return FreeLibrary(hModule) == TRUE;
    return TRUE;
}