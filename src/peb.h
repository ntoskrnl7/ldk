#pragma once

#include "ntdll/ntdll.h"

typedef struct _CURDIR {
     UNICODE_STRING DosPath;
     PVOID Handle;
} CURDIR, *PCURDIR;

typedef struct _RTL_DRIVE_LETTER_CURDIR {
     WORD Flags;
     WORD Length;
     ULONG TimeStamp;
     STRING DosPath;
} RTL_DRIVE_LETTER_CURDIR, *PRTL_DRIVE_LETTER_CURDIR;

typedef struct _RTL_USER_PROCESS_PARAMETERS {
     ULONG MaximumLength;
     ULONG Length;
     ULONG Flags;
     ULONG DebugFlags;
     PVOID ConsoleHandle;
     ULONG ConsoleFlags;
     PVOID StandardInput;
     PVOID StandardOutput;
     PVOID StandardError;
     CURDIR CurrentDirectory;
     UNICODE_STRING DllPath;
     UNICODE_STRING ImagePathName;
     UNICODE_STRING CommandLine;
     PVOID Environment;
     ULONG StartingX;
     ULONG StartingY;
     ULONG CountX;
     ULONG CountY;
     ULONG CountCharsX;
     ULONG CountCharsY;
     ULONG FillAttribute;
     ULONG WindowFlags;
     ULONG ShowWindowFlags;
     UNICODE_STRING WindowTitle;
     UNICODE_STRING DesktopInfo;
     UNICODE_STRING ShellInfo;
     UNICODE_STRING RuntimeData;
     RTL_DRIVE_LETTER_CURDIR CurrentDirectores[32];
     ULONG EnvironmentSize;
} RTL_USER_PROCESS_PARAMETERS, *PRTL_USER_PROCESS_PARAMETERS;

typedef struct _LDK_PEB {

	ULONG NtGlobalFlag;
	PRTL_USER_PROCESS_PARAMETERS ProcessParameters;

	PVOID ImageBaseAddress;
	SIZE_T ImageBaseSize;

     // \??\X:\~~~
	ANSI_STRING FullPathName;

	PVOID ProcessHeap;
	ULONG NumberOfProcessors;

	LARGE_INTEGER CriticalSectionTimeout;

	PDRIVER_OBJECT DriverObject;
	UNICODE_STRING RegistryPath;

	RTL_BITMAP TlsBitmap;
	ULONG TlsBitmapBits[32];

	RTL_BITMAP FlsBitmap;
	ULONG FlsBitmapBits[32];
     PFLS_CALLBACK_FUNCTION FlsCallbacks[LDK_FLS_SLOTS_SIZE];

	ERESOURCE ModuleListResource;
	LIST_ENTRY ModuleListHead;
	NPAGED_LOOKASIDE_LIST ModuleListLookaside;

     ULONG EnvironmentUpdateCount;

} LDK_PEB, *PLDK_PEB;



BOOLEAN
LdkIsConsoleHandle (
	_In_ HANDLE Handle
	);

BOOLEAN
LdkGetConsoleHandle (
	_In_ HANDLE Handle,
	_Out_ PHANDLE RealHandle
	);



PLDK_PEB
LdkCurrentPeb (
	VOID
	);

NTSTATUS
LdkpInitializePeb (
	_In_ PDRIVER_OBJECT DriverObject,
	_In_ PUNICODE_STRING RegistryPath
	);

VOID
LdkpTerminatePeb (
	VOID
	);