#pragma once

#include <Ldk/windows.h>

ULONG
BaseSetLastNTError(
	_In_ NTSTATUS Status
	);

PLARGE_INTEGER
BaseFormatTimeout(
	_Out_ PLARGE_INTEGER Timeout,
	_In_ DWORD Milliseconds
	);