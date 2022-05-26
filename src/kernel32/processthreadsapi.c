#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"



LDK_INITIALIZE_COMPONENT LdkpInitializeThreadContexts;
LDK_TERMINATE_COMPONENT LdkpTerminateThreadContexts;

EXPAND_STACK_CALLOUT LdkpThreadStartExpandStackAndCallout;
KSTART_ROUTINE LdkpThreadStartRoutine;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeThreadContexts)
#pragma alloc_text(PAGE, LdkpTerminateThreadContexts)
#pragma alloc_text(PAGE, LdkpThreadStartExpandStackAndCallout)
#pragma alloc_text(PAGE, LdkpThreadStartRoutine)
#pragma alloc_text(PAGE, CreateThread)
#pragma alloc_text(PAGE, OpenThread)
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

typedef struct _LDK_THREAD_CONTEXT {
	DWORD dwCreationFlags;
	SIZE_T dwStackSize;
	PTHREAD_START_ROUTINE ThreadStartRoutine;
	LPVOID lpThreadParameter;
} LDK_THREAD_CONTEXT, *PLDK_THREAD_CONTEXT;

PAGED_LOOKASIDE_LIST LdkpThreadContextLookaside;



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

	ExInitializePagedLookasideList( &LdkpThreadContextLookaside,
									NULL,
									NULL,
									0,
									sizeof(LDK_THREAD_CONTEXT),
									TAG_LDK_THREAD_CONTEXT,
									0 );

	return STATUS_SUCCESS;
}

VOID
LdkpTerminateThreadContexts (
	VOID
	)
{
	PAGED_CODE();

	ExDeletePagedLookasideList( &LdkpThreadContextLookaside );
}

_Function_class_(EXPAND_STACK_CALLOUT)
VOID
NTAPI
LdkpThreadStartExpandStackAndCallout (
    _In_ PLDK_THREAD_CONTEXT Context
    )
{
	PAGED_CODE();

	LPVOID lpThreadParameter = Context->lpThreadParameter;
	PTHREAD_START_ROUTINE ThreadStartRoutine = Context->ThreadStartRoutine;

	ExFreeToPagedLookasideList( &LdkpThreadContextLookaside,
								Context );

	ThreadStartRoutine( lpThreadParameter );

	LdkpInvokeFlsCallback( NtCurrentTeb() );
}

_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
VOID
LdkpThreadStartRoutine (
    _In_ PLDK_THREAD_CONTEXT Context
    )
{
	PAGED_CODE();

	if (FlagOn(Context->dwCreationFlags, CREATE_SUSPENDED)) {
		//PsSuspendThread(); :-(
		KdBreakPoint();
	}

	if (Context->dwStackSize > IoGetRemainingStackSize()) {
		if (NT_SUCCESS(KeExpandKernelStackAndCallout( LdkpThreadStartExpandStackAndCallout,
													  Context,
													  Context->dwStackSize ))) {
			return;
		}
	}

	LPVOID lpThreadParameter = Context->lpThreadParameter;
	PTHREAD_START_ROUTINE ThreadStartRoutine = Context->ThreadStartRoutine;

	ExFreeToPagedLookasideList( &LdkpThreadContextLookaside,
								Context );

	ThreadStartRoutine( lpThreadParameter );

	LdkpInvokeFlsCallback( NtCurrentTeb() );
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

	if (FlagOn(dwCreationFlags, CREATE_SUSPENDED)) {
		KdBreakPoint();
		SetLastError(ERROR_NOT_SUPPORTED);
		return NULL;
	}

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

	PLDK_THREAD_CONTEXT Context = ExAllocateFromPagedLookasideList( &LdkpThreadContextLookaside );
	if (! Context) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}
	Context->dwCreationFlags = dwCreationFlags;
	Context->dwStackSize = dwStackSize;
	Context->ThreadStartRoutine = lpStartAddress;
	Context->lpThreadParameter = lpParameter;

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
		ExFreeToPagedLookasideList( &LdkpThreadContextLookaside,
									Context );
		LdkSetLastNTError( Status );
		return NULL;
	}
	
	if (ARGUMENT_PRESENT(lpThreadId)) {
		*lpThreadId = HandleToUlong(ClientId.UniqueThread);
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
	return (DWORD)HandleToULong(PsGetCurrentProcessId());
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
    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes( &ObjectAttributes,
								NULL,
        						(bInheritHandle ? OBJ_INHERIT : 0) | OBJ_KERNEL_HANDLE,
        						NULL,
								NULL );
    CLIENT_ID ClientId;
    ClientId.UniqueThread = (HANDLE)NULL;
    ClientId.UniqueProcess = (HANDLE)LongToHandle(dwProcessId);
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

	// :-(
	KdBreakPoint();

	TerminateProcess( NtCurrentProcess(),
					  uExitCode );
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

	KdBreakPoint(); // :-(

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