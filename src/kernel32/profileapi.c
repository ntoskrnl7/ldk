#include "winbase.h"



WINBASEAPI
BOOL
WINAPI
QueryPerformanceCounter(
    _Out_ LARGE_INTEGER * lpPerformanceCount
    )
{
	LARGE_INTEGER PerformanceFrequency;
	*lpPerformanceCount = KeQueryPerformanceCounter(&PerformanceFrequency);
	return TRUE;
}

WINBASEAPI
BOOL
WINAPI
QueryPerformanceFrequency(
    _Out_ LARGE_INTEGER * lpFrequency
    )
{
	KeQueryPerformanceCounter(lpFrequency);
	return TRUE;
}