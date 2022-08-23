#pragma once

#ifndef _LDK_PEB_
#define _LDK_PEB_

#include <winnt.h>
#include <minwindef.h>

#define RTL_USER_PROC_SECURE_PROCESS            0x80000000

typedef struct _RTL_USER_PROCESS_PARAMETERS {
     ULONG MaximumLength;
     ULONG Length;
     ULONG Flags;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

#define FLG_APPLICATION_VERIFIER    			0x0100

typedef struct _LDK_PEB {
	ULONG NtGlobalFlag;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
} LDK_PEB, *PLDK_PEB;

#endif // _LDK_PEB_