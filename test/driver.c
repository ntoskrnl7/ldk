#include <wdm.h>

#include <ldk/ldk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

BOOLEAN
FileTest (
    VOID
    );

BOOLEAN
LegacyThreadPoolTest (
    VOID
    );

BOOLEAN
ThreadPoolTest (
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
NtdllCurrentDirectoryTest (
    VOID
    );

BOOLEAN
NtdllEnvironmentVariableTest (
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

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, DriverEntry)
#pragma alloc_text(PAGE, DriverUnload)
#endif

typedef
_IRQL_requires_max_(PASSIVE_LEVEL)
_IRQL_requires_same_
_Function_class_(LDK_TEST_ROUTINE)
BOOLEAN
LDK_TEST_ROUTINE (
    VOID
    );

typedef LDK_TEST_ROUTINE *PLDK_TEST_ROUTINE;

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    PLDK_TEST_ROUTINE Tests[] = {
        NtdllCurrentDirectoryTest,
        NtdllEnvironmentVariableTest,
        KeyedEventTest,
        LdrTest,
        FileTest,
        CurrentDirectoryTest,
        EnvironmentVariableTest,
        LegacyThreadPoolTest,
        ThreadPoolTest,
        ConditionVariableTest,
        LibraryTest,
        ConsoleTest,
        NULL
    };

    KdBreakPoint();

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    for (PLDK_TEST_ROUTINE *test = Tests; *test; test++) {
        if (! (*test)()) {
            LdkTerminate();
            return STATUS_UNSUCCESSFUL;
        }
    }

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