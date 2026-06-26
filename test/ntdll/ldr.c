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
    ANSI_STRING ProcName = RTL_CONSTANT_STRING("TestFunction");
    ANSI_STRING OrdinalImportProcName = RTL_CONSTANT_STRING("TestOrdinalImportFunction");
    PVOID DllHandle = NULL;
    PVOID DllHandle2 = NULL;
    PVOID LookupHandle = NULL;
    PVOID OrdinalImportHandle = NULL;
    TEST_FN TestFn = NULL;
    TEST_FN OrdinalFn = NULL;
    TEST_FN OrdinalImportFn = NULL;

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
    if (OrdinalImportHandle != NULL) {
        LdrUnloadDll( OrdinalImportHandle );
    }
    if (DllHandle2 != NULL) {
        LdrUnloadDll( DllHandle2 );
    }
    if (DllHandle != NULL) {
        LdrUnloadDll( DllHandle );
    }
    return Result;
}
