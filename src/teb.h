#pragma once

#include "ntdll/ntdll.h"

#define LDK_TLS_SLOTS_SIZE		1024
#define LDK_FLS_SLOTS_SIZE		1024

typedef struct _LDK_FLS_SLOT {
	PVOID Data;
	PFLS_CALLBACK_FUNCTION Callback;
} LDK_FLS_SLOT, *PLDK_FLS_SLOT;



#define STATIC_UNICODE_BUFFER_LENGTH 261
typedef struct _LDK_TEB {

	EX_RUNDOWN_REF RundownProtect;

	LIST_ENTRY ActiveLinks;

	PLDK_FLS_SLOT FlsSlots;

	PVOID* TlsSlots;

	DWORD LastErrorValue;

	CLIENT_ID ClientId;

	CLIENT_ID RealClientId;

	PETHREAD Thread;

	KSEMAPHORE KeyedWaitSemaphore;

	LIST_ENTRY KeyedWaitChain;

	PVOID KeyedWaitValue;

	ULONG HardErrorsAreDisabled;

	UNICODE_STRING StaticUnicodeString;

	WCHAR StaticUnicodeBuffer[STATIC_UNICODE_BUFFER_LENGTH];

	ULONG HardErrorMode;

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