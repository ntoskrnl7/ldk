#include "ntdll.h"

#include "../ldk.h"

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