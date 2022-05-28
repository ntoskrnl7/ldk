#pragma once

#ifndef _LDK_PEB_
#define _LDK_PEB_

typedef struct _LDK_PEB {
	ULONG NtGlobalFlag;
} LDK_PEB, *PLDK_PEB;

#endif // _LDK_PEB_