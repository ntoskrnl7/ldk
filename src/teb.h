#pragma once

#include "ntdll/ntdll.h"

#define STATIC_UNICODE_BUFFER_LENGTH 1024

typedef struct _LDK_TEB {

	EX_RUNDOWN_REF RundownProtect;

	PETHREAD Thread;

	LIST_ENTRY ActiveLinks;

	DWORD LastErrorValue;

	CLIENT_ID ClientId;

	CLIENT_ID RealClientId;

	KSEMAPHORE KeyedWaitSemaphore;

	LIST_ENTRY KeyedWaitChain;

	PVOID KeyedWaitValue;

	ULONG HardErrorsAreDisabled;

	UNICODE_STRING StaticUnicodeString;

	WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];

	ULONG HardErrorMode;

	PVOID TlsSlots[LDK_TLS_SLOTS_SIZE];
	PVOID FlsSlots[LDK_FLS_SLOTS_SIZE];

	struct _LDK_DISPATCH_EXCEPTION_STACK_VARIABLES* OldDispatchExceptionStackVariables;

	SLIST_ENTRY TempLinks;

} LDK_TEB, *PLDK_TEB;



PLDK_TEB
LdkCurrentTeb (
	VOID
	);



NTSTATUS
LdkpInitializeTebMap (
	VOID
	);

VOID
LdkpTerminateTebMap (
	VOID
	);