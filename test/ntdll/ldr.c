#if _KERNEL_MODE
#include <Ldk/ntdll.h>

BOOLEAN
LdrTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LdrTest)
#endif
#else
#include <Windows.h>

EXTERN_C_START

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

#define DbgPrint        printf
#define PAGED_CODE()
#endif

BOOLEAN
LdrTest (
    VOID
    )
{
    PAGED_CODE();

    // //
    // // TestFunction함수를 가진 Test.dll를 만들어주세요
    // //

    // typedef LONG(__stdcall* TEST_FN)(LONG);

    // NTSTATUS Status;
    // UNICODE_STRING dllName = RTL_CONSTANT_STRING(L"Test.dll");
    // PVOID dllHandle;
    // Status = LdrLoadDll(NULL, NULL, &dllName, &dllHandle);
    // if (! NT_SUCCESS(Status)) {
    //     return FALSE;
    // }

    // ANSI_STRING procName = RTL_CONSTANT_STRING("TestFunction");
    // TEST_FN testFn;
    // Status = LdrGetProcedureAddress(dllHandle, &procName, 0, (PVOID*)&testFn);
    // if (! NT_SUCCESS(Status)) {
    //     LdrUnloadDll(dllHandle);
    //     return FALSE;
    // }
    // if (testFn(10) != 10) {
    //     LdrUnloadDll(dllHandle);
    //     return FALSE;
    // }
    // return NT_SUCCESS(LdrUnloadDll(dllHandle));

    return TRUE;
}