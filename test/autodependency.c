#include <windows.h>

__declspec(dllexport)
LONG
__stdcall
AutoDependencyFunction (
    _In_ LONG Value
    )
{
    return Value + 21;
}
