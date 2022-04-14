#pragma once

EXTERN_C_START

#define LDK_FLAG_SAFE_MODE				0x00000001
#define LDK_FLAG_VALID_MASK				0x0FFFFFFF

#define LDK_FLAG_LOCKED_BIT_INDEX		28
#define LDK_FLAG_LOCKED					(1 << LDK_FLAG_LOCKED_BIT_INDEX)
#define LDK_FLAG_INITIALIZED			0x80000000

#define LDK_IS_INITIALIZED				FlagOn(LdkGlobalFlags, LDK_FLAG_INITIALIZED)
#define LDK_IS_SAFE_MODE				FlagOn(LdkGlobalFlags, LDK_FLAG_SAFE_MODE)

#ifdef _X86_
 //LINK : error LNK2001: unresolved external symbol _LdkDriverEntry 

extern DRIVER_INITIALIZE LdkDriverEntry;
DECLSPEC_SELECTANY PDRIVER_INITIALIZE LdkpDriverEntryPtr = LdkDriverEntry;
#endif

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
LdkInitialize(
	_In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath,
	_In_ ULONG Flags
	);

VOID
LdkTerminate(
	VOID
	);



typedef struct _LDK_TEB * PLDK_TEB;
typedef struct _LDK_PEB* PLDK_PEB;

PLDK_TEB
LdkCurrentTeb (
	VOID
	);

PLDK_PEB
LdkCurrentPeb (
	VOID
	);

EXTERN_C_END