#include "winbase.h"
#include <Ldk/ntdll/ntxcapi.h>
#include "../ldk.h"



LDK_INITIALIZE_COMPONENT LdkpInitializeThreadpoolApiset;
LDK_TERMINATE_COMPONENT LdkpTerminateThreadpoolApiset;

LDK_INITIALIZE_COMPONENT LdkpInitializeThreadContexts;
LDK_TERMINATE_COMPONENT LdkpTerminateThreadContexts;

LDK_INITIALIZE_COMPONENT LdkpInitializeNls;

LDK_INITIALIZE_COMPONENT LdkpKernel32Initialize;
LDK_TERMINATE_COMPONENT LdkpKernel32Terminate;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpKernel32Initialize)
#pragma alloc_text(PAGE, LdkpKernel32Terminate)
#endif


#pragma warning(disable:4232)
VOID
LdkpInitializeSListHead (
    _Out_ PSLIST_HEADER SListHead
    )
{
	InitializeSListHead( SListHead );
}


#pragma warning(disable:4152)
LDK_FUNCTION_REGISTRATION LdkpKernel32FunctionRegistration[] = {
	//
	// fileapi.c
	//
	{
		"CreateFileA",
		CreateFileA
	},
	{
		"CreateFileW",
		CreateFileW
	},
	{
		"ReadFile",
		ReadFile
	},
	{
		"WriteFile",
		WriteFile
	},
	{
		"FlushFileBuffers",
		FlushFileBuffers
	},
	{
		"GetDriveTypeA",
		GetDriveTypeA
	},
	{
		"GetDriveTypeW",
		GetDriveTypeW
	},

	//
	// processenv.c
	//
	{
		"GetEnvironmentStrings",
		GetEnvironmentStrings
	},
	{
		"GetEnvironmentStringsW",
		GetEnvironmentStringsW
	},
	{
		"FreeEnvironmentStringsA",
		FreeEnvironmentStringsA
	},
	{
		"FreeEnvironmentStringsW",
		FreeEnvironmentStringsW
	},
	{
		"GetEnvironmentVariableA",
		GetEnvironmentVariableA
	},
	{
		"GetEnvironmentVariableW",
		GetEnvironmentVariableW
	},
	{
		"GetCurrentDirectoryA",
		GetCurrentDirectoryA
	},
	{
		"GetCurrentDirectoryW",
		GetCurrentDirectoryW
	},
	{
		"SetCurrentDirectoryA",
		SetCurrentDirectoryA
	},
	{
		"SetCurrentDirectoryW",
		SetCurrentDirectoryW
	},
	{
		"GetStdHandle",
		GetStdHandle
	},
	{
		"SetStdHandle",
		SetStdHandle
	},
	{
		"SetStdHandleEx",
		SetStdHandleEx
	},
	{
		"GetCommandLineA",
		GetCommandLineA
	},
	{
		"GetCommandLineW",
		GetCommandLineW
	},

	//
	// utilapiset.c
	//
	{
		"DecodePointer",
		DecodePointer
	},
	{
		"EncodePointer",
		EncodePointer
	},

	//
	// consoleapi.c
	//
	{
		"GetConsoleCP",
		GetConsoleCP
	},
	{
		"GetConsoleOutputCP",
		GetConsoleOutputCP
	},
	{
		"GetConsoleMode",
		GetConsoleMode
	},
	{
		"SetConsoleMode",
		SetConsoleMode
	},
	{
		"ReadConsoleA",
		ReadConsoleA
	},
	{
		"ReadConsoleW",
		ReadConsoleW
	},
	{
		"WriteConsoleA",
		WriteConsoleA
	},
	{
		"WriteConsoleW",
		WriteConsoleW
	},
	{
		"SetConsoleCtrlHandler",
		SetConsoleCtrlHandler
	},

	//
	// consoleapi2.c
	//
	{
		"SetConsoleCP",
		SetConsoleCP
	},
	{
		"SetConsoleOutputCP",
		SetConsoleOutputCP
	},

	//
	// handleapi.c
	//
	{
		"CloseHandle",
		CloseHandle
	},
	{
		"DuplicateHandle",
		DuplicateHandle
	},

	//
	// heapapi.c
	//
	{
		"HeapCreate",
		HeapCreate
	},
	{
		"HeapDestroy",
		HeapDestroy
	},
	{
		"HeapAlloc",
		HeapAlloc
	},
	{
		"HeapFree",
		HeapFree
	},
	{
		"HeapReAlloc",
		HeapReAlloc
	},
	{
		"GetProcessHeap",
		GetProcessHeap
	},
	{
		"HeapSize",
		HeapSize
	},
	{
		"HeapValidate",
		HeapValidate
	},
	{
		"HeapCompact",
		HeapCompact
	},
	{
		"HeapWalk",
		HeapWalk
	},
	{
		"HeapQueryInformation",
		HeapQueryInformation
	},

	//
	// profileapi.c
	//
	{
		"QueryPerformanceCounter",
		QueryPerformanceCounter
	},
	{
		"QueryPerformanceFrequency",
		QueryPerformanceFrequency
	},

	//
	// errhandlingapi.c
	//
	{
		"GetLastError",
		GetLastError
	},
	{
		"SetLastError",
		SetLastError
	},
	{
		"UnhandledExceptionFilter",
		UnhandledExceptionFilter
	},
	{
		"SetUnhandledExceptionFilter",
		SetUnhandledExceptionFilter
	},
	{
		"RaiseException",
		RaiseException
	},
	{
		"GetErrorMode",
		GetErrorMode
	},
	{
		"SetErrorMode",
		SetErrorMode
	},

	//
	// synchapi.h
	//
	{
		"InitializeConditionVariable",
		InitializeConditionVariable
	},
	{
		"SleepConditionVariableCS",
		SleepConditionVariableCS
	},
	{
		"SleepConditionVariableSRW",
		SleepConditionVariableSRW
	},
	{
		"WakeConditionVariable",
		WakeConditionVariable
	},
	{
		"WakeAllConditionVariable",
		WakeAllConditionVariable
	},
	{
		"InitializeCriticalSection",
		InitializeCriticalSection
	},
	{
		"InitializeCriticalSectionAndSpinCount",
		InitializeCriticalSectionAndSpinCount
	},
	{
		"DeleteCriticalSection",
		DeleteCriticalSection
	},
	{
		"EnterCriticalSection",
		EnterCriticalSection
	},
	{
		"LeaveCriticalSection",
		LeaveCriticalSection
	},
	{
		"InitOnceExecuteOnce",
		InitOnceExecuteOnce
	},
	{
		"WaitForSingleObject",
		WaitForSingleObject
	},
	{
		"WaitForSingleObjectEx",
		WaitForSingleObjectEx
	},
	{
		"WaitForMultipleObjects",
		WaitForMultipleObjects
	},
	{
		"WaitForMultipleObjectsEx",
		WaitForMultipleObjectsEx
	},
	{
		"Sleep",
		Sleep
	},

	//
	// fiberapi.c
	//
	{
		"FlsAlloc",
		FlsAlloc
	},
	{
		"FlsFree",
		FlsFree
	},
	{
		"FlsGetValue",
		FlsGetValue
	},
	{
		"FlsSetValue",
		FlsSetValue
	},

	//
	// processthreadsapi.c
	//
	{
		"CreateThread",
		CreateThread
	},
	{
		"OpenThread",
		OpenThread
	},
	{
		"GetThreadPriority",
		GetThreadPriority
	},
	{
		"SetThreadPriority",
		SetThreadPriority
	},
	{
		"GetThreadPriorityBoost",
		GetThreadPriorityBoost
	},
	{
		"SetThreadPriorityBoost",
		SetThreadPriorityBoost
	},
	{
		"ExitThread",
		ExitThread
	},
	{
		"SwitchToThread",
		SwitchToThread
	},
	{
		"GetExitCodeThread",
		GetExitCodeThread
	},
	{
		"GetCurrentThreadStackLimits",
		GetCurrentThreadStackLimits
	},
	{
		"GetThreadTimes",
		GetThreadTimes
	},
	{
		"GetCurrentThread",
		GetCurrentThread
	},
	{
		"GetCurrentThreadId",
		GetCurrentThreadId
	},
	{
		"GetCurrentProcess",
		GetCurrentProcess
	},
	{
		"GetCurrentProcessId",
		GetCurrentProcessId
	},
	{
		"OpenProcess",
		OpenProcess
	},
	{
		"TerminateProcess",
		TerminateProcess
	},
	{
		"ExitProcess",
		ExitProcess
	},
	{
		"GetExitCodeProcess",
		GetExitCodeProcess
	},
	{
		"IsProcessorFeaturePresent",
		IsProcessorFeaturePresent
	},
	{
		"GetStartupInfoA",
		GetStartupInfoA
	},
	{
		"GetStartupInfoW",
		GetStartupInfoW
	},

	//
	// libloaderapi.c
	//
	{
		"GetProcAddress",
		GetProcAddress
	},
	{
		"LoadLibraryA",
		LoadLibraryA
	},
	{
		"LoadLibraryW",
		LoadLibraryW
	},
	{
		"FreeLibrary",
		FreeLibrary
	},
	{
		"LoadLibraryExA",
		LoadLibraryExA
	},
	{
		"LoadLibraryExW",
		LoadLibraryExW
	},
	{
		"GetModuleHandleA",
		GetModuleHandleA
	},
	{
		"GetModuleHandleW",
		GetModuleHandleW
	},

	{
		"GetModuleHandleExA",
		GetModuleHandleExA
	},
	{
		"GetModuleHandleExW",
		GetModuleHandleExW
	},
	{
		"GetModuleFileNameA",
		GetModuleFileNameA
	},
	{
		"GetModuleFileNameW",
		GetModuleFileNameW
	},

	//
	// sysinfoappi.h
	//
	{
		"GetSystemTime",
		GetSystemTime
	},
	{
		"GetLocalTime",
		GetLocalTime
	},
	{
		"SetLocalTime",
		SetLocalTime
	},
	{
		"GetSystemInfo",
		GetSystemInfo
	},
	{
		"GetSystemTimeAsFileTime",
		GetSystemTimeAsFileTime
	},
	{
		"GetTickCount",
		GetTickCount
	},
	{
		"GetTickCount64",
		GetTickCount64
	},
	{
		"GetVersion",
		GetVersion
	},
	{
		"GetVersionExA",
		GetVersionExA
	},
	{
		"GlobalMemoryStatusEx",
		GlobalMemoryStatusEx
	},

	//
	// timezoneapi.c
	//
	{
		"FileTimeToSystemTime",
		FileTimeToSystemTime
	},
	{
		"SystemTimeToFileTime",
		SystemTimeToFileTime
	},
	{
		"GetTimeZoneInformation",
		GetTimeZoneInformation
	},
	{
		"SystemTimeToTzSpecificLocalTime",
		SystemTimeToTzSpecificLocalTime
	},

	//
	// debugapi.c
	//
	{
		"IsDebuggerPresent",
		IsDebuggerPresent
	},
	{
		"DebugBreak",
		DebugBreak
	},
	{
		"OutputDebugStringA",
		OutputDebugStringA
	},
	{
		"OutputDebugStringW",
		OutputDebugStringW
	},

	//
	// nammepipe.c
	//
	{
		"CreatePipe",
		CreatePipe
	},
	{
		"PeekNamedPipe",
		PeekNamedPipe
	},

	//
	// winnls.c
	//
	{
		"IsValidCodePage",
		IsValidCodePage
	},
	{
		"GetACP",
		GetACP
	},
	{
		"GetOEMCP",
		GetOEMCP
	},
	{
		"GetCPInfo",
		GetCPInfo
	},
	{
		"GetCPInfoExA",
		GetCPInfoExA
	},
	{
		"GetCPInfoExW",
		GetCPInfoExW
	},
	{
		"IsValidLocale",
		IsValidLocale
	},
	{
		"LCIDToLocaleName",
		LCIDToLocaleName
	},
	{
		"LocaleNameToLCID",
		LocaleNameToLCID
	},
	{
		"GetUserDefaultLCID",
		GetUserDefaultLCID
	},
	{
		"GetUserDefaultLocaleName",
		GetUserDefaultLocaleName
	},

	//
	// etc
	//
	{
		"InitializeSListHead",
		LdkpInitializeSListHead
	},
	{
		"RtlCaptureContext",
		RtlCaptureContext
	},
	{
		"RtlCaptureStackBackTrace",
		RtlCaptureStackBackTrace
	},
	{
		"RtlUnwind",
		RtlUnwind
	},
	{
		"RtlPcToFileHeader",
		RtlPcToFileHeader
	},
	{
		"DbgPrint",
		DbgPrint
	},
#if _AMD64_
	{
		"RtlLookupFunctionEntry",
		RtlLookupFunctionEntry
	},
	{
		"RtlUnwindEx",
		RtlUnwindEx
	},
	{
		"RtlVirtualUnwind",
		RtlVirtualUnwind
	},
#endif
	{
		NULL
	}
};



LDK_MODULE LdkpKernel32Module = {
    RTL_CONSTANT_STRING("KERNEL32.DLL"),
    RTL_CONSTANT_STRING("\\SystemRoot\\system32\\KERNEL32.DLL"),
    LdkpKernel32FunctionRegistration,
    NULL
};



NTSTATUS
LdkpKernel32Initialize (
    VOID
    )
{
	PAGED_CODE();

	SetFileApisToANSI();

	NTSTATUS Status = LdkpInitializeThreadpoolApiset();
	if (! NT_SUCCESS(Status)) {
		return Status;
	}

	Status = LdkpInitializeThreadContexts();
	if (! NT_SUCCESS(Status)) {
		LdkpTerminateThreadpoolApiset();
		return Status;
	}

	return LdkpInitializeNls();
}

VOID
LdkpKernel32Terminate (
    VOID
    )
{
	PAGED_CODE();

	LdkpTerminateThreadContexts();
	LdkpTerminateThreadpoolApiset();
}