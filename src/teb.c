#include "ldk.h"
#include "teb.h"

LIST_ENTRY LdkpTebListHead;
EX_SPIN_LOCK LdkpTebListLock;

NPAGED_LOOKASIDE_LIST LdkpTebLookaside;

SLIST_HEADER LdkpTemporaryTebListHead;

NTSTATUS
LdkpInitializeTeb (
	_Inout_ PLDK_TEB Teb,
	_In_ PETHREAD Thread
	);

VOID
LdkpTerminateTeb (
	_Inout_ PLDK_TEB Teb
	);

VOID
LdkpInvokeFlsCallback (
	_Inout_ PLDK_TEB Teb
	);



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeTebMap)
#pragma alloc_text(PAGE, LdkpInvokeFlsCallback)
#endif



PLDK_TEB
LdkpCreateTeb (
	_In_ PETHREAD Thread
	)
{
	PLDK_TEB Teb = ExAllocateFromNPagedLookasideList( &LdkpTebLookaside );
	if (! Teb) {
		return NULL;
	}
	if (! NT_SUCCESS(LdkpInitializeTeb(Teb, Thread))) {
		ExFreeToNPagedLookasideList(&LdkpTebLookaside, Teb);
		return NULL;
	}
	return Teb;
}

PLDK_TEB
LdkCreateTeb (
	_In_ PETHREAD Thread
	)
{
	PLDK_TEB Teb = LdkpCreateTeb( Thread );
	KIRQL OldIrql = ExAcquireSpinLockExclusive( &LdkpTebListLock );
	InsertHeadList( &LdkpTebListHead,
					&Teb->ActiveLinks );
	ExReleaseSpinLockExclusive( &LdkpTebListLock,
								OldIrql );
	return Teb;
}

VOID
LdkDeleteTeb (
	_In_ PLDK_TEB Teb
	)
{
	LdkpTerminateTeb( Teb );
	ExFreeToNPagedLookasideList( &LdkpTebLookaside,
								 Teb );
}

PLDK_TEB
LdkLookTebByThread (
	_In_ PETHREAD Thread
	)
{
	PLDK_TEB Teb;
	PLIST_ENTRY NextEntry;
	KIRQL OldIrql= ExAcquireSpinLockShared( &LdkpTebListLock );
 
	for (NextEntry = LdkpTebListHead.Flink;
		NextEntry != &LdkpTebListHead;
		NextEntry = NextEntry->Flink) {

		Teb = CONTAINING_RECORD(NextEntry, LDK_TEB, ActiveLinks);

		if (Teb->Thread == Thread) {
			ExReleaseSpinLockShared( &LdkpTebListLock,
									 OldIrql );
			return Teb;
		}

	}

	ExReleaseSpinLockShared( &LdkpTebListLock,
							 OldIrql );
	return NULL;
}

PTEB
LdkGetNextTebRundownProtection (
	_In_ PTEB Teb
	)
{
	PTEB Next;
	KIRQL OldIrql= ExAcquireSpinLockShared( &LdkpTebListLock );	
	if (Teb->ActiveLinks.Flink == &LdkpTebListHead) {
		Next = CONTAINING_RECORD(LdkpTebListHead.Flink, LDK_TEB, ActiveLinks);
	} else {
		Next = CONTAINING_RECORD(Teb->ActiveLinks.Flink, LDK_TEB, ActiveLinks);
	}
	ExReleaseSpinLockShared( &LdkpTebListLock,
							 OldIrql );

	ExReleaseRundownProtection( &Teb->RundownProtect );

	if (ExAcquireRundownProtection( &Next->RundownProtect )) {
		return Next;
	}
	return NULL;
}

PLDK_TEB
LDKAPI
LdkCurrentTeb (
	VOID
	)
{
	//
	// 스택의 최하단에 Teb가 있다면, Teb를 반환하도록 합니다.
	// 동기화 후 목록을 순회하는 행동은 너무 느리므로 이러한 방법을 적용하였습니다.
	//
	ULONG_PTR LowLimit, HighLimit;
	IoGetStackLimits( &LowLimit,
					  &HighLimit );
	
	PLDK_TEB Teb = *(PLDK_TEB *)LowLimit;
	if (Teb && MmIsAddressValid( Teb ) && Teb->Thread == PsGetCurrentThread()) {
		return Teb;
	}

	//
	// KeExpandKernelStackAndCallout/Ex 함수 등을 사용하여 스택 확장등으로 인해서
	// TEB를 못얻어올 경우가 존재하기때문에 TebMap에서 TEB를 찾도록 합니다.
	//
	Teb = LdkLookTebByThread( PsGetCurrentThread() );
	if (Teb) {
		*(PLDK_TEB *)LowLimit = Teb;
		return Teb;
	}

	//
	// LDK가 종료되는 중이라면, 임시 목록에 새 TEB를 추가 후 반환하도록 합니다.
	//
	if (LDK_IS_SHUTDOWN_IN_PROGRESS) {

		KdBreakPoint();

		Teb = LdkpCreateTeb( PsGetCurrentThread() );
		NT_ASSERT(Teb);

		InterlockedPushEntrySList( &LdkpTemporaryTebListHead,
								   &Teb->TempLinks );
		*(PLDK_TEB *)LowLimit = Teb;
		return Teb;
	}

	//
	// TEB가 없다면 새로 생성합니다.
	//
	Teb = LdkCreateTeb( PsGetCurrentThread() );
	NT_ASSERT(Teb);
	*(PLDK_TEB *)LowLimit = Teb;
	return Teb;
}

NTSTATUS
LdkpInitializeTebMap (
	VOID
	) 
{
	PAGED_CODE();

	LdkpTebListLock = 0;
	InitializeListHead( &LdkpTebListHead );
	InitializeSListHead( &LdkpTemporaryTebListHead );

	ExInitializeNPagedLookasideList( &LdkpTebLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_TEB),
									 'beTK',
									 0 );

	return STATUS_SUCCESS;
}

VOID
LdkpInvokeFlsCallback (
	_Inout_ PLDK_TEB Teb
	)
{
	PAGED_CODE();

	if (ExAcquireRundownProtection( &Teb->RundownProtect )) {
		for (DWORD i = 0; i < LDK_FLS_SLOTS_SIZE; i++) {
			PVOID Data;
			PFLS_CALLBACK_FUNCTION Callback;
#pragma warning(disable:4055)
			Callback = NtCurrentPeb()->FlsCallbacks[i];
#pragma warning(default:4055)
			Data = InterlockedExchangePointer( &Teb->FlsSlots[i],
											   NULL );
			if (Callback) {
				Callback(Data);
			}				
		}
		ExReleaseRundownProtection( &Teb->RundownProtect );
	}
}

VOID
LdkpTerminateTebMap (
	VOID
	)
{	
	//
	// 현재 스레드의 TEB가 없다면 미리 생성해놓습니다.
	// LdkpInvokeFlsCallback 호출 시,
	// Fls Callback 함수 내에 TEB 접근 코드가 존재한다면
	// TEB를 생성 후 등록하지 못하므로 미리 생성합니다.
	//
	PLDK_TEB Teb = LdkCurrentTeb();

	//
	// Fls Callback은 PASSIVE_LEVEL에서만 호출 가능한 함수를 호출할 가능성이 있으므로
	// LdkpTebListLock을 획득한 상태에서 LdkpInvokeFlsCallback를 호출하면 안됩니다.
	// 그래서 LdkpInvokeFlsCallback를 호출하기전에 TEB 목록을 임시 변수로 이동시키도록
	// 처리했습니다.

	LIST_ENTRY TebListHead;
	KIRQL OldIrql = ExAcquireSpinLockExclusive( &LdkpTebListLock );
	LdkpTebListHead.Blink->Flink = &TebListHead;
	TebListHead.Blink = LdkpTebListHead.Blink;
	LdkpTebListHead.Flink->Blink = &TebListHead;
	TebListHead.Flink = LdkpTebListHead.Flink;
	InitializeListHead( &LdkpTebListHead );
	ExReleaseSpinLockExclusive( &LdkpTebListLock,
							 	OldIrql );

	//
	// Teb에 등록된 Fls Callbck을 모두 호출 후 TEB를 할당 해제합니다.
	//
	PLIST_ENTRY Entry = RemoveHeadList( &TebListHead );
	while (Entry != &TebListHead) {
		Teb = CONTAINING_RECORD(Entry, LDK_TEB, ActiveLinks);
		Entry = RemoveHeadList( &TebListHead );
		LdkpInvokeFlsCallback( Teb );
		LdkDeleteTeb(Teb);
	}

	PSLIST_ENTRY TempEntry = InterlockedPopEntrySList( &LdkpTemporaryTebListHead );
	while (TempEntry) {
		Teb = CONTAINING_RECORD(TempEntry, LDK_TEB, TempLinks);
		LdkDeleteTeb( Teb );
		TempEntry = InterlockedPopEntrySList( &LdkpTemporaryTebListHead );
	};

	ExDeleteNPagedLookasideList( &LdkpTebLookaside );
}

NTSTATUS
LdkpInitializeTeb (
	_Inout_ PLDK_TEB Teb,
	_In_ PETHREAD Thread
	)
{
	RtlZeroMemory( Teb,
				   sizeof(LDK_TEB) );

	ExInitializeRundownProtection( &Teb->RundownProtect );

	Teb->Thread = Thread;

	RtlZeroMemory( Teb->TlsSlots,
				   sizeof(Teb->TlsSlots) );

	RtlZeroMemory( Teb->FlsSlots,
				   sizeof(Teb->FlsSlots) );

	KeInitializeSemaphore( &Teb->KeyedWaitSemaphore,
						   0L,
						   1L );
	InitializeListHead( &Teb->KeyedWaitChain );
	Teb->KeyedWaitValue = NULL;

	Teb->ClientId.UniqueProcess = PsGetCurrentProcessId();
	Teb->ClientId.UniqueThread = PsGetThreadId( Thread );

	Teb->RealClientId = Teb->ClientId;

    Teb->StaticUnicodeString.Buffer = Teb->StaticUnicodeBuffer;
    Teb->StaticUnicodeString.MaximumLength = (USHORT)sizeof(Teb->StaticUnicodeBuffer);
    Teb->StaticUnicodeString.Length = (USHORT)0;

	return STATUS_SUCCESS;
}

VOID
LdkpTerminateTeb (
	_Inout_ PLDK_TEB Teb
	)
{
	ExWaitForRundownProtectionRelease( &Teb->RundownProtect );
	ExRundownCompleted( &Teb->RundownProtect );
	RtlZeroMemory( Teb->TlsSlots,
				   sizeof(Teb->TlsSlots) );

	RtlZeroMemory( Teb->FlsSlots,
				   sizeof(Teb->FlsSlots) );
	Teb->Thread = NULL;
}
