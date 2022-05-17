#include <windows.h>

__declspec(dllexport)
LONG
TestFunction (
    _In_ LONG Value
    )
{
    return Value;
}