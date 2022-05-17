#include "winbase.h"

#include "../ntdll/ntdll.h"



WINBASEAPI
BOOL
WINAPI
QueueUserWorkItem (
    _In_ LPTHREAD_START_ROUTINE Function,
    _In_opt_ PVOID Context,
    _In_ ULONG Flags
    )
{
    PAGED_CODE();

    NTSTATUS Status = RtlQueueWorkItem( (WORKERCALLBACKFUNC)Function,
                                        Context,
                                        Flags );
    if (NT_SUCCESS(Status)) {
        return TRUE;
    }

    LdkSetLastNTError( Status );
    return FALSE;
}
