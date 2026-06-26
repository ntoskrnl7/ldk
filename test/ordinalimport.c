#include <windows.h>

#if defined(_M_IX86)
#pragma comment(linker, "/export:TestOrdinalImportFunction=_TestOrdinalImportFunction@4")
#else
#pragma comment(linker, "/export:TestOrdinalImportFunction")
#endif

__declspec(dllimport)
LONG
__stdcall
TestOrdinalFunction (
    _In_ LONG Value
    );

LONG
__stdcall
TestOrdinalImportFunction (
    _In_ LONG Value
    )
{
    return TestOrdinalFunction( Value );
}
