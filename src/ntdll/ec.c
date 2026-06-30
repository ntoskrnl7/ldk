#include "ntdll.h"

BOOLEAN
NTAPI
RtlIsEcCode (
    _In_ ULONG64 CodeAddress
    )
{
    UNREFERENCED_PARAMETER(CodeAddress);

    return FALSE;
}
