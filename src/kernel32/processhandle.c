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
#define TAG_LDK_WAIT_OBJECT        'wOdL'
#define TAG_LDK_WAIT_OBJECT_NAME   'nWdL'

typedef enum _LDK_PROCESS_HANDLE_TYPE {
	LdkProcessHandleTypeCurrentProcess,
	LdkProcessHandleTypeWaitObject
} LDK_PROCESS_HANDLE_TYPE;

typedef enum _LDK_WAIT_OBJECT_TYPE {
	LdkWaitObjectTypeMutant
} LDK_WAIT_OBJECT_TYPE;

typedef struct _LDK_WAIT_OBJECT {
	LONG ReferenceCount;
	LONG HandleCount;
	LDK_WAIT_OBJECT_TYPE Type;
	LIST_ENTRY NameLinks;
	BOOLEAN Named;
	UNICODE_STRING Name;
	union {
		KMUTANT Mutant;
	} Object;
} LDK_WAIT_OBJECT, *PLDK_WAIT_OBJECT;

typedef struct _LDK_PROCESS_HANDLE {
	ULONG Magic;
	LIST_ENTRY Links;
	ACCESS_MASK GrantedAccess;
	LDK_PROCESS_HANDLE_TYPE Type;
	union {
		PLDK_WAIT_OBJECT WaitObject;
	} Object;
} LDK_PROCESS_HANDLE, *PLDK_PROCESS_HANDLE;



LDK_INITIALIZE_COMPONENT LdkpInitializeProcessHandles;
LDK_TERMINATE_COMPONENT LdkpTerminateProcessHandles;

LIST_ENTRY LdkpProcessHandleListHead;
EX_SPIN_LOCK LdkpProcessHandleListLock;
NPAGED_LOOKASIDE_LIST LdkpProcessHandleLookaside;
NPAGED_LOOKASIDE_LIST LdkpWaitObjectLookaside;
FAST_MUTEX LdkpNamedWaitObjectLock;
LIST_ENTRY LdkpNamedWaitObjectListHead;



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

VOID
LdkpReferenceWaitObject (
	_In_ PLDK_WAIT_OBJECT WaitObject
	)
{
	InterlockedIncrement( &WaitObject->ReferenceCount );
}

VOID
LdkpReferenceWaitObjectHandle (
	_In_ PLDK_WAIT_OBJECT WaitObject
	)
{
	InterlockedIncrement( &WaitObject->HandleCount );
	LdkpReferenceWaitObject( WaitObject );
}

VOID
LdkpDereferenceWaitObject (
	_In_opt_ PLDK_WAIT_OBJECT WaitObject
	)
{
	if (WaitObject == NULL) {
		return;
	}

	if (InterlockedDecrement( &WaitObject->ReferenceCount ) == 0) {
		if (WaitObject->Name.Buffer != NULL) {
			ExFreePoolWithTag( WaitObject->Name.Buffer,
							   TAG_LDK_WAIT_OBJECT_NAME );
		}
		ExFreeToNPagedLookasideList( &LdkpWaitObjectLookaside,
									 WaitObject );
	}
}

VOID
LdkpReleaseWaitObjectHandleReference (
	_In_opt_ PLDK_WAIT_OBJECT WaitObject
	)
{
	if (WaitObject == NULL) {
		return;
	}

	if (InterlockedDecrement( &WaitObject->HandleCount ) == 0 &&
		WaitObject->Named) {
		ExAcquireFastMutex( &LdkpNamedWaitObjectLock );
		if (WaitObject->Named) {
			RemoveEntryList( &WaitObject->NameLinks );
			InitializeListHead( &WaitObject->NameLinks );
			WaitObject->Named = FALSE;
		}
		ExReleaseFastMutex( &LdkpNamedWaitObjectLock );
	}

	LdkpDereferenceWaitObject( WaitObject );
}

PVOID
LdkpGetWaitObjectDispatcher (
	_In_ PLDK_WAIT_OBJECT WaitObject
	)
{
	switch (WaitObject->Type) {
	case LdkWaitObjectTypeMutant:
		return &WaitObject->Object.Mutant;
	default:
		return NULL;
	}
}

PLDK_WAIT_OBJECT
LdkpGetWaitObjectFromDispatcher (
	_In_ PVOID Object
	)
{
	return CONTAINING_RECORD( Object,
							  LDK_WAIT_OBJECT,
							  Object.Mutant );
}

PLDK_WAIT_OBJECT
LdkpFindNamedWaitObjectLocked (
	_In_ PCUNICODE_STRING Name,
	_In_ LDK_WAIT_OBJECT_TYPE Type
	)
{
	PLIST_ENTRY Entry;

	for (Entry = LdkpNamedWaitObjectListHead.Flink;
		 Entry != &LdkpNamedWaitObjectListHead;
		 Entry = Entry->Flink) {
		PLDK_WAIT_OBJECT WaitObject;

		WaitObject = CONTAINING_RECORD( Entry,
										LDK_WAIT_OBJECT,
										NameLinks );
		if (WaitObject->Type == Type &&
			RtlEqualUnicodeString( &WaitObject->Name,
								   Name,
								   FALSE )) {
			return WaitObject;
		}
	}

	return NULL;
}

BOOLEAN
LdkpCopyWaitObjectName (
	_In_ PLDK_WAIT_OBJECT WaitObject,
	_In_ PCUNICODE_STRING Name
	)
{
	WaitObject->Name.Buffer = NULL;
	WaitObject->Name.Length = 0;
	WaitObject->Name.MaximumLength = 0;

	if (Name == NULL ||
		Name->Buffer == NULL ||
		Name->Length == 0) {
		return TRUE;
	}

#pragma warning(disable:4996)
	WaitObject->Name.Buffer = ExAllocatePoolWithTag( NonPagedPool,
													 Name->Length + sizeof(WCHAR),
													 TAG_LDK_WAIT_OBJECT_NAME );
#pragma warning(default:4996)
	if (WaitObject->Name.Buffer == NULL) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return FALSE;
	}

	WaitObject->Name.Length = Name->Length;
	WaitObject->Name.MaximumLength = Name->Length + sizeof(WCHAR);
	RtlCopyMemory( WaitObject->Name.Buffer,
				   Name->Buffer,
				   Name->Length );
	WaitObject->Name.Buffer[Name->Length / sizeof(WCHAR)] = UNICODE_NULL;
	return TRUE;
}

HANDLE
LdkpCreateProcessHandle (
	_In_ DWORD DesiredAccess,
	_In_ BOOL InheritHandle,
	_In_ LDK_PROCESS_HANDLE_TYPE Type,
	_In_opt_ PLDK_WAIT_OBJECT WaitObject
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
	ProcessHandle->Type = Type;
	ProcessHandle->Object.WaitObject = WaitObject;

	OldIrql = ExAcquireSpinLockExclusive( &LdkpProcessHandleListLock );
	InsertTailList( &LdkpProcessHandleListHead,
					&ProcessHandle->Links );
	ExReleaseSpinLockExclusive( &LdkpProcessHandleListLock,
								OldIrql );

	return LdkpEncodeProcessHandle( ProcessHandle );
}

HANDLE
LdkCreateCurrentProcessHandle (
	_In_ DWORD DesiredAccess,
	_In_ BOOL InheritHandle
	)
{
	return LdkpCreateProcessHandle( DesiredAccess,
									InheritHandle,
									LdkProcessHandleTypeCurrentProcess,
									NULL );
}

HANDLE
LdkCreateMutantHandle (
	_In_ DWORD DesiredAccess,
	_In_ BOOL InheritHandle,
	_In_ BOOLEAN InitialOwner,
	_In_opt_ PCUNICODE_STRING Name,
	_Out_opt_ PBOOLEAN AlreadyExists
	)
{
	PLDK_WAIT_OBJECT WaitObject;
	HANDLE Handle;

	if (AlreadyExists != NULL) {
		*AlreadyExists = FALSE;
	}

	if (Name != NULL &&
		Name->Buffer != NULL &&
		Name->Length != 0) {
		ExAcquireFastMutex( &LdkpNamedWaitObjectLock );
		WaitObject = LdkpFindNamedWaitObjectLocked( Name,
													LdkWaitObjectTypeMutant );
		if (WaitObject != NULL) {
			LdkpReferenceWaitObjectHandle( WaitObject );
		}
		ExReleaseFastMutex( &LdkpNamedWaitObjectLock );

		if (WaitObject != NULL) {
			Handle = LdkpCreateProcessHandle( DesiredAccess,
											  InheritHandle,
											  LdkProcessHandleTypeWaitObject,
											  WaitObject );
			if (Handle == NULL) {
				LdkpReleaseWaitObjectHandleReference( WaitObject );
				return NULL;
			}
			if (AlreadyExists != NULL) {
				*AlreadyExists = TRUE;
			}
			return Handle;
		}
	}

	WaitObject = ExAllocateFromNPagedLookasideList( &LdkpWaitObjectLookaside );
	if (WaitObject == NULL) {
		LdkSetLastNTError( STATUS_INSUFFICIENT_RESOURCES );
		return NULL;
	}

	WaitObject->ReferenceCount = 1;
	WaitObject->HandleCount = 1;
	WaitObject->Type = LdkWaitObjectTypeMutant;
	WaitObject->Named = FALSE;
	InitializeListHead( &WaitObject->NameLinks );
	if (! LdkpCopyWaitObjectName( WaitObject,
								  Name )) {
		LdkpDereferenceWaitObject( WaitObject );
		return NULL;
	}
	KeInitializeMutant( &WaitObject->Object.Mutant,
						 InitialOwner );

	if (Name != NULL &&
		Name->Buffer != NULL &&
		Name->Length != 0) {
		PLDK_WAIT_OBJECT ExistingWaitObject;

		ExAcquireFastMutex( &LdkpNamedWaitObjectLock );
		ExistingWaitObject = LdkpFindNamedWaitObjectLocked( Name,
															LdkWaitObjectTypeMutant );
		if (ExistingWaitObject == NULL) {
			WaitObject->Named = TRUE;
			InsertTailList( &LdkpNamedWaitObjectListHead,
							&WaitObject->NameLinks );
		} else {
			LdkpReferenceWaitObjectHandle( ExistingWaitObject );
		}
		ExReleaseFastMutex( &LdkpNamedWaitObjectLock );

		if (ExistingWaitObject != NULL) {
			LdkpReleaseWaitObjectHandleReference( WaitObject );
			WaitObject = ExistingWaitObject;
			if (AlreadyExists != NULL) {
				*AlreadyExists = TRUE;
			}
		}
	}

	Handle = LdkpCreateProcessHandle( DesiredAccess,
									  InheritHandle,
									  LdkProcessHandleTypeWaitObject,
									  WaitObject );
	if (Handle == NULL) {
		LdkpReleaseWaitObjectHandleReference( WaitObject );
	}

	return Handle;
}

HANDLE
LdkOpenMutantHandle (
	_In_ DWORD DesiredAccess,
	_In_ BOOL InheritHandle,
	_In_ PCUNICODE_STRING Name
	)
{
	PLDK_WAIT_OBJECT WaitObject;
	HANDLE Handle;

	WaitObject = NULL;

	if (Name == NULL ||
		Name->Buffer == NULL ||
		Name->Length == 0) {
		SetLastError( ERROR_INVALID_PARAMETER );
		return NULL;
	}

	ExAcquireFastMutex( &LdkpNamedWaitObjectLock );
	WaitObject = LdkpFindNamedWaitObjectLocked( Name,
												LdkWaitObjectTypeMutant );
	if (WaitObject != NULL) {
		LdkpReferenceWaitObjectHandle( WaitObject );
	}
	ExReleaseFastMutex( &LdkpNamedWaitObjectLock );

	if (WaitObject == NULL) {
		SetLastError( ERROR_FILE_NOT_FOUND );
		return NULL;
	}

	Handle = LdkpCreateProcessHandle( DesiredAccess,
									  InheritHandle,
									  LdkProcessHandleTypeWaitObject,
									  WaitObject );
	if (Handle == NULL) {
		LdkpReleaseWaitObjectHandleReference( WaitObject );
	}

	return Handle;
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

	if (ProcessHandle->Type == LdkProcessHandleTypeWaitObject) {
		LdkpReleaseWaitObjectHandleReference( ProcessHandle->Object.WaitObject );
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
	LDK_PROCESS_HANDLE_TYPE SourceType;
	PLDK_WAIT_OBJECT SourceWaitObject;
	KIRQL OldIrql;

	*Handled = FALSE;
	SourceType = LdkProcessHandleTypeCurrentProcess;
	SourceWaitObject = NULL;

	if (LdkpIsCurrentProcessPseudoHandle( SourceHandle )) {
		SourceAccess = MAXIMUM_ALLOWED;
		*Handled = TRUE;
	} else if (LdkpIsTaggedProcessHandle( SourceHandle )) {
		*Handled = TRUE;

		OldIrql = ExAcquireSpinLockShared( &LdkpProcessHandleListLock );
		ProcessHandle = LdkpLookupProcessHandleLocked( SourceHandle );
		if (ProcessHandle != NULL) {
			SourceAccess = ProcessHandle->GrantedAccess;
			SourceType = ProcessHandle->Type;
			if (SourceType == LdkProcessHandleTypeWaitObject) {
				SourceWaitObject = ProcessHandle->Object.WaitObject;
				LdkpReferenceWaitObject( SourceWaitObject );
			}
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

	*TargetHandle = LdkpCreateProcessHandle( NewAccess,
											 InheritHandle,
											 SourceType,
											 SourceWaitObject );
	if (*TargetHandle == NULL) {
		if (SourceWaitObject != NULL) {
			LdkpReleaseWaitObjectHandleReference( SourceWaitObject );
		}
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
LdkReleaseMutantHandle (
	_In_ HANDLE Handle,
	_Out_ PBOOL Handled
	)
{
	PLDK_PROCESS_HANDLE ProcessHandle;
	PLDK_WAIT_OBJECT WaitObject;
	KIRQL OldIrql;
	NTSTATUS Status;
	BOOLEAN HasAccess;
	BOOLEAN IsMutantHandle;

	*Handled = FALSE;
	WaitObject = NULL;
	HasAccess = FALSE;
	IsMutantHandle = FALSE;

	if (! LdkpIsTaggedProcessHandle( Handle )) {
		return FALSE;
	}

	*Handled = TRUE;

	OldIrql = ExAcquireSpinLockShared( &LdkpProcessHandleListLock );
	ProcessHandle = LdkpLookupProcessHandleLocked( Handle );
	if (ProcessHandle != NULL &&
		ProcessHandle->Type == LdkProcessHandleTypeWaitObject &&
		ProcessHandle->Object.WaitObject != NULL &&
		ProcessHandle->Object.WaitObject->Type == LdkWaitObjectTypeMutant) {
		IsMutantHandle = TRUE;
		HasAccess = LdkpHasAccess( ProcessHandle->GrantedAccess,
								   MUTEX_MODIFY_STATE );
		if (HasAccess) {
			WaitObject = ProcessHandle->Object.WaitObject;
			LdkpReferenceWaitObject( WaitObject );
		}
	}
	ExReleaseSpinLockShared( &LdkpProcessHandleListLock,
							 OldIrql );

	if (! IsMutantHandle) {
		SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}

	if (! HasAccess) {
		SetLastError( ERROR_ACCESS_DENIED );
		return FALSE;
	}

	__try {
		KeReleaseMutant( &WaitObject->Object.Mutant,
						 IO_NO_INCREMENT,
						 FALSE,
						 FALSE );
		Status = STATUS_SUCCESS;
	} __except (EXCEPTION_EXECUTE_HANDLER) {
		Status = GetExceptionCode();
	}

	LdkpDereferenceWaitObject( WaitObject );

	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError( Status );
		return FALSE;
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
LdkResolveProcessHandleForNativeQuery (
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
						 PROCESS_QUERY_LIMITED_INFORMATION ) &&
		! LdkpHasAccess( GrantedAccess,
						 PROCESS_QUERY_INFORMATION )) {
		SetLastError( ERROR_ACCESS_DENIED );
		return FALSE;
	}

	*NativeProcessHandle = ZwCurrentProcess();
	return TRUE;
}

BOOL
LdkGetProcessIdHandle (
	_In_ HANDLE ProcessHandle,
	_Out_ LPDWORD ProcessId,
	_Out_ PBOOL Handled
	)
{
	PLDK_PROCESS_HANDLE LdkProcessHandle;
	KIRQL OldIrql;
	ACCESS_MASK GrantedAccess = 0;

	*Handled = FALSE;

	if (LdkpIsCurrentProcessPseudoHandle( ProcessHandle )) {
		*Handled = TRUE;
		*ProcessId = LdkCurrentPeb()->ProcessId;
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

	*ProcessId = LdkCurrentPeb()->ProcessId;
	return TRUE;
}

BOOL
LdkGetProcessImageNameHandle (
	_In_ HANDLE ProcessHandle,
	_In_ DWORD Flags,
	_Out_ PCUNICODE_STRING *ImageName,
	_Out_ PANSI_STRING NativeImageName,
	_Out_ PBOOL Handled
	)
{
	PLDK_PROCESS_HANDLE LdkProcessHandle;
	KIRQL OldIrql;
	ACCESS_MASK GrantedAccess = 0;

	*Handled = FALSE;
	*ImageName = NULL;
	RtlZeroMemory( NativeImageName,
				   sizeof(*NativeImageName) );

	if (LdkpIsCurrentProcessPseudoHandle( ProcessHandle )) {
		*Handled = TRUE;
	} else if (LdkpIsTaggedProcessHandle( ProcessHandle )) {
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
	} else {
		return FALSE;
	}

	if (Flags == PROCESS_NAME_NATIVE) {
		*NativeImageName = LdkCurrentPeb()->FullPathName;
		if (NativeImageName->Length == 0) {
			SetLastError( ERROR_NOT_FOUND );
			return FALSE;
		}
		return TRUE;
	}

	*ImageName = &LdkCurrentPeb()->ProcessParameters->ImagePathName;
	if ((*ImageName)->Length == 0) {
		SetLastError( ERROR_NOT_FOUND );
		return FALSE;
	}
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
LdkValidateProcessHandleForSetInformation (
	_In_ HANDLE ProcessHandle,
	_Out_ PBOOL Handled
	)
{
	PLDK_PROCESS_HANDLE LdkProcessHandle;
	KIRQL OldIrql;
	ACCESS_MASK GrantedAccess = 0;

	*Handled = FALSE;

	if (LdkpIsCurrentProcessPseudoHandle( ProcessHandle )) {
		*Handled = TRUE;
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
						 PROCESS_SET_INFORMATION )) {
		SetLastError( ERROR_ACCESS_DENIED );
		return FALSE;
	}

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
	PLDK_WAIT_OBJECT WaitObject;
	KIRQL OldIrql;
	ACCESS_MASK GrantedAccess = 0;
	LDK_PROCESS_HANDLE_TYPE Type;
	BOOLEAN HasAccess;

	*Object = NULL;
	*IsLdkObject = FALSE;
	WaitObject = NULL;
	Type = LdkProcessHandleTypeCurrentProcess;
	HasAccess = FALSE;

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
		Type = LdkProcessHandle->Type;
		HasAccess = LdkpHasAccess( GrantedAccess,
								   SYNCHRONIZE );
		if (HasAccess &&
			Type == LdkProcessHandleTypeWaitObject) {
			WaitObject = LdkProcessHandle->Object.WaitObject;
			LdkpReferenceWaitObject( WaitObject );
		}
	}
	ExReleaseSpinLockShared( &LdkpProcessHandleListLock,
							 OldIrql );

	if (LdkProcessHandle == NULL) {
		return STATUS_INVALID_HANDLE;
	}

	if (! HasAccess) {
		return STATUS_ACCESS_DENIED;
	}

	if (Type == LdkProcessHandleTypeWaitObject) {
		*Object = LdkpGetWaitObjectDispatcher( WaitObject );
		if (*Object == NULL) {
			LdkpDereferenceWaitObject( WaitObject );
			return STATUS_INVALID_HANDLE;
		}
	} else {
		*Object = &LdkCurrentPeb()->ProcessExitEvent;
	}
	*IsLdkObject = TRUE;
	return STATUS_SUCCESS;
}

VOID
LdkDereferenceProcessWaitObject (
	_In_ PVOID Object,
	_In_ BOOLEAN IsLdkObject
	)
{
	if (IsLdkObject &&
		Object != NULL &&
		Object != &LdkCurrentPeb()->ProcessExitEvent) {
		LdkpDereferenceWaitObject( LdkpGetWaitObjectFromDispatcher( Object ) );
	} else if (! IsLdkObject &&
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
	ExInitializeFastMutex( &LdkpNamedWaitObjectLock );
	InitializeListHead( &LdkpNamedWaitObjectListHead );
	ExInitializeNPagedLookasideList( &LdkpProcessHandleLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_PROCESS_HANDLE),
									 TAG_LDK_PROCESS_HANDLE,
									 0 );
	ExInitializeNPagedLookasideList( &LdkpWaitObjectLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_WAIT_OBJECT),
									 TAG_LDK_WAIT_OBJECT,
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
		if (ProcessHandle->Type == LdkProcessHandleTypeWaitObject) {
			LdkpReleaseWaitObjectHandleReference( ProcessHandle->Object.WaitObject );
		}
		ExFreeToNPagedLookasideList( &LdkpProcessHandleLookaside,
									 ProcessHandle );
	}

	ExDeleteNPagedLookasideList( &LdkpWaitObjectLookaside );
	ExDeleteNPagedLookasideList( &LdkpProcessHandleLookaside );
}
