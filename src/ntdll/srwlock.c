#include "ntdll.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlInitializeSRWLock)
#pragma alloc_text(PAGE, RtlAcquireSRWLockShared)
#pragma alloc_text(PAGE, RtlReleaseSRWLockShared)
#pragma alloc_text(PAGE, RtlTryAcquireSRWLockShared)
#pragma alloc_text(PAGE, RtlAcquireSRWLockExclusive)
#pragma alloc_text(PAGE, RtlReleaseSRWLockExclusive)
#pragma alloc_text(PAGE, RtlTryAcquireSRWLockExclusive)
#endif



#define __ALIGNED(n)        __declspec(align(n))
#define NtClose             ZwClose
#pragma warning(disable: 4201)

/*
* ReactOS :-)
* 
* https://github.com/reactos/reactos/blob/3fa57b8ff7fcee47b8e2ed869aecaf4515603f3f/sdk/lib/rtl/srw.c
*/

#include "reactos/srw.c"