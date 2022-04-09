#include "ldk.h"
#include "teb.h"

LIST_ENTRY LdkpTebListHead;
EX_SPIN_LOCK LdkpTebListLock;

NPAGED_LOOKASIDE_LIST LdkpTebLookaside;
NPAGED_LOOKASIDE_LIST LdkpTebStaticUnicodeBufferLookaside;
NPAGED_LOOKASIDE_LIST LdkpTebTlsLookaside;
NPAGED_LOOKASIDE_LIST LdkpTebFlsLookaside;

NTSTATUS
LdkpInitializeTeb (
	_Inout_ PLDK_TEB Teb,
	_In_ PETHREAD Thread
	);

VOID
LdkpTerminateTeb (
	_Inout_ PLDK_TEB Teb
	);

PLDK_TEB
LdkCreateTeb (
	_In_ PETHREAD Thread
	)
{
	PLDK_TEB Teb;
	KIRQL OldIrql;

	Teb = ExAllocateFromNPagedLookasideList(&LdkpTebLookaside);
	if (! Teb) {
		return NULL;
	}

	if (! NT_SUCCESS(LdkpInitializeTeb(Teb, Thread))) {
		ExFreeToNPagedLookasideList(&LdkpTebLookaside, Teb);
		return NULL;
	}

	OldIrql = ExAcquireSpinLockExclusive(&LdkpTebListLock);
	InsertHeadList(&LdkpTebListHead, &Teb->ActiveLinks);
	ExReleaseSpinLockExclusive(&LdkpTebListLock, OldIrql);

	return Teb;
}

VOID
LdkDeleteTeb (
	_In_ PLDK_TEB Teb
	)
{
	LdkpTerminateTeb(Teb);
	ExFreeToNPagedLookasideList(&LdkpTebLookaside, Teb);
}

PLDK_TEB
LdkLookTebByThread (
	_In_ PETHREAD Thread
	)
{
	PLDK_TEB Teb;
	PLIST_ENTRY NextEntry;
	KIRQL OldIrql;
	OldIrql = ExAcquireSpinLockShared(&LdkpTebListLock);

	for (NextEntry = LdkpTebListHead.Flink;
		NextEntry != &LdkpTebListHead;
		NextEntry = NextEntry->Flink) {

		Teb = CONTAINING_RECORD(NextEntry, LDK_TEB, ActiveLinks);

		if (Teb->Thread == Thread) {
			ExReleaseSpinLockShared(&LdkpTebListLock, OldIrql);
			return Teb;
		}

	}

	ExReleaseSpinLockShared(&LdkpTebListLock, OldIrql);
	return NULL;
}

PLDK_TEB
LdkCurrentTeb (
	VOID
	)
{
	PLDK_TEB Teb = NULL;
	ULONG_PTR LowLimit, HighLimit;
	
	IoGetStackLimits(&LowLimit, &HighLimit);
	
	Teb = *(PLDK_TEB *)LowLimit;
	if (Teb && MmIsAddressValid(Teb) && Teb->Thread == PsGetCurrentThread()) {
		return Teb;
	}

	//
	// KeExpandKernelStackAndCallout/Ex 함수 등을 사용하여 스택 확장등으로 인해서
	// TEB를 못얻어올 경우가 존재하기때문에 TebMap에서 TEB를 찾도록 합니다.
	//
	
	Teb = LdkLookTebByThread(PsGetCurrentThread());
	if (Teb) {
		*(PLDK_TEB *)LowLimit = Teb;
		return Teb;
	}

	//
	// TEB가 없다면 새로 생성합니다.
	//

	Teb = LdkCreateTeb(PsGetCurrentThread());
	NT_ASSERT(Teb);

	*(PLDK_TEB *)LowLimit = Teb;
	return Teb;
}

NTSTATUS
LdkInitializeTebMap (
	VOID
	) 
{
	LdkpTebListLock = 0;
	InitializeListHead(&LdkpTebListHead);

	ExInitializeNPagedLookasideList(&LdkpTebLookaside, NULL, NULL, 0, sizeof(LDK_TEB), 'beTA', 0);
	ExInitializeNPagedLookasideList(&LdkpTebTlsLookaside, NULL, NULL, 0, LDK_TLS_SLOTS_SIZE * sizeof(PVOID), 'slTT', 0);
	ExInitializeNPagedLookasideList(&LdkpTebFlsLookaside, NULL, NULL, 0, LDK_FLS_SLOTS_SIZE * sizeof(LDK_FLS_SLOT), 'slFT', 0);
	ExInitializeNPagedLookasideList(&LdkpTebStaticUnicodeBufferLookaside, NULL, NULL, 0, 260 * sizeof(WCHAR), 'BcUT', 0);

	return STATUS_SUCCESS;
}

VOID
LdkTerminateTebMap (
	VOID
	)
{
	PLDK_TEB Teb;
	PLIST_ENTRY Entry;
	KIRQL OldIrql;

	OldIrql = ExAcquireSpinLockExclusive(&LdkpTebListLock);

	Entry = RemoveHeadList(&LdkpTebListHead);

	while (Entry != &LdkpTebListHead) {
		Teb = CONTAINING_RECORD(Entry, LDK_TEB, ActiveLinks);
		Entry = RemoveHeadList(&LdkpTebListHead);
		LdkDeleteTeb(Teb);
	}

	ExReleaseSpinLockExclusive(&LdkpTebListLock, OldIrql);

	ExDeleteNPagedLookasideList(&LdkpTebLookaside);
	ExDeleteNPagedLookasideList(&LdkpTebTlsLookaside);
	ExDeleteNPagedLookasideList(&LdkpTebFlsLookaside);
	ExDeleteNPagedLookasideList(&LdkpTebStaticUnicodeBufferLookaside);
}

NTSTATUS
LdkpInitializeTeb (
	_Inout_ PLDK_TEB Teb,
	_In_ PETHREAD Thread
	)
{
	RtlZeroMemory(Teb, sizeof(LDK_TEB));

	ExInitializeRundownProtection(&Teb->RundownProtect);	

	Teb->Thread = Thread;

	Teb->TlsSlots = ExAllocateFromNPagedLookasideList(&LdkpTebTlsLookaside);
	if (!Teb->TlsSlots) {
		KdBreakPoint();
		LdkpTerminateTeb(Teb);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(Teb->TlsSlots, LDK_TLS_SLOTS_SIZE * sizeof(PVOID));

	Teb->FlsSlots = ExAllocateFromNPagedLookasideList(&LdkpTebFlsLookaside);
	if (!Teb->FlsSlots) {
		KdBreakPoint();
		LdkpTerminateTeb(Teb);
		return STATUS_INSUFFICIENT_RESOURCES;
	}
	RtlZeroMemory(Teb->FlsSlots, LDK_FLS_SLOTS_SIZE * sizeof(LDK_FLS_SLOT));

	Teb->StaticUnicodeBuffer = ExAllocateFromNPagedLookasideList(&LdkpTebStaticUnicodeBufferLookaside);
	if (!Teb->StaticUnicodeBuffer) {
		KdBreakPoint();
		LdkpTerminateTeb(Teb);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	KeInitializeSemaphore(&Teb->KeyedWaitSemaphore, 0L, 1L);
	InitializeListHead(&Teb->KeyedWaitChain);
	Teb->KeyedWaitValue = NULL;

	Teb->ClientId.UniqueProcess = PsGetCurrentProcessId();
	Teb->ClientId.UniqueThread = PsGetThreadId(Thread);

	Teb->RealClientId = Teb->ClientId;

	return STATUS_SUCCESS;
}

VOID
LdkpTerminateTeb (
	_Inout_ PLDK_TEB Teb
	)
{
	ExWaitForRundownProtectionRelease(&Teb->RundownProtect);
	ExRundownCompleted(&Teb->RundownProtect);

	Teb->Thread = NULL;

	if (Teb->TlsSlots) {
		ExFreeToNPagedLookasideList(&LdkpTebTlsLookaside, Teb->TlsSlots);
		Teb->TlsSlots = NULL;
	}

	if (Teb->FlsSlots) {
		PVOID Data;
		PFLS_CALLBACK_FUNCTION Callback;
		PLDK_FLS_SLOT Slot;

		for (DWORD i = 0; i < LDK_FLS_SLOTS_SIZE; i++) {
			Slot = &Teb->FlsSlots[i];
			Data = InterlockedExchangePointer(&Slot->Data, NULL);
#pragma warning(disable:4055)
			Callback = (PFLS_CALLBACK_FUNCTION)InterlockedExchangePointer((PVOID *)&Slot->Callback, NULL);
#pragma warning(default:4055)
			if (Callback) {
				NT_ASSERT(MmIsAddressValid((PVOID)(ULONG_PTR)Callback));
				Callback(Data);
			}
		}

		ExFreeToNPagedLookasideList(&LdkpTebFlsLookaside, Teb->FlsSlots);
		Teb->FlsSlots = NULL;
	}

	if (Teb->StaticUnicodeBuffer) {
		ExFreeToNPagedLookasideList(&LdkpTebStaticUnicodeBufferLookaside, Teb->StaticUnicodeBuffer);
		Teb->StaticUnicodeBuffer = NULL;
	}
}
