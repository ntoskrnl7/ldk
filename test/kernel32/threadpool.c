#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
QueueUserWorkItemTest (
    VOID
    );

DWORD
WINAPI
LegacyThreadWorkFunction (
    PLONG Count
    );

BOOLEAN
ThreadpoolWorkTimerWaitCleanupGroupTest (
    VOID
    );

VOID
NTAPI
ThreadPoolTestWorkCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    );

VOID
NTAPI
ThreadPoolTestRaceDllCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    );

VOID
NTAPI
ThreadPoolTestDeferredFreeCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    );

VOID
NTAPI
ThreadPoolTestFinalizationWorkCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    );

VOID
NTAPI
ThreadPoolTestFinalizationCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context
    );

VOID
NTAPI
ThreadPoolTestCleanupGroupWorkCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    );

VOID
NTAPI
ThreadPoolTestCleanupGroupCancelCallback (
    _Inout_opt_ PVOID ObjectContext,
    _Inout_opt_ PVOID CleanupContext
    );

VOID
NTAPI
ThreadPoolTestTimerCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_TIMER             Timer
    );

VOID
NTAPI
ThreadPoolTestWaitCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WAIT              Wait,
    _In_        TP_WAIT_RESULT        WaitResult
    );

VOID
NTAPI
ThreadPoolTestTimerWaitCleanupCallback (
    _Inout_opt_ PVOID ObjectContext,
    _Inout_opt_ PVOID CleanupContext
    );

VOID
NTAPI
ThreadPoolTestLegacyTimerCallback (
    _Inout_opt_ PVOID Context,
    _In_ BOOLEAN TimerOrWaitFired
    );

VOID
NTAPI
ThreadPoolTestLegacyWaitCallback (
    _Inout_opt_ PVOID Context,
    _In_ BOOLEAN TimerOrWaitFired
    );

VOID
ThreadPoolTestRelativeFileTime (
    _Out_ PFILETIME FileTime,
    _In_ LONG Milliseconds
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, QueueUserWorkItemTest)
#pragma alloc_text(PAGE, LegacyThreadWorkFunction)
#pragma alloc_text(PAGE, ThreadpoolWorkTimerWaitCleanupGroupTest)
#pragma alloc_text(PAGE, ThreadPoolTestWorkCallback)
#pragma alloc_text(PAGE, ThreadPoolTestRaceDllCallback)
#pragma alloc_text(PAGE, ThreadPoolTestDeferredFreeCallback)
#pragma alloc_text(PAGE, ThreadPoolTestFinalizationWorkCallback)
#pragma alloc_text(PAGE, ThreadPoolTestFinalizationCallback)
#pragma alloc_text(PAGE, ThreadPoolTestCleanupGroupWorkCallback)
#pragma alloc_text(PAGE, ThreadPoolTestCleanupGroupCancelCallback)
#pragma alloc_text(PAGE, ThreadPoolTestTimerCallback)
#pragma alloc_text(PAGE, ThreadPoolTestWaitCallback)
#pragma alloc_text(PAGE, ThreadPoolTestTimerWaitCleanupCallback)
#pragma alloc_text(PAGE, ThreadPoolTestLegacyTimerCallback)
#pragma alloc_text(PAGE, ThreadPoolTestLegacyWaitCallback)
#pragma alloc_text(PAGE, ThreadPoolTestRelativeFileTime)
#endif
#else
#include <windows.h>
#include <stdio.h>

#define DbgPrint        printf
#define PAGED_CODE()
#endif

typedef struct _THREADPOOL_MODULE_CONTEXT {
    HMODULE Module;
    LONG Count;
    LONG ModuleWasLoaded;
} THREADPOOL_MODULE_CONTEXT, *PTHREADPOOL_MODULE_CONTEXT;

typedef struct _THREADPOOL_FINALIZATION_CONTEXT {
    LONG WorkCallbacks;
    LONG FinalizationCallbacks;
    LONG ContextMatches;
    PVOID ExpectedContext;
} THREADPOOL_FINALIZATION_CONTEXT, *PTHREADPOOL_FINALIZATION_CONTEXT;

typedef struct _THREADPOOL_CLEANUP_GROUP_CONTEXT {
    LONG WorkCallbacks;
    LONG CleanupCallbacks;
    LONG CleanupContextMatches;
    LONG ObjectContextMatches;
    PVOID ExpectedCleanupContext;
} THREADPOOL_CLEANUP_GROUP_CONTEXT, *PTHREADPOOL_CLEANUP_GROUP_CONTEXT;

typedef struct _THREADPOOL_TIMER_WAIT_CONTEXT {
    LONG TimerCallbacks;
    LONG WaitCallbacks;
    LONG WaitSignaledCallbacks;
    LONG WaitTimeoutCallbacks;
    LONG CleanupCallbacks;
    LONG CleanupContextMatches;
    LONG ObjectContextMatches;
    PVOID ExpectedCleanupContext;
} THREADPOOL_TIMER_WAIT_CONTEXT, *PTHREADPOOL_TIMER_WAIT_CONTEXT;

typedef struct _THREADPOOL_LEGACY_CONTEXT {
    LONG TimerCallbacks;
    LONG TimerBooleanCallbacks;
    LONG WaitCallbacks;
    LONG WaitSignaledCallbacks;
    LONG WaitTimeoutCallbacks;
} THREADPOOL_LEGACY_CONTEXT, *PTHREADPOOL_LEGACY_CONTEXT;

DWORD
WINAPI
LegacyThreadWorkFunction (
    PLONG Count
    )
{
    PAGED_CODE();

    InterlockedIncrement( Count );

    return 0;
}

BOOLEAN
QueueUserWorkItemTest (
    VOID
    )
{
    LONG count = 10;
    LONG processed = 0;

    PAGED_CODE();

    for (count = 0; count < 10; count++) {
        if (! QueueUserWorkItem(LegacyThreadWorkFunction, &processed, WT_EXECUTEDEFAULT)) {
            break;
        }
    }

    while (InterlockedCompareExchange( &processed, 0, count ) != count) {
        YieldProcessor();
    }
    return TRUE;
}



VOID
NTAPI
ThreadPoolTestWorkCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    )
{
    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    PAGED_CODE();

    InterlockedDecrement((PLONG)Context);
}

VOID
NTAPI
ThreadPoolTestRaceDllCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    )
{
    PTHREADPOOL_MODULE_CONTEXT ModuleContext = (PTHREADPOOL_MODULE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    PAGED_CODE();

    FreeLibrary( ModuleContext->Module );
    if (GetModuleHandleW( L"Test.dll" ) != NULL) {
        InterlockedIncrement( &ModuleContext->ModuleWasLoaded );
    }
    InterlockedIncrement( &ModuleContext->Count );
}

VOID
NTAPI
ThreadPoolTestDeferredFreeCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    )
{
    PTHREADPOOL_MODULE_CONTEXT ModuleContext = (PTHREADPOOL_MODULE_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Work);

    PAGED_CODE();

    FreeLibraryWhenCallbackReturns( Instance,
                                    ModuleContext->Module );
    InterlockedIncrement( &ModuleContext->Count );
}

VOID
NTAPI
ThreadPoolTestFinalizationWorkCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    )
{
    PTHREADPOOL_FINALIZATION_CONTEXT FinalizationContext = (PTHREADPOOL_FINALIZATION_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    PAGED_CODE();

    if (FinalizationContext != NULL) {
        InterlockedIncrement( &FinalizationContext->WorkCallbacks );
    }
}

VOID
NTAPI
ThreadPoolTestFinalizationCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context
    )
{
    PTHREADPOOL_FINALIZATION_CONTEXT FinalizationContext = (PTHREADPOOL_FINALIZATION_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Instance);

    PAGED_CODE();

    if (FinalizationContext == NULL) {
        return;
    }

    InterlockedIncrement( &FinalizationContext->FinalizationCallbacks );
    if (Context == FinalizationContext->ExpectedContext) {
        InterlockedIncrement( &FinalizationContext->ContextMatches );
    }
}

VOID
NTAPI
ThreadPoolTestCleanupGroupWorkCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    )
{
    PTHREADPOOL_CLEANUP_GROUP_CONTEXT CleanupContext = (PTHREADPOOL_CLEANUP_GROUP_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Work);

    PAGED_CODE();

    InterlockedIncrement( &CleanupContext->WorkCallbacks );
}

VOID
NTAPI
ThreadPoolTestCleanupGroupCancelCallback (
    _Inout_opt_ PVOID ObjectContext,
    _Inout_opt_ PVOID CleanupContext
    )
{
    PTHREADPOOL_CLEANUP_GROUP_CONTEXT GroupContext = (PTHREADPOOL_CLEANUP_GROUP_CONTEXT)ObjectContext;

    PAGED_CODE();

    if (GroupContext == NULL) {
        return;
    }

    InterlockedIncrement( &GroupContext->CleanupCallbacks );
    if (CleanupContext == GroupContext->ExpectedCleanupContext) {
        InterlockedIncrement( &GroupContext->CleanupContextMatches );
    }
    if (ObjectContext == GroupContext) {
        InterlockedIncrement( &GroupContext->ObjectContextMatches );
    }
}

VOID
NTAPI
ThreadPoolTestTimerCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_TIMER             Timer
    )
{
    PTHREADPOOL_TIMER_WAIT_CONTEXT TimerWaitContext = (PTHREADPOOL_TIMER_WAIT_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Timer);

    PAGED_CODE();

    InterlockedIncrement( &TimerWaitContext->TimerCallbacks );
}

VOID
NTAPI
ThreadPoolTestWaitCallback (
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WAIT              Wait,
    _In_        TP_WAIT_RESULT        WaitResult
    )
{
    PTHREADPOOL_TIMER_WAIT_CONTEXT TimerWaitContext = (PTHREADPOOL_TIMER_WAIT_CONTEXT)Context;

    UNREFERENCED_PARAMETER(Instance);
    UNREFERENCED_PARAMETER(Wait);

    PAGED_CODE();

    InterlockedIncrement( &TimerWaitContext->WaitCallbacks );
    if (WaitResult == WAIT_TIMEOUT) {
        InterlockedIncrement( &TimerWaitContext->WaitTimeoutCallbacks );
    } else if (WaitResult == WAIT_OBJECT_0) {
        InterlockedIncrement( &TimerWaitContext->WaitSignaledCallbacks );
    }
}

VOID
NTAPI
ThreadPoolTestTimerWaitCleanupCallback (
    _Inout_opt_ PVOID ObjectContext,
    _Inout_opt_ PVOID CleanupContext
    )
{
    PTHREADPOOL_TIMER_WAIT_CONTEXT TimerWaitContext = (PTHREADPOOL_TIMER_WAIT_CONTEXT)ObjectContext;

    PAGED_CODE();

    if (TimerWaitContext == NULL) {
        return;
    }

    InterlockedIncrement( &TimerWaitContext->CleanupCallbacks );
    if (CleanupContext == TimerWaitContext->ExpectedCleanupContext) {
        InterlockedIncrement( &TimerWaitContext->CleanupContextMatches );
    }
    if (ObjectContext == TimerWaitContext) {
        InterlockedIncrement( &TimerWaitContext->ObjectContextMatches );
    }
}

VOID
NTAPI
ThreadPoolTestLegacyTimerCallback (
    _Inout_opt_ PVOID Context,
    _In_ BOOLEAN TimerOrWaitFired
    )
{
    PTHREADPOOL_LEGACY_CONTEXT LegacyContext = (PTHREADPOOL_LEGACY_CONTEXT)Context;

    PAGED_CODE();

    InterlockedIncrement( &LegacyContext->TimerCallbacks );
    if (TimerOrWaitFired) {
        InterlockedIncrement( &LegacyContext->TimerBooleanCallbacks );
    }
}

VOID
NTAPI
ThreadPoolTestLegacyWaitCallback (
    _Inout_opt_ PVOID Context,
    _In_ BOOLEAN TimerOrWaitFired
    )
{
    PTHREADPOOL_LEGACY_CONTEXT LegacyContext = (PTHREADPOOL_LEGACY_CONTEXT)Context;

    PAGED_CODE();

    InterlockedIncrement( &LegacyContext->WaitCallbacks );
    if (TimerOrWaitFired) {
        InterlockedIncrement( &LegacyContext->WaitTimeoutCallbacks );
    } else {
        InterlockedIncrement( &LegacyContext->WaitSignaledCallbacks );
    }
}

VOID
ThreadPoolTestRelativeFileTime (
    _Out_ PFILETIME FileTime,
    _In_ LONG Milliseconds
    )
{
    LARGE_INTEGER DueTime;

    PAGED_CODE();

    DueTime.QuadPart = -((LONGLONG)Milliseconds * 10000);
    FileTime->dwLowDateTime = DueTime.LowPart;
    FileTime->dwHighDateTime = DueTime.HighPart;
}

BOOLEAN
ThreadpoolWorkTimerWaitCleanupGroupTest (
    VOID
    )
{
    PTP_WORK works[10] = { NULL, };
    LONG count = ARRAYSIZE(works);
    PTP_WORK work = NULL;
    TP_CALLBACK_ENVIRON CallbackEnvironment;
    THREADPOOL_MODULE_CONTEXT ModuleContext;
    THREADPOOL_FINALIZATION_CONTEXT FinalizationContext;
    THREADPOOL_CLEANUP_GROUP_CONTEXT CleanupGroupContext;
    THREADPOOL_TIMER_WAIT_CONTEXT TimerWaitContext;
    THREADPOOL_LEGACY_CONTEXT LegacyContext;
    PTP_CLEANUP_GROUP CleanupGroup = NULL;
    PTP_TIMER Timer = NULL;
    PTP_WAIT Wait = NULL;
    PTP_POOL CustomPool = NULL;
    HANDLE WaitEvent = NULL;
    HANDLE LegacyTimer = NULL;
    HANDLE LegacyWait = NULL;
    HANDLE LegacyEvent = NULL;
    FILETIME DueTime;
    TP_POOL_STACK_INFORMATION StackInformation;
    PVOID CleanupToken = &CleanupGroupContext;
    PVOID TimerWaitCleanupToken = &TimerWaitContext;
    BOOLEAN Result = FALSE;

    PAGED_CODE();
    RtlZeroMemory( &ModuleContext,
                   sizeof(ModuleContext) );
    RtlZeroMemory( &FinalizationContext,
                   sizeof(FinalizationContext) );
    RtlZeroMemory( &CleanupGroupContext,
                   sizeof(CleanupGroupContext) );
    RtlZeroMemory( &TimerWaitContext,
                   sizeof(TimerWaitContext) );
    RtlZeroMemory( &LegacyContext,
                   sizeof(LegacyContext) );

    for (int i = 0; i < ARRAYSIZE(works); ++i) {
        works[i] = CreateThreadpoolWork(ThreadPoolTestWorkCallback, &count, NULL);
        if (works[i] == NULL) {
            DbgPrint("[Failed] CreateThreadpoolWork basic ErrorCode = %lu\n", GetLastError());
            goto FinalCleanup;
        }
    }

    for (int i = 0; i < ARRAYSIZE(works); ++i) {
        SubmitThreadpoolWork(works[i]);
    }

    for (int i = 0; i < ARRAYSIZE(works); ++i) {
        if (works[i]) {
            WaitForThreadpoolWorkCallbacks(works[i], FALSE);
            CloseThreadpoolWork(works[i]);
            works[i] = NULL;
        }
    }

    if (count != 0) {
        DbgPrint("[Failed] Threadpool basic callbacks Count = %ld\n", count);
        goto FinalCleanup;
    }

    count = 2;
    work = CreateThreadpoolWork(ThreadPoolTestWorkCallback, &count, NULL);
    if (work == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWork reuse ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }

    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    CloseThreadpoolWork(work);
    work = NULL;

    if (count != 0) {
        DbgPrint("[Failed] Threadpool reuse Count = %ld\n", count);
        goto FinalCleanup;
    }

    CustomPool = CreateThreadpool( NULL );
    if (CustomPool == NULL) {
        DbgPrint("[Failed] CreateThreadpool ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SetThreadpoolThreadMaximum( CustomPool,
                                2 );
    if (!SetThreadpoolThreadMinimum( CustomPool,
                                     1 )) {
        DbgPrint("[Failed] SetThreadpoolThreadMinimum ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }

    RtlZeroMemory( &StackInformation,
                   sizeof(StackInformation) );
    if (!QueryThreadpoolStackInformation( CustomPool,
                                          &StackInformation )) {
        DbgPrint("[Failed] QueryThreadpoolStackInformation ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    if (!SetThreadpoolStackInformation( CustomPool,
                                        &StackInformation )) {
        DbgPrint("[Failed] SetThreadpoolStackInformation ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }

    TpInitializeCallbackEnviron( &CallbackEnvironment );
    TpSetCallbackThreadpool( &CallbackEnvironment,
                             CustomPool );
    count = 1;
    work = CreateThreadpoolWork(ThreadPoolTestWorkCallback, &count, &CallbackEnvironment);
    if (work == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWork custom pool ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    CloseThreadpoolWork(work);
    work = NULL;
    TpDestroyCallbackEnviron( &CallbackEnvironment );

    if (count != 0) {
        DbgPrint("[Failed] Threadpool custom pool Count = %ld\n", count);
        goto FinalCleanup;
    }
    DbgPrint("[Success] Threadpool custom pool callback environment\n");
    CloseThreadpool( CustomPool );
    CustomPool = NULL;

    TpInitializeCallbackEnviron( &CallbackEnvironment );
    TpSetCallbackLongFunction( &CallbackEnvironment );
    TpSetCallbackPersistent( &CallbackEnvironment );
#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
    TpSetCallbackPriority( &CallbackEnvironment,
                           TP_CALLBACK_PRIORITY_HIGH );
#endif

    count = 1;
    work = CreateThreadpoolWork(ThreadPoolTestWorkCallback, &count, &CallbackEnvironment);
    if (work == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWork callback environment ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    CloseThreadpoolWork(work);
    work = NULL;
    TpDestroyCallbackEnviron( &CallbackEnvironment );

    if (count != 0) {
        DbgPrint("[Failed] Threadpool callback environment Count = %ld\n", count);
        goto FinalCleanup;
    }

    RtlZeroMemory( &ModuleContext,
                   sizeof(ModuleContext) );
    ModuleContext.Module = LoadLibraryW( L"Test.dll" );
    if (ModuleContext.Module == NULL) {
        DbgPrint("[Failed] LoadLibraryW(Test.dll) for RaceWithDll ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }

    TpInitializeCallbackEnviron( &CallbackEnvironment );
    TpSetCallbackRaceWithDll( &CallbackEnvironment,
                              ModuleContext.Module );
    work = CreateThreadpoolWork(ThreadPoolTestRaceDllCallback, &ModuleContext, &CallbackEnvironment);
    if (work == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWork RaceWithDll ErrorCode = %lu\n", GetLastError());
        FreeLibrary( ModuleContext.Module );
        ModuleContext.Module = NULL;
        goto FinalCleanup;
    }
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    CloseThreadpoolWork(work);
    work = NULL;
    TpDestroyCallbackEnviron( &CallbackEnvironment );

    if (ModuleContext.Count != 1 ||
        ModuleContext.ModuleWasLoaded != 1) {
        DbgPrint("[Failed] Threadpool RaceWithDll Count = %ld Loaded = %ld Handle = %p\n",
                 ModuleContext.Count,
                 ModuleContext.ModuleWasLoaded,
                 GetModuleHandleW( L"Test.dll" ));
        if (GetModuleHandleW( L"Test.dll" ) != NULL) {
            FreeLibrary( ModuleContext.Module );
        }
        ModuleContext.Module = NULL;
        goto FinalCleanup;
    }

#if _KERNEL_MODE
    if (GetModuleHandleW( L"Test.dll" ) != NULL) {
        DbgPrint("[Failed] Threadpool RaceWithDll release Handle = %p\n",
                 GetModuleHandleW( L"Test.dll" ));
        FreeLibrary( ModuleContext.Module );
        ModuleContext.Module = NULL;
        goto FinalCleanup;
    }
#else
    if (GetModuleHandleW( L"Test.dll" ) != NULL) {
        FreeLibrary( ModuleContext.Module );
    }
#endif
    ModuleContext.Module = NULL;

    RtlZeroMemory( &ModuleContext,
                   sizeof(ModuleContext) );
    ModuleContext.Module = LoadLibraryW( L"Test.dll" );
    if (ModuleContext.Module == NULL) {
        DbgPrint("[Failed] LoadLibraryW(Test.dll) for deferred free ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }

    work = CreateThreadpoolWork(ThreadPoolTestDeferredFreeCallback, &ModuleContext, NULL);
    if (work == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWork deferred free ErrorCode = %lu\n", GetLastError());
        FreeLibrary( ModuleContext.Module );
        ModuleContext.Module = NULL;
        goto FinalCleanup;
    }
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    CloseThreadpoolWork(work);
    work = NULL;

    if (ModuleContext.Count != 1 ||
        GetModuleHandleW( L"Test.dll" ) != NULL) {
        DbgPrint("[Failed] Threadpool deferred free Count = %ld Handle = %p\n",
                 ModuleContext.Count,
                 GetModuleHandleW( L"Test.dll" ));
        if (GetModuleHandleW( L"Test.dll" ) != NULL) {
            FreeLibrary( ModuleContext.Module );
        }
        ModuleContext.Module = NULL;
        goto FinalCleanup;
    }
    ModuleContext.Module = NULL;

    FinalizationContext.ExpectedContext = &FinalizationContext;
    TpInitializeCallbackEnviron( &CallbackEnvironment );
    TpSetCallbackFinalizationCallback( &CallbackEnvironment,
                                       ThreadPoolTestFinalizationCallback );
    work = CreateThreadpoolWork(ThreadPoolTestFinalizationWorkCallback,
                                &FinalizationContext,
                                &CallbackEnvironment);
    if (work == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWork finalization callback ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SubmitThreadpoolWork(work);
    WaitForThreadpoolWorkCallbacks(work, FALSE);
    CloseThreadpoolWork(work);
    work = NULL;
    TpDestroyCallbackEnviron( &CallbackEnvironment );

    if (FinalizationContext.WorkCallbacks != 1 ||
        FinalizationContext.FinalizationCallbacks != 1 ||
        FinalizationContext.ContextMatches != 1) {
        DbgPrint("[Failed] Threadpool finalization callback Work = %ld Finalization = %ld Context = %ld\n",
                 FinalizationContext.WorkCallbacks,
                 FinalizationContext.FinalizationCallbacks,
                 FinalizationContext.ContextMatches);
        goto FinalCleanup;
    }
    DbgPrint("[Success] Threadpool finalization callback\n");

    CleanupGroup = CreateThreadpoolCleanupGroup();
    if (CleanupGroup == NULL) {
        DbgPrint("[Failed] CreateThreadpoolCleanupGroup ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }

    TpInitializeCallbackEnviron( &CallbackEnvironment );
    TpSetCallbackCleanupGroup( &CallbackEnvironment,
                               CleanupGroup,
                               NULL );
    for (int i = 0; i < 4; i++) {
        work = CreateThreadpoolWork(ThreadPoolTestCleanupGroupWorkCallback,
                                    &CleanupGroupContext,
                                    &CallbackEnvironment);
        if (work == NULL) {
            DbgPrint("[Failed] CreateThreadpoolWork cleanup group submit ErrorCode = %lu\n", GetLastError());
            goto FinalCleanup;
        }
        SubmitThreadpoolWork(work);
        work = NULL;
    }
    CloseThreadpoolCleanupGroupMembers( CleanupGroup,
                                        FALSE,
                                        NULL );
    TpDestroyCallbackEnviron( &CallbackEnvironment );

    if (CleanupGroupContext.WorkCallbacks != 4) {
        DbgPrint("[Failed] Threadpool cleanup group submit WorkCallbacks = %ld\n",
                 CleanupGroupContext.WorkCallbacks);
        goto FinalCleanup;
    }

    CleanupGroupContext.ExpectedCleanupContext = CleanupToken;
    TpInitializeCallbackEnviron( &CallbackEnvironment );
    TpSetCallbackCleanupGroup( &CallbackEnvironment,
                               CleanupGroup,
                               ThreadPoolTestCleanupGroupCancelCallback );
    for (int i = 0; i < 3; i++) {
        work = CreateThreadpoolWork(ThreadPoolTestCleanupGroupWorkCallback,
                                    &CleanupGroupContext,
                                    &CallbackEnvironment);
        if (work == NULL) {
            DbgPrint("[Failed] CreateThreadpoolWork cleanup group cancel ErrorCode = %lu\n", GetLastError());
            goto FinalCleanup;
        }
        work = NULL;
    }
    CloseThreadpoolCleanupGroupMembers( CleanupGroup,
                                        TRUE,
                                        CleanupToken );
    TpDestroyCallbackEnviron( &CallbackEnvironment );

    if (CleanupGroupContext.WorkCallbacks != 4 ||
        CleanupGroupContext.CleanupCallbacks != 3 ||
        CleanupGroupContext.CleanupContextMatches != 3 ||
        CleanupGroupContext.ObjectContextMatches != 3) {
        DbgPrint("[Failed] Threadpool cleanup group cancel Work = %ld Cleanup = %ld CleanupContext = %ld ObjectContext = %ld\n",
                 CleanupGroupContext.WorkCallbacks,
                 CleanupGroupContext.CleanupCallbacks,
                 CleanupGroupContext.CleanupContextMatches,
                 CleanupGroupContext.ObjectContextMatches);
        goto FinalCleanup;
    }

    TpInitializeCallbackEnviron( &CallbackEnvironment );
    TpSetCallbackCleanupGroup( &CallbackEnvironment,
                               CleanupGroup,
                               ThreadPoolTestCleanupGroupCancelCallback );
    work = CreateThreadpoolWork(ThreadPoolTestCleanupGroupWorkCallback,
                                &CleanupGroupContext,
                                &CallbackEnvironment);
    if (work == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWork cleanup group unregister ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    CloseThreadpoolWork(work);
    work = NULL;
    CloseThreadpoolCleanupGroupMembers( CleanupGroup,
                                        TRUE,
                                        CleanupToken );
    TpDestroyCallbackEnviron( &CallbackEnvironment );

    if (CleanupGroupContext.CleanupCallbacks != 3 ||
        CleanupGroupContext.WorkCallbacks != 4) {
        DbgPrint("[Failed] Threadpool cleanup group unregister Work = %ld Cleanup = %ld\n",
                 CleanupGroupContext.WorkCallbacks,
                 CleanupGroupContext.CleanupCallbacks);
        goto FinalCleanup;
    }

    ThreadPoolTestRelativeFileTime( &DueTime,
                                    20 );
    Timer = CreateThreadpoolTimer( ThreadPoolTestTimerCallback,
                                   &TimerWaitContext,
                                   NULL );
    if (Timer == NULL) {
        DbgPrint("[Failed] CreateThreadpoolTimer basic ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SetThreadpoolTimer( Timer,
                        &DueTime,
                        0,
                        0 );
    for (int i = 0; i < 200 && TimerWaitContext.TimerCallbacks < 1; i++) {
        Sleep( 1 );
    }
    WaitForThreadpoolTimerCallbacks( Timer,
                                     FALSE );
    if (TimerWaitContext.TimerCallbacks != 1 ||
        ! IsThreadpoolTimerSet( Timer )) {
        DbgPrint("[Failed] Threadpool timer basic Count = %ld IsSet = %d\n",
                 TimerWaitContext.TimerCallbacks,
                 IsThreadpoolTimerSet( Timer ));
        goto FinalCleanup;
    }
    SetThreadpoolTimer( Timer,
                        NULL,
                        0,
                        0 );
    if (IsThreadpoolTimerSet( Timer )) {
        DbgPrint("[Failed] Threadpool timer basic cancel after fire IsSet = %d\n",
                 IsThreadpoolTimerSet( Timer ));
        goto FinalCleanup;
    }
    CloseThreadpoolTimer( Timer );
    Timer = NULL;

    ThreadPoolTestRelativeFileTime( &DueTime,
                                    500 );
    Timer = CreateThreadpoolTimer( ThreadPoolTestTimerCallback,
                                   &TimerWaitContext,
                                   NULL );
    if (Timer == NULL) {
        DbgPrint("[Failed] CreateThreadpoolTimer cancel ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SetThreadpoolTimer( Timer,
                        &DueTime,
                        0,
                        0 );
    if (! IsThreadpoolTimerSet( Timer )) {
        DbgPrint("[Failed] Threadpool timer cancel was not set\n");
        goto FinalCleanup;
    }
    SetThreadpoolTimer( Timer,
                        NULL,
                        0,
                        0 );
    WaitForThreadpoolTimerCallbacks( Timer,
                                     TRUE );
    Sleep( 20 );
    if (TimerWaitContext.TimerCallbacks != 1 ||
        IsThreadpoolTimerSet( Timer )) {
        DbgPrint("[Failed] Threadpool timer cancel Count = %ld IsSet = %d\n",
                 TimerWaitContext.TimerCallbacks,
                 IsThreadpoolTimerSet( Timer ));
        goto FinalCleanup;
    }
    CloseThreadpoolTimer( Timer );
    Timer = NULL;

    WaitEvent = CreateEventW( NULL,
                              FALSE,
                              FALSE,
                              NULL );
    if (WaitEvent == NULL) {
        DbgPrint("[Failed] CreateEventW for threadpool wait ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    Wait = CreateThreadpoolWait( ThreadPoolTestWaitCallback,
                                 &TimerWaitContext,
                                 NULL );
    if (Wait == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWait signal ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SetThreadpoolWait( Wait,
                       WaitEvent,
                       NULL );
    SetEvent( WaitEvent );
    for (int i = 0; i < 200 && TimerWaitContext.WaitSignaledCallbacks < 1; i++) {
        Sleep( 1 );
    }
    WaitForThreadpoolWaitCallbacks( Wait,
                                    FALSE );
    if (TimerWaitContext.WaitCallbacks != 1 ||
        TimerWaitContext.WaitSignaledCallbacks != 1 ||
        TimerWaitContext.WaitTimeoutCallbacks != 0) {
        DbgPrint("[Failed] Threadpool wait signal Wait = %ld Signal = %ld Timeout = %ld\n",
                 TimerWaitContext.WaitCallbacks,
                 TimerWaitContext.WaitSignaledCallbacks,
                 TimerWaitContext.WaitTimeoutCallbacks);
        goto FinalCleanup;
    }
    CloseThreadpoolWait( Wait );
    Wait = NULL;
    CloseHandle( WaitEvent );
    WaitEvent = NULL;

    WaitEvent = CreateEventW( NULL,
                              FALSE,
                              FALSE,
                              NULL );
    if (WaitEvent == NULL) {
        DbgPrint("[Failed] CreateEventW for threadpool wait timeout ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    ThreadPoolTestRelativeFileTime( &DueTime,
                                    20 );
    Wait = CreateThreadpoolWait( ThreadPoolTestWaitCallback,
                                 &TimerWaitContext,
                                 NULL );
    if (Wait == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWait timeout ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SetThreadpoolWait( Wait,
                       WaitEvent,
                       &DueTime );
    for (int i = 0; i < 200 && TimerWaitContext.WaitTimeoutCallbacks < 1; i++) {
        Sleep( 1 );
    }
    WaitForThreadpoolWaitCallbacks( Wait,
                                    FALSE );
    if (TimerWaitContext.WaitCallbacks != 2 ||
        TimerWaitContext.WaitSignaledCallbacks != 1 ||
        TimerWaitContext.WaitTimeoutCallbacks != 1) {
        DbgPrint("[Failed] Threadpool wait timeout Wait = %ld Signal = %ld Timeout = %ld\n",
                 TimerWaitContext.WaitCallbacks,
                 TimerWaitContext.WaitSignaledCallbacks,
                 TimerWaitContext.WaitTimeoutCallbacks);
        goto FinalCleanup;
    }
    CloseThreadpoolWait( Wait );
    Wait = NULL;
    CloseHandle( WaitEvent );
    WaitEvent = NULL;

    if (! CreateTimerQueueTimer( &LegacyTimer,
                                 NULL,
                                 ThreadPoolTestLegacyTimerCallback,
                                 &LegacyContext,
                                 20,
                                 0,
                                 WT_EXECUTEDEFAULT )) {
        DbgPrint("[Failed] CreateTimerQueueTimer basic ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    for (int i = 0; i < 200 && LegacyContext.TimerCallbacks < 1; i++) {
        Sleep( 1 );
    }
    if (! DeleteTimerQueueTimer( NULL,
                                 LegacyTimer,
                                 INVALID_HANDLE_VALUE )) {
        DbgPrint("[Failed] DeleteTimerQueueTimer basic ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    LegacyTimer = NULL;
    if (LegacyContext.TimerCallbacks != 1 ||
        LegacyContext.TimerBooleanCallbacks != 1) {
        DbgPrint("[Failed] Legacy timer queue Count = %ld Boolean = %ld\n",
                 LegacyContext.TimerCallbacks,
                 LegacyContext.TimerBooleanCallbacks);
        goto FinalCleanup;
    }

    LegacyEvent = CreateEventW( NULL,
                                FALSE,
                                FALSE,
                                NULL );
    if (LegacyEvent == NULL) {
        DbgPrint("[Failed] CreateEventW for legacy registered wait ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    if (! RegisterWaitForSingleObject( &LegacyWait,
                                       LegacyEvent,
                                       ThreadPoolTestLegacyWaitCallback,
                                       &LegacyContext,
                                       INFINITE,
                                       WT_EXECUTEONLYONCE )) {
        DbgPrint("[Failed] RegisterWaitForSingleObject signal ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SetEvent( LegacyEvent );
    for (int i = 0; i < 200 && LegacyContext.WaitSignaledCallbacks < 1; i++) {
        Sleep( 1 );
    }
    if (! UnregisterWaitEx( LegacyWait,
                            INVALID_HANDLE_VALUE )) {
        DbgPrint("[Failed] UnregisterWaitEx signal ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    LegacyWait = NULL;
    CloseHandle( LegacyEvent );
    LegacyEvent = NULL;
    if (LegacyContext.WaitCallbacks != 1 ||
        LegacyContext.WaitSignaledCallbacks != 1 ||
        LegacyContext.WaitTimeoutCallbacks != 0) {
        DbgPrint("[Failed] Legacy registered wait signal Wait = %ld Signal = %ld Timeout = %ld\n",
                 LegacyContext.WaitCallbacks,
                 LegacyContext.WaitSignaledCallbacks,
                 LegacyContext.WaitTimeoutCallbacks);
        goto FinalCleanup;
    }

    LegacyEvent = CreateEventW( NULL,
                                FALSE,
                                FALSE,
                                NULL );
    if (LegacyEvent == NULL) {
        DbgPrint("[Failed] CreateEventW for legacy registered wait timeout ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    if (! RegisterWaitForSingleObject( &LegacyWait,
                                       LegacyEvent,
                                       ThreadPoolTestLegacyWaitCallback,
                                       &LegacyContext,
                                       20,
                                       WT_EXECUTEONLYONCE )) {
        DbgPrint("[Failed] RegisterWaitForSingleObject timeout ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    for (int i = 0; i < 200 && LegacyContext.WaitTimeoutCallbacks < 1; i++) {
        Sleep( 1 );
    }
    if (! UnregisterWaitEx( LegacyWait,
                            INVALID_HANDLE_VALUE )) {
        DbgPrint("[Failed] UnregisterWaitEx timeout ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    LegacyWait = NULL;
    CloseHandle( LegacyEvent );
    LegacyEvent = NULL;
    if (LegacyContext.WaitCallbacks != 2 ||
        LegacyContext.WaitSignaledCallbacks != 1 ||
        LegacyContext.WaitTimeoutCallbacks != 1) {
        DbgPrint("[Failed] Legacy registered wait timeout Wait = %ld Signal = %ld Timeout = %ld\n",
                 LegacyContext.WaitCallbacks,
                 LegacyContext.WaitSignaledCallbacks,
                 LegacyContext.WaitTimeoutCallbacks);
        goto FinalCleanup;
    }

    TimerWaitContext.ExpectedCleanupContext = TimerWaitCleanupToken;
    TpInitializeCallbackEnviron( &CallbackEnvironment );
    TpSetCallbackCleanupGroup( &CallbackEnvironment,
                               CleanupGroup,
                               ThreadPoolTestTimerWaitCleanupCallback );

    ThreadPoolTestRelativeFileTime( &DueTime,
                                    500 );
    Timer = CreateThreadpoolTimer( ThreadPoolTestTimerCallback,
                                   &TimerWaitContext,
                                   &CallbackEnvironment );
    if (Timer == NULL) {
        DbgPrint("[Failed] CreateThreadpoolTimer cleanup group ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SetThreadpoolTimer( Timer,
                        &DueTime,
                        0,
                        0 );

    WaitEvent = CreateEventW( NULL,
                              FALSE,
                              FALSE,
                              NULL );
    if (WaitEvent == NULL) {
        DbgPrint("[Failed] CreateEventW cleanup group wait ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    Wait = CreateThreadpoolWait( ThreadPoolTestWaitCallback,
                                 &TimerWaitContext,
                                 &CallbackEnvironment );
    if (Wait == NULL) {
        DbgPrint("[Failed] CreateThreadpoolWait cleanup group ErrorCode = %lu\n", GetLastError());
        goto FinalCleanup;
    }
    SetThreadpoolWait( Wait,
                       WaitEvent,
                       &DueTime );
    CloseThreadpoolCleanupGroupMembers( CleanupGroup,
                                        TRUE,
                                        TimerWaitCleanupToken );
    Timer = NULL;
    Wait = NULL;
    TpDestroyCallbackEnviron( &CallbackEnvironment );
    CloseHandle( WaitEvent );
    WaitEvent = NULL;

    if (TimerWaitContext.TimerCallbacks != 1 ||
        TimerWaitContext.WaitCallbacks != 2 ||
        TimerWaitContext.CleanupCallbacks != 2 ||
        TimerWaitContext.CleanupContextMatches != 2 ||
        TimerWaitContext.ObjectContextMatches != 2) {
        DbgPrint("[Failed] Threadpool timer/wait cleanup group Timer = %ld Wait = %ld Cleanup = %ld CleanupContext = %ld ObjectContext = %ld\n",
                 TimerWaitContext.TimerCallbacks,
                 TimerWaitContext.WaitCallbacks,
                 TimerWaitContext.CleanupCallbacks,
                 TimerWaitContext.CleanupContextMatches,
                 TimerWaitContext.ObjectContextMatches);
        goto FinalCleanup;
    }

    CloseThreadpoolCleanupGroup( CleanupGroup );
    CleanupGroup = NULL;

    DbgPrint("[Success] Threadpool callback environment, cleanup group, timer, wait, and reuse\n");
    Result = TRUE;

FinalCleanup:
    if (Timer) {
        WaitForThreadpoolTimerCallbacks( Timer,
                                         TRUE );
        CloseThreadpoolTimer( Timer );
    }
    if (Wait) {
        WaitForThreadpoolWaitCallbacks( Wait,
                                        TRUE );
        CloseThreadpoolWait( Wait );
    }
    if (WaitEvent) {
        CloseHandle( WaitEvent );
    }
    if (LegacyTimer) {
        DeleteTimerQueueTimer( NULL,
                               LegacyTimer,
                               INVALID_HANDLE_VALUE );
    }
    if (LegacyWait) {
        UnregisterWaitEx( LegacyWait,
                          INVALID_HANDLE_VALUE );
    }
    if (LegacyEvent) {
        CloseHandle( LegacyEvent );
    }
    if (work) {
        WaitForThreadpoolWorkCallbacks(work, TRUE);
        CloseThreadpoolWork(work);
    }
    for (int i = 0; i < ARRAYSIZE(works); ++i) {
        if (works[i]) {
            WaitForThreadpoolWorkCallbacks(works[i], TRUE);
            CloseThreadpoolWork(works[i]);
        }
    }
    if (ModuleContext.Module != NULL &&
        GetModuleHandleW( L"Test.dll" ) != NULL) {
        FreeLibrary( ModuleContext.Module );
    }
    if (CleanupGroup != NULL) {
        CloseThreadpoolCleanupGroupMembers( CleanupGroup,
                                            TRUE,
                                            CleanupToken );
        CloseThreadpoolCleanupGroup( CleanupGroup );
    }
    if (CustomPool != NULL) {
        CloseThreadpool( CustomPool );
    }
    return Result;
}
