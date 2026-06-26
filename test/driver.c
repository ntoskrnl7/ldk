#include <wdm.h>

#include <ldk/ldk.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

BOOLEAN
FileTest (
    VOID
    );

BOOLEAN
HeapApiTest (
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
IcuTest (
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
        { "KeyedEventTest", KeyedEventTest },
        { "LdrTest", LdrTest },
        { "FibersTest", FibersTest },
        { "FileTest", FileTest },
        { "HeapApiTest", HeapApiTest },
        { "FormatMessageTest", FormatMessageTest },
        { "NlsTest", NlsTest },
        { "IcuTest", IcuTest },
        { "CurrentDirectoryTest", CurrentDirectoryTest },
        { "EnvironmentVariableTest", EnvironmentVariableTest },
        { "LegacyThreadPoolTest", LegacyThreadPoolTest },
        { "ThreadPoolTest", ThreadPoolTest },
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
