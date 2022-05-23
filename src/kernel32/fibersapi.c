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
	DWORD Index = FLS_OUT_OF_INDEXES;

	PAGED_CODE();

	RtlAcquirePebLock();
	try {
		Index = RtlFindClearBitsAndSet( &NtCurrentPeb()->FlsBitmap,
										1,
										0 );
		if (Index == 0xFFFFFFFF) {
			LdkSetLastNTError( STATUS_NO_MEMORY );
		} else {
#pragma warning(disable:4054 4055)
			InterlockedExchangePointer( (PVOID *)&NtCurrentPeb()->FlsCallbacks[Index],
										(PVOID)lpCallback );
#pragma warning(default:4054 4055)
			InterlockedExchangePointer( &NtCurrentTeb()->FlsSlots[Index],
										NULL );
		}
	} finally {
		RtlReleasePebLock();
	}
	return Index;
}

WINBASEAPI
PVOID
WINAPI
FlsGetValue (
    _In_ DWORD dwFlsIndex
    )
{
	if (dwFlsIndex < LDK_FLS_SLOTS_SIZE) {
		return NtCurrentTeb()->FlsSlots[dwFlsIndex];
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
		InterlockedExchangePointer( &NtCurrentTeb()->FlsSlots[dwFlsIndex],
									lpFlsData );
		return TRUE;
	}
	LdkSetLastNTError( STATUS_INVALID_PARAMETER );
	return FALSE;
}

PTEB
LdkGetNextTebRundownProtection (
	_In_ PTEB Teb
	);

WINBASEAPI
BOOL
WINAPI
FlsFree (
    _In_ DWORD dwFlsIndex
    )
{
	BOOLEAN Found = FALSE;

	PAGED_CODE();

	RtlAcquirePebLock();
	try {
		if (dwFlsIndex >= LDK_FLS_SLOTS_SIZE) {
			leave;
		}
		PRTL_BITMAP Bitmap = &NtCurrentPeb()->FlsBitmap;
		Found = RtlAreBitsSet( Bitmap,
							   dwFlsIndex,
							   1 );
		if (Found) {
			PFLS_CALLBACK_FUNCTION Callback;
#pragma warning(disable:4055)
			Callback = (PFLS_CALLBACK_FUNCTION)InterlockedExchangePointer( (PVOID *)&NtCurrentPeb()->FlsCallbacks[dwFlsIndex],
																		   NULL );
#pragma warning(default:4055)
			if (Callback) {
				PTEB InitialTeb = NtCurrentTeb();
				if (! ExAcquireRundownProtection( &InitialTeb->RundownProtect )) {
					return FALSE;
				}
				PTEB Teb = InitialTeb;
				PVOID Data;
				do {
					Data = InterlockedExchangePointer( &Teb->FlsSlots[dwFlsIndex],
													   NULL );
					if (Data) {
						Callback( Data );
					}
					Teb = LdkGetNextTebRundownProtection( Teb );
				} while (Teb != InitialTeb);
				ExReleaseRundownProtection( &Teb->RundownProtect );
			}
			RtlClearBits( Bitmap,
						  dwFlsIndex,
						  1 );
		}
	} finally {
		RtlReleasePebLock();
	}
	return Found;
}