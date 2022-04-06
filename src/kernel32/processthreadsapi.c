#include "winbase.h"
#include "../nt/ntpsapi.h"
#include "../nt/ntexapi.h"
#include "../nt/zwapi.h"



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
	TerminateProcess(NtCurrentProcess(), uExitCode);
}

WINBASEAPI
BOOL
WINAPI
TerminateProcess(
    _In_ HANDLE hProcess,
    _In_ UINT uExitCode
    )
{
	KdBreakPoint(); // :-(

	if (hProcess == NULL) {
		SetLastError(ERROR_INVALID_HANDLE);
		return FALSE;
	}
	NTSTATUS Status = ZwTerminateProcess(hProcess, (NTSTATUS)uExitCode);
	if (NT_SUCCESS(Status)) {
		return TRUE;
	}
	BaseSetLastNTError(Status);
	return FALSE;
}



#define TAG_LDK_THREAD_CONTEXT			'xtCT'

typedef struct _LDK_THREAD_CONTEXT {
	DWORD dwCreationFlags;
	SIZE_T dwStackSize;
	PTHREAD_START_ROUTINE ThreadStartRoutine;
	LPVOID lpThreadParameter;
} LDK_THREAD_CONTEXT, *PLDK_THREAD_CONTEXT;

_Function_class_(EXPAND_STACK_CALLOUT)
VOID
NTAPI
LdkpThreadStartExpandStackAndCallout (
    _In_ PLDK_THREAD_CONTEXT Context
    )
{
	KIRQL OldIrql = KeGetCurrentIrql();

	Context->ThreadStartRoutine(Context->lpThreadParameter);

	if (OldIrql != KeGetCurrentIrql()) {
		KdBreakPoint();
		KeLowerIrql(OldIrql);
	}

	ExFreePoolWithTag(Context, TAG_LDK_THREAD_CONTEXT);
}

_IRQL_requires_same_
_Function_class_(KSTART_ROUTINE)
VOID
LdkpThreadStartRoutine (
    _In_ PLDK_THREAD_CONTEXT Context
    )
{
	NTSTATUS Status;
	ULONG_PTR StackSize = IoGetRemainingStackSize();

	if (FlagOn(Context->dwCreationFlags, CREATE_SUSPENDED)) {
		//PsSuspendThread(); :-(
		KdBreakPoint();
	}

	if (Context->dwStackSize > StackSize) {
		Status = KeExpandKernelStackAndCallout(LdkpThreadStartExpandStackAndCallout, Context, Context->dwStackSize);
		if (NT_SUCCESS(Status)) {
			return;
		}
	}

	Context->ThreadStartRoutine(Context->lpThreadParameter);

	ExFreePoolWithTag(Context, TAG_LDK_THREAD_CONTEXT);
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
	NTSTATUS Status;
	HANDLE ThreadHandle;

	OBJECT_ATTRIBUTES ObjectAttributes;
	CLIENT_ID ClientId;
	
	PLDK_THREAD_CONTEXT Context;

	if (FlagOn(dwCreationFlags, CREATE_SUSPENDED)) {
		KdBreakPoint();
		SetLastError(ERROR_NOT_SUPPORTED);
		return NULL;
	}
	
	Context = ExAllocatePoolWithTag(PagedPool, sizeof(LDK_THREAD_CONTEXT), TAG_LDK_THREAD_CONTEXT);
	if (!Context) {
		BaseSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}

	Context->dwCreationFlags = dwCreationFlags;
	Context->dwStackSize = dwStackSize;
	Context->ThreadStartRoutine = lpStartAddress;
	Context->lpThreadParameter = lpParameter;

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

	Status = PsCreateSystemThread(
				&ThreadHandle,
				THREAD_ALL_ACCESS,
				&ObjectAttributes,
				NULL,
				&ClientId,
				LdkpThreadStartRoutine,
				Context );

	if (! NT_SUCCESS(Status)) {
		ExFreePoolWithTag(Context, TAG_LDK_THREAD_CONTEXT);
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

	Status = ZwQueryInformationThread(
		hThread,
		ThreadBasicInformation,
		&BasicInformation,
		sizeof(BasicInformation),
		NULL
	);

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

	Status = ZwQueryInformationThread(
		hThread,
		ThreadTimes,
		(PVOID)&TimeInfo,
		sizeof(TimeInfo),
		NULL
	);
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
BOOL
WINAPI
SwitchToThread(
    VOID
    )
{
    return ZwYieldExecution() != STATUS_NO_YIELD_PERFORMED;
}



WINBASEAPI
BOOL
WINAPI
IsProcessorFeaturePresent(
	_In_ DWORD ProcessorFeature
	)
{
	return (BOOL)ExIsProcessorFeaturePresent(ProcessorFeature);
}