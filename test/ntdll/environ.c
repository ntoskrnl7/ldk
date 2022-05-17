#if _KERNEL_MODE
#include <Ldk/ntdll.h>

BOOLEAN
NtdllEnvironmentVariableTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtdllEnvironmentVariableTest)
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

_At_(DestinationString->Buffer, _Post_equal_to_(SourceString))
_At_(DestinationString->Length, _Post_equal_to_(_String_length_(SourceString) * sizeof(WCHAR)))
_At_(DestinationString->MaximumLength, _Post_equal_to_((_String_length_(SourceString)+1) * sizeof(WCHAR)))
NTSYSAPI
VOID
NTAPI
RtlInitUnicodeString(
    _Out_ PUNICODE_STRING DestinationString,
    _In_opt_z_ __drv_aliasesMem PCWSTR SourceString
    );

_Must_inspect_result_
NTSYSAPI
BOOLEAN
NTAPI
RtlEqualUnicodeString(
    _In_ PCUNICODE_STRING String1,
    _In_ PCUNICODE_STRING String2,
    _In_ BOOLEAN CaseInSensitive
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U (
    _Inout_opt_ PVOID Environment,
    _In_ PCUNICODE_STRING Name,
    _Inout_ PUNICODE_STRING Value
    );

NTSYSAPI
NTSTATUS
NTAPI
RtlSetEnvironmentVariable (
    _Inout_opt_ PVOID *Environment,
    _In_ PCUNICODE_STRING Name,
    _In_opt_ PCUNICODE_STRING Value
    );

EXTERN_C_END
#pragma comment(lib, "ntdll.lib")

#define PAGED_CODE()
#endif

BOOLEAN
NtdllEnvironmentVariableTest (
    VOID
    )
{
    PAGED_CODE();

    UNICODE_STRING name = RTL_CONSTANT_STRING(L"windir");
    UNICODE_STRING value;
    WCHAR buffer[128];
    value.Buffer = buffer;
    value.MaximumLength = sizeof(buffer);
    NTSTATUS Status = RtlQueryEnvironmentVariable_U( NULL,
                                                     &name,
                                                     &value );
    if (! NT_SUCCESS(Status)) {
        return FALSE;
    }

    RtlInitUnicodeString( &name,
                          L"SystemRoot" );
    Status = RtlQueryEnvironmentVariable_U( NULL,
                                            &name,
                                            &value );
    if (! NT_SUCCESS(Status)) {
        return FALSE;
    }

    // Add TestVar
    RtlInitUnicodeString( &name,
                          L"TestVar" );
    UNICODE_STRING expect;
    RtlInitUnicodeString( &expect,
                          L"Test" );
    Status = RtlSetEnvironmentVariable( NULL,
                                        &name,
                                        &expect );
    if (! NT_SUCCESS(Status)) {
        return FALSE;
    }
    Status = RtlQueryEnvironmentVariable_U( NULL,
                                            &name,
                                            &value );
    if (! NT_SUCCESS(Status)) {
        return FALSE;
    }
    if (! RtlEqualUnicodeString( &value,
                                 &expect,
                                 FALSE )) {
        return FALSE;
    }

    // Remove TestVar
    Status = RtlSetEnvironmentVariable( NULL,
                                        &name,
                                        NULL );
    if (! NT_SUCCESS(Status)) {
        return FALSE;
    }
    Status = RtlQueryEnvironmentVariable_U( NULL,
                                            &name,
                                            &value );
    if (NT_SUCCESS(Status)) {
        return FALSE;
    }
    return TRUE;
}