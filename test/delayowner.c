#include <windows.h>

#if defined(_M_IX86)
#pragma comment(linker, "/export:DelayOwnerProbeFunction=_DelayOwnerProbeFunction@4")
#pragma comment(linker, "/export:DelayOwnerCallFunction=_DelayOwnerCallFunction@4")
#else
#pragma comment(linker, "/export:DelayOwnerProbeFunction")
#pragma comment(linker, "/export:DelayOwnerCallFunction")
#endif

__declspec(dllimport)
LONG
__stdcall
DelayDependencyFunction (
    _In_ LONG Value
    );

LONG
__stdcall
DelayOwnerProbeFunction (
    _In_ LONG Value
    )
{
    return Value + 2;
}

LONG
__stdcall
DelayOwnerCallFunction (
    _In_ LONG Value
    )
{
    return DelayDependencyFunction( Value ) + 5;
}
