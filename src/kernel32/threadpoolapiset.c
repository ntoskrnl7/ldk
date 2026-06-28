#include "winbase.h"

#include <Ldk/ntdll/ntldr.h>

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
	LONG Flags;
	LONG OutstandingCount;

	KEVENT Event;

    PVOID CallbackParameter;
    PTP_WORK_CALLBACK WorkCallback;
    TP_CALLBACK_ENVIRON CallbackEnvironment;
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

NTSTATUS
LdkpCaptureThreadpoolCallbackEnvironment (
	_In_opt_ PTP_CALLBACK_ENVIRON Source,
	_Out_ PTP_CALLBACK_ENVIRON Destination
	);

VOID
LdkpDereferenceThreadpoolWork (
	_Inout_ PTP_WORK Work
	);

VOID
LdkpCompleteThreadpoolWorkItem (
	_Inout_ PTP_WORK Work
	);



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeThreadpoolApiset)
#pragma alloc_text(PAGE, LdkpTerminateThreadpoolApiset)
#pragma alloc_text(PAGE, LdkpCaptureThreadpoolCallbackEnvironment)
#pragma alloc_text(PAGE, CreateThreadpoolWork)
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

NTSTATUS
LdkpCaptureThreadpoolCallbackEnvironment (
	_In_opt_ PTP_CALLBACK_ENVIRON Source,
	_Out_ PTP_CALLBACK_ENVIRON Destination
	)
{
	PAGED_CODE();

	if (! ARGUMENT_PRESENT(Source)) {
		*Destination = LdkpDefaultCallbackEnviron;
		return STATUS_SUCCESS;
	}

	if (Source->Version != 1 &&
		Source->Version != 3) {
		return STATUS_INVALID_PARAMETER;
	}

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
	if (Source->Version == 3 &&
		Source->Size != sizeof(TP_CALLBACK_ENVIRON)) {
		return STATUS_INVALID_PARAMETER;
	}

	if (Source->Version == 3 &&
		Source->CallbackPriority >= TP_CALLBACK_PRIORITY_INVALID) {
		return STATUS_INVALID_PARAMETER;
	}
#endif

	if ((Source->Pool != NULL &&
		 Source->Pool != &LdkpDefaultThreadPool) ||
		Source->CleanupGroup != NULL ||
		Source->CleanupGroupCancelCallback != NULL ||
		Source->FinalizationCallback != NULL) {
		return STATUS_NOT_SUPPORTED;
	}

	if (Source->ActivationContext != NULL &&
		Source->ActivationContext != (struct _ACTIVATION_CONTEXT *)(LONG_PTR)-1) {
		return STATUS_NOT_SUPPORTED;
	}

	*Destination = *Source;
	if (Destination->Pool == NULL) {
		Destination->Pool = &LdkpDefaultThreadPool;
	}
	return STATUS_SUCCESS;
}

VOID
LdkpDereferenceThreadpoolWork (
	_Inout_ PTP_WORK Work
	)
{
	if (InterlockedDecrement( &Work->ReferenceCount ) == 0) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolWorkLookaside,
									 Work );
	}
}

VOID
LdkpCompleteThreadpoolWorkItem (
	_Inout_ PTP_WORK Work
	)
{
	if (InterlockedDecrement( &Work->OutstandingCount ) == 0) {
		InterlockedAnd( &Work->Flags,
						~LDK_TP_WORK_FLAGS_CANCEL_REQUESTED );
		KeSetEvent( &Work->Event,
					IO_NO_INCREMENT,
					FALSE );
	}

	LdkpDereferenceThreadpoolWork( Work );
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
	NTSTATUS Status;

	PAGED_CODE();

	if (! ARGUMENT_PRESENT(pfnwk)) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	PTP_WORK work = ExAllocateFromNPagedLookasideList( &LdkpThreadpoolWorkLookaside );
	if (!work) {
		LdkSetLastNTError(STATUS_INSUFFICIENT_RESOURCES);
		return NULL;
	}

	Status = LdkpCaptureThreadpoolCallbackEnvironment( pcbe,
													   &work->CallbackEnvironment );
	if (! NT_SUCCESS(Status)) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolWorkLookaside,
									 work );
		LdkSetLastNTError( Status );
		return NULL;
	}

	KeInitializeEvent( &work->Event,
					   NotificationEvent,
					   TRUE );

	work->ReferenceCount = 1;
	work->Flags = 0;
	work->OutstandingCount = 0;
	work->WorkCallback = pfnwk;
	work->CallbackParameter = pv;
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
	SINGLE_LIST_ENTRY ModuleList;
	BOOLEAN InvokeCallback;
	HMODULE RaceDll;
	BOOLEAN ReleaseRaceDll;
	NTSTATUS Status;

	InvokeCallback = ! FlagOn( InterlockedCompareExchange( &Work->Flags,
															0,
															0 ),
							   LDK_TP_WORK_FLAGS_CANCEL_REQUESTED );

	TP_CALLBACK_INSTANCE Instance;
	Instance.ModuleList.Next = NULL;
	KeInitializeSpinLock( &Instance.Lock );
	Instance.DeletePending = FALSE;

	ReleaseRaceDll = FALSE;
	RaceDll = (HMODULE)Work->CallbackEnvironment.RaceDll;
	if (InvokeCallback &&
		RaceDll != NULL) {
		Status = LdrAddRefDll( 0,
							   RaceDll );
		ReleaseRaceDll = NT_SUCCESS(Status);
	}

	if (InvokeCallback) {
		Work->WorkCallback( &Instance,
							Work->CallbackParameter,
							Work );
	}

	KIRQL OldIrql;
	KeAcquireSpinLock( &Instance.Lock,
					   &OldIrql );
	Instance.DeletePending = TRUE;
	ModuleList.Next = Instance.ModuleList.Next;
	Instance.ModuleList.Next = NULL;
	KeReleaseSpinLock( &Instance.Lock,
					   OldIrql );

	PSINGLE_LIST_ENTRY Entry = ModuleList.Next;
	while (Entry) {
		PSINGLE_LIST_ENTRY NextEntry = Entry->Next;
		PLDK_FREE_MODULE_ENTRY module = CONTAINING_RECORD(Entry, LDK_FREE_MODULE_ENTRY, Links);
		FreeLibrary( module->hModule );
		ExFreeToNPagedLookasideList( &LdkpFreeModuleEntryLookaside,
									 Entry );
		Entry = NextEntry;
	}

	if (ReleaseRaceDll) {
		FreeLibrary( RaceDll );
	}

	ExFreeToNPagedLookasideList( &LdkpThreadpoolWorkQueItemLookaside,
								 Item );
	LdkpCompleteThreadpoolWorkItem( Work );
}

WINBASEAPI
VOID
WINAPI
SubmitThreadpoolWork (
    _Inout_ PTP_WORK pwk
    )
{
	if (FlagOn( InterlockedCompareExchange( &pwk->Flags,
											0,
											0 ),
				LDK_TP_WORK_FLAGS_DELETE_REQUESTED )) {
		LdkSetLastNTError( STATUS_DELETE_PENDING );
		return;
	}

	InterlockedIncrement( &pwk->ReferenceCount );
	InterlockedIncrement( &pwk->OutstandingCount );
	KeResetEvent( &pwk->Event );

	PLDK_THREAD_POOL_WORK_ITEM item = ExAllocateFromNPagedLookasideList( &LdkpThreadpoolWorkQueItemLookaside );
	if (!item) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		if (InterlockedDecrement( &pwk->OutstandingCount ) == 0) {
			KeSetEvent( &pwk->Event,
						IO_NO_INCREMENT,
						FALSE );
		}
		LdkpDereferenceThreadpoolWork( pwk );
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
		LdkSetLastNTError( STATUS_DELETE_PENDING );
		return;
	}

	if (fCancelPendingCallbacks) {
		InterlockedOr( &pwk->Flags,
					   LDK_TP_WORK_FLAGS_CANCEL_REQUESTED );
	}

	NTSTATUS Status = KeWaitForSingleObject( &pwk->Event,
											 Executive,
											 KernelMode,
											 FALSE,
											 NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
	}
	if (fCancelPendingCallbacks) {
		InterlockedAnd( &pwk->Flags,
						~LDK_TP_WORK_FLAGS_CANCEL_REQUESTED );
	}
	LdkpDereferenceThreadpoolWork( pwk );
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
	PLDK_FREE_MODULE_ENTRY Module;
	KIRQL OldIrql;

	Module = ExAllocateFromNPagedLookasideList( &LdkpFreeModuleEntryLookaside );
	if (! Module) {
		return;
	}
	Module->hModule = mod;

	KeAcquireSpinLock( &pci->Lock,
					   &OldIrql );

	if (pci->DeletePending) {
		KeReleaseSpinLock( &pci->Lock,
						   OldIrql );
		ExFreeToNPagedLookasideList( &LdkpFreeModuleEntryLookaside,
									 Module );
		return;
	}
	PushEntryList( &pci->ModuleList,
				   &Module->Links );
	KeReleaseSpinLock( &pci->Lock,
					   OldIrql );
}
