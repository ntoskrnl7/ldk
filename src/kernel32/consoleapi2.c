#include "winbase.h"
#include "../peb.h"



extern UINT LdkpConsoleCp;
extern UINT LdkpConsoleOutputCP;



WINBASEAPI
BOOL
WINAPI
SetConsoleCP(
    _In_ UINT wCodePageID
    )
{
    if (! IsValidCodePage(wCodePageID)) {
        return FALSE;
    }
    LdkpConsoleCp = wCodePageID;
    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
SetConsoleOutputCP(
    _In_ UINT wCodePageID
    )
{
    if (! IsValidCodePage(wCodePageID)) {
        return FALSE;
    }
    LdkpConsoleOutputCP = wCodePageID;
    return TRUE;
}