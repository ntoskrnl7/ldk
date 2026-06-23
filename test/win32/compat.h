#pragma once

#include <windows.h>
#include <stdio.h>

#ifndef DbgPrint
#define DbgPrint printf
#endif

#ifndef FlagOn
#define FlagOn(_Flags, _SingleFlag) (((_Flags) & (_SingleFlag)) != 0)
#endif

#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000)
#endif
