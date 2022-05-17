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
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&LdkCurrentPeb()->ModuleListResource, TRUE);
}

VOID
NTAPI
RtlReleasePebLock (
    VOID
    )
{
	ExReleaseResourceLite(&LdkCurrentPeb()->ModuleListResource);
	KeLeaveCriticalRegion();
}