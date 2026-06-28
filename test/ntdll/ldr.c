#if _KERNEL_MODE
#include <Ldk/Windows.h>
#include <Ldk/ntdll.h>
#include "../../src/teb.h"

BOOLEAN
LdrTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdrTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))

#else
#include <Windows.h>
#include <stdio.h>

EXTERN_C_START

#ifndef NTSYSAPI
#define NTSYSAPI __declspec(dllimport)
#endif

#ifndef _LDK_WIN32_NTSTATUS_DEFINED
#define _LDK_WIN32_NTSTATUS_DEFINED
typedef LONG NTSTATUS;
#endif

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#ifndef STATUS_DLL_INIT_FAILED
#define STATUS_DLL_INIT_FAILED ((NTSTATUS)0xC0000142)
#endif

typedef struct _STRING {
    USHORT Length;
    USHORT MaximumLength;
#ifdef MIDL_PASS
    [size_is(MaximumLength), length_is(Length)]
#endif // MIDL_PASS
    _Field_size_bytes_part_opt_(MaximumLength, Length) PCHAR Buffer;
} STRING;
typedef STRING* PSTRING;
typedef STRING ANSI_STRING;
typedef PSTRING PANSI_STRING;

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

//
// This works "generically" for Unicode and Ansi/Oem strings.
// Usage:
//   const static UNICODE_STRING FooU = RTL_CONSTANT_STRING(L"Foo");
//   const static         STRING Foo  = RTL_CONSTANT_STRING( "Foo");
// instead of the slower:
//   UNICODE_STRING FooU;
//           STRING Foo;
//   RtlInitUnicodeString(&FooU, L"Foo");
//          RtlInitString(&Foo ,  "Foo");
//
// Or:
//   const static char szFoo[] = "Foo";
//   const static STRING sFoo = RTL_CONSTANT_STRING(szFoo);
//
// This will compile without error or warning in C++. C will get a warning.
//
#ifdef __cplusplus
extern "C++"
{
    char _RTL_CONSTANT_STRING_type_check(const char* s);
    char _RTL_CONSTANT_STRING_type_check(const WCHAR* s);
    // __typeof would be desirable here instead of sizeof.
    template <size_t N> class _RTL_CONSTANT_STRING_remove_const_template_class;
template <> class _RTL_CONSTANT_STRING_remove_const_template_class<sizeof(char)> { public: typedef  char T; };
template <> class _RTL_CONSTANT_STRING_remove_const_template_class<sizeof(WCHAR)> { public: typedef WCHAR T; };
#define _RTL_CONSTANT_STRING_remove_const_macro(s) \
    (const_cast<_RTL_CONSTANT_STRING_remove_const_template_class<sizeof((s)[0])>::T*>(s))
}
#else
char _RTL_CONSTANT_STRING_type_check(const void* s);
#define _RTL_CONSTANT_STRING_remove_const_macro(s) (s)
#endif
#define RTL_CONSTANT_STRING(s) \
{ \
    sizeof( s ) - sizeof( (s)[0] ), \
    sizeof( s ) / sizeof(_RTL_CONSTANT_STRING_type_check(s)), \
    _RTL_CONSTANT_STRING_remove_const_macro(s) \
}

NTSYSAPI
NTSTATUS
NTAPI
LdrLoadDll (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrUnloadDll (
    _In_ PVOID DllHandle
    );

#ifndef LDR_ADDREF_DLL_PIN
#define LDR_ADDREF_DLL_PIN 0x00000001
#endif

NTSYSAPI
NTSTATUS
NTAPI
LdrAddRefDll (
    _In_ ULONG Flags,
    _In_ PVOID DllHandle
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrGetProcedureAddress (
    _In_ PVOID DllHandle,
    _In_opt_ PANSI_STRING ProcedureName,
    _In_opt_ ULONG ProcedureNumber,
    _Out_ PVOID* ProcedureAddress
    );

NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandle (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
    );

#ifndef LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT
#define LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT 0x00000001
#endif

#ifndef LDR_GET_DLL_HANDLE_EX_PIN
#define LDR_GET_DLL_HANDLE_EX_PIN 0x00000002
#endif

NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandleEx (
    _In_ ULONG Flags,
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
    );

EXTERN_C_END

#pragma comment(lib, "ntdll.lib")

#define PAGED_CODE()
#endif

#define TLS_EVENT_TLS_PROCESS_ATTACH 0x101
#define TLS_EVENT_DLL_PROCESS_ATTACH 0x102
#define TLS_EVENT_TLS_THREAD_ATTACH  0x201
#define TLS_EVENT_TLS_THREAD_DETACH  0x301
#define TLS_EVENT_TLS_PROCESS_DETACH 0x401
#define TLS_EVENT_DLL_PROCESS_DETACH 0x402
#define TLS_TEST_LOG_CAPACITY        16
#define TLS_STATIC_INITIAL_VALUE     7
#define TLS_STATIC_THREAD_VALUE      22
#define TLS_STATIC_MAIN_VALUE        11
#define TLS_STATIC_INVALID_INDEX     0xffffffff

#define THREAD_NOTIFY_EVENT_PROCESS_ATTACH 0x111
#define THREAD_NOTIFY_EVENT_THREAD_ATTACH  0x211
#define THREAD_NOTIFY_EVENT_THREAD_DETACH  0x311
#define THREAD_NOTIFY_EVENT_PROCESS_DETACH 0x411
#define THREAD_NOTIFY_TEST_LOG_CAPACITY    16

typedef PVOID(__stdcall* TLS_STATIC_TLS_ACCESSOR_FN)(ULONG);
typedef VOID(__stdcall* TLS_SET_STATIC_TLS_ACCESSOR_FN)(TLS_STATIC_TLS_ACCESSOR_FN);
typedef ULONG(__stdcall* TLS_GET_STATIC_INDEX_FN)(VOID);
typedef LONG(__stdcall* TLS_GET_STATIC_VALUE_FN)(VOID);
typedef VOID(__stdcall* TLS_SET_STATIC_VALUE_FN)(LONG);

#if _KERNEL_MODE
static
PVOID
__stdcall
LdkTestGetStaticTlsBlock (
    _In_ ULONG Index
    )
{
    PLDK_TEB Teb = NtCurrentTeb();

    if (Index >= RTL_NUMBER_OF(Teb->TlsSlots)) {
        return NULL;
    }

    return Teb->TlsSlots[Index];
}
#else
#if defined(_M_AMD64) || defined(_M_ARM64) || defined(_M_ARM64EC)
#define LDK_TEST_TEB_STATIC_TLS_OFFSET 0x58
#else
#define LDK_TEST_TEB_STATIC_TLS_OFFSET 0x2c
#endif

static
PVOID
__stdcall
LdkTestGetStaticTlsBlock (
    _In_ ULONG Index
    )
{
    PVOID *TlsArray;

    if (Index >= 4096) {
        return NULL;
    }

    TlsArray = *(PVOID **)((PBYTE)NtCurrentTeb() + LDK_TEST_TEB_STATIC_TLS_OFFSET);
    if (!TlsArray) {
        return NULL;
    }

    return TlsArray[Index];
}
#endif

typedef struct _TLS_STATIC_THREAD_CONTEXT {
    TLS_GET_STATIC_VALUE_FN GetValue;
    TLS_SET_STATIC_VALUE_FN SetValue;
    LONG Counter;
    LONG Failure;
    LONG InitialValue;
    LONG FinalValue;
} TLS_STATIC_THREAD_CONTEXT, *PTLS_STATIC_THREAD_CONTEXT;

static
DWORD
WINAPI
TlsStaticTestThreadProc (
    _In_opt_ LPVOID Parameter
    )
{
    PTLS_STATIC_THREAD_CONTEXT Context = (PTLS_STATIC_THREAD_CONTEXT)Parameter;

    if (!Context ||
        !Context->GetValue ||
        !Context->SetValue) {
        return 1;
    }

    Context->InitialValue = Context->GetValue();
    if (Context->InitialValue != TLS_STATIC_INITIAL_VALUE) {
        Context->Failure = 1;
        return 1;
    }

    Context->SetValue( TLS_STATIC_THREAD_VALUE );
    Context->FinalValue = Context->GetValue();
    if (Context->FinalValue != TLS_STATIC_THREAD_VALUE) {
        Context->Failure = 2;
        return 1;
    }

    InterlockedIncrement( &Context->Counter );
    return 0;
}

static
DWORD
WINAPI
ThreadNotifyTestThreadProc (
    _In_opt_ LPVOID Parameter
    )
{
    LONG *Counter = (LONG *)Parameter;

    if (Counter) {
        InterlockedIncrement( Counter );
    }

    return 0;
}

BOOLEAN
LdrTest (
    VOID
    )
{
    PAGED_CODE();

    typedef LONG(__stdcall* TEST_FN)(LONG);
    typedef VOID(__stdcall* TLS_SET_LOG_FN)(LONG *, LONG *, LONG);
    typedef LONG(__stdcall* TLS_GET_COUNT_FN)(VOID);
    typedef LONG(__stdcall* TLS_GET_EVENT_FN)(LONG);
    typedef VOID(__stdcall* THREAD_NOTIFY_SET_LOG_FN)(LONG *, LONG *, LONG);
    typedef LONG(__stdcall* THREAD_NOTIFY_GET_COUNT_FN)(VOID);
    typedef LONG(__stdcall* THREAD_NOTIFY_GET_EVENT_FN)(LONG);

    WCHAR DllPath[MAX_PATH];
    DWORD Length;
    NTSTATUS Status;
    BOOLEAN Result = FALSE;
    UNICODE_STRING DllName = RTL_CONSTANT_STRING(L"Test.dll");
    UNICODE_STRING OrdinalImportName = RTL_CONSTANT_STRING(L"OrdinalImport.dll");
    UNICODE_STRING AutoDependencyName = RTL_CONSTANT_STRING(L"AutoDependency.dll");
    UNICODE_STRING DependencyOwnerName = RTL_CONSTANT_STRING(L"DependencyOwner.dll");
    UNICODE_STRING DelayOwnerName = RTL_CONSTANT_STRING(L"DelayOwner.dll");
    UNICODE_STRING TlsCallbackName = RTL_CONSTANT_STRING(L"TlsCallback.dll");
    UNICODE_STRING ThreadNotifyName = RTL_CONSTANT_STRING(L"ThreadNotify.dll");
    UNICODE_STRING FailAttachName = RTL_CONSTANT_STRING(L"FailAttach.dll");
    ANSI_STRING ProcName = RTL_CONSTANT_STRING("TestFunction");
    ANSI_STRING OrdinalImportProcName = RTL_CONSTANT_STRING("TestOrdinalImportFunction");
    ANSI_STRING DependencyOwnerProcName = RTL_CONSTANT_STRING("DependencyOwnerFunction");
    ANSI_STRING DelayOwnerProbeProcName = RTL_CONSTANT_STRING("DelayOwnerProbeFunction");
    ANSI_STRING DelayOwnerCallProcName = RTL_CONSTANT_STRING("DelayOwnerCallFunction");
    ANSI_STRING TlsSetLogProcName = RTL_CONSTANT_STRING("TlsCallbackSetLog");
    ANSI_STRING TlsGetCountProcName = RTL_CONSTANT_STRING("TlsCallbackGetCount");
    ANSI_STRING TlsGetEventProcName = RTL_CONSTANT_STRING("TlsCallbackGetEvent");
    ANSI_STRING TlsSetStaticTlsAccessorProcName = RTL_CONSTANT_STRING("TlsCallbackSetStaticTlsAccessor");
    ANSI_STRING TlsGetStaticIndexProcName = RTL_CONSTANT_STRING("TlsCallbackGetStaticIndex");
    ANSI_STRING TlsGetStaticValueProcName = RTL_CONSTANT_STRING("TlsCallbackGetStaticValue");
    ANSI_STRING TlsSetStaticValueProcName = RTL_CONSTANT_STRING("TlsCallbackSetStaticValue");
    ANSI_STRING ThreadNotifySetLogProcName = RTL_CONSTANT_STRING("ThreadNotifySetLog");
    ANSI_STRING ThreadNotifyGetCountProcName = RTL_CONSTANT_STRING("ThreadNotifyGetCount");
    ANSI_STRING ThreadNotifyGetEventProcName = RTL_CONSTANT_STRING("ThreadNotifyGetEvent");
    PVOID DllHandle = NULL;
    PVOID DllHandle2 = NULL;
    PVOID LookupHandle = NULL;
    PVOID OrdinalImportHandle = NULL;
    PVOID AutoDependencyLookupHandle = NULL;
    PVOID DependencyOwnerHandle = NULL;
    PVOID DelayOwnerHandle = NULL;
    PVOID TlsCallbackHandle = NULL;
    PVOID ThreadNotifyHandle = NULL;
    PVOID FailAttachHandle = NULL;
    HMODULE DelayDependencyModule = NULL;
    TEST_FN TestFn = NULL;
    TEST_FN OrdinalFn = NULL;
    TEST_FN OrdinalImportFn = NULL;
    TEST_FN DependencyOwnerFn = NULL;
    TEST_FN DelayOwnerProbeFn = NULL;
    TEST_FN DelayOwnerCallFn = NULL;
    TLS_SET_LOG_FN TlsSetLogFn = NULL;
    TLS_GET_COUNT_FN TlsGetCountFn = NULL;
    TLS_GET_EVENT_FN TlsGetEventFn = NULL;
    TLS_SET_STATIC_TLS_ACCESSOR_FN TlsSetStaticTlsAccessorFn = NULL;
    TLS_GET_STATIC_INDEX_FN TlsGetStaticIndexFn = NULL;
    TLS_GET_STATIC_VALUE_FN TlsGetStaticValueFn = NULL;
    TLS_SET_STATIC_VALUE_FN TlsSetStaticValueFn = NULL;
    THREAD_NOTIFY_SET_LOG_FN ThreadNotifySetLogFn = NULL;
    THREAD_NOTIFY_GET_COUNT_FN ThreadNotifyGetCountFn = NULL;
    THREAD_NOTIFY_GET_EVENT_FN ThreadNotifyGetEventFn = NULL;

    printf("Ldr Test\n");

    Length = GetModuleFileNameW( NULL,
                                 DllPath,
                                 RTL_NUMBER_OF(DllPath) );
    if (Length == 0 ||
        Length >= RTL_NUMBER_OF(DllPath)) {
        printf("[Failed] LdrTest GetModuleFileNameW ErrorCode = %lu\n",
               GetLastError());
        return FALSE;
    }

    for (DWORD Index = Length; Index != 0; Index--) {
        if (DllPath[Index - 1] == L'\\' ||
            DllPath[Index - 1] == L'/') {
            DllPath[Index - 1] = UNICODE_NULL;
            break;
        }
    }

    if (DllPath[0] == L'\\' &&
        DllPath[1] == L'?' &&
        DllPath[2] == L'?' &&
        DllPath[3] == L'\\') {
        SIZE_T CharacterCount = wcslen( DllPath + 4 ) + 1;
        RtlMoveMemory( DllPath,
                       DllPath + 4,
                       CharacterCount * sizeof(WCHAR) );
    }

    Status = LdrLoadDll( DllPath,
                         NULL,
                         &DllName,
                         &DllHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrLoadDll(Test.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrLoadDll( DllPath,
                         NULL,
                         &DllName,
                         &DllHandle2 );
    if (! NT_SUCCESS(Status) ||
        DllHandle2 != DllHandle) {
        printf("[Failed] LdrLoadDll(Test.dll) refcount Status = 0x%08x Handle = %p Handle2 = %p\n",
               Status,
               DllHandle,
               DllHandle2);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( DllHandle,
                                     &ProcName,
                                     0,
                                     (PVOID*)&TestFn );
    if (! NT_SUCCESS(Status) ||
        TestFn(10) != 10) {
        printf("[Failed] LdrGetProcedureAddress(TestFunction) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( DllHandle,
                                     NULL,
                                     7,
                                     (PVOID*)&OrdinalFn );
    if (! NT_SUCCESS(Status) ||
        OrdinalFn(10) != 17) {
        printf("[Failed] LdrGetProcedureAddress(Test.dll, ordinal 7) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &DllName,
                              &LookupHandle );
    if (! NT_SUCCESS(Status) ||
        LookupHandle != DllHandle) {
        printf("[Failed] LdrGetDllHandle(Test.dll) Status = 0x%08x Handle = %p Lookup = %p\n",
               Status,
               DllHandle,
               LookupHandle);
        goto Cleanup;
    }

    LookupHandle = NULL;
    Status = LdrGetDllHandle( DllPath,
                              NULL,
                              &DllName,
                              &LookupHandle );
    if (! NT_SUCCESS(Status) ||
        LookupHandle != DllHandle) {
        printf("[Failed] LdrGetDllHandle(Test.dll, path-aware) Status = 0x%08x Handle = %p Lookup = %p\n",
               Status,
               DllHandle,
               LookupHandle);
        goto Cleanup;
    }

    LookupHandle = NULL;
    Status = LdrGetDllHandleEx( LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT,
                                NULL,
                                NULL,
                                &DllName,
                                &LookupHandle );
    if (! NT_SUCCESS(Status) ||
        LookupHandle != DllHandle) {
        printf("[Failed] LdrGetDllHandleEx(Test.dll, UNCHANGED_REFCOUNT) Status = 0x%08x Handle = %p Lookup = %p\n",
               Status,
               DllHandle,
               LookupHandle);
        goto Cleanup;
    }

    LookupHandle = NULL;
    Status = LdrGetDllHandleEx( 0,
                                DllPath,
                                NULL,
                                &DllName,
                                &LookupHandle );
    if (! NT_SUCCESS(Status) ||
        LookupHandle != DllHandle) {
        printf("[Failed] LdrGetDllHandleEx(Test.dll) Status = 0x%08x Handle = %p Lookup = %p\n",
               Status,
               DllHandle,
               LookupHandle);
        goto Cleanup;
    }

    Status = LdrUnloadDll( LookupHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrUnloadDll(Test.dll LdrGetDllHandleEx reference) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    LookupHandle = NULL;
    Status = LdrGetDllHandleEx( 4,
                                NULL,
                                NULL,
                                &DllName,
                                &LookupHandle );
    if (! NT_SUCCESS(Status) ||
        LookupHandle != DllHandle) {
        printf("[Failed] LdrGetDllHandleEx(Test.dll, compatibility flag 4) Status = 0x%08x Handle = %p Lookup = %p\n",
               Status,
               DllHandle,
               LookupHandle);
        goto Cleanup;
    }

    Status = LdrUnloadDll( LookupHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrUnloadDll(Test.dll compatibility flag 4 reference) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetDllHandleEx( LDR_GET_DLL_HANDLE_EX_UNCHANGED_REFCOUNT |
                                LDR_GET_DLL_HANDLE_EX_PIN,
                                NULL,
                                NULL,
                                &DllName,
                                &LookupHandle );
    if (NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetDllHandleEx accepted incompatible flags\n");
        goto Cleanup;
    }

    printf("[Success] LdrGetDllHandleEx refcount\n");

    Status = LdrUnloadDll( DllHandle2 );
    DllHandle2 = NULL;
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] first LdrUnloadDll(Test.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    LookupHandle = NULL;
    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &DllName,
                              &LookupHandle );
    if (! NT_SUCCESS(Status) ||
        LookupHandle != DllHandle) {
        printf("[Failed] LdrLoadDll refcount did not keep Test.dll loaded Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrAddRefDll( 0,
                           DllHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrAddRefDll(Test.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrUnloadDll( DllHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrUnloadDll(Test.dll add-ref reference) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    LookupHandle = NULL;
    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &DllName,
                              &LookupHandle );
    if (! NT_SUCCESS(Status) ||
        LookupHandle != DllHandle) {
        printf("[Failed] LdrAddRefDll did not keep Test.dll loaded Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrAddRefDll( LDR_ADDREF_DLL_PIN << 1,
                           DllHandle );
    if (NT_SUCCESS(Status)) {
        printf("[Failed] LdrAddRefDll accepted invalid flags\n");
        goto Cleanup;
    }

    printf("[Success] LdrAddRefDll refcount\n");

    Status = LdrLoadDll( DllPath,
                         NULL,
                         &OrdinalImportName,
                         &OrdinalImportHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrLoadDll(OrdinalImport.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( OrdinalImportHandle,
                                     &OrdinalImportProcName,
                                     0,
                                     (PVOID*)&OrdinalImportFn );
    if (! NT_SUCCESS(Status) ||
        OrdinalImportFn(10) != 17) {
        printf("[Failed] LdrGetProcedureAddress(TestOrdinalImportFunction) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrUnloadDll( OrdinalImportHandle );
    OrdinalImportHandle = NULL;
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrUnloadDll(OrdinalImport.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &AutoDependencyName,
                              &AutoDependencyLookupHandle );
    if (NT_SUCCESS(Status)) {
        printf("[Failed] AutoDependency.dll was loaded before dependency-owner test\n");
        goto Cleanup;
    }

    Status = LdrLoadDll( DllPath,
                         NULL,
                         &DependencyOwnerName,
                         &DependencyOwnerHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrLoadDll(DependencyOwner.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &AutoDependencyName,
                              &AutoDependencyLookupHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] AutoDependency.dll was not retained for DependencyOwner.dll Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( DependencyOwnerHandle,
                                     &DependencyOwnerProcName,
                                     0,
                                     (PVOID*)&DependencyOwnerFn );
    if (! NT_SUCCESS(Status) ||
        DependencyOwnerFn(10) != 34) {
        printf("[Failed] LdrGetProcedureAddress(DependencyOwnerFunction) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrUnloadDll( DependencyOwnerHandle );
    DependencyOwnerHandle = NULL;
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrUnloadDll(DependencyOwner.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    AutoDependencyLookupHandle = NULL;
    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &AutoDependencyName,
                              &AutoDependencyLookupHandle );
    if (NT_SUCCESS(Status)) {
        printf("[Failed] AutoDependency.dll stayed loaded after DependencyOwner.dll unload\n");
        goto Cleanup;
    }

    Status = LdrLoadDll( DllPath,
                         NULL,
                         &TlsCallbackName,
                         &TlsCallbackHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrLoadDll(TlsCallback.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( TlsCallbackHandle,
                                     &TlsSetLogProcName,
                                     0,
                                     (PVOID*)&TlsSetLogFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(TlsCallbackSetLog) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( TlsCallbackHandle,
                                     &TlsGetCountProcName,
                                     0,
                                     (PVOID*)&TlsGetCountFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(TlsCallbackGetCount) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( TlsCallbackHandle,
                                     &TlsGetEventProcName,
                                     0,
                                     (PVOID*)&TlsGetEventFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(TlsCallbackGetEvent) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( TlsCallbackHandle,
                                     &TlsSetStaticTlsAccessorProcName,
                                     0,
                                     (PVOID*)&TlsSetStaticTlsAccessorFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(TlsCallbackSetStaticTlsAccessor) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( TlsCallbackHandle,
                                     &TlsGetStaticIndexProcName,
                                     0,
                                     (PVOID*)&TlsGetStaticIndexFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(TlsCallbackGetStaticIndex) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( TlsCallbackHandle,
                                     &TlsGetStaticValueProcName,
                                     0,
                                     (PVOID*)&TlsGetStaticValueFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(TlsCallbackGetStaticValue) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( TlsCallbackHandle,
                                     &TlsSetStaticValueProcName,
                                     0,
                                     (PVOID*)&TlsSetStaticValueFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(TlsCallbackSetStaticValue) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    if (TlsGetCountFn() < 2 ||
        TlsGetEventFn(0) != TLS_EVENT_TLS_PROCESS_ATTACH ||
        TlsGetEventFn(1) != TLS_EVENT_DLL_PROCESS_ATTACH) {
        printf("[Failed] TlsCallback native process attach order Count = %ld Event0 = 0x%lx Event1 = 0x%lx\n",
               TlsGetCountFn(),
               TlsGetEventFn(0),
               TlsGetEventFn(1));
        goto Cleanup;
    }

    TlsSetStaticTlsAccessorFn( LdkTestGetStaticTlsBlock );
    if (TlsGetStaticIndexFn() == TLS_STATIC_INVALID_INDEX ||
        TlsGetStaticValueFn() != TLS_STATIC_INITIAL_VALUE) {
        printf("[Failed] TlsCallback native static TLS main initial Index = %lu Value = %ld\n",
               TlsGetStaticIndexFn(),
               TlsGetStaticValueFn());
        goto Cleanup;
    }

    TlsSetStaticValueFn( TLS_STATIC_MAIN_VALUE );
    if (TlsGetStaticValueFn() != TLS_STATIC_MAIN_VALUE) {
        printf("[Failed] TlsCallback native static TLS main set Value = %ld\n",
               TlsGetStaticValueFn());
        goto Cleanup;
    }

    LONG TlsThreadLogCount = 0;
    LONG TlsThreadLog[TLS_TEST_LOG_CAPACITY] = { 0 };
    TlsSetLogFn( &TlsThreadLogCount,
                 TlsThreadLog,
                 TLS_TEST_LOG_CAPACITY );

    TLS_STATIC_THREAD_CONTEXT TlsStaticContext = { 0 };
    TlsStaticContext.GetValue = TlsGetStaticValueFn;
    TlsStaticContext.SetValue = TlsSetStaticValueFn;
    HANDLE TlsThread = CreateThread( NULL,
                                     0,
                                     TlsStaticTestThreadProc,
                                     &TlsStaticContext,
                                     0,
                                     NULL );
    if (! TlsThread) {
        printf("[Failed] TlsCallback native CreateThread ErrorCode = %lu\n",
               GetLastError());
        goto Cleanup;
    }

    DWORD TlsWait = WaitForSingleObject( TlsThread,
                                         5000 );
    CloseHandle( TlsThread );
    if (TlsWait != WAIT_OBJECT_0 ||
        TlsStaticContext.Counter != 1 ||
        TlsStaticContext.Failure != 0 ||
        TlsStaticContext.InitialValue != TLS_STATIC_INITIAL_VALUE ||
        TlsStaticContext.FinalValue != TLS_STATIC_THREAD_VALUE ||
        TlsGetStaticValueFn() != TLS_STATIC_MAIN_VALUE ||
        TlsThreadLogCount < 2 ||
        TlsThreadLog[0] != TLS_EVENT_TLS_THREAD_ATTACH ||
        TlsThreadLog[1] != TLS_EVENT_TLS_THREAD_DETACH) {
        printf("[Failed] TlsCallback native thread/static TLS Wait = 0x%08lx Counter = %ld Failure = %ld Initial = %ld Final = %ld Main = %ld Count = %ld Event0 = 0x%lx Event1 = 0x%lx\n",
               TlsWait,
               TlsStaticContext.Counter,
               TlsStaticContext.Failure,
               TlsStaticContext.InitialValue,
               TlsStaticContext.FinalValue,
               TlsGetStaticValueFn(),
               TlsThreadLogCount,
               TlsThreadLog[0],
               TlsThreadLog[1]);
        goto Cleanup;
    }

    ULONG TlsStaticIndex = TlsGetStaticIndexFn();
    LONG TlsStaticMainValue = TlsGetStaticValueFn();

    LONG TlsDetachLogCount = 0;
    LONG TlsDetachLog[TLS_TEST_LOG_CAPACITY] = { 0 };
    TlsSetLogFn( &TlsDetachLogCount,
                 TlsDetachLog,
                 TLS_TEST_LOG_CAPACITY );

    Status = LdrUnloadDll( TlsCallbackHandle );
    TlsCallbackHandle = NULL;
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrUnloadDll(TlsCallback.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    if (TlsDetachLogCount < 2 ||
        TlsDetachLog[0] != TLS_EVENT_TLS_PROCESS_DETACH ||
        TlsDetachLog[1] != TLS_EVENT_DLL_PROCESS_DETACH) {
        printf("[Failed] TlsCallback native process detach order Count = %ld Event0 = 0x%lx Event1 = 0x%lx\n",
               TlsDetachLogCount,
               TlsDetachLog[0],
               TlsDetachLog[1]);
        goto Cleanup;
    }
    printf("[Success] LdrLoadDll(TlsCallback.dll) TLS callbacks and static TLS storage Index = %lu Main = %ld ThreadInitial = %ld ThreadFinal = %ld\n",
           TlsStaticIndex,
           TlsStaticMainValue,
           TlsStaticContext.InitialValue,
           TlsStaticContext.FinalValue);

    Status = LdrLoadDll( DllPath,
                         NULL,
                         &ThreadNotifyName,
                         &ThreadNotifyHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrLoadDll(ThreadNotify.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( ThreadNotifyHandle,
                                     &ThreadNotifySetLogProcName,
                                     0,
                                     (PVOID*)&ThreadNotifySetLogFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(ThreadNotifySetLog) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( ThreadNotifyHandle,
                                     &ThreadNotifyGetCountProcName,
                                     0,
                                     (PVOID*)&ThreadNotifyGetCountFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(ThreadNotifyGetCount) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( ThreadNotifyHandle,
                                     &ThreadNotifyGetEventProcName,
                                     0,
                                     (PVOID*)&ThreadNotifyGetEventFn );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrGetProcedureAddress(ThreadNotifyGetEvent) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    if (ThreadNotifyGetCountFn() < 1 ||
        ThreadNotifyGetEventFn(0) != THREAD_NOTIFY_EVENT_PROCESS_ATTACH) {
        printf("[Failed] ThreadNotify native process attach Count = %ld Event0 = 0x%lx\n",
               ThreadNotifyGetCountFn(),
               ThreadNotifyGetEventFn(0));
        goto Cleanup;
    }

    LONG ThreadNotifyLogCount = 0;
    LONG ThreadNotifyLog[THREAD_NOTIFY_TEST_LOG_CAPACITY] = { 0 };
    ThreadNotifySetLogFn( &ThreadNotifyLogCount,
                          ThreadNotifyLog,
                          THREAD_NOTIFY_TEST_LOG_CAPACITY );

    LONG ThreadNotifyCounter = 0;
    HANDLE ThreadNotifyThread = CreateThread( NULL,
                                              0,
                                              ThreadNotifyTestThreadProc,
                                              &ThreadNotifyCounter,
                                              0,
                                              NULL );
    if (! ThreadNotifyThread) {
        printf("[Failed] ThreadNotify native CreateThread ErrorCode = %lu\n",
               GetLastError());
        goto Cleanup;
    }

    DWORD ThreadNotifyWait = WaitForSingleObject( ThreadNotifyThread,
                                                  5000 );
    CloseHandle( ThreadNotifyThread );
    if (ThreadNotifyWait != WAIT_OBJECT_0 ||
        ThreadNotifyCounter != 1 ||
        ThreadNotifyLogCount < 2 ||
        ThreadNotifyLog[0] != THREAD_NOTIFY_EVENT_THREAD_ATTACH ||
        ThreadNotifyLog[1] != THREAD_NOTIFY_EVENT_THREAD_DETACH) {
        printf("[Failed] ThreadNotify native thread events Wait = 0x%08lx Counter = %ld Count = %ld Event0 = 0x%lx Event1 = 0x%lx\n",
               ThreadNotifyWait,
               ThreadNotifyCounter,
               ThreadNotifyLogCount,
               ThreadNotifyLog[0],
               ThreadNotifyLog[1]);
        goto Cleanup;
    }

    LONG ThreadNotifyDetachLogCount = 0;
    LONG ThreadNotifyDetachLog[THREAD_NOTIFY_TEST_LOG_CAPACITY] = { 0 };
    ThreadNotifySetLogFn( &ThreadNotifyDetachLogCount,
                          ThreadNotifyDetachLog,
                          THREAD_NOTIFY_TEST_LOG_CAPACITY );

    Status = LdrUnloadDll( ThreadNotifyHandle );
    ThreadNotifyHandle = NULL;
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrUnloadDll(ThreadNotify.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    if (ThreadNotifyDetachLogCount < 1 ||
        ThreadNotifyDetachLog[0] != THREAD_NOTIFY_EVENT_PROCESS_DETACH) {
        printf("[Failed] ThreadNotify native process detach Count = %ld Event0 = 0x%lx\n",
               ThreadNotifyDetachLogCount,
               ThreadNotifyDetachLog[0]);
        goto Cleanup;
    }
    printf("[Success] LdrLoadDll(ThreadNotify.dll) DllMain thread notifications\n");

    if (GetModuleHandleW( L"DelayDependency.dll" ) != NULL) {
        printf("[Failed] DelayDependency.dll was loaded before delay-load test\n");
        goto Cleanup;
    }

    Status = LdrLoadDll( DllPath,
                         NULL,
                         &DelayOwnerName,
                         &DelayOwnerHandle );
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrLoadDll(DelayOwner.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    if (GetModuleHandleW( L"DelayDependency.dll" ) != NULL) {
        printf("[Failed] DelayDependency.dll was loaded before first native delay call\n");
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( DelayOwnerHandle,
                                     &DelayOwnerProbeProcName,
                                     0,
                                     (PVOID*)&DelayOwnerProbeFn );
    if (! NT_SUCCESS(Status) ||
        DelayOwnerProbeFn(10) != 12) {
        printf("[Failed] LdrGetProcedureAddress(DelayOwnerProbeFunction) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    Status = LdrGetProcedureAddress( DelayOwnerHandle,
                                     &DelayOwnerCallProcName,
                                     0,
                                     (PVOID*)&DelayOwnerCallFn );
    if (! NT_SUCCESS(Status) ||
        DelayOwnerCallFn(10) != 55) {
        printf("[Failed] LdrGetProcedureAddress(DelayOwnerCallFunction) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    DelayDependencyModule = GetModuleHandleW( L"DelayDependency.dll" );
    if (! DelayDependencyModule) {
        printf("[Failed] DelayDependency.dll was not loaded by native delay call\n");
        goto Cleanup;
    }

    Status = LdrUnloadDll( DelayOwnerHandle );
    DelayOwnerHandle = NULL;
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] LdrUnloadDll(DelayOwner.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    DelayDependencyModule = GetModuleHandleW( L"DelayDependency.dll" );
    if (DelayDependencyModule &&
        ! FreeLibrary( DelayDependencyModule )) {
        printf("[Failed] FreeLibrary(DelayDependency.dll) ErrorCode = %lu\n",
               GetLastError());
        goto Cleanup;
    }
    DelayDependencyModule = NULL;
    printf("[Success] LdrLoadDll(DelayOwner.dll) delay-load first-call resolution\n");

    FailAttachHandle = NULL;
    Status = LdrLoadDll( DllPath,
                         NULL,
                         &FailAttachName,
                         &FailAttachHandle );
    if (Status != STATUS_DLL_INIT_FAILED) {
        printf("[Failed] LdrLoadDll(FailAttach.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    LookupHandle = NULL;
    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &FailAttachName,
                              &LookupHandle );
    if (NT_SUCCESS(Status)) {
        printf("[Failed] FailAttach.dll stayed loaded after DllMain failure\n");
        goto Cleanup;
    }

    AutoDependencyLookupHandle = NULL;
    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &AutoDependencyName,
                              &AutoDependencyLookupHandle );
    if (NT_SUCCESS(Status)) {
        printf("[Failed] AutoDependency.dll stayed loaded after FailAttach.dll failure\n");
        goto Cleanup;
    }
    printf("[Success] LdrLoadDll(FailAttach.dll) DllMain failure cleanup\n");

    Status = LdrUnloadDll( DllHandle );
    DllHandle = NULL;
    if (! NT_SUCCESS(Status)) {
        printf("[Failed] second LdrUnloadDll(Test.dll) Status = 0x%08x\n",
               Status);
        goto Cleanup;
    }

    LookupHandle = NULL;
    Status = LdrGetDllHandle( NULL,
                              NULL,
                              &DllName,
                              &LookupHandle );
    if (NT_SUCCESS(Status)) {
        printf("[Failed] Test.dll is still loaded after balanced LdrUnloadDll calls\n");
        goto Cleanup;
    }

    printf("[Success] Ldr Test\n\n");
    Result = TRUE;

Cleanup:
    if (TlsCallbackHandle != NULL) {
        LdrUnloadDll( TlsCallbackHandle );
    }
    if (ThreadNotifyHandle != NULL) {
        LdrUnloadDll( ThreadNotifyHandle );
    }
    if (DelayOwnerHandle != NULL) {
        LdrUnloadDll( DelayOwnerHandle );
    }
    if (DependencyOwnerHandle != NULL) {
        LdrUnloadDll( DependencyOwnerHandle );
    }
    if (OrdinalImportHandle != NULL) {
        LdrUnloadDll( OrdinalImportHandle );
    }
    if (DllHandle2 != NULL) {
        LdrUnloadDll( DllHandle2 );
    }
    if (DllHandle != NULL) {
        LdrUnloadDll( DllHandle );
    }
    DelayDependencyModule = GetModuleHandleW( L"DelayDependency.dll" );
    if (DelayDependencyModule != NULL) {
        FreeLibrary( DelayDependencyModule );
    }
    return Result;
}
