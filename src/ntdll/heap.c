#include "ntdll.h"

HANDLE
NTAPI
RtlGetProcessHeap(
    VOID
    )
{
    return NtCurrentPeb()->ProcessHeap;
}