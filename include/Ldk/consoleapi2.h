#pragma once

#ifndef _APISETCONSOLEL2_
#define _APISETCONSOLEL2_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "minwinbase.h"

EXTERN_C_START

WINBASEAPI
BOOL
WINAPI
SetConsoleCP(
    _In_ UINT wCodePageID
    );

WINBASEAPI
BOOL
WINAPI
SetConsoleOutputCP(
    _In_ UINT wCodePageID
    );

EXTERN_C_END

#endif // _APISETCONSOLEL2_