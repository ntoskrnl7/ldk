#include <ntddk.h>

#include <ldk/ldk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

BOOLEAN
FileTest (
    VOID
    );

BOOLEAN
PipeTest (
    VOID
    );

BOOLEAN
HeapApiTest (
    VOID
    );

BOOLEAN
UtilityApiTest (
    VOID
    );

BOOLEAN
AdvapiCompatibilityTest (
    VOID
    );

BOOLEAN
FormatMessageTest (
    VOID
    );

BOOLEAN
NlsTest (
    VOID
    );

BOOLEAN
RegistryApiTest (
    VOID
    );

BOOLEAN
IcuTest (
    VOID
    );

BOOLEAN
CombaseCompatibilityTest (
    VOID
    );

BOOLEAN
QueueUserWorkItemTest (
    VOID
    );

BOOLEAN
ThreadpoolWorkTimerWaitCleanupGroupTest (
    VOID
    );

BOOLEAN
FibersTest (
    VOID
    );

BOOLEAN
ConditionVariableTest (
    VOID
    );

BOOLEAN
CurrentDirectoryTest (
    VOID
    );

BOOLEAN
EnvironmentVariableTest (
    VOID
    );

BOOLEAN
LibraryTest (
    VOID
    );

BOOLEAN
ConsoleTest (
    VOID
    );

BOOLEAN
ProcessApiTest (
    VOID
    );

BOOLEAN
NtdllCurrentDirectoryTest (
    VOID
    );

BOOLEAN
NtdllEnvironmentVariableTest (
    VOID
    );

BOOLEAN
NtdllEcCodeCompatibilityTest (
    VOID
    );

BOOLEAN
LdrTest (
    VOID
    );

BOOLEAN
KeyedEventTest (
    VOID
    );

BOOLEAN
WaitOnAddressTest (
    VOID
    );

BOOLEAN
TebExpandedStackTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#pragma alloc_text(PAGE, TebExpandedStackTest)
#endif

typedef struct _TEB_EXPANDED_STACK_TEST_CONTEXT {
    PLDK_TEB ExpectedTeb;
    PLDK_TEB ActualTeb;
    NTSTATUS Status;
} TEB_EXPANDED_STACK_TEST_CONTEXT, *PTEB_EXPANDED_STACK_TEST_CONTEXT;

VOID
TebExpandedStackTestCallout (
    _In_ PVOID Parameter
    )
{
    PTEB_EXPANDED_STACK_TEST_CONTEXT Context = Parameter;

    Context->ActualTeb = LdkCurrentTeb();
    if (Context->ActualTeb == NULL) {
        Context->Status = STATUS_UNSUCCESSFUL;
        return;
    }

    if (Context->ActualTeb != Context->ExpectedTeb) {
        Context->Status = STATUS_OBJECT_TYPE_MISMATCH;
        return;
    }

    Context->Status = STATUS_SUCCESS;
}

BOOLEAN
TebExpandedStackTest (
    VOID
    )
{
    TEB_EXPANDED_STACK_TEST_CONTEXT Context;
    NTSTATUS Status;

    PAGED_CODE();

    Context.ExpectedTeb = LdkCurrentTeb();
    Context.ActualTeb = NULL;
    Context.Status = STATUS_UNSUCCESSFUL;

    Status = KeExpandKernelStackAndCalloutEx(
                 TebExpandedStackTestCallout,
                 &Context,
                 0x4000,
                 TRUE,
                 NULL );
    if (! NT_SUCCESS(Status)) {
        DbgPrintEx( DPFLTR_IHVDRIVER_ID,
                    DPFLTR_ERROR_LEVEL,
                    "[Failed] TebExpandedStackTest KeExpandKernelStackAndCalloutEx Status = 0x%08X\n",
                    Status );
        return FALSE;
    }

    if (! NT_SUCCESS(Context.Status)) {
        DbgPrintEx( DPFLTR_IHVDRIVER_ID,
                    DPFLTR_ERROR_LEVEL,
                    "[Failed] TebExpandedStackTest Context.Status = 0x%08X ExpectedTeb = %p ActualTeb = %p\n",
                    Context.Status,
                    Context.ExpectedTeb,
                    Context.ActualTeb );
        return FALSE;
    }

    return TRUE;
}

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(LDK_TEST_ROUTINE)
BOOLEAN
LDK_TEST_ROUTINE (
    VOID
    );

typedef LDK_TEST_ROUTINE *PLDK_TEST_ROUTINE;

typedef struct _LDK_TEST_ENTRY {
    PCSTR Name;
    PLDK_TEST_ROUTINE Routine;
} LDK_TEST_ENTRY, *PLDK_TEST_ENTRY;

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    LDK_TEST_ENTRY Tests[] = {
        { "NtdllCurrentDirectoryTest", NtdllCurrentDirectoryTest },
        { "NtdllEnvironmentVariableTest", NtdllEnvironmentVariableTest },
        { "NtdllEcCodeCompatibilityTest", NtdllEcCodeCompatibilityTest },
        { "KeyedEventTest", KeyedEventTest },
        { "LdrTest", LdrTest },
        { "TebExpandedStackTest", TebExpandedStackTest },
        { "FibersTest", FibersTest },
        { "FileTest", FileTest },
        { "PipeTest", PipeTest },
        { "HeapApiTest", HeapApiTest },
        { "UtilityApiTest", UtilityApiTest },
        { "AdvapiCompatibilityTest", AdvapiCompatibilityTest },
        { "FormatMessageTest", FormatMessageTest },
        { "NlsTest", NlsTest },
        { "RegistryApiTest", RegistryApiTest },
        { "IcuTest", IcuTest },
        { "CombaseCompatibilityTest", CombaseCompatibilityTest },
        { "CurrentDirectoryTest", CurrentDirectoryTest },
        { "EnvironmentVariableTest", EnvironmentVariableTest },
        { "QueueUserWorkItemTest", QueueUserWorkItemTest },
        { "ThreadpoolWorkTimerWaitCleanupGroupTest", ThreadpoolWorkTimerWaitCleanupGroupTest },
        { "ConditionVariableTest", ConditionVariableTest },
        { "WaitOnAddressTest", WaitOnAddressTest },
        { "LibraryTest", LibraryTest },
        { "ConsoleTest", ConsoleTest },
        { "ProcessApiTest", ProcessApiTest },
        { NULL, NULL }
    };

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    for (PLDK_TEST_ENTRY Test = Tests; Test->Routine; Test++) {
        DbgPrintEx( DPFLTR_IHVDRIVER_ID,
                    DPFLTR_INFO_LEVEL,
                    "[LdkTest] RUN  %s\n",
                    Test->Name );

        if (! Test->Routine()) {
            DbgPrintEx( DPFLTR_IHVDRIVER_ID,
                        DPFLTR_ERROR_LEVEL,
                        "[LdkTest] FAIL %s\n",
                        Test->Name );
            LdkTerminate();
            return STATUS_UNSUCCESSFUL;
        }

        DbgPrintEx( DPFLTR_IHVDRIVER_ID,
                    DPFLTR_INFO_LEVEL,
                    "[LdkTest] PASS %s\n",
                    Test->Name );
    }

    DbgPrintEx( DPFLTR_IHVDRIVER_ID,
                DPFLTR_INFO_LEVEL,
                "[LdkTest] PASS all tests\n" );

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}

VOID
DriverUnload (
    _In_ PDRIVER_OBJECT DriverObject
    )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(DriverObject);
}
