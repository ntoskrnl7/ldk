#include <windows.h>

#if defined(_M_IX86)
#pragma comment(linker, "/export:DelayDependencyFunction=_DelayDependencyFunction@4")
#else
#pragma comment(linker, "/export:DelayDependencyFunction")
#endif

__declspec(dllexport)
LONG
__stdcall
DelayDependencyFunction (
    _In_ LONG Value
    )
{
    return Value + 40;
}
