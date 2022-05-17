#include "ntdll.h"
#include "../ldk.h"

ULONG
NTAPI
RtlGetThreadErrorMode(
    VOID
    )
{
    return NtCurrentTeb()->HardErrorMode;
}

NTSTATUS
NTAPI
RtlSetThreadErrorMode (
    _In_  ULONG  NewMode,
    _Out_opt_ PULONG OldMode
    )
{
#if defined(BUILD_WOW6432)
    PTEB64 Teb = NtCurrentTeb64();
#else
    PTEB Teb = NtCurrentTeb();
#endif

    if (NewMode & ~(RTL_ERRORMODE_FAILCRITICALERRORS |
                    RTL_ERRORMODE_NOGPFAULTERRORBOX |
                    RTL_ERRORMODE_NOOPENFILEERRORBOX)) {
        return STATUS_INVALID_PARAMETER_1;
    }

    if (OldMode) {
        *OldMode = Teb->HardErrorMode;
    }
    Teb->HardErrorMode = NewMode;

    return TRUE;
}