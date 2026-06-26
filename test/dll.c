#include <windows.h>

#if defined(_M_IX86)
#pragma comment(linker, "/export:TestFunction=_TestFunction@4")
#pragma comment(linker, "/export:_TestOrdinalFunction@4,@7,NONAME")
#else
#pragma comment(linker, "/export:TestOrdinalFunction,@7,NONAME")
#endif

__declspec(dllexport)
LONG
__stdcall
TestFunction (
    _In_ LONG Value
    )
{
    return Value;
}

LONG
__stdcall
TestOrdinalFunction (
    _In_ LONG Value
    )
{
    return Value + 7;
}
