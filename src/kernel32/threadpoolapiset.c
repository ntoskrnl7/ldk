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

typedef struct _TP_CLEANUP_GROUP
{
	KSPIN_LOCK Lock;
	LIST_ENTRY WorkList;
	LONG Flags;
} TP_CLEANUP_GROUP, *PTP_CLEANUP_GROUP;

#define LDK_TP_CLEANUP_GROUP_FLAGS_CLOSED		0x00000001

#define TAG_TP_CLEANUP_GROUP	'pCgT'
NPAGED_LOOKASIDE_LIST LdkpThreadpoolCleanupGroupLookaside;


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
	LIST_ENTRY CleanupGroupLinks;
	PTP_CLEANUP_GROUP CleanupGroup;
	PTP_CLEANUP_GROUP_CANCEL_CALLBACK CleanupGroupCancelCallback;

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

NTSTATUS
LdkpRegisterThreadpoolCleanupGroupWork (
	_Inout_ PTP_WORK Work
	);

VOID
LdkpUnregisterThreadpoolCleanupGroupWork (
	_Inout_ PTP_WORK Work
	);



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeThreadpoolApiset)
#pragma alloc_text(PAGE, LdkpTerminateThreadpoolApiset)
#pragma alloc_text(PAGE, LdkpCaptureThreadpoolCallbackEnvironment)
#pragma alloc_text(PAGE, LdkpRegisterThreadpoolCleanupGroupWork)
#pragma alloc_text(PAGE, LdkpUnregisterThreadpoolCleanupGroupWork)
#pragma alloc_text(PAGE, CreateThreadpoolCleanupGroup)
#pragma alloc_text(PAGE, CloseThreadpoolCleanupGroup)
#pragma alloc_text(PAGE, CloseThreadpoolCleanupGroupMembers)
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

	ExInitializeNPagedLookasideList( &LdkpThreadpoolCleanupGroupLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(TP_CLEANUP_GROUP),
									 TAG_TP_CLEANUP_GROUP,
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
	ExDeleteNPagedLookasideList( &LdkpThreadpoolCleanupGroupLookaside );
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
		Source->FinalizationCallback != NULL) {
		return STATUS_NOT_SUPPORTED;
	}

	if (Source->CleanupGroup == NULL &&
		Source->CleanupGroupCancelCallback != NULL) {
		return STATUS_INVALID_PARAMETER;
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

NTSTATUS
LdkpRegisterThreadpoolCleanupGroupWork (
	_Inout_ PTP_WORK Work
	)
{
	PTP_CLEANUP_GROUP CleanupGroup;
	KIRQL OldIrql;

	PAGED_CODE();

	CleanupGroup = Work->CallbackEnvironment.CleanupGroup;
	if (CleanupGroup == NULL) {
		return STATUS_SUCCESS;
	}

	KeAcquireSpinLock( &CleanupGroup->Lock,
					   &OldIrql );
	if (FlagOn( CleanupGroup->Flags,
				LDK_TP_CLEANUP_GROUP_FLAGS_CLOSED )) {
		KeReleaseSpinLock( &CleanupGroup->Lock,
						   OldIrql );
		return STATUS_DELETE_PENDING;
	}

	Work->CleanupGroup = CleanupGroup;
	Work->CleanupGroupCancelCallback = Work->CallbackEnvironment.CleanupGroupCancelCallback;
	InsertTailList( &CleanupGroup->WorkList,
					&Work->CleanupGroupLinks );
	KeReleaseSpinLock( &CleanupGroup->Lock,
					   OldIrql );
	return STATUS_SUCCESS;
}

VOID
LdkpUnregisterThreadpoolCleanupGroupWork (
	_Inout_ PTP_WORK Work
	)
{
	PTP_CLEANUP_GROUP CleanupGroup;
	KIRQL OldIrql;

	PAGED_CODE();

	CleanupGroup = Work->CleanupGroup;
	if (CleanupGroup == NULL) {
		return;
	}

	KeAcquireSpinLock( &CleanupGroup->Lock,
					   &OldIrql );
	if (Work->CleanupGroup == CleanupGroup) {
		RemoveEntryList( &Work->CleanupGroupLinks );
		InitializeListHead( &Work->CleanupGroupLinks );
		Work->CleanupGroup = NULL;
		Work->CleanupGroupCancelCallback = NULL;
	}
	KeReleaseSpinLock( &CleanupGroup->Lock,
					   OldIrql );
}



WINBASEAPI
PTP_CLEANUP_GROUP
WINAPI
CreateThreadpoolCleanupGroup (
	VOID
	)
{
	PTP_CLEANUP_GROUP CleanupGroup;

	PAGED_CODE();

	CleanupGroup = ExAllocateFromNPagedLookasideList( &LdkpThreadpoolCleanupGroupLookaside );
	if (CleanupGroup == NULL) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}

	KeInitializeSpinLock( &CleanupGroup->Lock );
	InitializeListHead( &CleanupGroup->WorkList );
	CleanupGroup->Flags = 0;
	return CleanupGroup;
}

WINBASEAPI
VOID
WINAPI
CloseThreadpoolCleanupGroupMembers (
	_Inout_ PTP_CLEANUP_GROUP ptpcg,
	_In_ BOOL fCancelPendingCallbacks,
	_Inout_opt_ PVOID pvCleanupContext
	)
{
	LIST_ENTRY WorkList;
	KIRQL OldIrql;

	PAGED_CODE();

	InitializeListHead( &WorkList );

	KeAcquireSpinLock( &ptpcg->Lock,
					   &OldIrql );
	while (! IsListEmpty( &ptpcg->WorkList )) {
		PLIST_ENTRY Entry = RemoveHeadList( &ptpcg->WorkList );
		PTP_WORK Work = CONTAINING_RECORD(Entry,
										   TP_WORK,
										   CleanupGroupLinks);

		Work->CleanupGroup = NULL;
		InsertTailList( &WorkList,
						&Work->CleanupGroupLinks );
	}
	KeReleaseSpinLock( &ptpcg->Lock,
					   OldIrql );

	while (! IsListEmpty( &WorkList )) {
		PLIST_ENTRY Entry = RemoveHeadList( &WorkList );
		PTP_WORK Work = CONTAINING_RECORD(Entry,
										   TP_WORK,
										   CleanupGroupLinks);
		PTP_CLEANUP_GROUP_CANCEL_CALLBACK CancelCallback;
		PVOID ObjectContext;

		InitializeListHead( &Work->CleanupGroupLinks );
		InterlockedOr( &Work->Flags,
					   LDK_TP_WORK_FLAGS_DELETE_REQUESTED );

		WaitForThreadpoolWorkCallbacks( Work,
										fCancelPendingCallbacks );

		CancelCallback = Work->CleanupGroupCancelCallback;
		ObjectContext = Work->CallbackParameter;
		Work->CleanupGroupCancelCallback = NULL;
		if (CancelCallback != NULL) {
			CancelCallback( ObjectContext,
							pvCleanupContext );
		}

		LdkpDereferenceThreadpoolWork( Work );
	}
}

WINBASEAPI
VOID
WINAPI
CloseThreadpoolCleanupGroup (
	_Inout_ PTP_CLEANUP_GROUP ptpcg
	)
{
	KIRQL OldIrql;

	PAGED_CODE();

	KeAcquireSpinLock( &ptpcg->Lock,
					   &OldIrql );
	ptpcg->Flags |= LDK_TP_CLEANUP_GROUP_FLAGS_CLOSED;
	while (! IsListEmpty( &ptpcg->WorkList )) {
		PLIST_ENTRY Entry = RemoveHeadList( &ptpcg->WorkList );
		PTP_WORK Work = CONTAINING_RECORD(Entry,
										   TP_WORK,
										   CleanupGroupLinks);
		Work->CleanupGroup = NULL;
		Work->CleanupGroupCancelCallback = NULL;
		InitializeListHead( &Work->CleanupGroupLinks );
	}
	KeReleaseSpinLock( &ptpcg->Lock,
					   OldIrql );

	ExFreeToNPagedLookasideList( &LdkpThreadpoolCleanupGroupLookaside,
								 ptpcg );
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
	InitializeListHead( &work->CleanupGroupLinks );

	work->ReferenceCount = 1;
	work->Flags = 0;
	work->OutstandingCount = 0;
	work->CleanupGroup = NULL;
	work->CleanupGroupCancelCallback = NULL;
	work->WorkCallback = pfnwk;
	work->CallbackParameter = pv;

	Status = LdkpRegisterThreadpoolCleanupGroupWork( work );
	if (! NT_SUCCESS(Status)) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolWorkLookaside,
									 work );
		LdkSetLastNTError( Status );
		return NULL;
	}

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
	LdkpUnregisterThreadpoolCleanupGroupWork( pwk );
	InterlockedOr( &pwk->Flags,
				   LDK_TP_WORK_FLAGS_DELETE_REQUESTED );
	LdkpDereferenceThreadpoolWork( pwk );
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
