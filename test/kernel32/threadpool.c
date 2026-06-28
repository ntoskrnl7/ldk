#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
LegacyThreadPoolTest (
    VOID
    );

DWORD
WINAPI
LegacyThreadWorkFunction (
    PLONG Count
    );

BOOLEAN
ThreadPoolTest (
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LegacyThreadPoolTest)
#pragma alloc_text(PAGE, LegacyThreadWorkFunction)
#pragma alloc_text(PAGE, ThreadPoolTest)
#pragma alloc_text(PAGE, ThreadPoolTestWorkCallback)
#pragma alloc_text(PAGE, ThreadPoolTestRaceDllCallback)
#pragma alloc_text(PAGE, ThreadPoolTestDeferredFreeCallback)
#pragma alloc_text(PAGE, ThreadPoolTestCleanupGroupWorkCallback)
#pragma alloc_text(PAGE, ThreadPoolTestCleanupGroupCancelCallback)
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

typedef struct _THREADPOOL_CLEANUP_GROUP_CONTEXT {
    LONG WorkCallbacks;
    LONG CleanupCallbacks;
    LONG CleanupContextMatches;
    LONG ObjectContextMatches;
    PVOID ExpectedCleanupContext;
} THREADPOOL_CLEANUP_GROUP_CONTEXT, *PTHREADPOOL_CLEANUP_GROUP_CONTEXT;

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
LegacyThreadPoolTest (
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

BOOLEAN
ThreadPoolTest (
    VOID
    )
{
    PTP_WORK works[10] = { NULL, };
    LONG count = ARRAYSIZE(works);
    PTP_WORK work = NULL;
    TP_CALLBACK_ENVIRON CallbackEnvironment;
    THREADPOOL_MODULE_CONTEXT ModuleContext;
    THREADPOOL_CLEANUP_GROUP_CONTEXT CleanupGroupContext;
    PTP_CLEANUP_GROUP CleanupGroup = NULL;
    PVOID CleanupToken = &CleanupGroupContext;
    BOOLEAN Result = FALSE;

    PAGED_CODE();
    RtlZeroMemory( &ModuleContext,
                   sizeof(ModuleContext) );
    RtlZeroMemory( &CleanupGroupContext,
                   sizeof(CleanupGroupContext) );

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
        ModuleContext.ModuleWasLoaded != 1 ||
        GetModuleHandleW( L"Test.dll" ) != NULL) {
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

    CloseThreadpoolCleanupGroup( CleanupGroup );
    CleanupGroup = NULL;

    DbgPrint("[Success] Threadpool callback environment, cleanup group, and reuse\n");
    Result = TRUE;

FinalCleanup:
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
    return Result;
}
