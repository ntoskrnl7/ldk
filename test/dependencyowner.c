#include <windows.h>

#if defined(_M_IX86)
#pragma comment(linker, "/export:DependencyOwnerFunction=_DependencyOwnerFunction@4")
#else
#pragma comment(linker, "/export:DependencyOwnerFunction")
#endif

__declspec(dllimport)
LONG
__stdcall
AutoDependencyFunction (
    _In_ LONG Value
    );

LONG
__stdcall
DependencyOwnerFunction (
    _In_ LONG Value
    )
{
    return AutoDependencyFunction( Value ) + 3;
}
