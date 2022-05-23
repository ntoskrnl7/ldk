#include "ntdll.h"
#include "../ldk.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlAcquirePebLock)
#pragma alloc_text(PAGE, RtlReleasePebLock)
#endif



VOID
NTAPI
RtlAcquirePebLock (
    VOID
    )
{
    PAGED_CODE();

    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite( &LdkCurrentPeb()->ModuleListResource,
                                    TRUE );
}

VOID
NTAPI
RtlReleasePebLock (
    VOID
    )
{
    PAGED_CODE();

	ExReleaseResourceLite( &LdkCurrentPeb()->ModuleListResource );
	KeLeaveCriticalRegion();
}