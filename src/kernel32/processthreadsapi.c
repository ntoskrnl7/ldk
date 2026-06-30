#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"
#include "../peb.h"



LDK_INITIALIZE_COMPONENT LdkpInitializeThreadContexts;
LDK_TERMINATE_COMPONENT LdkpTerminateThreadContexts;

EXPAND_STACK_CALLOUT LdkpThreadStartExpandStackAndCallout;
KSTART_ROUTINE LdkpThreadStartRoutine;

typedef struct _LDK_THREAD_CONTEXT LDK_THREAD_CONTEXT, *PLDK_THREAD_CONTEXT;
typedef struct _LDK_THREAD_DESCRIPTION_ENTRY LDK_THREAD_DESCRIPTION_ENTRY, *PLDK_THREAD_DESCRIPTION_ENTRY;

NTSTATUS
LdkpGetThreadIdFromHandle (
	_In_ HANDLE ThreadHandle,
	_Out_ PHANDLE ThreadId
	);

PTEB
LdkGetNextTebRundownProtection (
	_In_ PTEB Teb
	);

PLDK_THREAD_CONTEXT
LdkpFindThreadContextByIdLocked (
	_In_ HANDLE ThreadId
	);

VOID
LdkpInsertThreadContext (
	_Inout_ PLDK_THREAD_CONTEXT Context
	);

VOID
LdkpRemoveThreadContext (
	_Inout_ PLDK_THREAD_CONTEXT Context
	);

PLDK_THREAD_DESCRIPTION_ENTRY
LdkpFindThreadDescriptionByIdLocked (
	_In_ HANDLE ThreadId
	);

VOID
LdkpFreeThreadDescriptionEntry (
	_In_ PLDK_THREAD_DESCRIPTION_ENTRY Entry
	);

HRESULT
LdkpHResultFromWin32Error (
	_In_ DWORD ErrorCode
	);

HRESULT
LdkpHResultFromNtStatus (
	_In_ NTSTATUS Status
	);

BOOL
LdkpCopyProcessImageNameA (
	_In_ PCANSI_STRING Source,
	_Out_writes_to_(*Size, *Size) LPSTR Buffer,
	_Inout_ PDWORD Size
	);

BOOL
LdkpCopyProcessImageNameW (
	_In_ PCUNICODE_STRING Source,
	_Out_writes_to_(*Size, *Size) LPWSTR Buffer,
	_Inout_ PDWORD Size
	);

NTSTATUS
LdkpQueryNativeProcessImageName (
	_In_ HANDLE ProcessHandle,
	_Outptr_ PUNICODE_STRING *ImageName,
	_Outptr_ PVOID *FreeBuffer
	);



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeThreadContexts)
#pragma alloc_text(PAGE, LdkpTerminateThreadContexts)
#pragma alloc_text(PAGE, LdkpThreadStartExpandStackAndCallout)
#pragma alloc_text(PAGE, LdkpThreadStartRoutine)
#pragma alloc_text(PAGE, LdkpGetThreadIdFromHandle)
#pragma alloc_text(PAGE, LdkpFindThreadContextByIdLocked)
#pragma alloc_text(PAGE, LdkpInsertThreadContext)
#pragma alloc_text(PAGE, LdkpRemoveThreadContext)
#pragma alloc_text(PAGE, LdkpFindThreadDescriptionByIdLocked)
#pragma alloc_text(PAGE, LdkpFreeThreadDescriptionEntry)
#pragma alloc_text(PAGE, CreateThread)
#pragma alloc_text(PAGE, OpenThread)
#pragma alloc_text(PAGE, SuspendThread)
#pragma alloc_text(PAGE, ResumeThread)
#pragma alloc_text(PAGE, GetThreadPriority)
#pragma alloc_text(PAGE, SetThreadPriority)
#pragma alloc_text(PAGE, GetThreadPriorityBoost)
#pragma alloc_text(PAGE, SetThreadPriorityBoost)
#pragma alloc_text(PAGE, GetExitCodeThread)
#pragma alloc_text(PAGE, ExitThread)
#pragma alloc_text(PAGE, GetThreadTimes)
#pragma alloc_text(PAGE, GetProcessTimes)
#pragma alloc_text(PAGE, GetProcessId)
#pragma alloc_text(PAGE, GetProcessAffinityMask)
#pragma alloc_text(PAGE, SetProcessAffinityMask)
#pragma alloc_text(PAGE, GetThreadId)
#pragma alloc_text(PAGE, GetThreadGroupAffinity)
#pragma alloc_text(PAGE, SetThreadGroupAffinity)
#pragma alloc_text(PAGE, QueryFullProcessImageNameA)
#pragma alloc_text(PAGE, QueryFullProcessImageNameW)
#pragma alloc_text(PAGE, SetThreadDescription)
#pragma alloc_text(PAGE, GetThreadDescription)
#pragma alloc_text(PAGE, ExitProcess)
#pragma alloc_text(PAGE, TerminateProcess)
#pragma alloc_text(PAGE, IsProcessorFeaturePresent)
#pragma alloc_text(PAGE, TlsAlloc)
#pragma alloc_text(PAGE, TlsFree)
#endif



#define TAG_LDK_THREAD_CONTEXT			'xtCT'

struct _LDK_THREAD_CONTEXT {
	LIST_ENTRY Links;
	KEVENT StartEvent;
	HANDLE ThreadId;
	LONG StartupSuspendCount;
	BOOLEAN Linked;
	BOOLEAN StartReleased;
	DWORD dwCreationFlags;
	SIZE_T dwStackSize;
	PTHREAD_START_ROUTINE ThreadStartRoutine;
	LPVOID lpThreadParameter;
	DWORD ExitCode;
};

struct _LDK_THREAD_DESCRIPTION_ENTRY {
	LIST_ENTRY Links;
	HANDLE ThreadId;
	PWSTR Description;
	SIZE_T DescriptionBytes;
};

typedef struct _LDK_THREAD_CALLOUT_CONTEXT {
	PTHREAD_START_ROUTINE ThreadStartRoutine;
	LPVOID lpThreadParameter;
	DWORD ExitCode;
} LDK_THREAD_CALLOUT_CONTEXT, *PLDK_THREAD_CALLOUT_CONTEXT;

NPAGED_LOOKASIDE_LIST LdkpThreadContextLookaside;
FAST_MUTEX LdkpThreadContextListMutex;
LIST_ENTRY LdkpThreadContextListHead;
FAST_MUTEX LdkpThreadDescriptionListMutex;
LIST_ENTRY LdkpThreadDescriptionListHead;

#ifndef LDK_S_OK
#define LDK_S_OK ((HRESULT)0L)
#endif

#ifndef LDK_E_INVALIDARG
#define LDK_E_INVALIDARG ((HRESULT)0x80070057L)
#endif

#ifndef LDK_E_OUTOFMEMORY
#define LDK_E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#endif

#define LDK_HRESULT_FACILITY_WIN32 7
#define LDK_PROCESS_IMAGE_FILE_NAME_CLASS ((PROCESSINFOCLASS)27)
#define LDK_QUERY_FULL_PROCESS_IMAGE_NAME_SUPPORTED_FLAGS PROCESS_NAME_NATIVE



static
DWORD_PTR
LdkpQueryPrimaryProcessorAffinityMask (
	VOID
	)
{
	SYSTEM_INFO SystemInfo;
	DWORD_PTR Mask;

	PAGED_CODE();

	RtlZeroMemory( &SystemInfo,
				   sizeof(SystemInfo) );
	GetNativeSystemInfo( &SystemInfo );

	Mask = SystemInfo.dwActiveProcessorMask;
	if (Mask == 0) {
		Mask = (DWORD_PTR)KeQueryGroupAffinity( 0 );
	}

	return Mask != 0 ? Mask : (DWORD_PTR)1;
}

static
BOOL
LdkpValidatePrimaryProcessorAffinityMask (
	_In_ DWORD_PTR Mask
	)
{
	DWORD_PTR ActiveMask;

	PAGED_CODE();

	ActiveMask = LdkpQueryPrimaryProcessorAffinityMask();
	return Mask != 0 &&
		   (Mask & ~ActiveMask) == 0;
}

static
VOID
LdkpQueryCurrentThreadGroupAffinity (
	_Out_ PGROUP_AFFINITY GroupAffinity
	)
{
	PROCESSOR_NUMBER ProcessorNumber;
	KAFFINITY ActiveMask;

	PAGED_CODE();

	RtlZeroMemory( &ProcessorNumber,
				   sizeof(ProcessorNumber) );
	KeGetCurrentProcessorNumberEx( &ProcessorNumber );

	ActiveMask = KeQueryGroupAffinity( ProcessorNumber.Group );
	if (ActiveMask == 0 &&
		ProcessorNumber.Group == 0) {
		ActiveMask = (KAFFINITY)LdkpQueryPrimaryProcessorAffinityMask();
	}

	RtlZeroMemory( GroupAffinity,
				   sizeof(*GroupAffinity) );
	GroupAffinity->Group = ProcessorNumber.Group;
	GroupAffinity->Mask = ActiveMask != 0 ? ActiveMask : (KAFFINITY)1;
}

static
BOOL
LdkpValidateThreadGroupAffinity (
	_In_ const GROUP_AFFINITY *GroupAffinity
	)
{
	WORD ActiveGroupCount;
	KAFFINITY ActiveMask;

	PAGED_CODE();

	if (GroupAffinity == NULL ||
		GroupAffinity->Mask == 0) {
		return FALSE;
	}

	ActiveGroupCount = KeQueryActiveGroupCount();
	if (ActiveGroupCount == 0) {
		ActiveGroupCount = 1;
	}

	if (GroupAffinity->Group >= ActiveGroupCount) {
		return FALSE;
	}

	ActiveMask = KeQueryGroupAffinity( GroupAffinity->Group );
	if (ActiveMask == 0 &&
		GroupAffinity->Group == 0) {
		ActiveMask = (KAFFINITY)LdkpQueryPrimaryProcessorAffinityMask();
	}

	return ActiveMask != 0 &&
		   (GroupAffinity->Mask & ~ActiveMask) == 0;
}

static
BOOL
LdkpValidateThreadHandleForAffinity (
	_In_ HANDLE ThreadHandle
	)
{
	HANDLE ThreadId;
	NTSTATUS Status;

	PAGED_CODE();

	Status = LdkpGetThreadIdFromHandle( ThreadHandle,
										&ThreadId );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FALSE;
	}

	return TRUE;
}

VOID
LdkpInvokeFlsCallback (
	_Inout_ PLDK_TEB Teb
	);



NTSTATUS
LdkpInitializeThreadContexts (
	VOID
	)
{
	PAGED_CODE();

	ExInitializeNPagedLookasideList( &LdkpThreadContextLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_THREAD_CONTEXT),
									 TAG_LDK_THREAD_CONTEXT,
									 0 );
	ExInitializeFastMutex( &LdkpThreadContextListMutex );
	InitializeListHead( &LdkpThreadContextListHead );
	ExInitializeFastMutex( &LdkpThreadDescriptionListMutex );
	InitializeListHead( &LdkpThreadDescriptionListHead );

	return STATUS_SUCCESS;
}

VOID
LdkpTerminateThreadContexts (
	VOID
	)
{
	PAGED_CODE();

	ExAcquireFastMutex( &LdkpThreadContextListMutex );
	if (! IsListEmpty( &LdkpThreadContextListHead )) {
		LDK_DIAGNOSTIC_BREAK();
	}
	ExReleaseFastMutex( &LdkpThreadContextListMutex );

	ExAcquireFastMutex( &LdkpThreadDescriptionListMutex );
	while (! IsListEmpty( &LdkpThreadDescriptionListHead )) {
		PLIST_ENTRY Link = RemoveHeadList( &LdkpThreadDescriptionListHead );
		PLDK_THREAD_DESCRIPTION_ENTRY Entry = CONTAINING_RECORD( Link,
																 LDK_THREAD_DESCRIPTION_ENTRY,
																 Links );
		ExReleaseFastMutex( &LdkpThreadDescriptionListMutex );
		LdkpFreeThreadDescriptionEntry( Entry );
		ExAcquireFastMutex( &LdkpThreadDescriptionListMutex );
	}
	ExReleaseFastMutex( &LdkpThreadDescriptionListMutex );

	ExDeleteNPagedLookasideList( &LdkpThreadContextLookaside );
}

NTSTATUS
LdkpGetThreadIdFromHandle (
	_In_ HANDLE ThreadHandle,
	_Out_ PHANDLE ThreadId
	)
{
	PAGED_CODE();

	THREAD_BASIC_INFORMATION BasicInformation;
	NTSTATUS Status = ZwQueryInformationThread( ThreadHandle,
												ThreadBasicInformation,
												&BasicInformation,
												sizeof(BasicInformation),
												NULL );
	if (NT_SUCCESS(Status)) {
		*ThreadId = BasicInformation.ClientId.UniqueThread;
	}

	return Status;
}

PLDK_THREAD_CONTEXT
LdkpFindThreadContextByIdLocked (
	_In_ HANDLE ThreadId
	)
{
	PAGED_CODE();

	PLIST_ENTRY Current;

	for (Current = LdkpThreadContextListHead.Flink;
		 Current != &LdkpThreadContextListHead;
		 Current = Current->Flink) {
		PLDK_THREAD_CONTEXT Context = CONTAINING_RECORD( Current,
														 LDK_THREAD_CONTEXT,
														 Links );
		if (Context->ThreadId == ThreadId) {
			return Context;
		}
	}

	return NULL;
}

VOID
LdkpInsertThreadContext (
	_Inout_ PLDK_THREAD_CONTEXT Context
	)
{
	PAGED_CODE();

	ExAcquireFastMutex( &LdkpThreadContextListMutex );
	InsertTailList( &LdkpThreadContextListHead,
					&Context->Links );
	Context->Linked = TRUE;
	ExReleaseFastMutex( &LdkpThreadContextListMutex );
}

VOID
LdkpRemoveThreadContext (
	_Inout_ PLDK_THREAD_CONTEXT Context
	)
{
	PAGED_CODE();

	ExAcquireFastMutex( &LdkpThreadContextListMutex );
	if (Context->Linked) {
		RemoveEntryList( &Context->Links );
		Context->Linked = FALSE;
	}
	ExReleaseFastMutex( &LdkpThreadContextListMutex );
}

PLDK_THREAD_DESCRIPTION_ENTRY
LdkpFindThreadDescriptionByIdLocked (
	_In_ HANDLE ThreadId
	)
{
	PAGED_CODE();

	PLIST_ENTRY Current;

	for (Current = LdkpThreadDescriptionListHead.Flink;
		 Current != &LdkpThreadDescriptionListHead;
		 Current = Current->Flink) {
		PLDK_THREAD_DESCRIPTION_ENTRY Entry = CONTAINING_RECORD( Current,
																 LDK_THREAD_DESCRIPTION_ENTRY,
																 Links );
		if (Entry->ThreadId == ThreadId) {
			return Entry;
		}
	}

	return NULL;
}

VOID
LdkpFreeThreadDescriptionEntry (
	_In_ PLDK_THREAD_DESCRIPTION_ENTRY Entry
	)
{
	PAGED_CODE();

	if (Entry->Description != NULL) {
		RtlFreeHeap( RtlProcessHeap(),
					 0,
					 Entry->Description );
	}

	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 Entry );
}

HRESULT
LdkpHResultFromWin32Error (
	_In_ DWORD ErrorCode
	)
{
	if (ErrorCode == ERROR_SUCCESS) {
		return LDK_S_OK;
	}

	if (ErrorCode & 0x80000000) {
		return (HRESULT)ErrorCode;
	}

	return (HRESULT)(((ErrorCode) & 0x0000FFFF) |
					 (LDK_HRESULT_FACILITY_WIN32 << 16) |
					 0x80000000);
}

HRESULT
LdkpHResultFromNtStatus (
	_In_ NTSTATUS Status
	)
{
	return LdkpHResultFromWin32Error( RtlNtStatusToDosError( Status ) );
}

_Function_class_(EXPAND_STACK_CALLOUT)
VOID
NTAPI
LdkpThreadStartExpandStackAndCallout (
    _In_ PLDK_THREAD_CALLOUT_CONTEXT Context
    )
{
	PAGED_CODE();

	LPVOID lpThreadParameter = Context->lpThreadParameter;
	PTHREAD_START_ROUTINE ThreadStartRoutine = Context->ThreadStartRoutine;

	LdkpCallThreadNotifications( DLL_THREAD_ATTACH );
	Context->ExitCode = ThreadStartRoutine( lpThreadParameter );
	LdkpCallThreadNotifications( DLL_THREAD_DETACH );

	LdkpInvokeFlsCallback( NtCurrentTeb() );
}

_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
VOID
LdkpThreadStartRoutine (
    _In_ PLDK_THREAD_CONTEXT Context
    )
{
	LPVOID lpThreadParameter;
	PTHREAD_START_ROUTINE ThreadStartRoutine;
	SIZE_T dwStackSize;
	NTSTATUS Status;

	PAGED_CODE();

	lpThreadParameter = Context->lpThreadParameter;
	ThreadStartRoutine = Context->ThreadStartRoutine;
	dwStackSize = Context->dwStackSize;

	if (FlagOn(Context->dwCreationFlags, CREATE_SUSPENDED)) {
		Status = KeWaitForSingleObject( &Context->StartEvent,
										Executive,
										KernelMode,
										FALSE,
										NULL );
		LdkpRemoveThreadContext( Context );
		if (! NT_SUCCESS(Status)) {
			ExFreeToNPagedLookasideList( &LdkpThreadContextLookaside,
										 Context );
			PsTerminateSystemThread( Status );
			return;
		}
	}

	if (dwStackSize > IoGetRemainingStackSize()) {
		LDK_THREAD_CALLOUT_CONTEXT CalloutContext;

		CalloutContext.ThreadStartRoutine = ThreadStartRoutine;
		CalloutContext.lpThreadParameter = lpThreadParameter;
		CalloutContext.ExitCode = 0;

		ExFreeToNPagedLookasideList( &LdkpThreadContextLookaside,
									 Context );

		if (NT_SUCCESS(KeExpandKernelStackAndCallout( LdkpThreadStartExpandStackAndCallout,
													  &CalloutContext,
													  dwStackSize ))) {
			DWORD ExitCode = CalloutContext.ExitCode;
			PsTerminateSystemThread( (NTSTATUS)ExitCode );
			return;
		}
	} else {
		ExFreeToNPagedLookasideList( &LdkpThreadContextLookaside,
									 Context );
	}

	LdkpCallThreadNotifications( DLL_THREAD_ATTACH );
	DWORD ExitCode = ThreadStartRoutine( lpThreadParameter );
	LdkpCallThreadNotifications( DLL_THREAD_DETACH );

	LdkpInvokeFlsCallback( NtCurrentTeb() );

	PsTerminateSystemThread( (NTSTATUS)ExitCode );
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateThread (
    _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_ SIZE_T dwStackSize,
    _In_ LPTHREAD_START_ROUTINE lpStartAddress,
    _In_opt_ __drv_aliasesMem LPVOID lpParameter,
    _In_ DWORD dwCreationFlags,
    _Out_opt_ LPDWORD lpThreadId
    )
{
	PAGED_CODE();

	BOOLEAN CreateSuspended = FlagOn(dwCreationFlags, CREATE_SUSPENDED);
	OBJECT_ATTRIBUTES ObjectAttributes;
	InitializeObjectAttributes( &ObjectAttributes,
								NULL,
								OBJ_KERNEL_HANDLE,
								NULL,
								NULL );

	if (ARGUMENT_PRESENT(lpThreadAttributes)) {
		ObjectAttributes.SecurityDescriptor = lpThreadAttributes->lpSecurityDescriptor;
		if (lpThreadAttributes->bInheritHandle) {
			ObjectAttributes.Attributes |= OBJ_INHERIT;
		}
	}

	PLDK_THREAD_CONTEXT Context = ExAllocateFromNPagedLookasideList( &LdkpThreadContextLookaside );
	if (! Context) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}
	RtlZeroMemory( Context,
				   sizeof(*Context) );
	InitializeListHead( &Context->Links );
	KeInitializeEvent( &Context->StartEvent,
					   NotificationEvent,
					   ! CreateSuspended );
	Context->dwCreationFlags = dwCreationFlags;
	Context->dwStackSize = dwStackSize;
	Context->ThreadStartRoutine = lpStartAddress;
	Context->lpThreadParameter = lpParameter;
	Context->ExitCode = 0;
	Context->StartupSuspendCount = CreateSuspended ? 1 : 0;

	HANDLE ThreadHandle;
	CLIENT_ID ClientId;
	NTSTATUS Status = PsCreateSystemThread( &ThreadHandle,
											THREAD_ALL_ACCESS,
											&ObjectAttributes,
											NULL,
											&ClientId,
											LdkpThreadStartRoutine,
											Context );
	if (! NT_SUCCESS(Status)) {
		ExFreeToNPagedLookasideList( &LdkpThreadContextLookaside,
									 Context );
		LdkSetLastNTError( Status );
		return NULL;
	}
	
	if (ARGUMENT_PRESENT(lpThreadId)) {
		*lpThreadId = HandleToUlong(ClientId.UniqueThread);
	}

	if (CreateSuspended) {
		Context->ThreadId = ClientId.UniqueThread;
		LdkpInsertThreadContext( Context );
	}

	return ThreadHandle;
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
OpenThread (
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwThreadId
    )
{
	PAGED_CODE();

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes( &ObjectAttributes,
								NULL,
        						(bInheritHandle ? OBJ_INHERIT : 0) | OBJ_KERNEL_HANDLE,
        						NULL,
								NULL );
    CLIENT_ID ClientId;
    ClientId.UniqueThread = LongToHandle(dwThreadId);
    ClientId.UniqueProcess = NULL;
    HANDLE Handle;
    NTSTATUS Status = ZwOpenThread( &Handle,
						   			(ACCESS_MASK)dwDesiredAccess,
						   			&ObjectAttributes,
						   			&ClientId );
    if (NT_SUCCESS(Status)) {
		return Handle;
    }
	LdkSetLastNTError( Status );
	return NULL;
}

WINBASEAPI
DWORD
WINAPI
SuspendThread (
    _In_ HANDLE hThread
    )
{
	PAGED_CODE();

	HANDLE ThreadId;
	NTSTATUS Status = LdkpGetThreadIdFromHandle( hThread,
												 &ThreadId );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return (DWORD)-1;
	}

	ExAcquireFastMutex( &LdkpThreadContextListMutex );

	PLDK_THREAD_CONTEXT Context = LdkpFindThreadContextByIdLocked( ThreadId );
	if (! Context) {
		ExReleaseFastMutex( &LdkpThreadContextListMutex );
		SetLastError( ERROR_NOT_SUPPORTED );
		return (DWORD)-1;
	}

	LONG PreviousCount = Context->StartupSuspendCount;
	if (Context->StartReleased ||
		PreviousCount == 0) {
		ExReleaseFastMutex( &LdkpThreadContextListMutex );
		SetLastError( ERROR_NOT_SUPPORTED );
		return (DWORD)-1;
	}

	if (PreviousCount >= MAXIMUM_SUSPEND_COUNT) {
		ExReleaseFastMutex( &LdkpThreadContextListMutex );
		SetLastError( ERROR_INVALID_PARAMETER );
		return (DWORD)-1;
	}

	Context->StartupSuspendCount = PreviousCount + 1;

	ExReleaseFastMutex( &LdkpThreadContextListMutex );
	return (DWORD)PreviousCount;
}

WINBASEAPI
DWORD
WINAPI
ResumeThread (
    _In_ HANDLE hThread
    )
{
	PAGED_CODE();

	HANDLE ThreadId;
	NTSTATUS Status = LdkpGetThreadIdFromHandle( hThread,
												 &ThreadId );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return (DWORD)-1;
	}

	ExAcquireFastMutex( &LdkpThreadContextListMutex );

	PLDK_THREAD_CONTEXT Context = LdkpFindThreadContextByIdLocked( ThreadId );
	if (! Context) {
		ExReleaseFastMutex( &LdkpThreadContextListMutex );
		return 0;
	}

	LONG PreviousCount = Context->StartupSuspendCount;
	if (PreviousCount > 0) {
		Context->StartupSuspendCount = PreviousCount - 1;
		if (Context->StartupSuspendCount == 0) {
			Context->StartReleased = TRUE;
			KeSetEvent( &Context->StartEvent,
						IO_NO_INCREMENT,
						FALSE );
		}
	}

	ExReleaseFastMutex( &LdkpThreadContextListMutex );
	return (DWORD)PreviousCount;
}

WINBASEAPI
int
WINAPI
GetThreadPriority (
    _In_ HANDLE hThread
    )
{
	PAGED_CODE();

	THREAD_BASIC_INFORMATION BasicInformation;
	NTSTATUS Status = ZwQueryInformationThread( hThread,
									   			ThreadBasicInformation,
									   			&BasicInformation,
									   			sizeof(BasicInformation),
									   			NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return (int)THREAD_PRIORITY_ERROR_RETURN;
	}
	int BasePriority = (int)BasicInformation.BasePriority;
	if (BasePriority == ((HIGH_PRIORITY + 1) / 2)) {
		BasePriority = THREAD_PRIORITY_TIME_CRITICAL;
	} else if ( BasePriority == -((HIGH_PRIORITY + 1) / 2) ) {
		BasePriority = THREAD_PRIORITY_IDLE;
	}
	return BasePriority;
}

WINBASEAPI
BOOL
WINAPI
SetThreadPriority (
    _In_ HANDLE hThread,
    _In_ int nPriority
    )
{
	PAGED_CODE();

    LONG BasePriority= (LONG)nPriority;
    if (BasePriority == THREAD_PRIORITY_TIME_CRITICAL) {
        BasePriority = ((HIGH_PRIORITY + 1) / 2);
    }
    else if (BasePriority == THREAD_PRIORITY_IDLE ) {
        BasePriority = -((HIGH_PRIORITY + 1) / 2);
    }
    
    NTSTATUS Status = ZwSetInformationThread( hThread,
									 		  ThreadBasePriority,
									 		  &BasePriority,
									 		  sizeof(BasePriority) );
    if (NT_SUCCESS(Status)) {
		return TRUE;
	}
	LdkSetLastNTError(Status);
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetThreadPriorityBoost (
    _In_ HANDLE hThread,
    _Out_ PBOOL pDisablePriorityBoost
    )
{
	PAGED_CODE();
    
    ULONG DisableBoost;
	NTSTATUS Status = ZwQueryInformationThread( hThread,
									  		    ThreadPriorityBoost,
									   			&DisableBoost,
									   			sizeof(DisableBoost),
									   			NULL );
	if (NT_SUCCESS(Status)) {
		*pDisablePriorityBoost = DisableBoost;
    	return TRUE;
	}
	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
SetThreadPriorityBoost (
    _In_ HANDLE hThread,
    _In_ BOOL bDisablePriorityBoost
    )
{
	PAGED_CODE();

    ULONG DisableBoost = bDisablePriorityBoost ? TRUE : FALSE;

    NTSTATUS Status = ZwSetInformationThread( hThread,
											  ThreadPriorityBoost,
									 		  &DisableBoost,
									 		  sizeof(DisableBoost) );
    if (NT_SUCCESS(Status)) {
		return TRUE;
	}
	LdkSetLastNTError(Status);
	return FALSE;
}

WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
ExitThread (
    _In_ DWORD dwExitCode
    )
{
	PAGED_CODE();

	LdkpCallThreadNotifications( DLL_THREAD_DETACH );
	LdkpInvokeFlsCallback( NtCurrentTeb() );

	PsTerminateSystemThread( (NTSTATUS)dwExitCode );
}

WINBASEAPI
HANDLE
WINAPI
GetCurrentThread (
    VOID
    )
{
	return ZwCurrentThread();
}

WINBASEAPI
DWORD
WINAPI
GetCurrentThreadId (
    VOID
    )
{
	return (DWORD)HandleToULong(PsGetCurrentThreadId());
}

WINBASEAPI
DWORD
WINAPI
GetThreadId (
    _In_ HANDLE Thread
    )
{
	HANDLE ThreadId;
	NTSTATUS Status;

	PAGED_CODE();

	Status = LdkpGetThreadIdFromHandle( Thread,
										&ThreadId );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return 0;
	}

	return HandleToULong( ThreadId );
}

WINBASEAPI
BOOL
WINAPI
GetThreadGroupAffinity (
    _In_ HANDLE hThread,
    _Out_ PGROUP_AFFINITY GroupAffinity
    )
{
	PAGED_CODE();

	if (GroupAffinity == NULL) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (! LdkpValidateThreadHandleForAffinity( hThread )) {
		return FALSE;
	}

	LdkpQueryCurrentThreadGroupAffinity( GroupAffinity );
	return TRUE;
}

WINBASEAPI
BOOL
WINAPI
SetThreadGroupAffinity (
    _In_ HANDLE hThread,
    _In_ CONST GROUP_AFFINITY *GroupAffinity,
    _Out_opt_ PGROUP_AFFINITY PreviousGroupAffinity
    )
{
	PAGED_CODE();

	if (! LdkpValidateThreadGroupAffinity( GroupAffinity )) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (! LdkpValidateThreadHandleForAffinity( hThread )) {
		return FALSE;
	}

	if (PreviousGroupAffinity != NULL) {
		LdkpQueryCurrentThreadGroupAffinity( PreviousGroupAffinity );
	}

	return TRUE;
}

WINBASEAPI
HRESULT
WINAPI
SetThreadDescription (
    _In_ HANDLE hThread,
    _In_ PCWSTR lpThreadDescription
    )
{
	PAGED_CODE();

	if (lpThreadDescription == NULL) {
		return LDK_E_INVALIDARG;
	}

	HANDLE ThreadId;
	NTSTATUS Status = LdkpGetThreadIdFromHandle( hThread,
												 &ThreadId );
	if (! NT_SUCCESS(Status)) {
		return LdkpHResultFromNtStatus( Status );
	}

	UNICODE_STRING DescriptionString;
	RtlInitUnicodeString( &DescriptionString,
						  lpThreadDescription );

	SIZE_T DescriptionBytes = DescriptionString.Length + sizeof(WCHAR);
	PWSTR NewDescription = RtlAllocateHeap( RtlProcessHeap(),
											0,
											DescriptionBytes );
	if (NewDescription == NULL) {
		return LDK_E_OUTOFMEMORY;
	}

	RtlCopyMemory( NewDescription,
				   lpThreadDescription,
				   DescriptionString.Length );
	NewDescription[DescriptionString.Length / sizeof(WCHAR)] = UNICODE_NULL;

	PLDK_THREAD_DESCRIPTION_ENTRY NewEntry = RtlAllocateHeap( RtlProcessHeap(),
															  HEAP_ZERO_MEMORY,
															  sizeof(*NewEntry) );
	if (NewEntry == NULL) {
		RtlFreeHeap( RtlProcessHeap(),
					 0,
					 NewDescription );
		return LDK_E_OUTOFMEMORY;
	}

	InitializeListHead( &NewEntry->Links );
	NewEntry->ThreadId = ThreadId;

	PWSTR OldDescription = NULL;
	ExAcquireFastMutex( &LdkpThreadDescriptionListMutex );

	PLDK_THREAD_DESCRIPTION_ENTRY Entry = LdkpFindThreadDescriptionByIdLocked( ThreadId );
	if (Entry == NULL) {
		Entry = NewEntry;
		NewEntry = NULL;
		InsertTailList( &LdkpThreadDescriptionListHead,
						&Entry->Links );
	}

	OldDescription = Entry->Description;
	Entry->Description = NewDescription;
	Entry->DescriptionBytes = DescriptionBytes;

	ExReleaseFastMutex( &LdkpThreadDescriptionListMutex );

	if (OldDescription != NULL) {
		RtlFreeHeap( RtlProcessHeap(),
					 0,
					 OldDescription );
	}

	if (NewEntry != NULL) {
		LdkpFreeThreadDescriptionEntry( NewEntry );
	}

	return LDK_S_OK;
}

WINBASEAPI
HRESULT
WINAPI
GetThreadDescription (
    _In_ HANDLE hThread,
    _Outptr_result_z_ PWSTR *ppszThreadDescription
    )
{
	PAGED_CODE();

	if (ppszThreadDescription == NULL) {
		return LDK_E_INVALIDARG;
	}

	*ppszThreadDescription = NULL;

	HANDLE ThreadId;
	NTSTATUS Status = LdkpGetThreadIdFromHandle( hThread,
												 &ThreadId );
	if (! NT_SUCCESS(Status)) {
		return LdkpHResultFromNtStatus( Status );
	}

	for (;;) {
		SIZE_T DescriptionBytes;

		ExAcquireFastMutex( &LdkpThreadDescriptionListMutex );
		PLDK_THREAD_DESCRIPTION_ENTRY Entry = LdkpFindThreadDescriptionByIdLocked( ThreadId );
		DescriptionBytes = (Entry != NULL) ? Entry->DescriptionBytes : sizeof(WCHAR);
		ExReleaseFastMutex( &LdkpThreadDescriptionListMutex );

		PWSTR Description = LocalAlloc( LMEM_FIXED,
										DescriptionBytes );
		if (Description == NULL) {
			return LDK_E_OUTOFMEMORY;
		}

		ExAcquireFastMutex( &LdkpThreadDescriptionListMutex );
		Entry = LdkpFindThreadDescriptionByIdLocked( ThreadId );
		SIZE_T CurrentBytes = (Entry != NULL) ? Entry->DescriptionBytes : sizeof(WCHAR);
		if (CurrentBytes == DescriptionBytes) {
			if (Entry != NULL) {
				RtlCopyMemory( Description,
							   Entry->Description,
							   DescriptionBytes );
			} else {
				Description[0] = UNICODE_NULL;
			}
			ExReleaseFastMutex( &LdkpThreadDescriptionListMutex );
			*ppszThreadDescription = Description;
			return LDK_S_OK;
		}
		ExReleaseFastMutex( &LdkpThreadDescriptionListMutex );

		LocalFree( Description );
	}
}

WINBASEAPI
_Success_(return != 0)
BOOL
WINAPI
GetExitCodeThread (
    _In_ HANDLE hThread,
    _Out_ LPDWORD lpExitCode
    )
{
	PAGED_CODE();

	THREAD_BASIC_INFORMATION BasicInformation;
	NTSTATUS Status = ZwQueryInformationThread( hThread,
									   			ThreadBasicInformation,
									   			&BasicInformation,
									   			sizeof(BasicInformation),
									   			NULL );
	if (NT_SUCCESS(Status)) {
		*lpExitCode = BasicInformation.ExitStatus;
		return TRUE;
	}
	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetThreadTimes (
    _In_ HANDLE hThread,
    _Out_ LPFILETIME lpCreationTime,
    _Out_ LPFILETIME lpExitTime,
    _Out_ LPFILETIME lpKernelTime,
    _Out_ LPFILETIME lpUserTime
    )
{
	PAGED_CODE();
	KERNEL_USER_TIMES TimeInfo;
	NTSTATUS Status = ZwQueryInformationThread( hThread,
												ThreadTimes,
									  			(PVOID)&TimeInfo,
									  			sizeof(TimeInfo),
									  			NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FALSE;
	}
	*lpCreationTime = *(LPFILETIME)&TimeInfo.CreateTime;
	*lpExitTime = *(LPFILETIME)&TimeInfo.ExitTime;
	*lpKernelTime = *(LPFILETIME)&TimeInfo.KernelTime;
	*lpUserTime = *(LPFILETIME)&TimeInfo.UserTime;
	return TRUE;
}

WINBASEAPI
BOOL
WINAPI
GetProcessTimes (
    _In_ HANDLE hProcess,
    _Out_ LPFILETIME lpCreationTime,
    _Out_ LPFILETIME lpExitTime,
    _Out_ LPFILETIME lpKernelTime,
    _Out_ LPFILETIME lpUserTime
    )
{
	HANDLE NativeProcessHandle;
	KERNEL_USER_TIMES TimeInfo;
	NTSTATUS Status;

	PAGED_CODE();

	if (! LdkResolveProcessHandleForNativeQuery( hProcess,
												 &NativeProcessHandle )) {
		return FALSE;
	}

	Status = ZwQueryInformationProcess( NativeProcessHandle,
										ProcessTimes,
										(PVOID)&TimeInfo,
										sizeof(TimeInfo),
										NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FALSE;
	}

	*lpCreationTime = *(LPFILETIME)&TimeInfo.CreateTime;
	*lpExitTime = *(LPFILETIME)&TimeInfo.ExitTime;
	*lpKernelTime = *(LPFILETIME)&TimeInfo.KernelTime;
	*lpUserTime = *(LPFILETIME)&TimeInfo.UserTime;
	return TRUE;
}

WINBASEAPI
VOID
WINAPI
GetCurrentThreadStackLimits (
    _Out_ PULONG_PTR LowLimit,
    _Out_ PULONG_PTR HighLimit
    )
{
	PAGED_CODE();

	IoGetStackLimits( LowLimit,
					  HighLimit );
}

WINBASEAPI
DWORD
WINAPI
TlsAlloc (
	VOID
	)
{
	DWORD Index = TLS_OUT_OF_INDEXES;

	PAGED_CODE();

	RtlAcquirePebLock();
	try {
		Index = RtlFindClearBitsAndSet( &NtCurrentPeb()->TlsBitmap,
										1,
										0 );
		if (Index == TLS_OUT_OF_INDEXES) {
			LdkSetLastNTError( STATUS_NO_MEMORY );
		} else {
			InterlockedExchangePointer( &NtCurrentTeb()->TlsSlots[Index],
										NULL );
		}
	} finally {
		RtlReleasePebLock();
	}

	return Index;
}

WINBASEAPI
LPVOID
WINAPI
TlsGetValue (
    _In_ DWORD dwTlsIndex
    )
{
	PVOID Value;

	if (dwTlsIndex >= LDK_TLS_SLOTS_SIZE) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	Value = NtCurrentTeb()->TlsSlots[dwTlsIndex];
	SetLastError( ERROR_SUCCESS );
	return Value;
}

WINBASEAPI
BOOL
WINAPI
TlsSetValue (
    _In_ DWORD dwTlsIndex,
    _In_opt_ LPVOID lpTlsValue
    )
{
	if (dwTlsIndex >= LDK_TLS_SLOTS_SIZE) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	InterlockedExchangePointer( &NtCurrentTeb()->TlsSlots[dwTlsIndex],
								lpTlsValue );
	return TRUE;
}

WINBASEAPI
BOOL
WINAPI
TlsFree (
    _In_ DWORD dwTlsIndex
    )
{
	BOOLEAN Found = FALSE;

	PAGED_CODE();

	RtlAcquirePebLock();
	try {
		if (dwTlsIndex >= LDK_TLS_SLOTS_SIZE) {
			leave;
		}

		Found = RtlAreBitsSet( &NtCurrentPeb()->TlsBitmap,
							   dwTlsIndex,
							   1 );
		if (Found) {
			PTEB InitialTeb = NtCurrentTeb();
			if (! ExAcquireRundownProtection( &InitialTeb->RundownProtect )) {
				Found = FALSE;
				leave;
			}

			PTEB Teb = InitialTeb;
			do {
				InterlockedExchangePointer( &Teb->TlsSlots[dwTlsIndex],
											NULL );
				Teb = LdkGetNextTebRundownProtection( Teb );
			} while (Teb && Teb != InitialTeb);

			if (Teb == InitialTeb) {
				ExReleaseRundownProtection( &Teb->RundownProtect );
			}

			RtlClearBits( &NtCurrentPeb()->TlsBitmap,
						  dwTlsIndex,
						  1 );
		}
	} finally {
		RtlReleasePebLock();
	}

	if (! Found) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	return TRUE;
}

WINBASEAPI
BOOL
WINAPI
SwitchToThread (
    VOID
    )
{
	PAGED_CODE();

    return ZwYieldExecution() != STATUS_NO_YIELD_PERFORMED;
}



WINBASEAPI
HANDLE
WINAPI
GetCurrentProcess (
    VOID
    )
{
	return ZwCurrentProcess();
}

WINBASEAPI
DWORD
WINAPI
GetCurrentProcessId (
    VOID
    )
{
	return LdkCurrentPeb()->ProcessId;
}

WINBASEAPI
DWORD
WINAPI
GetProcessId (
    _In_ HANDLE Process
    )
{
	BOOL Handled;
	DWORD ProcessId;
	PROCESS_BASIC_INFORMATION BasicInformation;
	NTSTATUS Status;

	PAGED_CODE();

	if (LdkGetProcessIdHandle( Process,
							   &ProcessId,
							   &Handled )) {
		return ProcessId;
	}
	if (Handled) {
		return 0;
	}

	Status = ZwQueryInformationProcess( Process,
										ProcessBasicInformation,
										&BasicInformation,
										sizeof(BasicInformation),
										NULL );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return 0;
	}

	return (DWORD)(ULONG_PTR)BasicInformation.UniqueProcessId;
}

WINBASEAPI
BOOL
WINAPI
GetProcessAffinityMask (
    _In_ HANDLE hProcess,
    _Out_ PDWORD_PTR lpProcessAffinityMask,
    _Out_ PDWORD_PTR lpSystemAffinityMask
    )
{
	HANDLE NativeProcessHandle;
	DWORD_PTR ActiveMask;

	PAGED_CODE();

	if (lpProcessAffinityMask == NULL ||
		lpSystemAffinityMask == NULL) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (! LdkResolveProcessHandleForNativeQuery( hProcess,
												 &NativeProcessHandle )) {
		return FALSE;
	}

	UNREFERENCED_PARAMETER( NativeProcessHandle );

	ActiveMask = LdkpQueryPrimaryProcessorAffinityMask();
	*lpProcessAffinityMask = ActiveMask;
	*lpSystemAffinityMask = ActiveMask;
	return TRUE;
}

WINBASEAPI
BOOL
WINAPI
SetProcessAffinityMask (
    _In_ HANDLE hProcess,
    _In_ DWORD_PTR dwProcessAffinityMask
    )
{
	BOOL Handled;

	PAGED_CODE();

	if (! LdkpValidatePrimaryProcessorAffinityMask( dwProcessAffinityMask )) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (LdkValidateProcessHandleForSetInformation( hProcess,
												   &Handled )) {
		return TRUE;
	}
	if (Handled) {
		return FALSE;
	}

	SetLastError( ERROR_NOT_SUPPORTED );
	return FALSE;
}

BOOL
LdkpCopyProcessImageNameA (
	_In_ PCANSI_STRING Source,
	_Out_writes_to_(*Size, *Size) LPSTR Buffer,
	_Inout_ PDWORD Size
	)
{
	DWORD Length;

	PAGED_CODE();

	if (Source == NULL ||
		Source->Buffer == NULL ||
		Source->Length == 0) {
		SetLastError( ERROR_NOT_FOUND );
		return FALSE;
	}

	if (Buffer == NULL ||
		Size == NULL ||
		*Size == 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	Length = Source->Length;
	if (*Size <= Length) {
		*Size = Length + 1;
		SetLastError( ERROR_INSUFFICIENT_BUFFER );
		return FALSE;
	}

	RtlCopyMemory( Buffer,
				   Source->Buffer,
				   Length );
	Buffer[Length] = ANSI_NULL;
	*Size = Length;
	return TRUE;
}

BOOL
LdkpCopyProcessImageNameW (
	_In_ PCUNICODE_STRING Source,
	_Out_writes_to_(*Size, *Size) LPWSTR Buffer,
	_Inout_ PDWORD Size
	)
{
	DWORD Length;

	PAGED_CODE();

	if (Source == NULL ||
		Source->Buffer == NULL ||
		Source->Length == 0) {
		SetLastError( ERROR_NOT_FOUND );
		return FALSE;
	}

	if (Buffer == NULL ||
		Size == NULL ||
		*Size == 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	Length = Source->Length / sizeof(WCHAR);
	if (*Size <= Length) {
		*Size = Length + 1;
		SetLastError( ERROR_INSUFFICIENT_BUFFER );
		return FALSE;
	}

	RtlCopyMemory( Buffer,
				   Source->Buffer,
				   Source->Length );
	Buffer[Length] = UNICODE_NULL;
	*Size = Length;
	return TRUE;
}

NTSTATUS
LdkpQueryNativeProcessImageName (
	_In_ HANDLE ProcessHandle,
	_Outptr_ PUNICODE_STRING *ImageName,
	_Outptr_ PVOID *FreeBuffer
	)
{
	NTSTATUS Status;
	ULONG ReturnLength;
	ULONG BufferLength;
	PVOID Buffer;

	PAGED_CODE();

	*ImageName = NULL;
	*FreeBuffer = NULL;
	ReturnLength = 0;
	BufferLength = sizeof(UNICODE_STRING) + (MAX_PATH * sizeof(WCHAR));

	for (;;) {
		Buffer = RtlAllocateHeap( RtlProcessHeap(),
								  HEAP_ZERO_MEMORY,
								  BufferLength );
		if (Buffer == NULL) {
			return STATUS_INSUFFICIENT_RESOURCES;
		}

		Status = ZwQueryInformationProcess( ProcessHandle,
											LDK_PROCESS_IMAGE_FILE_NAME_CLASS,
											Buffer,
											BufferLength,
											&ReturnLength );
		if (NT_SUCCESS(Status)) {
			*ImageName = (PUNICODE_STRING)Buffer;
			*FreeBuffer = Buffer;
			return STATUS_SUCCESS;
		}

		RtlFreeHeap( RtlProcessHeap(),
					 0,
					 Buffer );

		if ((Status != STATUS_INFO_LENGTH_MISMATCH) &&
			(Status != STATUS_BUFFER_OVERFLOW) &&
			(Status != STATUS_BUFFER_TOO_SMALL)) {
			return Status;
		}

		if (ReturnLength <= BufferLength) {
			return Status;
		}

		BufferLength = ReturnLength;
	}
}

WINBASEAPI
BOOL
WINAPI
QueryFullProcessImageNameA (
    _In_ HANDLE hProcess,
    _In_ DWORD dwFlags,
    _Out_writes_to_(*lpdwSize, *lpdwSize) LPSTR lpExeName,
    _Inout_ PDWORD lpdwSize
    )
{
	BOOL Handled;
	PCUNICODE_STRING ImageName;
	ANSI_STRING NativeImageName;
	ANSI_STRING ImageNameA;
	NTSTATUS Status;
	HANDLE NativeProcessHandle;
	PUNICODE_STRING NativeImageNameW;
	PVOID NativeImageNameBuffer;
	BOOL Result;

	PAGED_CODE();

	if ((dwFlags & ~LDK_QUERY_FULL_PROCESS_IMAGE_NAME_SUPPORTED_FLAGS) != 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (lpdwSize == NULL ||
		lpExeName == NULL ||
		*lpdwSize == 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (LdkGetProcessImageNameHandle( hProcess,
									  dwFlags,
									  &ImageName,
									  &NativeImageName,
									  &Handled )) {
		if (dwFlags == PROCESS_NAME_NATIVE) {
			return LdkpCopyProcessImageNameA( &NativeImageName,
											  lpExeName,
											  lpdwSize );
		}

		RtlZeroMemory( &ImageNameA,
					   sizeof(ImageNameA) );
		Status = LdkUnicodeStringToAnsiString( &ImageNameA,
											   ImageName,
											   TRUE );
		if (! NT_SUCCESS(Status)) {
			LdkSetLastNTError( Status );
			return FALSE;
		}

		Result = LdkpCopyProcessImageNameA( &ImageNameA,
											lpExeName,
											lpdwSize );
		LdkFreeAnsiString( &ImageNameA );
		return Result;
	}
	if (Handled) {
		return FALSE;
	}

	if (dwFlags != PROCESS_NAME_NATIVE) {
		SetLastError( ERROR_NOT_SUPPORTED );
		return FALSE;
	}

	if (! LdkResolveProcessHandleForNativeQuery( hProcess,
												 &NativeProcessHandle )) {
		return FALSE;
	}

	Status = LdkpQueryNativeProcessImageName( NativeProcessHandle,
											 &NativeImageNameW,
											 &NativeImageNameBuffer );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FALSE;
	}

	RtlZeroMemory( &ImageNameA,
				   sizeof(ImageNameA) );
	Status = LdkUnicodeStringToAnsiString( &ImageNameA,
										   NativeImageNameW,
										   TRUE );
	if (! NT_SUCCESS(Status)) {
		RtlFreeHeap( RtlProcessHeap(),
					 0,
					 NativeImageNameBuffer );
		LdkSetLastNTError( Status );
		return FALSE;
	}

	Result = LdkpCopyProcessImageNameA( &ImageNameA,
										lpExeName,
										lpdwSize );
	LdkFreeAnsiString( &ImageNameA );
	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 NativeImageNameBuffer );
	return Result;
}

WINBASEAPI
BOOL
WINAPI
QueryFullProcessImageNameW (
    _In_ HANDLE hProcess,
    _In_ DWORD dwFlags,
    _Out_writes_to_(*lpdwSize, *lpdwSize) LPWSTR lpExeName,
    _Inout_ PDWORD lpdwSize
    )
{
	BOOL Handled;
	PCUNICODE_STRING ImageName;
	ANSI_STRING NativeImageName;
	UNICODE_STRING NativeImageNameW;
	NTSTATUS Status;
	HANDLE NativeProcessHandle;
	PUNICODE_STRING QueriedImageName;
	PVOID QueriedImageNameBuffer;
	BOOL Result;

	PAGED_CODE();

	if ((dwFlags & ~LDK_QUERY_FULL_PROCESS_IMAGE_NAME_SUPPORTED_FLAGS) != 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (lpdwSize == NULL ||
		lpExeName == NULL ||
		*lpdwSize == 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return FALSE;
	}

	if (LdkGetProcessImageNameHandle( hProcess,
									  dwFlags,
									  &ImageName,
									  &NativeImageName,
									  &Handled )) {
		if (dwFlags == PROCESS_NAME_NATIVE) {
			RtlZeroMemory( &NativeImageNameW,
						   sizeof(NativeImageNameW) );
			Status = LdkAnsiStringToUnicodeString( &NativeImageNameW,
												   &NativeImageName,
												   TRUE );
			if (! NT_SUCCESS(Status)) {
				LdkSetLastNTError( Status );
				return FALSE;
			}

			Result = LdkpCopyProcessImageNameW( &NativeImageNameW,
												lpExeName,
												lpdwSize );
			LdkFreeUnicodeString( &NativeImageNameW );
			return Result;
		}

		return LdkpCopyProcessImageNameW( ImageName,
										  lpExeName,
										  lpdwSize );
	}
	if (Handled) {
		return FALSE;
	}

	if (dwFlags != PROCESS_NAME_NATIVE) {
		SetLastError( ERROR_NOT_SUPPORTED );
		return FALSE;
	}

	if (! LdkResolveProcessHandleForNativeQuery( hProcess,
												 &NativeProcessHandle )) {
		return FALSE;
	}

	Status = LdkpQueryNativeProcessImageName( NativeProcessHandle,
											 &QueriedImageName,
											 &QueriedImageNameBuffer );
	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FALSE;
	}

	Result = LdkpCopyProcessImageNameW( QueriedImageName,
										lpExeName,
										lpdwSize );
	RtlFreeHeap( RtlProcessHeap(),
				 0,
				 QueriedImageNameBuffer );
	return Result;
}

WINBASEAPI
HANDLE
WINAPI
OpenProcess (
    _In_ DWORD dwDesiredAccess,
    _In_ BOOL bInheritHandle,
    _In_ DWORD dwProcessId
    )
{
	if (dwProcessId == 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	if (dwProcessId == LdkCurrentPeb()->ProcessId) {
		return LdkCreateCurrentProcessHandle( dwDesiredAccess,
											  bInheritHandle );
	}

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes( &ObjectAttributes,
								NULL,
        						(bInheritHandle ? OBJ_INHERIT : 0) | OBJ_KERNEL_HANDLE,
        						NULL,
								NULL );
    CLIENT_ID ClientId;
    ClientId.UniqueThread = (HANDLE)NULL;
    ClientId.UniqueProcess = (HANDLE)(ULONG_PTR)dwProcessId;
    HANDLE Handle;
    NTSTATUS Status = ZwOpenProcess( &Handle,
									 (ACCESS_MASK)dwDesiredAccess,
									 &ObjectAttributes,
									 &ClientId );
    if (NT_SUCCESS(Status)) {
		return Handle;
    }
	LdkSetLastNTError( Status );
	return NULL;
}

WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
ExitProcess (
    _In_ UINT uExitCode
    )
{
	PAGED_CODE();

	BOOL Handled;

	LdkTerminateProcessHandle( NtCurrentProcess(),
							   uExitCode,
							   &Handled );

	ExitThread( uExitCode );
}

WINBASEAPI
BOOL
WINAPI
TerminateProcess (
    _In_ HANDLE hProcess,
    _In_ UINT uExitCode
    )
{
	PAGED_CODE();

	BOOL Handled;

	if (LdkTerminateProcessHandle( hProcess,
								   uExitCode,
								   &Handled )) {
		return TRUE;
	}
	if (Handled) {
		return FALSE;
	}

	if (hProcess == NULL) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	NTSTATUS Status = ZwTerminateProcess( hProcess,
										  (NTSTATUS)uExitCode );
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}
	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetExitCodeProcess (
    _In_ HANDLE hProcess,
    _Out_ LPDWORD lpExitCode
    )
{
	PAGED_CODE();

	BOOL Handled;

	if (LdkGetExitCodeProcessHandle( hProcess,
									 lpExitCode,
									 &Handled )) {
		return TRUE;
	}
	if (Handled) {
		return FALSE;
	}

	PROCESS_BASIC_INFORMATION BasicInformation;
	NTSTATUS Status = ZwQueryInformationProcess( hProcess,
												 ProcessBasicInformation,
												 &BasicInformation,
												 sizeof(BasicInformation),
												 NULL );
	if (NT_SUCCESS(Status)) {
		*lpExitCode = BasicInformation.ExitStatus;
		return TRUE;
	}
	LdkSetLastNTError( Status );
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
IsProcessorFeaturePresent (
	_In_ DWORD ProcessorFeature
	)
{
	PAGED_CODE();

	return (BOOL)ExIsProcessorFeaturePresent( ProcessorFeature );
}



WINBASEAPI
VOID
WINAPI
GetStartupInfoA (
    _Out_ LPSTARTUPINFOA lpStartupInfo
    )
{
	RtlZeroMemory( lpStartupInfo,
				   sizeof(STARTUPINFOA) );
	lpStartupInfo->cb = sizeof(STARTUPINFOA);
	lpStartupInfo->hStdInput = NtCurrentPeb()->ProcessParameters->StandardInput;
	lpStartupInfo->hStdOutput = NtCurrentPeb()->ProcessParameters->StandardOutput;
	lpStartupInfo->hStdError = NtCurrentPeb()->ProcessParameters->StandardError;
	SetFlag(lpStartupInfo->dwFlags, STARTF_USESTDHANDLES);
}

WINBASEAPI
VOID
WINAPI
GetStartupInfoW (
    _Out_ LPSTARTUPINFOW lpStartupInfo
    )
{
	RtlZeroMemory( lpStartupInfo,
				   sizeof(STARTUPINFOW) );
	lpStartupInfo->cb = sizeof(STARTUPINFOW);
	lpStartupInfo->hStdInput = NtCurrentPeb()->ProcessParameters->StandardInput;
	lpStartupInfo->hStdOutput = NtCurrentPeb()->ProcessParameters->StandardOutput;
	lpStartupInfo->hStdError = NtCurrentPeb()->ProcessParameters->StandardError;
	SetFlag(lpStartupInfo->dwFlags, STARTF_USESTDHANDLES);
}
