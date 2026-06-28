#include <windows.h>

#if defined(_MSC_VER)
#pragma runtime_checks("", off)
#endif

#if defined(_M_IX86)
#pragma comment(linker, "/export:FailAttachFunction=_FailAttachFunction@4")
#else
#pragma comment(linker, "/export:FailAttachFunction")
#endif

__declspec(dllimport)
LONG
__stdcall
AutoDependencyFunction (
    _In_ LONG Value
    );

LONG
__stdcall
FailAttachFunction (
    _In_ LONG Value
    )
{
    return AutoDependencyFunction( Value );
}

BOOL
WINAPI
FailAttachDllMain (
    _In_ HINSTANCE Module,
    _In_ DWORD Reason,
    _In_opt_ LPVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Module );
    UNREFERENCED_PARAMETER( Reserved );

    if (Reason == DLL_PROCESS_ATTACH) {
        return FALSE;
    }

    return TRUE;
}
