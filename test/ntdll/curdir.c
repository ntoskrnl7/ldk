#if _KERNEL_MODE
#include <Ldk/ntdll.h>

BOOLEAN
NtdllCurrentDirectoryTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtdllCurrentDirectoryTest)
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

#pragma comment(lib, "ntdll.lib")

#define DbgPrint        printf
#define PAGED_CODE()
#endif

BOOLEAN
NtdllCurrentDirectoryTest (
    VOID
    )
{
    PAGED_CODE();
    UNICODE_STRING actual;
    WCHAR buffer[512];
    actual.Buffer = buffer;
    actual.MaximumLength = sizeof(buffer);
    actual.Length = (USHORT)RtlGetCurrentDirectory_U( sizeof(buffer),
                                                      buffer );
    if (actual.Length == 0) {
        return FALSE;
    }

    UNICODE_STRING path = RTL_CONSTANT_STRING(L"C:\\");
    NTSTATUS Status = RtlSetCurrentDirectory_U( &path );
    if (! NT_SUCCESS(Status)) {
        return FALSE;
    }
    actual.Length = (USHORT)RtlGetCurrentDirectory_U( sizeof(buffer),
                                                      buffer );
    if (actual.Length == 0) {
        return FALSE;
    }
    if (! RtlEqualUnicodeString( &actual,
                                 &path,
                                 FALSE )) {
        return FALSE;
    }
    return TRUE;
}