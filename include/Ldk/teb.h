#pragma once

#ifndef _LDK_TEB_
#define _LDK_TEB_

#include "peb.h"

typedef struct _LDK_TEB {
	PLDK_PEB ProcessEnvironmentBlock;
} LDK_TEB, *PLDK_TEB;

#endif // _LDK_TEB_