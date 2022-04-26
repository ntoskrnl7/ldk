#include "winbase.h"
#include "../nt/ntpsapi.h"
#include "../nt/ntexapi.h"
#include "../nt/zwapi.h"



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
#pragma alloc_text(PAGE, GetExitCodeThread)
#pragma alloc_text(PAGE, GetThreadTimes)
#pragma alloc_text(PAGE, ExitProcess)
#pragma alloc_text(PAGE, TerminateProcess)
#endif



#define TAG_LDK_THREAD_CONTEXT			'xtCT'

typedef struct _LDK_THREAD_CONTEXT {
	DWORD dwCreationFlags;
	SIZE_T dwStackSize;
	PTHREAD_START_ROUTINE ThreadStartRoutine;
	LPVOID lpThreadParameter;
} LDK_THREAD_CONTEXT, *PLDK_THREAD_CONTEXT;

PAGED_LOOKASIDE_LIST LdkpThreadContextLookaside;


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

	Context->ThreadStartRoutine( Context->lpThreadParameter );

	ExFreeToPagedLookasideList( &LdkpThreadContextLookaside,
								Context );
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

	Context->ThreadStartRoutine( Context->lpThreadParameter );

	ExFreeToPagedLookasideList( &LdkpThreadContextLookaside,
								Context );
}

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
CreateThread(
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

	PLDK_THREAD_CONTEXT Context = ExAllocateFromPagedLookasideList( &LdkpThreadContextLookaside );
	if (!Context) {
		BaseSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}

	Context->dwCreationFlags = dwCreationFlags;
	Context->dwStackSize = dwStackSize;
	Context->ThreadStartRoutine = lpStartAddress;
	Context->lpThreadParameter = lpParameter;

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

	NTSTATUS Status;
	HANDLE ThreadHandle;
	CLIENT_ID ClientId;

	Status = PsCreateSystemThread( &ThreadHandle,
								   THREAD_ALL_ACCESS,
								   &ObjectAttributes,
								   NULL,
								   &ClientId,
								   LdkpThreadStartRoutine,
								   Context );
	if (! NT_SUCCESS(Status)) {
		ExFreeToPagedLookasideList( &LdkpThreadContextLookaside,
									Context );
		BaseSetLastNTError( Status );
		return NULL;
	}
	
	if (ARGUMENT_PRESENT(lpThreadId)) {
		*lpThreadId = HandleToUlong(ClientId.UniqueThread);
	}

	return ThreadHandle;
}

WINBASEAPI
HANDLE
WINAPI
GetCurrentThread(
    VOID
    )
{
	return ZwCurrentThread();
}

WINBASEAPI
DWORD
WINAPI
GetCurrentThreadId(
    VOID
    )
{
	return (DWORD)HandleToULong(PsGetCurrentThreadId());
}

WINBASEAPI
_Success_(return != 0)
BOOL
WINAPI
GetExitCodeThread(
    _In_ HANDLE hThread,
    _Out_ LPDWORD lpExitCode
    )
{
	NTSTATUS Status;
	THREAD_BASIC_INFORMATION BasicInformation;

	PAGED_CODE();

	Status = ZwQueryInformationThread( hThread,
									   ThreadBasicInformation,
									   &BasicInformation,
									   sizeof(BasicInformation),
									   NULL );
	if (NT_SUCCESS(Status)) {
		*lpExitCode = BasicInformation.ExitStatus;
		return TRUE;
	}

	BaseSetLastNTError(Status);
	return FALSE;
}

WINBASEAPI
BOOL
WINAPI
GetThreadTimes(
    _In_ HANDLE hThread,
    _Out_ LPFILETIME lpCreationTime,
    _Out_ LPFILETIME lpExitTime,
    _Out_ LPFILETIME lpKernelTime,
    _Out_ LPFILETIME lpUserTime
    )
{
	NTSTATUS Status;
	KERNEL_USER_TIMES TimeInfo;

	PAGED_CODE();

	Status = ZwQueryInformationThread( hThread,
									  ThreadTimes,
									  (PVOID)&TimeInfo,
									  sizeof(TimeInfo),
									  NULL );
	if (!NT_SUCCESS(Status)) {
		BaseSetLastNTError(Status);
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
GetCurrentThreadStackLimits(
    _Out_ PULONG_PTR LowLimit,
    _Out_ PULONG_PTR HighLimit
    )
{
	IoGetStackLimits(LowLimit, HighLimit);
}

WINBASEAPI
BOOL
WINAPI
SwitchToThread(
    VOID
    )
{
    return ZwYieldExecution() != STATUS_NO_YIELD_PERFORMED;
}



WINBASEAPI
HANDLE
WINAPI
GetCurrentProcess(
    VOID
    )
{
	return ZwCurrentProcess();
}

WINBASEAPI
DWORD
WINAPI
GetCurrentProcessId(
    VOID
    )
{
	return (DWORD)HandleToULong(PsGetCurrentProcessId());
}

WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
ExitProcess(
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
TerminateProcess(
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
	BaseSetLastNTError( Status );
	return FALSE;
}



WINBASEAPI
BOOL
WINAPI
IsProcessorFeaturePresent(
	_In_ DWORD ProcessorFeature
	)
{
	return (BOOL)ExIsProcessorFeaturePresent( ProcessorFeature );
}