#include <windows.h>

#if defined(_M_IX86)
#pragma comment(linker, "/export:TestFunction=_TestFunction@4")
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
