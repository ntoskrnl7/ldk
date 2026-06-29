#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"
#include "../peb.h"



LDK_INITIALIZE_COMPONENT LdkpInitializeThreadContexts;
LDK_TERMINATE_COMPONENT LdkpTerminateThreadContexts;

EXPAND_STACK_CALLOUT LdkpThreadStartExpandStackAndCallout;
KSTART_ROUTINE LdkpThreadStartRoutine;

typedef struct _LDK_THREAD_CONTEXT LDK_THREAD_CONTEXT, *PLDK_THREAD_CONTEXT;

NTSTATUS
LdkpGetThreadIdFromHandle (
	_In_ HANDLE ThreadHandle,
	_Out_ PHANDLE ThreadId
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



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeThreadContexts)
#pragma alloc_text(PAGE, LdkpTerminateThreadContexts)
#pragma alloc_text(PAGE, LdkpThreadStartExpandStackAndCallout)
#pragma alloc_text(PAGE, LdkpThreadStartRoutine)
#pragma alloc_text(PAGE, LdkpGetThreadIdFromHandle)
#pragma alloc_text(PAGE, LdkpFindThreadContextByIdLocked)
#pragma alloc_text(PAGE, LdkpInsertThreadContext)
#pragma alloc_text(PAGE, LdkpRemoveThreadContext)
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
#pragma alloc_text(PAGE, ExitProcess)
#pragma alloc_text(PAGE, TerminateProcess)
#pragma alloc_text(PAGE, IsProcessorFeaturePresent)
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

typedef struct _LDK_THREAD_CALLOUT_CONTEXT {
	PTHREAD_START_ROUTINE ThreadStartRoutine;
	LPVOID lpThreadParameter;
	DWORD ExitCode;
} LDK_THREAD_CALLOUT_CONTEXT, *PLDK_THREAD_CALLOUT_CONTEXT;

NPAGED_LOOKASIDE_LIST LdkpThreadContextLookaside;
FAST_MUTEX LdkpThreadContextListMutex;
LIST_ENTRY LdkpThreadContextListHead;



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
