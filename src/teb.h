#pragma once

#include "ntdll/ntdll.h"

#define LDK_TLS_SLOTS_SIZE		1024
#define LDK_FLS_SLOTS_SIZE		1024

typedef struct _LDK_FLS_SLOT {
	PVOID Data;
	PFLS_CALLBACK_FUNCTION Callback;
} LDK_FLS_SLOT, *PLDK_FLS_SLOT;

typedef struct _LDK_TEB {

	EX_RUNDOWN_REF RundownProtect;

	LIST_ENTRY ActiveLinks;

	PWSTR StaticUnicodeBuffer;

	PLDK_FLS_SLOT FlsSlots;

	PVOID* TlsSlots;

	DWORD LastErrorValue;

	CLIENT_ID ClientId;

	CLIENT_ID RealClientId;

	PETHREAD Thread;

	KSEMAPHORE KeyedWaitSemaphore;

	LIST_ENTRY KeyedWaitChain;

	PVOID KeyedWaitValue;

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