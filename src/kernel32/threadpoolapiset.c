#include "winbase.h"

#include <Ldk/ntdll/ntldr.h>

#ifndef THREAD_WAIT_OBJECTS
#define THREAD_WAIT_OBJECTS 3
#endif

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
	LIST_ENTRY ObjectList;
	LONG Flags;
} TP_CLEANUP_GROUP, *PTP_CLEANUP_GROUP;

#define LDK_TP_CLEANUP_GROUP_FLAGS_CLOSED		0x00000001

#define TAG_TP_CLEANUP_GROUP	'pCgT'
NPAGED_LOOKASIDE_LIST LdkpThreadpoolCleanupGroupLookaside;


typedef enum _LDK_TP_CLEANUP_GROUP_OBJECT_TYPE
{
	LdkTpCleanupGroupObjectWork,
	LdkTpCleanupGroupObjectTimer,
	LdkTpCleanupGroupObjectWait
} LDK_TP_CLEANUP_GROUP_OBJECT_TYPE;

typedef struct _LDK_TP_CLEANUP_GROUP_MEMBER
{
	LIST_ENTRY Links;
	PTP_CLEANUP_GROUP CleanupGroup;
	PTP_CLEANUP_GROUP_CANCEL_CALLBACK CleanupGroupCancelCallback;
	PVOID ObjectContext;
	LDK_TP_CLEANUP_GROUP_OBJECT_TYPE ObjectType;
	PVOID Object;
} LDK_TP_CLEANUP_GROUP_MEMBER, *PLDK_TP_CLEANUP_GROUP_MEMBER;


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
	LDK_TP_CLEANUP_GROUP_MEMBER CleanupGroupMember;

    PVOID CallbackParameter;
    PTP_WORK_CALLBACK WorkCallback;
    TP_CALLBACK_ENVIRON CallbackEnvironment;
} TP_WORK, *PTP_WORK;

NPAGED_LOOKASIDE_LIST LdkpThreadpoolWorkLookaside;


#define TAG_TP_TIMER	'rTpT'

typedef struct _TP_TIMER
{
	LONG ReferenceCount;

#define LDK_TP_TIMER_FLAGS_DELETE_REQUESTED		0x00000001
#define LDK_TP_TIMER_FLAGS_CANCEL_REQUESTED		0x00000002
#define LDK_TP_TIMER_FLAGS_SET					0x00000004
#define LDK_TP_TIMER_FLAGS_EXPIRED				0x00000008
	LONG Flags;
	LONG OutstandingCount;

	KEVENT Event;
	KEVENT ControlEvent;
	KSPIN_LOCK Lock;
	HANDLE ThreadHandle;
	HANDLE ThreadId;
	LARGE_INTEGER DueTime;
	DWORD Period;
	DWORD WindowLength;

	LDK_TP_CLEANUP_GROUP_MEMBER CleanupGroupMember;
	PVOID CallbackParameter;
	PTP_TIMER_CALLBACK TimerCallback;
	TP_CALLBACK_ENVIRON CallbackEnvironment;
} TP_TIMER, *PTP_TIMER;

NPAGED_LOOKASIDE_LIST LdkpThreadpoolTimerLookaside;


#define TAG_TP_WAIT		'tWpT'

typedef struct _TP_WAIT
{
	LONG ReferenceCount;

#define LDK_TP_WAIT_FLAGS_DELETE_REQUESTED		0x00000001
#define LDK_TP_WAIT_FLAGS_CANCEL_REQUESTED		0x00000002
#define LDK_TP_WAIT_FLAGS_SET					0x00000004
	LONG Flags;
	LONG OutstandingCount;

	KEVENT Event;
	KEVENT ControlEvent;
	KSPIN_LOCK Lock;
	HANDLE ThreadHandle;
	HANDLE ThreadId;
	PVOID WaitObject;
	LARGE_INTEGER Timeout;
	BOOLEAN HasTimeout;

	LDK_TP_CLEANUP_GROUP_MEMBER CleanupGroupMember;
	PVOID CallbackParameter;
	PTP_WAIT_CALLBACK WaitCallback;
	TP_CALLBACK_ENVIRON CallbackEnvironment;
} TP_WAIT, *PTP_WAIT;

NPAGED_LOOKASIDE_LIST LdkpThreadpoolWaitLookaside;



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

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
VOID
LdkpThreadpoolTimerThreadRoutine (
	_In_ PTP_TIMER Timer
	);

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
VOID
LdkpThreadpoolWaitThreadRoutine (
	_In_ PTP_WAIT Wait
	);

NTSTATUS
LdkpCaptureThreadpoolCallbackEnvironment (
	_In_opt_ PTP_CALLBACK_ENVIRON Source,
	_Out_ PTP_CALLBACK_ENVIRON Destination
	);

BOOLEAN
LdkpBeginThreadpoolCallback (
	_In_ PTP_CALLBACK_ENVIRON CallbackEnvironment,
	_Out_ PTP_CALLBACK_INSTANCE Instance,
	_Out_ HMODULE *RaceDll
	);

VOID
LdkpEndThreadpoolCallback (
	_Inout_ PTP_CALLBACK_INSTANCE Instance,
	_In_opt_ HMODULE RaceDll,
	_In_ BOOLEAN ReleaseRaceDll
	);

VOID
LdkpDereferenceThreadpoolWork (
	_Inout_ PTP_WORK Work
	);

VOID
LdkpCompleteThreadpoolWorkItem (
	_Inout_ PTP_WORK Work
	);

VOID
LdkpDereferenceThreadpoolTimer (
	_Inout_ PTP_TIMER Timer
	);

VOID
LdkpCompleteThreadpoolTimerCallback (
	_Inout_ PTP_TIMER Timer
	);

VOID
LdkpCloseThreadpoolTimerInternal (
	_Inout_ PTP_TIMER Timer
	);

VOID
LdkpDereferenceThreadpoolWait (
	_Inout_ PTP_WAIT Wait
	);

VOID
LdkpCompleteThreadpoolWaitCallback (
	_Inout_ PTP_WAIT Wait
	);

VOID
LdkpCloseThreadpoolWaitInternal (
	_Inout_ PTP_WAIT Wait
	);

NTSTATUS
LdkpRegisterThreadpoolCleanupGroupMember (
	_In_ PTP_CALLBACK_ENVIRON CallbackEnvironment,
	_In_ LDK_TP_CLEANUP_GROUP_OBJECT_TYPE ObjectType,
	_Inout_ PVOID Object,
	_In_opt_ PVOID ObjectContext,
	_Inout_ PLDK_TP_CLEANUP_GROUP_MEMBER Member
	);

VOID
LdkpUnregisterThreadpoolCleanupGroupMember (
	_Inout_ PLDK_TP_CLEANUP_GROUP_MEMBER Member
	);



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeThreadpoolApiset)
#pragma alloc_text(PAGE, LdkpTerminateThreadpoolApiset)
#pragma alloc_text(PAGE, LdkpCaptureThreadpoolCallbackEnvironment)
#pragma alloc_text(PAGE, LdkpRegisterThreadpoolCleanupGroupMember)
#pragma alloc_text(PAGE, LdkpUnregisterThreadpoolCleanupGroupMember)
#pragma alloc_text(PAGE, CreateThreadpoolCleanupGroup)
#pragma alloc_text(PAGE, CloseThreadpoolCleanupGroup)
#pragma alloc_text(PAGE, CloseThreadpoolCleanupGroupMembers)
#pragma alloc_text(PAGE, CreateThreadpoolWork)
#pragma alloc_text(PAGE, WaitForThreadpoolWorkCallbacks)
#pragma alloc_text(PAGE, CreateThreadpoolTimer)
#pragma alloc_text(PAGE, SetThreadpoolTimer)
#pragma alloc_text(PAGE, IsThreadpoolTimerSet)
#pragma alloc_text(PAGE, WaitForThreadpoolTimerCallbacks)
#pragma alloc_text(PAGE, CloseThreadpoolTimer)
#pragma alloc_text(PAGE, CreateThreadpoolWait)
#pragma alloc_text(PAGE, SetThreadpoolWait)
#pragma alloc_text(PAGE, WaitForThreadpoolWaitCallbacks)
#pragma alloc_text(PAGE, CloseThreadpoolWait)
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

	ExInitializeNPagedLookasideList( &LdkpThreadpoolTimerLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(TP_TIMER),
									 TAG_TP_TIMER,
									 0 );

	ExInitializeNPagedLookasideList( &LdkpThreadpoolWaitLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(TP_WAIT),
									 TAG_TP_WAIT,
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
	ExDeleteNPagedLookasideList( &LdkpThreadpoolTimerLookaside );
	ExDeleteNPagedLookasideList( &LdkpThreadpoolWaitLookaside );
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

BOOLEAN
LdkpBeginThreadpoolCallback (
	_In_ PTP_CALLBACK_ENVIRON CallbackEnvironment,
	_Out_ PTP_CALLBACK_INSTANCE Instance,
	_Out_ HMODULE *RaceDll
	)
{
	BOOLEAN ReleaseRaceDll;
	NTSTATUS Status;

	Instance->ModuleList.Next = NULL;
	KeInitializeSpinLock( &Instance->Lock );
	Instance->DeletePending = FALSE;

	ReleaseRaceDll = FALSE;
	*RaceDll = (HMODULE)CallbackEnvironment->RaceDll;
	if (*RaceDll != NULL) {
		Status = LdrAddRefDll( 0,
							   *RaceDll );
		ReleaseRaceDll = NT_SUCCESS(Status);
	}

	return ReleaseRaceDll;
}

VOID
LdkpEndThreadpoolCallback (
	_Inout_ PTP_CALLBACK_INSTANCE Instance,
	_In_opt_ HMODULE RaceDll,
	_In_ BOOLEAN ReleaseRaceDll
	)
{
	SINGLE_LIST_ENTRY ModuleList;
	PSINGLE_LIST_ENTRY Entry;
	KIRQL OldIrql;

	KeAcquireSpinLock( &Instance->Lock,
					   &OldIrql );
	Instance->DeletePending = TRUE;
	ModuleList.Next = Instance->ModuleList.Next;
	Instance->ModuleList.Next = NULL;
	KeReleaseSpinLock( &Instance->Lock,
					   OldIrql );

	Entry = ModuleList.Next;
	while (Entry != NULL) {
		PSINGLE_LIST_ENTRY NextEntry = Entry->Next;
		PLDK_FREE_MODULE_ENTRY Module = CONTAINING_RECORD(Entry,
														   LDK_FREE_MODULE_ENTRY,
														   Links);
		FreeLibrary( Module->hModule );
		ExFreeToNPagedLookasideList( &LdkpFreeModuleEntryLookaside,
									 Module );
		Entry = NextEntry;
	}

	if (ReleaseRaceDll) {
		FreeLibrary( RaceDll );
	}
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

VOID
LdkpDereferenceThreadpoolTimer (
	_Inout_ PTP_TIMER Timer
	)
{
	if (InterlockedDecrement( &Timer->ReferenceCount ) == 0) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolTimerLookaside,
									 Timer );
	}
}

VOID
LdkpCompleteThreadpoolTimerCallback (
	_Inout_ PTP_TIMER Timer
	)
{
	if (InterlockedDecrement( &Timer->OutstandingCount ) == 0) {
		InterlockedAnd( &Timer->Flags,
						~LDK_TP_TIMER_FLAGS_CANCEL_REQUESTED );
		KeSetEvent( &Timer->Event,
					IO_NO_INCREMENT,
					FALSE );
	}

	LdkpDereferenceThreadpoolTimer( Timer );
}

VOID
LdkpCloseThreadpoolTimerInternal (
	_Inout_ PTP_TIMER Timer
	)
{
	LONG PreviousFlags;
	HANDLE ThreadHandle;

	PreviousFlags = InterlockedOr( &Timer->Flags,
								   LDK_TP_TIMER_FLAGS_DELETE_REQUESTED );
	KeSetEvent( &Timer->ControlEvent,
				IO_NO_INCREMENT,
				FALSE );

	ThreadHandle = Timer->ThreadHandle;
	if (! FlagOn( PreviousFlags,
				  LDK_TP_TIMER_FLAGS_DELETE_REQUESTED ) &&
		ThreadHandle != NULL &&
		Timer->ThreadId != PsGetCurrentThreadId()) {
		(VOID)ZwWaitForSingleObject( ThreadHandle,
									 FALSE,
									 NULL );
	}
	if (ThreadHandle != NULL) {
		Timer->ThreadHandle = NULL;
		(VOID)ZwClose( ThreadHandle );
	}
}

VOID
LdkpDereferenceThreadpoolWait (
	_Inout_ PTP_WAIT Wait
	)
{
	if (InterlockedDecrement( &Wait->ReferenceCount ) == 0) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolWaitLookaside,
									 Wait );
	}
}

VOID
LdkpCompleteThreadpoolWaitCallback (
	_Inout_ PTP_WAIT Wait
	)
{
	if (InterlockedDecrement( &Wait->OutstandingCount ) == 0) {
		InterlockedAnd( &Wait->Flags,
						~LDK_TP_WAIT_FLAGS_CANCEL_REQUESTED );
		KeSetEvent( &Wait->Event,
					IO_NO_INCREMENT,
					FALSE );
	}

	LdkpDereferenceThreadpoolWait( Wait );
}

VOID
LdkpCloseThreadpoolWaitInternal (
	_Inout_ PTP_WAIT Wait
	)
{
	LONG PreviousFlags;
	HANDLE ThreadHandle;
	PVOID OldWaitObject;
	KIRQL OldIrql;

	PreviousFlags = InterlockedOr( &Wait->Flags,
								   LDK_TP_WAIT_FLAGS_DELETE_REQUESTED );
	KeSetEvent( &Wait->ControlEvent,
				IO_NO_INCREMENT,
				FALSE );

	ThreadHandle = Wait->ThreadHandle;
	if (! FlagOn( PreviousFlags,
				  LDK_TP_WAIT_FLAGS_DELETE_REQUESTED ) &&
		ThreadHandle != NULL &&
		Wait->ThreadId != PsGetCurrentThreadId()) {
		(VOID)ZwWaitForSingleObject( ThreadHandle,
									 FALSE,
									 NULL );
	}
	if (ThreadHandle != NULL) {
		Wait->ThreadHandle = NULL;
		(VOID)ZwClose( ThreadHandle );
	}

	KeAcquireSpinLock( &Wait->Lock,
					   &OldIrql );
	OldWaitObject = Wait->WaitObject;
	Wait->WaitObject = NULL;
	ClearFlag( Wait->Flags,
			   LDK_TP_WAIT_FLAGS_SET );
	KeReleaseSpinLock( &Wait->Lock,
					   OldIrql );

	if (OldWaitObject != NULL) {
		ObDereferenceObject( OldWaitObject );
	}
}

NTSTATUS
LdkpRegisterThreadpoolCleanupGroupMember (
	_In_ PTP_CALLBACK_ENVIRON CallbackEnvironment,
	_In_ LDK_TP_CLEANUP_GROUP_OBJECT_TYPE ObjectType,
	_Inout_ PVOID Object,
	_In_opt_ PVOID ObjectContext,
	_Inout_ PLDK_TP_CLEANUP_GROUP_MEMBER Member
	)
{
	PTP_CLEANUP_GROUP CleanupGroup;
	KIRQL OldIrql;

	PAGED_CODE();

	CleanupGroup = CallbackEnvironment->CleanupGroup;
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

	Member->CleanupGroup = CleanupGroup;
	Member->CleanupGroupCancelCallback = CallbackEnvironment->CleanupGroupCancelCallback;
	Member->ObjectContext = ObjectContext;
	Member->ObjectType = ObjectType;
	Member->Object = Object;
	InsertTailList( &CleanupGroup->ObjectList,
					&Member->Links );
	KeReleaseSpinLock( &CleanupGroup->Lock,
					   OldIrql );
	return STATUS_SUCCESS;
}

VOID
LdkpUnregisterThreadpoolCleanupGroupMember (
	_Inout_ PLDK_TP_CLEANUP_GROUP_MEMBER Member
	)
{
	PTP_CLEANUP_GROUP CleanupGroup;
	KIRQL OldIrql;

	PAGED_CODE();

	CleanupGroup = Member->CleanupGroup;
	if (CleanupGroup == NULL) {
		return;
	}

	KeAcquireSpinLock( &CleanupGroup->Lock,
					   &OldIrql );
	if (Member->CleanupGroup == CleanupGroup) {
		RemoveEntryList( &Member->Links );
		InitializeListHead( &Member->Links );
		Member->CleanupGroup = NULL;
		Member->CleanupGroupCancelCallback = NULL;
		Member->ObjectContext = NULL;
		Member->Object = NULL;
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
	InitializeListHead( &CleanupGroup->ObjectList );
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
	while (! IsListEmpty( &ptpcg->ObjectList )) {
		PLIST_ENTRY Entry = RemoveHeadList( &ptpcg->ObjectList );
		PLDK_TP_CLEANUP_GROUP_MEMBER Member = CONTAINING_RECORD(Entry,
																 LDK_TP_CLEANUP_GROUP_MEMBER,
																 Links);

		Member->CleanupGroup = NULL;
		InsertTailList( &WorkList,
						&Member->Links );
	}
	KeReleaseSpinLock( &ptpcg->Lock,
					   OldIrql );

	while (! IsListEmpty( &WorkList )) {
		PLIST_ENTRY Entry = RemoveHeadList( &WorkList );
		PLDK_TP_CLEANUP_GROUP_MEMBER Member = CONTAINING_RECORD(Entry,
																 LDK_TP_CLEANUP_GROUP_MEMBER,
																 Links);
		PTP_CLEANUP_GROUP_CANCEL_CALLBACK CancelCallback;
		PVOID ObjectContext;

		InitializeListHead( &Member->Links );

		switch (Member->ObjectType) {
		case LdkTpCleanupGroupObjectWork:
		{
			PTP_WORK Work = (PTP_WORK)Member->Object;

			InterlockedOr( &Work->Flags,
						   LDK_TP_WORK_FLAGS_DELETE_REQUESTED );
			WaitForThreadpoolWorkCallbacks( Work,
											fCancelPendingCallbacks );
			break;
		}

		case LdkTpCleanupGroupObjectTimer:
		{
			PTP_TIMER Timer = (PTP_TIMER)Member->Object;

			if (fCancelPendingCallbacks) {
				InterlockedOr( &Timer->Flags,
							   LDK_TP_TIMER_FLAGS_CANCEL_REQUESTED );
			}
			LdkpCloseThreadpoolTimerInternal( Timer );
			WaitForThreadpoolTimerCallbacks( Timer,
											 fCancelPendingCallbacks );
			break;
		}

		case LdkTpCleanupGroupObjectWait:
		{
			PTP_WAIT Wait = (PTP_WAIT)Member->Object;

			if (fCancelPendingCallbacks) {
				InterlockedOr( &Wait->Flags,
							   LDK_TP_WAIT_FLAGS_CANCEL_REQUESTED );
			}
			LdkpCloseThreadpoolWaitInternal( Wait );
			WaitForThreadpoolWaitCallbacks( Wait,
											fCancelPendingCallbacks );
			break;
		}
		}

		CancelCallback = Member->CleanupGroupCancelCallback;
		ObjectContext = Member->ObjectContext;
		Member->CleanupGroupCancelCallback = NULL;
		Member->ObjectContext = NULL;
		Member->Object = NULL;
		if (CancelCallback != NULL) {
			CancelCallback( ObjectContext,
							pvCleanupContext );
		}

		switch (Member->ObjectType) {
		case LdkTpCleanupGroupObjectWork:
			LdkpDereferenceThreadpoolWork( CONTAINING_RECORD(Member,
															 TP_WORK,
															 CleanupGroupMember) );
			break;

		case LdkTpCleanupGroupObjectTimer:
			LdkpDereferenceThreadpoolTimer( CONTAINING_RECORD(Member,
															  TP_TIMER,
															  CleanupGroupMember) );
			break;

		case LdkTpCleanupGroupObjectWait:
			LdkpDereferenceThreadpoolWait( CONTAINING_RECORD(Member,
															 TP_WAIT,
															 CleanupGroupMember) );
			break;
		}
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
	while (! IsListEmpty( &ptpcg->ObjectList )) {
		PLIST_ENTRY Entry = RemoveHeadList( &ptpcg->ObjectList );
		PLDK_TP_CLEANUP_GROUP_MEMBER Member = CONTAINING_RECORD(Entry,
																 LDK_TP_CLEANUP_GROUP_MEMBER,
																 Links);

		Member->CleanupGroup = NULL;
		Member->CleanupGroupCancelCallback = NULL;
		Member->ObjectContext = NULL;
		Member->Object = NULL;
		InitializeListHead( &Member->Links );
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
	InitializeListHead( &work->CleanupGroupMember.Links );

	work->ReferenceCount = 1;
	work->Flags = 0;
	work->OutstandingCount = 0;
	work->CleanupGroupMember.CleanupGroup = NULL;
	work->CleanupGroupMember.CleanupGroupCancelCallback = NULL;
	work->CleanupGroupMember.ObjectContext = NULL;
	work->CleanupGroupMember.ObjectType = LdkTpCleanupGroupObjectWork;
	work->CleanupGroupMember.Object = NULL;
	work->WorkCallback = pfnwk;
	work->CallbackParameter = pv;

	Status = LdkpRegisterThreadpoolCleanupGroupMember( &work->CallbackEnvironment,
													   LdkTpCleanupGroupObjectWork,
													   work,
													   pv,
													   &work->CleanupGroupMember );
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
	BOOLEAN InvokeCallback;
	HMODULE RaceDll;
	BOOLEAN ReleaseRaceDll;
	TP_CALLBACK_INSTANCE Instance;

	InvokeCallback = ! FlagOn( InterlockedCompareExchange( &Work->Flags,
															0,
															0 ),
							   LDK_TP_WORK_FLAGS_CANCEL_REQUESTED );

	ReleaseRaceDll = LdkpBeginThreadpoolCallback( &Work->CallbackEnvironment,
												  &Instance,
												  &RaceDll );

	if (InvokeCallback) {
		Work->WorkCallback( &Instance,
							Work->CallbackParameter,
							Work );
	}

	LdkpEndThreadpoolCallback( &Instance,
							   RaceDll,
							   ReleaseRaceDll );

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
	LdkpUnregisterThreadpoolCleanupGroupMember( &pwk->CleanupGroupMember );
	InterlockedOr( &pwk->Flags,
				   LDK_TP_WORK_FLAGS_DELETE_REQUESTED );
	LdkpDereferenceThreadpoolWork( pwk );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
VOID
LdkpThreadpoolTimerThreadRoutine (
	_In_ PTP_TIMER Timer
	)
{
	for (;;) {
		LARGE_INTEGER DueTime;
		DWORD Period;
		LONG Flags;
		KIRQL OldIrql;
		NTSTATUS Status;

		KeAcquireSpinLock( &Timer->Lock,
						   &OldIrql );
		Flags = Timer->Flags;
		if (FlagOn( Flags,
					LDK_TP_TIMER_FLAGS_DELETE_REQUESTED )) {
			KeReleaseSpinLock( &Timer->Lock,
							   OldIrql );
			break;
		}

		if (! FlagOn( Flags,
					  LDK_TP_TIMER_FLAGS_SET ) ||
			FlagOn( Flags,
					LDK_TP_TIMER_FLAGS_EXPIRED )) {
			KeReleaseSpinLock( &Timer->Lock,
							   OldIrql );
			(VOID)KeWaitForSingleObject( &Timer->ControlEvent,
										 Executive,
										 KernelMode,
										 FALSE,
										 NULL );
			continue;
		}

		DueTime = Timer->DueTime;
		Period = Timer->Period;
		KeReleaseSpinLock( &Timer->Lock,
						   OldIrql );

		Status = KeWaitForSingleObject( &Timer->ControlEvent,
										Executive,
										KernelMode,
										FALSE,
										&DueTime );
		if (Status != STATUS_TIMEOUT) {
			continue;
		}

		KeAcquireSpinLock( &Timer->Lock,
						   &OldIrql );
		Flags = Timer->Flags;
		if (FlagOn( Flags,
					LDK_TP_TIMER_FLAGS_DELETE_REQUESTED ) ||
			! FlagOn( Flags,
					  LDK_TP_TIMER_FLAGS_SET )) {
			KeReleaseSpinLock( &Timer->Lock,
							   OldIrql );
			continue;
		}

		if (Period == 0) {
			SetFlag( Timer->Flags,
					 LDK_TP_TIMER_FLAGS_EXPIRED );
		} else {
			Timer->DueTime.QuadPart = -((LONGLONG)Period * 10000);
		}
		KeReleaseSpinLock( &Timer->Lock,
						   OldIrql );

		Flags = InterlockedCompareExchange( &Timer->Flags,
											0,
											0 );
		if (FlagOn( Flags,
					LDK_TP_TIMER_FLAGS_SET ) &&
			! FlagOn( Flags,
					  LDK_TP_TIMER_FLAGS_DELETE_REQUESTED | LDK_TP_TIMER_FLAGS_CANCEL_REQUESTED )) {
			TP_CALLBACK_INSTANCE Instance;
			HMODULE RaceDll;
			BOOLEAN ReleaseRaceDll;

			InterlockedIncrement( &Timer->ReferenceCount );
			InterlockedIncrement( &Timer->OutstandingCount );
			KeResetEvent( &Timer->Event );

			ReleaseRaceDll = LdkpBeginThreadpoolCallback( &Timer->CallbackEnvironment,
														  &Instance,
														  &RaceDll );
			Timer->TimerCallback( &Instance,
								  Timer->CallbackParameter,
								  Timer );
			LdkpEndThreadpoolCallback( &Instance,
									   RaceDll,
									   ReleaseRaceDll );
			LdkpCompleteThreadpoolTimerCallback( Timer );
		}
	}

	LdkpDereferenceThreadpoolTimer( Timer );
	PsTerminateSystemThread( STATUS_SUCCESS );
}

WINBASEAPI
_Must_inspect_result_
PTP_TIMER
WINAPI
CreateThreadpoolTimer (
	_In_ PTP_TIMER_CALLBACK pfnti,
	_Inout_opt_ PVOID pv,
	_In_opt_ PTP_CALLBACK_ENVIRON pcbe
	)
{
	PTP_TIMER Timer;
	NTSTATUS Status;
	HANDLE ThreadHandle;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES ObjectAttributes;

	PAGED_CODE();

	if (! ARGUMENT_PRESENT(pfnti)) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	Timer = ExAllocateFromNPagedLookasideList( &LdkpThreadpoolTimerLookaside );
	if (Timer == NULL) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}

	Status = LdkpCaptureThreadpoolCallbackEnvironment( pcbe,
													   &Timer->CallbackEnvironment );
	if (! NT_SUCCESS(Status)) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolTimerLookaside,
									 Timer );
		LdkSetLastNTError( Status );
		return NULL;
	}

	Timer->ReferenceCount = 2;
	Timer->Flags = 0;
	Timer->OutstandingCount = 0;
	KeInitializeEvent( &Timer->Event,
					   NotificationEvent,
					   TRUE );
	KeInitializeEvent( &Timer->ControlEvent,
					   SynchronizationEvent,
					   FALSE );
	KeInitializeSpinLock( &Timer->Lock );
	Timer->ThreadHandle = NULL;
	Timer->ThreadId = NULL;
	Timer->DueTime.QuadPart = 0;
	Timer->Period = 0;
	Timer->WindowLength = 0;
	InitializeListHead( &Timer->CleanupGroupMember.Links );
	Timer->CleanupGroupMember.CleanupGroup = NULL;
	Timer->CleanupGroupMember.CleanupGroupCancelCallback = NULL;
	Timer->CleanupGroupMember.ObjectContext = NULL;
	Timer->CleanupGroupMember.ObjectType = LdkTpCleanupGroupObjectTimer;
	Timer->CleanupGroupMember.Object = NULL;
	Timer->CallbackParameter = pv;
	Timer->TimerCallback = pfnti;

	InitializeObjectAttributes( &ObjectAttributes,
								NULL,
								OBJ_KERNEL_HANDLE,
								NULL,
								NULL );
	Status = PsCreateSystemThread( &ThreadHandle,
								   THREAD_ALL_ACCESS,
								   &ObjectAttributes,
								   NULL,
								   &ClientId,
								   LdkpThreadpoolTimerThreadRoutine,
								   Timer );
	if (! NT_SUCCESS(Status)) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolTimerLookaside,
									 Timer );
		LdkSetLastNTError( Status );
		return NULL;
	}

	Timer->ThreadHandle = ThreadHandle;
	Timer->ThreadId = ClientId.UniqueThread;

	Status = LdkpRegisterThreadpoolCleanupGroupMember( &Timer->CallbackEnvironment,
													   LdkTpCleanupGroupObjectTimer,
													   Timer,
													   pv,
													   &Timer->CleanupGroupMember );
	if (! NT_SUCCESS(Status)) {
		LdkpCloseThreadpoolTimerInternal( Timer );
		LdkpDereferenceThreadpoolTimer( Timer );
		LdkSetLastNTError( Status );
		return NULL;
	}

	return Timer;
}

WINBASEAPI
VOID
WINAPI
SetThreadpoolTimer (
	_Inout_ PTP_TIMER pti,
	_In_opt_ PFILETIME pftDueTime,
	_In_ DWORD msPeriod,
	_In_opt_ DWORD msWindowLength
	)
{
	LARGE_INTEGER DueTime;
	KIRQL OldIrql;

	PAGED_CODE();

	KeAcquireSpinLock( &pti->Lock,
					   &OldIrql );
	if (pftDueTime == NULL) {
		ClearFlag( pti->Flags,
				   LDK_TP_TIMER_FLAGS_SET | LDK_TP_TIMER_FLAGS_EXPIRED );
		pti->Period = 0;
		pti->WindowLength = 0;
	} else {
		DueTime.LowPart = pftDueTime->dwLowDateTime;
		DueTime.HighPart = pftDueTime->dwHighDateTime;
		pti->DueTime = DueTime;
		pti->Period = msPeriod;
		pti->WindowLength = msWindowLength;
		SetFlag( pti->Flags,
				 LDK_TP_TIMER_FLAGS_SET );
		ClearFlag( pti->Flags,
				   LDK_TP_TIMER_FLAGS_CANCEL_REQUESTED | LDK_TP_TIMER_FLAGS_EXPIRED );
	}
	KeSetEvent( &pti->ControlEvent,
				IO_NO_INCREMENT,
				FALSE );
	KeReleaseSpinLock( &pti->Lock,
					   OldIrql );
}

WINBASEAPI
BOOL
WINAPI
IsThreadpoolTimerSet (
	_Inout_ PTP_TIMER pti
	)
{
	PAGED_CODE();

	return FlagOn( InterlockedCompareExchange( &pti->Flags,
											   0,
											   0 ),
				   LDK_TP_TIMER_FLAGS_SET );
}

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolTimerCallbacks (
	_Inout_ PTP_TIMER pti,
	_In_ BOOL fCancelPendingCallbacks
	)
{
	NTSTATUS Status;

	PAGED_CODE();

	if (InterlockedIncrement( &pti->ReferenceCount ) <= 1) {
		LdkSetLastNTError( STATUS_DELETE_PENDING );
		return;
	}

	if (fCancelPendingCallbacks) {
		InterlockedOr( &pti->Flags,
					   LDK_TP_TIMER_FLAGS_CANCEL_REQUESTED );
		KeSetEvent( &pti->ControlEvent,
					IO_NO_INCREMENT,
					FALSE );
	}

	Status = KeWaitForSingleObject( &pti->Event,
									Executive,
									KernelMode,
									FALSE,
									NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
	}
	if (fCancelPendingCallbacks) {
		InterlockedAnd( &pti->Flags,
						~LDK_TP_TIMER_FLAGS_CANCEL_REQUESTED );
	}
	LdkpDereferenceThreadpoolTimer( pti );
}

WINBASEAPI
VOID
WINAPI
CloseThreadpoolTimer (
	_Inout_ PTP_TIMER pti
	)
{
	PAGED_CODE();

	LdkpUnregisterThreadpoolCleanupGroupMember( &pti->CleanupGroupMember );
	LdkpCloseThreadpoolTimerInternal( pti );
	LdkpDereferenceThreadpoolTimer( pti );
}

_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
VOID
LdkpThreadpoolWaitThreadRoutine (
	_In_ PTP_WAIT Wait
	)
{
	for (;;) {
		PVOID WaitObject;
		PVOID Objects[2];
		KWAIT_BLOCK WaitBlocks[THREAD_WAIT_OBJECTS];
		LARGE_INTEGER Timeout;
		PLARGE_INTEGER TimeoutPointer;
		LONG Flags;
		KIRQL OldIrql;
		NTSTATUS Status;
		TP_WAIT_RESULT WaitResult;

		KeAcquireSpinLock( &Wait->Lock,
						   &OldIrql );
		Flags = Wait->Flags;
		if (FlagOn( Flags,
					LDK_TP_WAIT_FLAGS_DELETE_REQUESTED )) {
			KeReleaseSpinLock( &Wait->Lock,
							   OldIrql );
			break;
		}

		if (! FlagOn( Flags,
					  LDK_TP_WAIT_FLAGS_SET ) ||
			Wait->WaitObject == NULL) {
			KeReleaseSpinLock( &Wait->Lock,
							   OldIrql );
			(VOID)KeWaitForSingleObject( &Wait->ControlEvent,
										 Executive,
										 KernelMode,
										 FALSE,
										 NULL );
			continue;
		}

		WaitObject = Wait->WaitObject;
		ObReferenceObject( WaitObject );
		Timeout = Wait->Timeout;
		TimeoutPointer = Wait->HasTimeout ? &Timeout : NULL;
		KeReleaseSpinLock( &Wait->Lock,
						   OldIrql );

		Objects[0] = WaitObject;
		Objects[1] = &Wait->ControlEvent;
		Status = KeWaitForMultipleObjects( RTL_NUMBER_OF(Objects),
										   Objects,
										   WaitAny,
										   Executive,
										   KernelMode,
										   FALSE,
										   TimeoutPointer,
										   WaitBlocks );
		ObDereferenceObject( WaitObject );

		if (Status == STATUS_WAIT_0 + 1) {
			continue;
		}

		if (Status != STATUS_WAIT_0 &&
			Status != STATUS_TIMEOUT) {
			continue;
		}

		KeAcquireSpinLock( &Wait->Lock,
						   &OldIrql );
		Flags = Wait->Flags;
		if (FlagOn( Flags,
					LDK_TP_WAIT_FLAGS_DELETE_REQUESTED ) ||
			! FlagOn( Flags,
					  LDK_TP_WAIT_FLAGS_SET )) {
			KeReleaseSpinLock( &Wait->Lock,
							   OldIrql );
			continue;
		}
		ClearFlag( Wait->Flags,
				   LDK_TP_WAIT_FLAGS_SET );
		KeReleaseSpinLock( &Wait->Lock,
						   OldIrql );

		Flags = InterlockedCompareExchange( &Wait->Flags,
											0,
											0 );
		if (FlagOn( Flags,
					LDK_TP_WAIT_FLAGS_DELETE_REQUESTED | LDK_TP_WAIT_FLAGS_CANCEL_REQUESTED )) {
			continue;
		}

		WaitResult = (Status == STATUS_TIMEOUT) ? WAIT_TIMEOUT : WAIT_OBJECT_0;
		InterlockedIncrement( &Wait->ReferenceCount );
		InterlockedIncrement( &Wait->OutstandingCount );
		KeResetEvent( &Wait->Event );

		TP_CALLBACK_INSTANCE Instance;
		HMODULE RaceDll;
		BOOLEAN ReleaseRaceDll = LdkpBeginThreadpoolCallback( &Wait->CallbackEnvironment,
															  &Instance,
															  &RaceDll );
		Wait->WaitCallback( &Instance,
							Wait->CallbackParameter,
							Wait,
							WaitResult );
		LdkpEndThreadpoolCallback( &Instance,
								   RaceDll,
								   ReleaseRaceDll );
		LdkpCompleteThreadpoolWaitCallback( Wait );
	}

	LdkpDereferenceThreadpoolWait( Wait );
	PsTerminateSystemThread( STATUS_SUCCESS );
}

WINBASEAPI
_Must_inspect_result_
PTP_WAIT
WINAPI
CreateThreadpoolWait (
	_In_ PTP_WAIT_CALLBACK pfnwa,
	_Inout_opt_ PVOID pv,
	_In_opt_ PTP_CALLBACK_ENVIRON pcbe
	)
{
	PTP_WAIT Wait;
	NTSTATUS Status;
	HANDLE ThreadHandle;
	CLIENT_ID ClientId;
	OBJECT_ATTRIBUTES ObjectAttributes;

	PAGED_CODE();

	if (! ARGUMENT_PRESENT(pfnwa)) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	Wait = ExAllocateFromNPagedLookasideList( &LdkpThreadpoolWaitLookaside );
	if (Wait == NULL) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}

	Status = LdkpCaptureThreadpoolCallbackEnvironment( pcbe,
													   &Wait->CallbackEnvironment );
	if (! NT_SUCCESS(Status)) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolWaitLookaside,
									 Wait );
		LdkSetLastNTError( Status );
		return NULL;
	}

	Wait->ReferenceCount = 2;
	Wait->Flags = 0;
	Wait->OutstandingCount = 0;
	KeInitializeEvent( &Wait->Event,
					   NotificationEvent,
					   TRUE );
	KeInitializeEvent( &Wait->ControlEvent,
					   SynchronizationEvent,
					   FALSE );
	KeInitializeSpinLock( &Wait->Lock );
	Wait->ThreadHandle = NULL;
	Wait->ThreadId = NULL;
	Wait->WaitObject = NULL;
	Wait->Timeout.QuadPart = 0;
	Wait->HasTimeout = FALSE;
	InitializeListHead( &Wait->CleanupGroupMember.Links );
	Wait->CleanupGroupMember.CleanupGroup = NULL;
	Wait->CleanupGroupMember.CleanupGroupCancelCallback = NULL;
	Wait->CleanupGroupMember.ObjectContext = NULL;
	Wait->CleanupGroupMember.ObjectType = LdkTpCleanupGroupObjectWait;
	Wait->CleanupGroupMember.Object = NULL;
	Wait->CallbackParameter = pv;
	Wait->WaitCallback = pfnwa;

	InitializeObjectAttributes( &ObjectAttributes,
								NULL,
								OBJ_KERNEL_HANDLE,
								NULL,
								NULL );
	Status = PsCreateSystemThread( &ThreadHandle,
								   THREAD_ALL_ACCESS,
								   &ObjectAttributes,
								   NULL,
								   &ClientId,
								   LdkpThreadpoolWaitThreadRoutine,
								   Wait );
	if (! NT_SUCCESS(Status)) {
		ExFreeToNPagedLookasideList( &LdkpThreadpoolWaitLookaside,
									 Wait );
		LdkSetLastNTError( Status );
		return NULL;
	}

	Wait->ThreadHandle = ThreadHandle;
	Wait->ThreadId = ClientId.UniqueThread;

	Status = LdkpRegisterThreadpoolCleanupGroupMember( &Wait->CallbackEnvironment,
													   LdkTpCleanupGroupObjectWait,
													   Wait,
													   pv,
													   &Wait->CleanupGroupMember );
	if (! NT_SUCCESS(Status)) {
		LdkpCloseThreadpoolWaitInternal( Wait );
		LdkpDereferenceThreadpoolWait( Wait );
		LdkSetLastNTError( Status );
		return NULL;
	}

	return Wait;
}

WINBASEAPI
VOID
WINAPI
SetThreadpoolWait (
	_Inout_ PTP_WAIT pwa,
	_In_opt_ HANDLE h,
	_In_opt_ PFILETIME pftTimeout
	)
{
	PVOID NewWaitObject;
	PVOID OldWaitObject;
	LARGE_INTEGER Timeout;
	BOOLEAN HasTimeout;
	KIRQL OldIrql;
	NTSTATUS Status;

	PAGED_CODE();

	NewWaitObject = NULL;
	OldWaitObject = NULL;
	HasTimeout = FALSE;
	Timeout.QuadPart = 0;

	if (h != NULL) {
		if (LdkIsProcessHandleCandidate( h )) {
			LdkSetLastNTError( STATUS_NOT_SUPPORTED );
			return;
		}

		Status = ObReferenceObjectByHandle( h,
											SYNCHRONIZE,
											NULL,
											KernelMode,
											&NewWaitObject,
											NULL );
		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return;
		}

		if (pftTimeout != NULL) {
			Timeout.LowPart = pftTimeout->dwLowDateTime;
			Timeout.HighPart = pftTimeout->dwHighDateTime;
			HasTimeout = TRUE;
		}
	}

	KeAcquireSpinLock( &pwa->Lock,
					   &OldIrql );
	OldWaitObject = pwa->WaitObject;
	pwa->WaitObject = NewWaitObject;
	pwa->Timeout = Timeout;
	pwa->HasTimeout = HasTimeout;
	if (h == NULL) {
		ClearFlag( pwa->Flags,
				   LDK_TP_WAIT_FLAGS_SET );
	} else {
		SetFlag( pwa->Flags,
				 LDK_TP_WAIT_FLAGS_SET );
		ClearFlag( pwa->Flags,
				   LDK_TP_WAIT_FLAGS_CANCEL_REQUESTED );
	}
	KeSetEvent( &pwa->ControlEvent,
				IO_NO_INCREMENT,
				FALSE );
	KeReleaseSpinLock( &pwa->Lock,
					   OldIrql );

	if (OldWaitObject != NULL) {
		ObDereferenceObject( OldWaitObject );
	}
}

WINBASEAPI
VOID
WINAPI
WaitForThreadpoolWaitCallbacks (
	_Inout_ PTP_WAIT pwa,
	_In_ BOOL fCancelPendingCallbacks
	)
{
	NTSTATUS Status;

	PAGED_CODE();

	if (InterlockedIncrement( &pwa->ReferenceCount ) <= 1) {
		LdkSetLastNTError( STATUS_DELETE_PENDING );
		return;
	}

	if (fCancelPendingCallbacks) {
		InterlockedOr( &pwa->Flags,
					   LDK_TP_WAIT_FLAGS_CANCEL_REQUESTED );
		KeSetEvent( &pwa->ControlEvent,
					IO_NO_INCREMENT,
					FALSE );
	}

	Status = KeWaitForSingleObject( &pwa->Event,
									Executive,
									KernelMode,
									FALSE,
									NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
	}
	if (fCancelPendingCallbacks) {
		InterlockedAnd( &pwa->Flags,
						~LDK_TP_WAIT_FLAGS_CANCEL_REQUESTED );
	}
	LdkpDereferenceThreadpoolWait( pwa );
}

WINBASEAPI
VOID
WINAPI
CloseThreadpoolWait (
	_Inout_ PTP_WAIT pwa
	)
{
	PAGED_CODE();

	LdkpUnregisterThreadpoolCleanupGroupMember( &pwa->CleanupGroupMember );
	LdkpCloseThreadpoolWaitInternal( pwa );
	LdkpDereferenceThreadpoolWait( pwa );
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
