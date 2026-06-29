#include "winbase.h"
#include "../ldk.h"

static PVOID volatile LdkpPointerEncodeCookie;
static PVOID volatile LdkpSystemPointerEncodeCookie;

static
ULONG_PTR
LdkpRotatePointerLeft (
    _In_ ULONG_PTR Value,
    _In_ UCHAR Shift
    )
{
    UCHAR Bits = (UCHAR)(sizeof(ULONG_PTR) * 8);

    Shift &= (UCHAR)(Bits - 1);
    if (Shift == 0) {
        return Value;
    }

    return (Value << Shift) | (Value >> (Bits - Shift));
}

static
ULONG_PTR
LdkpRotatePointerRight (
    _In_ ULONG_PTR Value,
    _In_ UCHAR Shift
    )
{
    UCHAR Bits = (UCHAR)(sizeof(ULONG_PTR) * 8);

    Shift &= (UCHAR)(Bits - 1);
    if (Shift == 0) {
        return Value;
    }

    return (Value >> Shift) | (Value << (Bits - Shift));
}

static
ULONG_PTR
LdkpMixPointerEncodeCookie (
    _In_ ULONG_PTR Value
    )
{
#if defined(_WIN64)
    Value ^= Value >> 33;
    Value *= 0xff51afd7ed558ccdULL;
    Value ^= Value >> 33;
    Value *= 0xc4ceb9fe1a85ec53ULL;
    Value ^= Value >> 33;
#else
    Value ^= Value >> 16;
    Value *= 0x7feb352dUL;
    Value ^= Value >> 15;
    Value *= 0x846ca68bUL;
    Value ^= Value >> 16;
#endif

    return Value | 1;
}

static
ULONG_PTR
LdkpCreatePointerEncodeCookie (
    _In_ ULONG_PTR Domain
    )
{
    LARGE_INTEGER Counter;
    LARGE_INTEGER TickCount;
    ULONG_PTR Cookie;

    Counter = KeQueryPerformanceCounter( NULL );
    KeQueryTickCount( &TickCount );

    Cookie = (ULONG_PTR)Counter.QuadPart;
    Cookie ^= (ULONG_PTR)TickCount.QuadPart;
    Cookie ^= (ULONG_PTR)PsGetCurrentProcessId();
    Cookie ^= (ULONG_PTR)PsGetCurrentThreadId();
    Cookie ^= Domain;
    Cookie ^= (ULONG_PTR)&Cookie;

    return LdkpMixPointerEncodeCookie( Cookie );
}

static
ULONG_PTR
LdkpGetPointerEncodeCookie (
    _Inout_ PVOID volatile *CookieSlot
    )
{
    ULONG_PTR Cookie;

    Cookie = (ULONG_PTR)*CookieSlot;
    if (Cookie == 0) {
        PVOID ExistingCookie;
        ULONG_PTR NewCookie;

        NewCookie = LdkpCreatePointerEncodeCookie( (ULONG_PTR)CookieSlot );
        ExistingCookie = InterlockedCompareExchangePointer( CookieSlot,
                                                            (PVOID)NewCookie,
                                                            NULL );
        Cookie = ExistingCookie != NULL ? (ULONG_PTR)ExistingCookie : NewCookie;
    }

    return Cookie;
}

static
PVOID
LdkpEncodePointer (
    _In_opt_ PVOID Ptr,
    _In_ ULONG_PTR Cookie
    )
{
    UCHAR Shift;

    Shift = (UCHAR)(Cookie & (sizeof(ULONG_PTR) * 8 - 1));

    return (PVOID)LdkpRotatePointerRight( ((ULONG_PTR)Ptr) ^ Cookie,
                                          Shift );
}

static
PVOID
LdkpDecodePointer (
    _In_opt_ PVOID Ptr,
    _In_ ULONG_PTR Cookie
    )
{
    UCHAR Shift;

    Shift = (UCHAR)(Cookie & (sizeof(ULONG_PTR) * 8 - 1));

    return (PVOID)(LdkpRotatePointerLeft( (ULONG_PTR)Ptr,
                                          Shift ) ^ Cookie);
}

static
ULONG_PTR
LdkpGetProcessPointerEncodeCookie (
    VOID
    )
{
    return LdkpGetPointerEncodeCookie( &LdkpPointerEncodeCookie );
}

static
ULONG_PTR
LdkpGetSystemPointerEncodeCookie (
    VOID
    )
{
    return LdkpGetPointerEncodeCookie( &LdkpSystemPointerEncodeCookie );
}

WINBASEAPI
_Ret_maybenull_
PVOID
WINAPI
EncodePointer (
    _In_opt_ PVOID Ptr
    )
{
    return LdkpEncodePointer( Ptr,
                              LdkpGetProcessPointerEncodeCookie() );
}

WINBASEAPI
_Ret_maybenull_
PVOID
WINAPI
DecodePointer (
    _In_opt_ PVOID Ptr
    )
{
    return LdkpDecodePointer( Ptr,
                              LdkpGetProcessPointerEncodeCookie() );
}

WINBASEAPI
_Ret_maybenull_
PVOID
WINAPI
EncodeSystemPointer (
    _In_opt_ PVOID Ptr
    )
{
    return LdkpEncodePointer( Ptr,
                              LdkpGetSystemPointerEncodeCookie() );
}

WINBASEAPI
_Ret_maybenull_
PVOID
WINAPI
DecodeSystemPointer (
    _In_opt_ PVOID Ptr
    )
{
    return LdkpDecodePointer( Ptr,
                              LdkpGetSystemPointerEncodeCookie() );
}
