#include "winbase.h"

#define TAG_FREE_MODULE_ENTRY		'mFdL'
typedef struct _LDK_FREE_MODULE_ENTRY {
	SINGLE_LIST_ENTRY Links;
	HMODULE hModule;
} LDK_FREE_MODULE_ENTRY, *PLDK_FREE_MODULE_ENTRY;

NPAGED_LOOKASIDE_LIST LdkpFreeModuleEntryLookaside;



typedef struct _TP_POOL
{
	EX_SPIN_LOCK Lock;
} TP_POOL, *PTP_POOL;


typedef struct _TP_CALLBACK_INSTANCE
{
	SINGLE_LIST_ENTRY ModuleList;
	KSPIN_LOCK Lock;
	BOOLEAN DeletePending;
} TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;


#define TAG_TP_WORK		'kWpT'

typedef struct _TP_WORK
{
	LONG ReferenceCount;

#define LDK_TP_WORK_FLAGS_DELETE_REQUESTED			0x00000001
#define LDK_TP_WORK_FLAGS_CANCEL_REQUESTED			0x00000002
#define LDK_TP_WORK_FLAGS_STARTED_BIT_INDEX			2
#define LDK_TP_WORK_FLAGS_STARTED					(1 << (LDK_TP_WORK_FLAGS_STARTED_BIT_INDEX))
	LONG Flags;

	KEVENT Event;

    PVOID CallbackParameter;
    PTP_WORK_CALLBACK WorkCallback;
    PTP_CALLBACK_ENVIRON CallbackEnvironment;
} TP_WORK, *PTP_WORK;

NPAGED_LOOKASIDE_LIST LdkpThreadpoolWorkLookaside;



TP_POOL LdkpDefaultThreadPool;

TP_CALLBACK_ENVIRON LdkpDefaultCallbackEnviron;



#define TAG_WORK_ITEM		'iWdL'

typedef struct _LDK_THREAD_POOL_WORK_ITEM {
	WORK_QUEUE_ITEM WorkItem;
	PTP_WORK Work;
} LDK_THREAD_POOL_WORK_ITEM, *PLDK_THREAD_POOL_WORK_ITEM;

NPAGED_LOOKASIDE_LIST LdkpThreadpoolWorkQueItemLookaside;



NTSTATUS
LdkpInitializeThreadpoolApiset (
	VOID
	);

VOID
LdkpTerminateThreadpoolApiset (
	VOID
	);

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(WORKER_THREAD_ROUTINE)
VOID
LdkpThreadpoolWokerThreadRoutine (
    _In_ PLDK_THREAD_POOL_WORK_ITEM Item
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeThreadpoolApiset)
#pragma alloc_text(PAGE, LdkpTerminateThreadpoolApiset)
#pragma alloc_text(PAGE, WaitForThreadpoolWorkCallbacks)
#endif



NTSTATUS
LdkpInitializeThreadpoolApiset (
	VOID
	)
{
	PAGED_CODE();

	TpInitializeCallbackEnviron( &LdkpDefaultCallbackEnviron );

	TpSetCallbackThreadpool( &LdkpDefaultCallbackEnviron,
							 &LdkpDefaultThreadPool );

	TpSetCallbackNoActivationContext( &LdkpDefaultCallbackEnviron );

	ExInitializeNPagedLookasideList( &LdkpThreadpoolWorkLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(TP_WORK),
									 TAG_TP_WORK,
									 0 );

	ExInitializeNPagedLookasideList( &LdkpThreadpoolWorkQueItemLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_THREAD_POOL_WORK_ITEM),
									 TAG_WORK_ITEM,
									 0 );

	ExInitializeNPagedLookasideList( &LdkpFreeModuleEntryLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_FREE_MODULE_ENTRY),
									 TAG_FREE_MODULE_ENTRY,
									 0 );
	return STATUS_SUCCESS;
}

VOID
LdkpTerminateThreadpoolApiset (
	VOID
	)
{
	PAGED_CODE();

	ExDeleteNPagedLookasideList( &LdkpThreadpoolWorkLookaside );
	ExDeleteNPagedLookasideList( &LdkpThreadpoolWorkQueItemLookaside );
	ExDeleteNPagedLookasideList( &LdkpFreeModuleEntryLookaside );
}



WINBASEAPI
_Must_inspect_result_
PTP_WORK
WINAPI
CreateThreadpoolWork (
    _In_ PTP_WORK_CALLBACK pfnwk,
    _Inout_opt_ PVOID pv,
    _In_opt_ PTP_CALLBACK_ENVIRON pcbe
    )
{
	if (ARGUMENT_PRESENT(pcbe)) {
		KdBreakPoint();
		LdkSetLastNTError(STATUS_NOT_SUPPORTED);
		return NULL;
	}

	PTP_WORK work = ExAllocateFromNPagedLookasideList( &LdkpThreadpoolWorkLookaside );
	if (!work) {
		LdkSetLastNTError(STATUS_INSUFFICIENT_RESOURCES);
		return NULL;
	}

	KeInitializeEvent( &work->Event,
					   NotificationEvent,
					   FALSE );

	work->ReferenceCount = 1;
	work->Flags = 0;
	work->WorkCallback = pfnwk;
	work->CallbackParameter = pv;
	work->CallbackEnvironment = pcbe ? pcbe : &LdkpDefaultCallbackEnviron;
	return work;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(WORKER_THREAD_ROUTINE)
VOID
LdkpThreadpoolWokerThreadRoutine (
    _In_ PLDK_THREAD_POOL_WORK_ITEM Item
    )
{
	PTP_WORK Work = Item->Work;

	if (FlagOn(Work->Flags, LDK_TP_WORK_FLAGS_CANCEL_REQUESTED)) {
		InterlockedDecrement( &Work->ReferenceCount );
		return;
	}

	TP_CALLBACK_INSTANCE Instance;
	Instance.ModuleList.Next = NULL;
	KeInitializeSpinLock( &Instance.Lock );
	Instance.DeletePending = FALSE;

	Work->WorkCallback( &Instance,
						Work->CallbackParameter,
						Work );

	KeSetEvent( &Work->Event,
				IO_NO_INCREMENT,
				FALSE );

	KIRQL OldIrql;
	KeAcquireSpinLock( &Instance.Lock,
					   &OldIrql );
	Instance.DeletePending = TRUE;
	PSINGLE_LIST_ENTRY Entry = PopEntryList( &Instance.ModuleList );
	while (Entry) {
		PLDK_FREE_MODULE_ENTRY module = CONTAINING_RECORD(Entry, LDK_FREE_MODULE_ENTRY, Links);
		FreeLibrary( module->hModule );
		ExFreeToNPagedLookasideList( &LdkpFreeModuleEntryLookaside,
									 Entry );
		Entry = PopEntryList( &Instance.ModuleList );
	}
	KeReleaseSpinLock( &Instance.Lock,
					   OldIrql );

	InterlockedDecrement( &Work->ReferenceCount );

	if (FlagOn(Work->Flags, LDK_TP_WORK_FLAGS_DELETE_REQUESTED)) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolWorkLookaside,
									 Work );
	}

	ExFreeToNPagedLookasideList( &LdkpThreadpoolWorkQueItemLookaside,
								 Item );
}

WINBASEAPI
VOID
WINAPI
SubmitThreadpoolWork (
    _Inout_ PTP_WORK pwk
    )
{
	if (InterlockedIncrement( &pwk->ReferenceCount ) <= 1) {
		KdBreakPoint();
		LdkSetLastNTError( STATUS_DELETE_PENDING );
		return;
	}

	if (InterlockedBitTestAndSet( &pwk->Flags,
								  LDK_TP_WORK_FLAGS_STARTED_BIT_INDEX )) {
		KdBreakPoint();
		LdkSetLastNTError(STATUS_ALREADY_REGISTERED);
		InterlockedDecrement( &pwk->ReferenceCount );
		return;
	}

	PLDK_THREAD_POOL_WORK_ITEM item = ExAllocateFromNPagedLookasideList( &LdkpThreadpoolWorkQueItemLookaside );
	if (!item) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		InterlockedDecrement( &pwk->ReferenceCount );
		return;
	}

	item->Work = pwk;

#pragma warning(disable: 4996)
    ExInitializeWorkItem( &item->WorkItem,
                          LdkpThreadpoolWokerThreadRoutine,
                          item );

    ExQueueWorkItem( &item->WorkItem,
                     DelayedWorkQueue );
#pragma warning(default: 4996)
}

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolWorkCallbacks (
    _Inout_ PTP_WORK pwk,
    _In_ BOOL fCancelPendingCallbacks
    )
{
	PAGED_CODE();

	if (InterlockedIncrement( &pwk->ReferenceCount ) <= 1) {
		KdBreakPoint();
		LdkSetLastNTError( STATUS_DELETE_PENDING );
		return;
	}

	if (fCancelPendingCallbacks && (! FlagOn(pwk->Flags, LDK_TP_WORK_FLAGS_STARTED))) {
		InterlockedOr( &pwk->Flags,
					   LDK_TP_WORK_FLAGS_CANCEL_REQUESTED );
		ASSERT(InterlockedDecrement( &pwk->ReferenceCount ) > 0);
		return;
	}

	NTSTATUS Status = KeWaitForSingleObject( &pwk->Event,
											 Executive,
											 KernelMode,
											 FALSE,
											 NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
	}
	ASSERT(InterlockedDecrement( &pwk->ReferenceCount ) > 0);
}

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWork (
    _Inout_ PTP_WORK pwk
    )
{
	if (InterlockedDecrement( &pwk->ReferenceCount ) == 0) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolWorkLookaside,
									 pwk );
	} else {
		InterlockedOr( &pwk->Flags, LDK_TP_WORK_FLAGS_DELETE_REQUESTED );
	}
}

WINBASEAPI
VOID
WINAPI
FreeLibraryWhenCallbackReturns (
    _Inout_ PTP_CALLBACK_INSTANCE pci,
    _In_ HMODULE mod
    )
{
	KIRQL OldIrql;
	KeAcquireSpinLock( &pci->Lock,
					   &OldIrql );

	if (pci->DeletePending) {
		KeReleaseSpinLock( &pci->Lock,
						   OldIrql );
		return;
	}
	PLDK_FREE_MODULE_ENTRY Module = ExAllocateFromNPagedLookasideList( &LdkpFreeModuleEntryLookaside );
	if (Module) {
		Module->hModule = mod;
		PushEntryList( &pci->ModuleList,
					   &Module->Links );
	}
	KeReleaseSpinLock( &pci->Lock,
					   OldIrql );
}