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
	Teb->ProcessEnvironmentBlock = LdkCurrentPeb();
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

PLDK_TEB
LdkReferenceTebByThread (
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

		if (Teb->Thread == Thread &&
			ExAcquireRundownProtection( &Teb->RundownProtect )) {
			ExReleaseSpinLockShared( &LdkpTebListLock,
									 OldIrql );
			return Teb;
		}

	}

	ExReleaseSpinLockShared( &LdkpTebListLock,
							 OldIrql );
	return NULL;
}

VOID
LdkDereferenceTeb (
	_Inout_ PLDK_TEB Teb
	)
{
	ExReleaseRundownProtection( &Teb->RundownProtect );
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
	// If a TEB is cached at the lowest stack address, return it.
	// Acquiring synchronization and walking the list on every lookup is too slow.
	//
	ULONG_PTR LowLimit, HighLimit;
	IoGetStackLimits( &LowLimit,
					  &HighLimit );
	
	PLDK_TEB *StackTebSlot = NULL;
	PLDK_TEB Teb = NULL;
	if (LowLimit &&
		LowLimit < HighLimit &&
		MmIsAddressValid( (PVOID)LowLimit )
		) {
		StackTebSlot = (PLDK_TEB *)LowLimit;
		Teb = *StackTebSlot;
	}

	if (Teb &&
		MmIsAddressValid( Teb ) &&
		MmIsAddressValid(Add2Ptr(Teb, FIELD_OFFSET(LDK_TEB, Thread))) &&
		Teb->Thread == PsGetCurrentThread()
		) {
		return Teb;
	}

	//
	// Stack expansion through KeExpandKernelStackAndCallout/Ex can hide the
	// cached stack TEB, so fall back to the TEB map.
	//
	Teb = LdkLookTebByThread( PsGetCurrentThread() );
	if (Teb) {
		if (StackTebSlot) {
			*StackTebSlot = Teb;
		}
		return Teb;
	}

	//
	// During LDK shutdown, create a TEB on the temporary list and return it.
	//
	if (LDK_IS_SHUTDOWN_IN_PROGRESS) {
		Teb = LdkpCreateTeb( PsGetCurrentThread() );
		NT_ASSERT(Teb);

		InterlockedPushEntrySList( &LdkpTemporaryTebListHead,
								   &Teb->TempLinks );
		if (StackTebSlot) {
			*StackTebSlot = Teb;
		}
		return Teb;
	}

	//
	// Create a new TEB if one does not already exist.
	//
	Teb = LdkCreateTeb( PsGetCurrentThread() );
	NT_ASSERT(Teb);
	if (StackTebSlot) {
		*StackTebSlot = Teb;
	}
	return Teb;
}


#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
LDK_INITIALIZE_COMPONENT LdkpInitializeDispatchExceptionStackVariables;
LDK_TERMINATE_COMPONENT LdkpTerminateDispatchExceptionStackVariables;
#endif

NTSTATUS
LdkpInitializeTebMap (
	VOID
	) 
{
	PAGED_CODE();

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
	NTSTATUS Status = LdkpInitializeDispatchExceptionStackVariables();
	if (!NT_SUCCESS(Status)) {
		return Status;
	}
#endif

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
			if (Callback && Data) {
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
	// Ensure the current thread has a TEB before invoking FLS callbacks.
	// A callback may access the TEB, and that path cannot safely create and
	// register a new TEB while callback teardown is already in progress.
	//
	PLDK_TEB Teb = LdkCurrentTeb();

	//
	// FLS callbacks may call routines that are only valid at PASSIVE_LEVEL, so
	// do not invoke them while holding LdkpTebListLock. Move the TEB list to a
	// local list before invoking callbacks.

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
	// Invoke all FLS callbacks registered on the TEB, then free the TEB.
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

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
	LdkpTerminateDispatchExceptionStackVariables();
#endif
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

	Teb->AlertByThreadIdPending = FALSE;

	Teb->ClientId.UniqueProcess = (HANDLE)(ULONG_PTR)LdkCurrentPeb()->ProcessId;
	Teb->ClientId.UniqueThread = PsGetThreadId( Thread );

	Teb->RealClientId.UniqueProcess = PsGetCurrentProcessId();
	Teb->RealClientId.UniqueThread = Teb->ClientId.UniqueThread;

    Teb->StaticUnicodeString.Buffer = Teb->StaticUnicodeBuffer;
    Teb->StaticUnicodeString.MaximumLength = (USHORT)sizeof(Teb->StaticUnicodeBuffer);
    Teb->StaticUnicodeString.Length = (USHORT)0;

	return STATUS_SUCCESS;
}

VOID
LdkFreeDispatchExceptionStackVariables (
    _In_ PVOID OldVariables
    );

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

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
	if (Teb->OldDispatchExceptionStackVariables) {
		LdkFreeDispatchExceptionStackVariables( Teb->OldDispatchExceptionStackVariables );
		Teb->OldDispatchExceptionStackVariables = NULL;
	}
#endif
}
