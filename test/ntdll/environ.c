#if _KERNEL_MODE
#include <Ldk/ntdll.h>

HANDLE
WINAPI
GetProcessHeap (
    VOID
    );

BOOL
WINAPI
HeapFree (
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _Frees_ptr_opt_ LPVOID lpMem
    );

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

#ifndef STATUS_VARIABLE_NOT_FOUND
#define STATUS_VARIABLE_NOT_FOUND ((NTSTATUS)0xC0000100)
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

static
BOOLEAN
LdkTestSetEnvironmentValue (
    _Inout_opt_ PVOID *Environment,
    _In_ PCWSTR NameText,
    _In_opt_ PCWSTR ValueText
    )
{
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    NTSTATUS Status;

    RtlInitUnicodeString( &Name,
                          NameText );
    RtlInitUnicodeString( &Value,
                          ValueText );

    Status = RtlSetEnvironmentVariable( Environment,
                                        &Name,
                                        ValueText ? &Value : NULL );
    return NT_SUCCESS(Status);
}

static
BOOLEAN
LdkTestQueryEnvironmentValue (
    _In_opt_ PVOID Environment,
    _In_ PCWSTR NameText,
    _In_ PCWSTR ExpectedText
    )
{
    UNICODE_STRING Name;
    UNICODE_STRING Expected;
    UNICODE_STRING Value;
    WCHAR Buffer[256];
    NTSTATUS Status;

    RtlInitUnicodeString( &Name,
                          NameText );
    RtlInitUnicodeString( &Expected,
                          ExpectedText );

    Value.Buffer = Buffer;
    Value.Length = 0;
    Value.MaximumLength = sizeof(Buffer);

    Status = RtlQueryEnvironmentVariable_U( Environment,
                                            &Name,
                                            &Value );
    if (!NT_SUCCESS(Status)) {
        return FALSE;
    }

    return RtlEqualUnicodeString( &Value,
                                  &Expected,
                                  FALSE );
}

static
BOOLEAN
LdkTestQueryEnvironmentMissing (
    _In_opt_ PVOID Environment,
    _In_ PCWSTR NameText
    )
{
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    WCHAR Buffer[16];
    NTSTATUS Status;

    RtlInitUnicodeString( &Name,
                          NameText );

    Value.Buffer = Buffer;
    Value.Length = 0;
    Value.MaximumLength = sizeof(Buffer);

    Status = RtlQueryEnvironmentVariable_U( Environment,
                                            &Name,
                                            &Value );
    return Status == STATUS_VARIABLE_NOT_FOUND;
}

static
VOID
LdkTestFreePrivateEnvironment (
    _In_opt_ PVOID Environment
    )
{
    if (!Environment) {
        return;
    }

    HeapFree( GetProcessHeap(),
              0,
              Environment );
}

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

    if (!LdkTestSetEnvironmentValue( NULL,
                                     L"LDK_RTL_ENV_PROCESS_MISSING_DELETE",
                                     NULL ) ||
        !LdkTestQueryEnvironmentMissing( NULL,
                                         L"LDK_RTL_ENV_PROCESS_MISSING_DELETE" )) {
        return FALSE;
    }

    if (!LdkTestSetEnvironmentValue( NULL,
                                     L"LDK_RTL_ENV_PROCESS",
                                     L"alpha" ) ||
        !LdkTestQueryEnvironmentValue( NULL,
                                       L"LDK_RTL_ENV_PROCESS",
                                       L"alpha" )) {
        return FALSE;
    }

    if (!LdkTestSetEnvironmentValue( NULL,
                                     L"LDK_RTL_ENV_PROCESS",
                                     L"beta" ) ||
        !LdkTestQueryEnvironmentValue( NULL,
                                       L"LDK_RTL_ENV_PROCESS",
                                       L"beta" )) {
        return FALSE;
    }

    if (!LdkTestSetEnvironmentValue( NULL,
                                     L"LDK_RTL_ENV_PROCESS",
                                     L"alpha-beta-gamma-long-value" ) ||
        !LdkTestQueryEnvironmentValue( NULL,
                                       L"LDK_RTL_ENV_PROCESS",
                                       L"alpha-beta-gamma-long-value" )) {
        return FALSE;
    }

    if (!LdkTestSetEnvironmentValue( NULL,
                                     L"LDK_RTL_ENV_PROCESS",
                                     L"z" ) ||
        !LdkTestQueryEnvironmentValue( NULL,
                                       L"LDK_RTL_ENV_PROCESS",
                                       L"z" )) {
        return FALSE;
    }

    if (!LdkTestSetEnvironmentValue( NULL,
                                     L"LDK_RTL_ENV_PROCESS",
                                     NULL ) ||
        !LdkTestQueryEnvironmentMissing( NULL,
                                         L"LDK_RTL_ENV_PROCESS" )) {
        return FALSE;
    }

    PVOID PrivateEnvironment = NULL;
    if (!LdkTestSetEnvironmentValue( &PrivateEnvironment,
                                     L"LDK_RTL_ENV_PRIVATE_MISSING_DELETE",
                                     NULL ) ||
        !LdkTestQueryEnvironmentMissing( PrivateEnvironment,
                                         L"LDK_RTL_ENV_PRIVATE_MISSING_DELETE" )) {
        LdkTestFreePrivateEnvironment( PrivateEnvironment );
        return FALSE;
    }

    if (!LdkTestSetEnvironmentValue( &PrivateEnvironment,
                                     L"LDK_RTL_ENV_PRIVATE_B",
                                     L"bravo" ) ||
        !LdkTestSetEnvironmentValue( &PrivateEnvironment,
                                     L"LDK_RTL_ENV_PRIVATE_A",
                                     L"alpha" ) ||
        !LdkTestQueryEnvironmentValue( PrivateEnvironment,
                                       L"LDK_RTL_ENV_PRIVATE_A",
                                       L"alpha" ) ||
        !LdkTestQueryEnvironmentValue( PrivateEnvironment,
                                       L"LDK_RTL_ENV_PRIVATE_B",
                                       L"bravo" )) {
        LdkTestFreePrivateEnvironment( PrivateEnvironment );
        return FALSE;
    }

    if (!LdkTestSetEnvironmentValue( &PrivateEnvironment,
                                     L"LDK_RTL_ENV_PRIVATE_A",
                                     L"alpha-beta-gamma-private-long-value" ) ||
        !LdkTestQueryEnvironmentValue( PrivateEnvironment,
                                       L"LDK_RTL_ENV_PRIVATE_A",
                                       L"alpha-beta-gamma-private-long-value" ) ||
        !LdkTestSetEnvironmentValue( &PrivateEnvironment,
                                     L"LDK_RTL_ENV_PRIVATE_A",
                                     NULL ) ||
        !LdkTestQueryEnvironmentMissing( PrivateEnvironment,
                                         L"LDK_RTL_ENV_PRIVATE_A" ) ||
        !LdkTestQueryEnvironmentValue( PrivateEnvironment,
                                       L"LDK_RTL_ENV_PRIVATE_B",
                                       L"bravo" )) {
        LdkTestFreePrivateEnvironment( PrivateEnvironment );
        return FALSE;
    }

    LdkTestFreePrivateEnvironment( PrivateEnvironment );
    return TRUE;
}
