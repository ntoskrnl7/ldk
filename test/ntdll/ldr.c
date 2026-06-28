#if _KERNEL_MODE
#include <Ldk/Windows.h>
#include <Ldk/ntdll.h>

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

NTSYSAPI
NTSTATUS
NTAPI
LdrGetDllHandle (
    _In_opt_ PWSTR DllPath,
    _In_opt_ PULONG DllCharacteristics,
    _In_ PUNICODE_STRING DllName,
    _Out_ PVOID* DllHandle
    );

EXTERN_C_END

#pragma comment(lib, "ntdll.lib")

#define PAGED_CODE()
#endif

BOOLEAN
LdrTest (
    VOID
    )
{
    PAGED_CODE();

    typedef LONG(__stdcall* TEST_FN)(LONG);

    WCHAR DllPath[MAX_PATH];
    DWORD Length;
    NTSTATUS Status;
    BOOLEAN Result = FALSE;
    UNICODE_STRING DllName = RTL_CONSTANT_STRING(L"Test.dll");
    UNICODE_STRING OrdinalImportName = RTL_CONSTANT_STRING(L"OrdinalImport.dll");
    UNICODE_STRING AutoDependencyName = RTL_CONSTANT_STRING(L"AutoDependency.dll");
    UNICODE_STRING DependencyOwnerName = RTL_CONSTANT_STRING(L"DependencyOwner.dll");
    UNICODE_STRING DelayOwnerName = RTL_CONSTANT_STRING(L"DelayOwner.dll");
    UNICODE_STRING FailAttachName = RTL_CONSTANT_STRING(L"FailAttach.dll");
    ANSI_STRING ProcName = RTL_CONSTANT_STRING("TestFunction");
    ANSI_STRING OrdinalImportProcName = RTL_CONSTANT_STRING("TestOrdinalImportFunction");
    ANSI_STRING DependencyOwnerProcName = RTL_CONSTANT_STRING("DependencyOwnerFunction");
    ANSI_STRING DelayOwnerProbeProcName = RTL_CONSTANT_STRING("DelayOwnerProbeFunction");
    ANSI_STRING DelayOwnerCallProcName = RTL_CONSTANT_STRING("DelayOwnerCallFunction");
    PVOID DllHandle = NULL;
    PVOID DllHandle2 = NULL;
    PVOID LookupHandle = NULL;
    PVOID OrdinalImportHandle = NULL;
    PVOID AutoDependencyLookupHandle = NULL;
    PVOID DependencyOwnerHandle = NULL;
    PVOID DelayOwnerHandle = NULL;
    PVOID FailAttachHandle = NULL;
    HMODULE DelayDependencyModule = NULL;
    TEST_FN TestFn = NULL;
    TEST_FN OrdinalFn = NULL;
    TEST_FN OrdinalImportFn = NULL;
    TEST_FN DependencyOwnerFn = NULL;
    TEST_FN DelayOwnerProbeFn = NULL;
    TEST_FN DelayOwnerCallFn = NULL;

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
