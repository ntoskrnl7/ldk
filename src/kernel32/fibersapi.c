#include "winbase.h"
#include "../ldk.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FlsAlloc)
#pragma alloc_text(PAGE, FlsFree)
#endif



WINBASEAPI
DWORD
WINAPI
FlsAlloc (
    _In_opt_ PFLS_CALLBACK_FUNCTION lpCallback
    )
{
	DWORD index = FLS_OUT_OF_INDEXES;

	PAGED_CODE();

	RtlAcquirePebLock();
	try {
		index = RtlFindClearBitsAndSet( &NtCurrentPeb()->FlsBitmap,
										1,
										0 );
		if (index == 0xFFFFFFFF) {
			LdkSetLastNTError( STATUS_NO_MEMORY );
		} else {
			PLDK_FLS_SLOT slot = &NtCurrentTeb()->FlsSlots[index];
#pragma warning(disable:4054 4055)
			InterlockedExchangePointer( (PVOID *)&slot->Callback,
										(PVOID)lpCallback );
#pragma warning(default:4054 4055)
			InterlockedExchangePointer( &slot->Data,
										NULL );
		}
	} finally {
		RtlReleasePebLock();
	}
	return index;
}

WINBASEAPI
PVOID
WINAPI
FlsGetValue (
    _In_ DWORD dwFlsIndex
    )
{
	if (dwFlsIndex < LDK_FLS_SLOTS_SIZE) {
		return NtCurrentTeb()->FlsSlots[dwFlsIndex].Data;
	}
	LdkSetLastNTError( STATUS_INVALID_PARAMETER );
	return NULL;
}

WINBASEAPI
BOOL
WINAPI
FlsSetValue (
    _In_ DWORD dwFlsIndex,
    _In_opt_ PVOID lpFlsData
    )
{
	if (dwFlsIndex < LDK_FLS_SLOTS_SIZE) {
		InterlockedExchangePointer( &NtCurrentTeb()->FlsSlots[dwFlsIndex].Data,
									lpFlsData );
		return TRUE;
	}
	LdkSetLastNTError(STATUS_INVALID_PARAMETER);
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
FlsFree (
    _In_ DWORD dwFlsIndex
    )
{
	BOOLEAN index = FALSE;
	PRTL_BITMAP bitmap;

	PAGED_CODE();

	RtlAcquirePebLock();
	try {
		if (dwFlsIndex >= LDK_FLS_SLOTS_SIZE) {
			leave;
		}
		bitmap = &NtCurrentPeb()->FlsBitmap;
		index = RtlAreBitsSet( bitmap,
							   dwFlsIndex,
							   1 );
		if (index) {
			PLDK_TEB Teb = NtCurrentTeb();
			if (ExAcquireRundownProtection( &Teb->RundownProtect )) {
				PVOID data;
				PFLS_CALLBACK_FUNCTION callback;
				PLDK_FLS_SLOT slot = &Teb->FlsSlots[dwFlsIndex];
#pragma warning(disable:4055)
				callback = (PFLS_CALLBACK_FUNCTION)InterlockedExchangePointer( (PVOID *)&slot->Callback,
																			   NULL );
#pragma warning(default:4055)
				data = InterlockedExchangePointer( &slot->Data,
												   NULL );
				if (callback) {
					callback( data );
				}
				ExReleaseRundownProtection(&Teb->RundownProtect);
			}
			RtlClearBits( bitmap,
						  dwFlsIndex,
						  1 );
		}
	} finally {
		RtlReleasePebLock();
	}
	return index;
}