#pragma once

#ifndef _WINDOWS_
#define _WINDOWS_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>

#define WINBASEAPI
#include <minwindef.h>
#include "winnt.h"
#include "minwinbase.h"

#include <winerror.h>

#include <windef.h>
#include "winbase.h"

#include "winnls.h"

#include "ldk.h"

#endif // _WINDOWS_