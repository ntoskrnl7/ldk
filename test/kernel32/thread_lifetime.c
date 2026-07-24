#include <Ldk/Windows.h>

BOOLEAN
ThreadLifetimeDrainTest (
	VOID
	);

BOOLEAN
ThreadLifetimeDrainFinish (
	VOID
	);

DWORD
WINAPI
ThreadLifetimeDrainWorker (
	_In_opt_ LPVOID Parameter
	);

DWORD
WINAPI
ThreadLifetimeSuspendedWorker (
	_In_opt_ LPVOID Parameter
	);

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ThreadLifetimeDrainTest)
#pragma alloc_text(PAGE, ThreadLifetimeDrainFinish)
#pragma alloc_text(PAGE, ThreadLifetimeDrainWorker)
#pragma alloc_text(PAGE, ThreadLifetimeSuspendedWorker)
#endif

#define LDK_THREAD_LIFETIME_DELAY_100NS (-2500000LL)
#define LDK_THREAD_LIFETIME_START_TIMEOUT_MS 1000

static KEVENT LdkThreadLifetimeStartedEvent;
static KEVENT LdkThreadLifetimeReleaseEvent;
static volatile LONG LdkThreadLifetimeCompleted;
static volatile LONG LdkThreadLifetimeSuspendedRan;

DWORD
WINAPI
ThreadLifetimeDrainWorker (
	_In_opt_ LPVOID Parameter
	)
{
	LARGE_INTEGER Delay;

	PAGED_CODE();
	UNREFERENCED_PARAMETER(Parameter);

	KeSetEvent( &LdkThreadLifetimeStartedEvent,
				IO_NO_INCREMENT,
				FALSE );
	KeWaitForSingleObject( &LdkThreadLifetimeReleaseEvent,
						   Executive,
						   KernelMode,
						   FALSE,
						   NULL );

	Delay.QuadPart = LDK_THREAD_LIFETIME_DELAY_100NS;
	KeDelayExecutionThread( KernelMode,
							FALSE,
							&Delay );
	InterlockedExchange( &LdkThreadLifetimeCompleted,
						 1 );
	return ERROR_SUCCESS;
}

DWORD
WINAPI
ThreadLifetimeSuspendedWorker (
	_In_opt_ LPVOID Parameter
	)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(Parameter);

	InterlockedExchange( &LdkThreadLifetimeSuspendedRan,
						 1 );
	return ERROR_SUCCESS;
}

BOOLEAN
ThreadLifetimeDrainTest (
	VOID
	)
{
	HANDLE Thread;
	HANDLE SuspendedThread;
	LARGE_INTEGER Timeout;
	NTSTATUS Status;

	PAGED_CODE();

	KeInitializeEvent( &LdkThreadLifetimeStartedEvent,
					   NotificationEvent,
					   FALSE );
	KeInitializeEvent( &LdkThreadLifetimeReleaseEvent,
					   NotificationEvent,
					   FALSE );
	LdkThreadLifetimeCompleted = 0;
	LdkThreadLifetimeSuspendedRan = 0;

	Thread = CreateThread( NULL,
						   0,
						   ThreadLifetimeDrainWorker,
						   NULL,
						   0,
						   NULL );
	if (Thread == NULL) {
		return FALSE;
	}

	Timeout.QuadPart =
		-(10LL * 1000 * LDK_THREAD_LIFETIME_START_TIMEOUT_MS);
	Status = KeWaitForSingleObject( &LdkThreadLifetimeStartedEvent,
									Executive,
									KernelMode,
									FALSE,
									&Timeout );
	if (! NT_SUCCESS(Status)) {
		KeSetEvent( &LdkThreadLifetimeReleaseEvent,
					IO_NO_INCREMENT,
					FALSE );
		WaitForSingleObject( Thread,
							 INFINITE );
		CloseHandle( Thread );
		return FALSE;
	}

	CloseHandle( Thread );

	SuspendedThread = CreateThread( NULL,
									0,
									ThreadLifetimeSuspendedWorker,
									NULL,
									CREATE_SUSPENDED,
									NULL );
	if (SuspendedThread == NULL) {
		KeSetEvent( &LdkThreadLifetimeReleaseEvent,
					IO_NO_INCREMENT,
					FALSE );
		return FALSE;
	}
	CloseHandle( SuspendedThread );

	return TRUE;
}

BOOLEAN
ThreadLifetimeDrainFinish (
	VOID
	)
{
	HANDLE Thread;
	DWORD ErrorCode;

	PAGED_CODE();

	KeSetEvent( &LdkThreadLifetimeReleaseEvent,
				IO_NO_INCREMENT,
				FALSE );
	LdkPrepareForTermination();

	if (InterlockedCompareExchange( &LdkThreadLifetimeCompleted,
									0,
									0 ) == 0) {
		return FALSE;
	}
	if (InterlockedCompareExchange( &LdkThreadLifetimeSuspendedRan,
									0,
									0 ) != 0) {
		return FALSE;
	}

	SetLastError( ERROR_SUCCESS );
	Thread = CreateThread( NULL,
						   0,
						   ThreadLifetimeDrainWorker,
						   NULL,
						   0,
						   NULL );
	ErrorCode = GetLastError();
	if (Thread != NULL) {
		CloseHandle( Thread );
		return FALSE;
	}

	return ErrorCode == ERROR_SHUTDOWN_IN_PROGRESS;
}
