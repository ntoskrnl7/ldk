#pragma once

#ifndef _LDK_
#define _LDK_

EXTERN_C_START

#define LDKAPI		__stdcall

#define LDK_FLAG_SAFE_MODE				0x00000001
#define LDK_FLAG_VALID_MASK				0x0FFFFFFF

#define LDK_FLAG_LOCKED_BIT_INDEX		28
#define LDK_FLAG_LOCKED					(1 << LDK_FLAG_LOCKED_BIT_INDEX)
#define LDK_SHUTDOWN_IN_PROGRESS		0x40000000 // 30
#define LDK_FLAG_INITIALIZED			0x80000000 // 31

#define LDK_IS_INITIALIZED				FlagOn(LdkGlobalFlags, LDK_FLAG_INITIALIZED)
#define LDK_IS_SAFE_MODE				FlagOn(LdkGlobalFlags, LDK_FLAG_SAFE_MODE)
#define LDK_IS_SHUTDOWN_IN_PROGRESS		FlagOn(LdkGlobalFlags, LDK_SHUTDOWN_IN_PROGRESS)

extern ULONG LdkGlobalFlags;

FORCEINLINE
VOID
LdkLockGlobalFlags (
	VOID
	)
{
	while (InterlockedBitTestAndSet((PLONG)&LdkGlobalFlags, LDK_FLAG_LOCKED_BIT_INDEX)) {
		YieldProcessor();
	}
}
FORCEINLINE
VOID
LdkUnlockGlobalFlags (
	VOID
	)
{
	InterlockedBitTestAndReset((PLONG)&LdkGlobalFlags, LDK_FLAG_LOCKED_BIT_INDEX);
}



NTSTATUS
LDKAPI
LdkInitialize(
	_In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath,
	_In_ ULONG Flags
	);

VOID
LDKAPI
LdkTerminate(
	VOID
	);



typedef struct _LDK_TEB * PLDK_TEB;
typedef struct _LDK_PEB* PLDK_PEB;

PLDK_TEB
LDKAPI
LdkCurrentTeb (
	VOID
	);

PLDK_PEB
LDKAPI
LdkCurrentPeb (
	VOID
	);



typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
(LDK_INITIALIZE_COMPONENT) (
	VOID
    );

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
(LDK_TERMINATE_COMPONENT) (
	VOID
    );

EXTERN_C_END

#endif // _LDK_