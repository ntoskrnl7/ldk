#include "winbase.h"
#include "../ldk.h"

#if (_WIN32_WINNT >= 0x0600)
WINBASEAPI
DWORD
WINAPI
FlsAlloc(
    _In_opt_ PFLS_CALLBACK_FUNCTION lpCallback
    )
{
	DWORD Index = FLS_OUT_OF_INDEXES;

	RtlAcquirePebLock();
	try {
        
		Index = RtlFindClearBitsAndSet(&NtCurrentPeb()->FlsBitmap, 1, 0);
		if (Index == 0xFFFFFFFF) {
			BaseSetLastNTError(STATUS_NO_MEMORY);
		} else {
			PLDK_FLS_SLOT Slot = &NtCurrentTeb()->FlsSlots[Index];
#pragma warning(disable:4054 4055)
			InterlockedExchangePointer((PVOID *)&Slot->Callback, (PVOID)lpCallback);
#pragma warning(default:4054 4055)
			InterlockedExchangePointer(&Slot->Data, NULL);
		}
	} finally {
		RtlReleasePebLock();
	}

	return Index;
}

WINBASEAPI
PVOID
WINAPI
FlsGetValue(
    _In_ DWORD dwFlsIndex
    )
{
	if (dwFlsIndex < LDK_FLS_SLOTS_SIZE) {
		return NtCurrentTeb()->FlsSlots[dwFlsIndex].Data;
	} else {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return NULL;
	}
}

WINBASEAPI
BOOL
WINAPI
FlsSetValue(
    _In_ DWORD dwFlsIndex,
    _In_opt_ PVOID lpFlsData
    )
{
	if (dwFlsIndex < LDK_FLS_SLOTS_SIZE) {
		PLDK_FLS_SLOT Slot = &NtCurrentTeb()->FlsSlots[dwFlsIndex];
		InterlockedExchangePointer(&Slot->Data, lpFlsData);
		return TRUE;
	} else {
		BaseSetLastNTError(STATUS_INVALID_PARAMETER);
		return FALSE;
	}
}

WINBASEAPI
BOOL
WINAPI
FlsFree(
    _In_ DWORD dwFlsIndex
    )
{
	BOOLEAN ValidIndex = FALSE;
	PRTL_BITMAP FlsBitmap;

	RtlAcquirePebLock();
	try {
		if (dwFlsIndex >= LDK_FLS_SLOTS_SIZE) {
			leave;
		}
		FlsBitmap = &NtCurrentPeb()->FlsBitmap;
		ValidIndex = RtlAreBitsSet(FlsBitmap, dwFlsIndex, 1);
		if (ValidIndex) {
			PLDK_TEB Teb = NtCurrentTeb();
			if (ExAcquireRundownProtection(&Teb->RundownProtect)) {

				PVOID Data;
				PFLS_CALLBACK_FUNCTION Callback;
				PLDK_FLS_SLOT Slot = &Teb->FlsSlots[dwFlsIndex];

#pragma warning(disable:4055)
				Callback = (PFLS_CALLBACK_FUNCTION)InterlockedExchangePointer((PVOID *)&Slot->Callback, NULL);
#pragma warning(default:4055)
				Data = InterlockedExchangePointer(&Slot->Data, NULL);

				if (Callback) {
					Callback(Data);
				}
				
				ExReleaseRundownProtection(&Teb->RundownProtect);
			}
			RtlClearBits(FlsBitmap, dwFlsIndex, 1);
		}
	} finally {
		RtlReleasePebLock();
	}
	return ValidIndex;
}
#endif