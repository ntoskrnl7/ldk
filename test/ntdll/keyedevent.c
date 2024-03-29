﻿#if _KERNEL_MODE
#include <Ldk/ntdll.h>

BOOLEAN
KeyedEventTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KeyedEventTest)
#endif

#include <Ldk/windows.h>
#else
#include <Windows.h>
#include <stdio.h>

EXTERN_C_START

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

typedef struct _UNICODE_STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength / 2), length_is((Length) / 2)] USHORT* Buffer;
#else // MIDL_PASS
    _Field_size_bytes_part_opt_(MaximumLength, Length) PWCH   Buffer;
#endif // MIDL_PASS
} UNICODE_STRING;
typedef UNICODE_STRING* PUNICODE_STRING;
typedef const UNICODE_STRING* PCUNICODE_STRING;

typedef struct _OBJECT_ATTRIBUTES {
    ULONG Length;
    HANDLE RootDirectory;
    PUNICODE_STRING ObjectName;
    ULONG Attributes;
    PVOID SecurityDescriptor;        // Points to type SECURITY_DESCRIPTOR
    PVOID SecurityQualityOfService;  // Points to type SECURITY_QUALITY_OF_SERVICE
} OBJECT_ATTRIBUTES;
typedef OBJECT_ATTRIBUTES *POBJECT_ATTRIBUTES;

#define KEYEDEVENT_WAIT 0x0001
#define KEYEDEVENT_WAKE 0x0002
#define KEYEDEVENT_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED | KEYEDEVENT_WAIT | KEYEDEVENT_WAKE)

NTSYSAPI
NTSTATUS
NTAPI
NtCreateKeyedEvent (
    _Out_ PHANDLE KeyedEventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_opt_ POBJECT_ATTRIBUTES ObjectAttributes,
    _In_ ULONG Flags
    );

NTSYSAPI
NTSTATUS
NTAPI
NtOpenKeyedEvent (
    _Out_ PHANDLE KeyedEventHandle,
    _In_ ACCESS_MASK DesiredAccess,
    _In_ POBJECT_ATTRIBUTES ObjectAttributes
    );

NTSYSAPI
NTSTATUS
NTAPI
NtReleaseKeyedEvent (
    _In_ HANDLE KeyedEventHandle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

NTSYSAPI
NTSTATUS
NTAPI
NtWaitForKeyedEvent (
    _In_ HANDLE KeyedEventHandle,
    _In_ PVOID Key,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

typedef enum _WAIT_TYPE {
    WaitAll,
    WaitAny,
    WaitNotification,
    WaitDequeue,
    WaitDpc
} WAIT_TYPE;

NTSYSAPI
NTSTATUS
NTAPI
NtWaitForMultipleObjects (
    _In_ ULONG Count,
    _In_reads_(Count) HANDLE Handles[],
    _In_ _Strict_type_match_ WAIT_TYPE WaitType,
    _In_ BOOLEAN Alertable,
    _In_opt_ PLARGE_INTEGER Timeout
    );

NTSYSAPI
NTSTATUS
NTAPI
NtClose (
    _In_ _Post_ptr_invalid_ HANDLE Handle
    );
EXTERN_C_END

#pragma comment(lib, "ntdll.lib")

#define DbgPrint        printf
#define PAGED_CODE()
#endif

HANDLE TestKeyedEventHandle;
LONG TestCount[4] = { 0, };

DWORD
WINAPI
KeyedEventTestThreadProc (
    LPVOID lpThreadParameter
    )
{
    PAGED_CODE();

    DbgPrint("%p - %d - start\n", lpThreadParameter, GetCurrentThreadId());

    NtWaitForKeyedEvent(TestKeyedEventHandle, lpThreadParameter, FALSE, NULL);

    InterlockedDecrement(&TestCount[(((ULONG_PTR)lpThreadParameter) >> 8) - 1]);

    DbgPrint("%p - %d - end\n", lpThreadParameter, GetCurrentThreadId());
    return 0;
}


BOOLEAN
KeyedEventTest (
    VOID
    )
{
    PAGED_CODE();

    BOOLEAN Result = TRUE;

    HANDLE Threads[10] = { NULL, };
    NTSTATUS Status = NtCreateKeyedEvent( &TestKeyedEventHandle,
                                          KEYEDEVENT_ALL_ACCESS,
                                          NULL,
                                          0 );
    if (! NT_SUCCESS(Status)) {
        return FALSE;
    }

    for (ULONG_PTR i = 0; i < 10; i++) {
        Threads[i] = CreateThread( NULL,
                                   0,
                                   KeyedEventTestThreadProc,
                                   (PVOID)((i % 4 + 1) << 8),
                                   0,
                                   NULL );
        TestCount[i % 4]++;
    }

    HANDLE Handles[3];
    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x100),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Handles[0] = Threads[0];
    Handles[1] = Threads[4];
    Handles[2] = Threads[8];
    NtWaitForMultipleObjects( 3,
                              Handles,
                              WaitAny,
                              FALSE,
                              NULL );
    if (TestCount[0] != 2) {
        Result = FALSE;
    }

    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x200),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Handles[0] = Threads[1];
    Handles[1] = Threads[5];
    Handles[2] = Threads[9];
    Status = NtWaitForMultipleObjects( 3,
                                       Handles,
                                       WaitAny,
                                       FALSE,
                                       NULL );
    if (TestCount[1] != 2) {
        Result = FALSE;
    }

    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x300),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Handles[0] = Threads[2];
    Handles[1] = Threads[6];
    NtWaitForMultipleObjects( 2,
                              Handles,
                              WaitAny,
                              FALSE,
                              NULL );
    if (TestCount[2] != 1) {
        Result = FALSE;
    }

    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x400),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Handles[0] = Threads[3];
    Handles[1] = Threads[7];
    NtWaitForMultipleObjects( 2,
                              Handles,
                              WaitAny,
                              FALSE,
                              NULL );
    if (TestCount[3] != 1) {
        Result = FALSE;
    }

    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x100),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x100),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x200),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x200),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x300),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }
    Status = NtReleaseKeyedEvent( TestKeyedEventHandle,
                                  (PVOID)(0x400),
                                  FALSE,
                                  NULL );
    if (! NT_SUCCESS(Status)) {
        Result = FALSE;
    }

    NtWaitForMultipleObjects( 10,
                              Threads,
                              WaitAll,
                              FALSE,
                              NULL );
    for (int i = 0; i < 10; i++) {
        if (Threads[i]) {
            NtClose(Threads[i]);
        }
    }

    NtClose(TestKeyedEventHandle);

    return Result;
}