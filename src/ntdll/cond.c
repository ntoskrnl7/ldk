#include "ntdll.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlInitializeConditionVariable)
#pragma alloc_text(PAGE, RtlWakeConditionVariable)
#pragma alloc_text(PAGE, RtlWakeAllConditionVariable)
#pragma alloc_text(PAGE, RtlSleepConditionVariableCS)
#pragma alloc_text(PAGE, RtlSleepConditionVariableSRW)
#endif



#define __ALIGNED(n)        __declspec(align(n))
#define NtClose             ZwClose

/*
* ReactOS :-)
* 
* https://github.com/reactos/reactos/blob/3fa57b8ff7fcee47b8e2ed869aecaf4515603f3f/sdk/lib/rtl/condvar.c
*/

#include "reactos/condvar.c"