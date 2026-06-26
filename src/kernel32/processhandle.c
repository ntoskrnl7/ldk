#include "winbase.h"
#include "../peb.h"



#ifndef DUPLICATE_CLOSE_SOURCE
#define DUPLICATE_CLOSE_SOURCE 0x00000001
#endif

#ifndef DUPLICATE_SAME_ACCESS
#define DUPLICATE_SAME_ACCESS 0x00000002
#endif



#define LDK_PROCESS_HANDLE_TAG     ((ULONG_PTR)0x1)
#define LDK_PROCESS_HANDLE_MAGIC   'hPdL'
#define TAG_LDK_PROCESS_HANDLE     'hPdL'

typedef struct _LDK_PROCESS_HANDLE {
	ULONG Magic;
	LIST_ENTRY Links;
	ACCESS_MASK GrantedAccess;
} LDK_PROCESS_HANDLE, *PLDK_PROCESS_HANDLE;



LDK_INITIALIZE_COMPONENT LdkpInitializeProcessHandles;
LDK_TERMINATE_COMPONENT LdkpTerminateProcessHandles;

LIST_ENTRY LdkpProcessHandleListHead;
EX_SPIN_LOCK LdkpProcessHandleListLock;
NPAGED_LOOKASIDE_LIST LdkpProcessHandleLookaside;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeProcessHandles)
#endif



FORCEINLINE
BOOLEAN
LdkpIsTaggedProcessHandle (
	_In_ HANDLE Handle
	)
{
	return Handle != NULL &&
		   (((ULONG_PTR)Handle & LDK_PROCESS_HANDLE_TAG) != 0);
}

FORCEINLINE
HANDLE
LdkpEncodeProcessHandle (
	_In_ PLDK_PROCESS_HANDLE ProcessHandle
	)
{
	return (HANDLE)((ULONG_PTR)ProcessHandle | LDK_PROCESS_HANDLE_TAG);
}

FORCEINLINE
PLDK_PROCESS_HANDLE
LdkpDecodeProcessHandle (
	_In_ HANDLE Handle
	)
{
	return (PLDK_PROCESS_HANDLE)((ULONG_PTR)Handle & ~LDK_PROCESS_HANDLE_TAG);
}

FORCEINLINE
BOOLEAN
LdkpIsCurrentProcessPseudoHandle (
	_In_ HANDLE Handle
	)
{
	return Handle == ZwCurrentProcess();
}

FORCEINLINE
BOOLEAN
LdkpHasAccess (
	_In_ ACCESS_MASK GrantedAccess,
	_In_ ACCESS_MASK DesiredAccess
	)
{
	return DesiredAccess == 0 ||
		   GrantedAccess == MAXIMUM_ALLOWED ||
		   ((GrantedAccess & DesiredAccess) == DesiredAccess);
}

FORCEINLINE
DWORD
LdkpGetProcessExitCode (
	VOID
	)
{
	return (DWORD)LdkCurrentPeb()->ProcessExitStatus;
}

VOID
LdkpSetProcessExitCode (
	_In_ UINT ExitCode
	)
{
	if (InterlockedCompareExchange( &LdkCurrentPeb()->ProcessExitStatus,
									(LONG)(NTSTATUS)ExitCode,
									STATUS_PENDING ) == STATUS_PENDING) {
		KeSetEvent( &LdkCurrentPeb()->ProcessExitEvent,
					IO_NO_INCREMENT,
					FALSE );
	}
}

PLDK_PROCESS_HANDLE
LdkpLookupProcessHandleLocked (
	_In_ HANDLE Handle
	)
{
	PLDK_PROCESS_HANDLE DecodedHandle;
	PLIST_ENTRY Entry;

	if (! LdkpIsTaggedProcessHandle( Handle )) {
		return NULL;
	}

	DecodedHandle = LdkpDecodeProcessHandle( Handle );
	for (Entry = LdkpProcessHandleListHead.Flink;
		 Entry != &LdkpProcessHandleListHead;
		 Entry = Entry->Flink) {
		PLDK_PROCESS_HANDLE ProcessHandle;

		ProcessHandle = CONTAINING_RECORD( Entry,
										   LDK_PROCESS_HANDLE,
										   Links );
		if (ProcessHandle == DecodedHandle &&
			ProcessHandle->Magic == LDK_PROCESS_HANDLE_MAGIC) {
			return ProcessHandle;
		}
	}

	return NULL;
}

HANDLE
LdkCreateCurrentProcessHandle (
	_In_ DWORD DesiredAccess,
	_In_ BOOL InheritHandle
	)
{
	PLDK_PROCESS_HANDLE ProcessHandle;
	KIRQL OldIrql;

	UNREFERENCED_PARAMETER(InheritHandle);

	ProcessHandle = ExAllocateFromNPagedLookasideList( &LdkpProcessHandleLookaside );
	if (ProcessHandle == NULL) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}

	ProcessHandle->Magic = LDK_PROCESS_HANDLE_MAGIC;
	ProcessHandle->GrantedAccess = (ACCESS_MASK)DesiredAccess;

	OldIrql = ExAcquireSpinLockExclusive( &LdkpProcessHandleListLock );
	InsertTailList( &LdkpProcessHandleListHead,
					&ProcessHandle->Links );
	ExReleaseSpinLockExclusive( &LdkpProcessHandleListLock,
								OldIrql );

	return LdkpEncodeProcessHandle( ProcessHandle );
}

BOOL
LdkCloseProcessHandle (
	_In_ HANDLE Handle,
	_Out_ PBOOL Handled
	)
{
	PLDK_PROCESS_HANDLE ProcessHandle;
	KIRQL OldIrql;

	*Handled = FALSE;

	if (LdkpIsCurrentProcessPseudoHandle( Handle )) {
		*Handled = TRUE;
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	if (! LdkpIsTaggedProcessHandle( Handle )) {
		return FALSE;
	}

	*Handled = TRUE;
	OldIrql = ExAcquireSpinLockExclusive( &LdkpProcessHandleListLock );
	ProcessHandle = LdkpLookupProcessHandleLocked( Handle );
	if (ProcessHandle != NULL) {
		RemoveEntryList( &ProcessHandle->Links );
		ProcessHandle->Magic = 0;
	}
	ExReleaseSpinLockExclusive( &LdkpProcessHandleListLock,
								OldIrql );

	if (ProcessHandle == NULL) {
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	ExFreeToNPagedLookasideList( &LdkpProcessHandleLookaside,
								 ProcessHandle );
	return TRUE;
}

BOOL
LdkDuplicateProcessHandle (
	_In_ HANDLE SourceHandle,
	_Outptr_ LPHANDLE TargetHandle,
	_In_ DWORD DesiredAccess,
	_In_ BOOL InheritHandle,
	_In_ DWORD Options,
	_Out_ PBOOL Handled
	)
{
	ACCESS_MASK SourceAccess = 0;
	ACCESS_MASK NewAccess;
	PLDK_PROCESS_HANDLE ProcessHandle;
	KIRQL OldIrql;

	*Handled = FALSE;

	if (LdkpIsCurrentProcessPseudoHandle( SourceHandle )) {
		SourceAccess = MAXIMUM_ALLOWED;
		*Handled = TRUE;
	} else if (LdkpIsTaggedProcessHandle( SourceHandle )) {
		*Handled = TRUE;

		OldIrql = ExAcquireSpinLockShared( &LdkpProcessHandleListLock );
		ProcessHandle = LdkpLookupProcessHandleLocked( SourceHandle );
		if (ProcessHandle != NULL) {
			SourceAccess = ProcessHandle->GrantedAccess;
		}
		ExReleaseSpinLockShared( &LdkpProcessHandleListLock,
								 OldIrql );

		if (ProcessHandle == NULL) {
			SetLastError( ERROR_INVALID_HANDLE );
			return FALSE;
		}
	} else {
		return FALSE;
	}

	NewAccess = FlagOn(Options, DUPLICATE_SAME_ACCESS) ?
				SourceAccess :
				(ACCESS_MASK)DesiredAccess;

	*TargetHandle = LdkCreateCurrentProcessHandle( NewAccess,
												   InheritHandle );
	if (*TargetHandle == NULL) {
		return FALSE;
	}

	if (FlagOn(Options, DUPLICATE_CLOSE_SOURCE) &&
		! LdkpIsCurrentProcessPseudoHandle( SourceHandle )) {
		BOOL CloseHandled;

		LdkCloseProcessHandle( SourceHandle,
							   &CloseHandled );
	}

	return TRUE;
}

BOOL
LdkResolveProcessHandleForNativeDuplicate (
	_In_ HANDLE ProcessHandle,
	_Out_ PHANDLE NativeProcessHandle
	)
{
	PLDK_PROCESS_HANDLE LdkProcessHandle;
	KIRQL OldIrql;
	ACCESS_MASK GrantedAccess = 0;

	if (LdkpIsCurrentProcessPseudoHandle( ProcessHandle )) {
		*NativeProcessHandle = ZwCurrentProcess();
		return TRUE;
	}

	if (! LdkpIsTaggedProcessHandle( ProcessHandle )) {
		*NativeProcessHandle = ProcessHandle;
		return TRUE;
	}

	OldIrql = ExAcquireSpinLockShared( &LdkpProcessHandleListLock );
	LdkProcessHandle = LdkpLookupProcessHandleLocked( ProcessHandle );
	if (LdkProcessHandle != NULL) {
		GrantedAccess = LdkProcessHandle->GrantedAccess;
	}
	ExReleaseSpinLockShared( &LdkpProcessHandleListLock,
							 OldIrql );

	if (LdkProcessHandle == NULL) {
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	if (! LdkpHasAccess( GrantedAccess,
						 PROCESS_DUP_HANDLE )) {
		SetLastError( ERROR_ACCESS_DENIED );
		return FALSE;
	}

	*NativeProcessHandle = ZwCurrentProcess();
	return TRUE;
}

BOOL
LdkGetExitCodeProcessHandle (
	_In_ HANDLE ProcessHandle,
	_Out_ LPDWORD ExitCode,
	_Out_ PBOOL Handled
	)
{
	PLDK_PROCESS_HANDLE LdkProcessHandle;
	KIRQL OldIrql;
	ACCESS_MASK GrantedAccess = 0;

	*Handled = FALSE;

	if (LdkpIsCurrentProcessPseudoHandle( ProcessHandle )) {
		*Handled = TRUE;
		*ExitCode = LdkpGetProcessExitCode();
		return TRUE;
	}

	if (! LdkpIsTaggedProcessHandle( ProcessHandle )) {
		return FALSE;
	}

	*Handled = TRUE;
	OldIrql = ExAcquireSpinLockShared( &LdkpProcessHandleListLock );
	LdkProcessHandle = LdkpLookupProcessHandleLocked( ProcessHandle );
	if (LdkProcessHandle != NULL) {
		GrantedAccess = LdkProcessHandle->GrantedAccess;
	}
	ExReleaseSpinLockShared( &LdkpProcessHandleListLock,
							 OldIrql );

	if (LdkProcessHandle == NULL) {
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	if (! LdkpHasAccess( GrantedAccess,
						 PROCESS_QUERY_LIMITED_INFORMATION ) &&
		! LdkpHasAccess( GrantedAccess,
						 PROCESS_QUERY_INFORMATION )) {
		SetLastError( ERROR_ACCESS_DENIED );
		return FALSE;
	}

	*ExitCode = LdkpGetProcessExitCode();
	return TRUE;
}

BOOL
LdkTerminateProcessHandle (
	_In_ HANDLE ProcessHandle,
	_In_ UINT ExitCode,
	_Out_ PBOOL Handled
	)
{
	PLDK_PROCESS_HANDLE LdkProcessHandle;
	KIRQL OldIrql;
	ACCESS_MASK GrantedAccess = 0;

	*Handled = FALSE;

	if (LdkpIsCurrentProcessPseudoHandle( ProcessHandle )) {
		*Handled = TRUE;
		LdkpSetProcessExitCode( ExitCode );
		return TRUE;
	}

	if (! LdkpIsTaggedProcessHandle( ProcessHandle )) {
		return FALSE;
	}

	*Handled = TRUE;
	OldIrql = ExAcquireSpinLockShared( &LdkpProcessHandleListLock );
	LdkProcessHandle = LdkpLookupProcessHandleLocked( ProcessHandle );
	if (LdkProcessHandle != NULL) {
		GrantedAccess = LdkProcessHandle->GrantedAccess;
	}
	ExReleaseSpinLockShared( &LdkpProcessHandleListLock,
							 OldIrql );

	if (LdkProcessHandle == NULL) {
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	if (! LdkpHasAccess( GrantedAccess,
						 PROCESS_TERMINATE )) {
		SetLastError( ERROR_ACCESS_DENIED );
		return FALSE;
	}

	LdkpSetProcessExitCode( ExitCode );
	return TRUE;
}

BOOLEAN
LdkIsProcessHandleCandidate (
	_In_ HANDLE Handle
	)
{
	return LdkpIsCurrentProcessPseudoHandle( Handle ) ||
		   LdkpIsTaggedProcessHandle( Handle );
}

NTSTATUS
LdkReferenceProcessWaitObject (
	_In_ HANDLE Handle,
	_Outptr_ PVOID *Object,
	_Out_ PBOOLEAN IsLdkObject
	)
{
	PLDK_PROCESS_HANDLE LdkProcessHandle;
	KIRQL OldIrql;
	ACCESS_MASK GrantedAccess = 0;

	*Object = NULL;
	*IsLdkObject = FALSE;

	if (LdkpIsCurrentProcessPseudoHandle( Handle )) {
		*Object = &LdkCurrentPeb()->ProcessExitEvent;
		*IsLdkObject = TRUE;
		return STATUS_SUCCESS;
	}

	if (! LdkpIsTaggedProcessHandle( Handle )) {
		return STATUS_OBJECT_TYPE_MISMATCH;
	}

	OldIrql = ExAcquireSpinLockShared( &LdkpProcessHandleListLock );
	LdkProcessHandle = LdkpLookupProcessHandleLocked( Handle );
	if (LdkProcessHandle != NULL) {
		GrantedAccess = LdkProcessHandle->GrantedAccess;
	}
	ExReleaseSpinLockShared( &LdkpProcessHandleListLock,
							 OldIrql );

	if (LdkProcessHandle == NULL) {
		return STATUS_INVALID_HANDLE;
	}

	if (! LdkpHasAccess( GrantedAccess,
						 SYNCHRONIZE )) {
		return STATUS_ACCESS_DENIED;
	}

	*Object = &LdkCurrentPeb()->ProcessExitEvent;
	*IsLdkObject = TRUE;
	return STATUS_SUCCESS;
}

VOID
LdkDereferenceProcessWaitObject (
	_In_ PVOID Object,
	_In_ BOOLEAN IsLdkObject
	)
{
	if (! IsLdkObject &&
		Object != NULL) {
		ObDereferenceObject( Object );
	}
}

NTSTATUS
LdkpInitializeProcessHandles (
	VOID
	)
{
	PAGED_CODE();

	LdkpProcessHandleListLock = 0;
	InitializeListHead( &LdkpProcessHandleListHead );
	ExInitializeNPagedLookasideList( &LdkpProcessHandleLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_PROCESS_HANDLE),
									 TAG_LDK_PROCESS_HANDLE,
									 0 );

	return STATUS_SUCCESS;
}

VOID
LdkpTerminateProcessHandles (
	VOID
	)
{
	LIST_ENTRY HandleList;
	KIRQL OldIrql;

	PAGED_CODE();

	InitializeListHead( &HandleList );

	OldIrql = ExAcquireSpinLockExclusive( &LdkpProcessHandleListLock );
	if (! IsListEmpty( &LdkpProcessHandleListHead )) {
		LdkpProcessHandleListHead.Blink->Flink = &HandleList;
		HandleList.Blink = LdkpProcessHandleListHead.Blink;
		LdkpProcessHandleListHead.Flink->Blink = &HandleList;
		HandleList.Flink = LdkpProcessHandleListHead.Flink;
		InitializeListHead( &LdkpProcessHandleListHead );
	}
	ExReleaseSpinLockExclusive( &LdkpProcessHandleListLock,
								OldIrql );

	while (! IsListEmpty( &HandleList )) {
		PLIST_ENTRY Entry;
		PLDK_PROCESS_HANDLE ProcessHandle;

		Entry = RemoveHeadList( &HandleList );
		ProcessHandle = CONTAINING_RECORD( Entry,
										   LDK_PROCESS_HANDLE,
										   Links );
		ProcessHandle->Magic = 0;
		ExFreeToNPagedLookasideList( &LdkpProcessHandleLookaside,
									 ProcessHandle );
	}

	ExDeleteNPagedLookasideList( &LdkpProcessHandleLookaside );
}
