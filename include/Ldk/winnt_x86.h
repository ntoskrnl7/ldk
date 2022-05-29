#pragma once

#ifdef _X86_

//
// Some intrinsics have a redundant __cdecl calling-convention specifier when
// not compiled with /clr:pure.
//

#if defined(_M_CEE_PURE)

#define CDECL_NON_WVMPURE

#else

#define CDECL_NON_WVMPURE __cdecl

#endif

// end_ntoshvp
//
// Disable these two pragmas that evaluate to "sti" "cli" on x86 so that driver
// writers to not leave them inadvertantly in their code.
//

#if !defined(MIDL_PASS)
#if !defined(RC_INVOKED)

#if _MSC_VER >= 1200
#pragma warning(push)
#endif // _MSC_VER >= 1200
//#pragma warning(disable:4164)   // disable C4164 warning so that apps that
                                // build with /Od don't get weird errors !

#if _MSC_VER >= 1200
#pragma warning( pop )
#else
#pragma warning( default:4164 ) // reenable C4164 warning
#endif // _MSC_VER >= 1200

#endif // !defined(MIDL_PASS)
#endif // !defined(RC_INVOKED)



// end_ntddk end_nthal
// begin_ntoshvp
#if defined(_M_IX86) && !defined(RC_INVOKED) && !defined(MIDL_PASS)

#ifdef __cplusplus
extern "C" {
#endif

#if !defined(_MANAGED)

//
// Define bit test intrinsics.
//

#define BitTest _bittest
#define BitTestAndComplement _bittestandcomplement
#define BitTestAndSet _bittestandset
#define BitTestAndReset _bittestandreset
#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndSetAcquire _interlockedbittestandset
#define InterlockedBitTestAndSetRelease _interlockedbittestandset
#define InterlockedBitTestAndSetNoFence _interlockedbittestandset
#define InterlockedBitTestAndReset _interlockedbittestandreset
#define InterlockedBitTestAndResetAcquire _interlockedbittestandreset
#define InterlockedBitTestAndResetRelease _interlockedbittestandreset
#define InterlockedBitTestAndResetNoFence _interlockedbittestandreset

_Must_inspect_result_
BOOLEAN
_bittest (
    _In_reads_bytes_((Offset/8)+1) LONG const *Base,
    _In_range_(>=,0) LONG Offset
    );

BOOLEAN
_bittestandcomplement (
    _Inout_updates_bytes_((Offset/8)+1) LONG *Base,
    _In_range_(>=,0) LONG Offset
    );

BOOLEAN
_bittestandset (
    _Inout_updates_bytes_((Offset/8)+1) LONG *Base,
    _In_range_(>=,0) LONG Offset
    );

BOOLEAN
_bittestandreset (
    _Inout_updates_bytes_((Offset/8)+1) LONG *Base,
    _In_range_(>=,0) LONG Offset
    );

BOOLEAN
_interlockedbittestandset (
    _Inout_updates_bytes_((Offset/8)+1) _Interlocked_operand_ LONG volatile *Base,
    _In_range_(>=,0) LONG Offset
    );

BOOLEAN
_interlockedbittestandreset (
    _Inout_updates_bytes_((Offset/8)+1) _Interlocked_operand_ LONG volatile *Base,
    _In_range_(>=,0) LONG Offset
    );

#pragma intrinsic(_bittest)
#pragma intrinsic(_bittestandcomplement)
#pragma intrinsic(_bittestandset)
#pragma intrinsic(_bittestandreset)
#pragma intrinsic(_interlockedbittestandset)
#pragma intrinsic(_interlockedbittestandreset)

//
// Define bit scan intrinsics.
//

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse

_Success_(return != 0)
BOOLEAN
_BitScanForward (
    _Out_ DWORD *Index,
    _In_ DWORD Mask
    );

_Success_(return != 0)
BOOLEAN
_BitScanReverse (
    _Out_ DWORD *Index,
    _In_ DWORD Mask
    );

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)

#if !defined(_WDMDDK_)
_Success_(return != 0)
FORCEINLINE
BOOLEAN
_InlineBitScanForward64 (
    _Out_ DWORD *Index,
    _In_ DWORD64 Mask
    )
{
    if (_BitScanForward(Index, (DWORD)Mask)) {
        return 1;
    }

    if (_BitScanForward(Index, (DWORD)(Mask >> 32))) {
        *Index += 32;
        return 1;
    }

    return 0;
}

#define BitScanForward64 _InlineBitScanForward64

_Success_(return != 0)
FORCEINLINE
BOOLEAN
_InlineBitScanReverse64 (
    _Out_ DWORD *Index,
    _In_ DWORD64 Mask
    )
{
    if (_BitScanReverse(Index, (DWORD)(Mask >> 32))) {
        *Index += 32;
        return 1;
    }

    if (_BitScanReverse(Index, (DWORD)Mask)) {
        return 1;
    }

    return 0;
}
#endif // !defined(_WDMDDK_aaa)

#define BitScanReverse64 _InlineBitScanReverse64

#endif // !defined(_MANAGED)

//
// Interlocked intrinsic functions.
//

#if !defined(_MANAGED)

#define InterlockedIncrement16 _InterlockedIncrement16
#define InterlockedIncrementAcquire16 _InterlockedIncrement16
#define InterlockedIncrementRelease16 _InterlockedIncrement16
#define InterlockedIncrementNoFence16 _InterlockedIncrement16

#define InterlockedDecrement16 _InterlockedDecrement16
#define InterlockedDecrementAcquire16 _InterlockedDecrement16
#define InterlockedDecrementRelease16 _InterlockedDecrement16
#define InterlockedDecrementNoFence16 _InterlockedDecrement16

#define InterlockedCompareExchange16 _InterlockedCompareExchange16
#define InterlockedCompareExchangeAcquire16 _InterlockedCompareExchange16
#define InterlockedCompareExchangeRelease16 _InterlockedCompareExchange16
#define InterlockedCompareExchangeNoFence16 _InterlockedCompareExchange16

#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedCompareExchangeAcquire64 _InterlockedCompareExchange64
#define InterlockedCompareExchangeRelease64 _InterlockedCompareExchange64
#define InterlockedCompareExchangeNoFence64 _InterlockedCompareExchange64

SHORT
InterlockedIncrement16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Addend
    );

SHORT
InterlockedDecrement16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Addend
    );

SHORT
InterlockedCompareExchange16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT ExChange,
    _In_ SHORT Comperand
    );

LONG64
InterlockedCompareExchange64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 ExChange,
    _In_ LONG64 Comperand
    );

#pragma intrinsic(_InterlockedIncrement16)
#pragma intrinsic(_InterlockedDecrement16)
#pragma intrinsic(_InterlockedCompareExchange16)
#pragma intrinsic(_InterlockedCompareExchange64)

#endif // !defined(_MANAGED)

#define InterlockedAnd _InterlockedAnd
#define InterlockedAndAcquire _InterlockedAnd
#define InterlockedAndRelease _InterlockedAnd
#define InterlockedAndNoFence _InterlockedAnd

#define InterlockedOr _InterlockedOr
#define InterlockedOrAcquire _InterlockedOr
#define InterlockedOrRelease _InterlockedOr
#define InterlockedOrNoFence _InterlockedOr

#define InterlockedXor _InterlockedXor
#define InterlockedXorAcquire _InterlockedXor
#define InterlockedXorRelease _InterlockedXor
#define InterlockedXorNoFence _InterlockedXor

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedIncrementAcquire _InterlockedIncrement
#define InterlockedIncrementRelease _InterlockedIncrement
#define InterlockedIncrementNoFence _InterlockedIncrement

#define InterlockedDecrement _InterlockedDecrement
#define InterlockedDecrementAcquire _InterlockedDecrement
#define InterlockedDecrementRelease _InterlockedDecrement
#define InterlockedDecrementNoFence _InterlockedDecrement

#define InterlockedAdd _InlineInterlockedAdd
#define InterlockedAddAcquire _InlineInterlockedAdd
#define InterlockedAddRelease _InlineInterlockedAdd
#define InterlockedAddNoFence _InlineInterlockedAdd
#define InterlockedAddNoFence64 _InlineInterlockedAdd64

#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAcquire _InterlockedExchange
#define InterlockedExchangeNoFence _InterlockedExchange

#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedExchangeAddAcquire _InterlockedExchangeAdd
#define InterlockedExchangeAddRelease _InterlockedExchangeAdd
#define InterlockedExchangeAddNoFence _InterlockedExchangeAdd

#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedCompareExchangeAcquire _InterlockedCompareExchange
#define InterlockedCompareExchangeRelease _InterlockedCompareExchange
#define InterlockedCompareExchangeNoFence _InterlockedCompareExchange

#define InterlockedExchange16 _InterlockedExchange16

LONG
InterlockedAnd (
    _Inout_ _Interlocked_operand_ LONG volatile *Destination,
    _In_ LONG Value
    );

LONG
InterlockedOr (
    _Inout_ _Interlocked_operand_ LONG volatile *Destination,
    _In_ LONG Value
    );

LONG
InterlockedXor (
    _Inout_ _Interlocked_operand_ LONG volatile *Destination,
    _In_ LONG Value
    );

LONG
CDECL_NON_WVMPURE
InterlockedIncrement (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend
    );

LONG
CDECL_NON_WVMPURE
InterlockedDecrement (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend
    );

LONG
__cdecl
InterlockedExchange (
    _Inout_ _Interlocked_operand_ LONG volatile *Target,
    _In_ LONG Value
    );

LONG
__cdecl
InterlockedExchangeAdd (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend,
    _In_ LONG Value
    );

#if !defined(_WDMDDK_)
FORCEINLINE
LONG
_InlineInterlockedAdd (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend,
    _In_ LONG Value
    )

{

    return InterlockedExchangeAdd(Addend, Value) + Value;
}

LONG
CDECL_NON_WVMPURE
InterlockedCompareExchange (
    _Inout_ _Interlocked_operand_ LONG volatile * Destination,
    _In_ LONG ExChange,
    _In_ LONG Comperand
    );

#undef _InterlockedExchangePointer

FORCEINLINE
_Ret_writes_(_Inexpressible_(Unknown))
PVOID
_InlineInterlockedExchangePointer(
    _Inout_ _At_(*Destination,
        _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
    _Interlocked_operand_ PVOID volatile * Destination,
    _In_opt_ PVOID Value
    )
{
    return (PVOID)InterlockedExchange((LONG volatile *) Destination,
                                      (LONG) Value);
}

#define InterlockedExchangePointer _InlineInterlockedExchangePointer
#define InterlockedExchangePointerAcquire _InlineInterlockedExchangePointer
#define InterlockedExchangePointerRelease _InlineInterlockedExchangePointer
#define InterlockedExchangePointerNoFence _InlineInterlockedExchangePointer

FORCEINLINE
_Ret_writes_(_Inexpressible_(Unknown))
PVOID
_InlineInterlockedCompareExchangePointer (
    _Inout_ _At_(*Destination,
        _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
    _Interlocked_operand_ PVOID volatile * Destination,
    _In_opt_ PVOID ExChange,
    _In_opt_ PVOID Comperand
    )
{
    return (PVOID)InterlockedCompareExchange((LONG volatile *) Destination,
                                             (LONG) ExChange,
                                             (LONG) Comperand);
}
#endif // !defined(_WDMDDK_aaa)

#define InterlockedCompareExchangePointer \
    _InlineInterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerAcquire \
    _InlineInterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerRelease \
    _InlineInterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerNoFence \
    _InlineInterlockedCompareExchangePointer

#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)

#if !defined(_MANAGED)

#if (_MSC_VER >= 1600)

#define InterlockedExchange8 _InterlockedExchange8
#define InterlockedExchange16 _InterlockedExchange16
#define InterlockedExchangeNoFence8 InterlockedExchange8
#define InterlockedExchangeAcquire8 InterlockedExchange8

CHAR
InterlockedExchange8 (
    _Inout_ _Interlocked_operand_ CHAR volatile *Target,
    _In_ CHAR Value
    );

SHORT
InterlockedExchange16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT ExChange
    );

#pragma intrinsic(_InterlockedExchange8)
#pragma intrinsic(_InterlockedExchange16)

#endif // _MSC_VER >= 1600

#if _MSC_FULL_VER >= 140041204

#define InterlockedExchangeAdd8 _InterlockedExchangeAdd8
#define InterlockedAnd8 _InterlockedAnd8
#define InterlockedAndAcquire8 _InterlockedAnd8
#define InterlockedAndRelease8 _InterlockedAnd8
#define InterlockedAndNoFence8 _InterlockedAnd8
#define InterlockedOr8 _InterlockedOr8
#define InterlockedOrAcquire8 _InterlockedOr8
#define InterlockedOrRelease8 _InterlockedOr8
#define InterlockedOrNoFence8 _InterlockedOr8
#define InterlockedXor8 _InterlockedXor8
#define InterlockedXorAcquire8 _InterlockedXor8
#define InterlockedXorRelease8 _InterlockedXor8
#define InterlockedXorNoFence8 _InterlockedXor8
#define InterlockedAnd16 _InterlockedAnd16
#define InterlockedOr16 _InterlockedOr16
#define InterlockedXor16 _InterlockedXor16
#define InterlockedCompareExchange16 _InterlockedCompareExchange16
#define InterlockedIncrement16 _InterlockedIncrement16
#define InterlockedDecrement16 _InterlockedDecrement16

char 
InterlockedExchangeAdd8 (
    _Inout_ _Interlocked_operand_ char volatile * _Addend, 
    _In_ char _Value
    );

char
InterlockedAnd8 (
    _Inout_ _Interlocked_operand_ char volatile *Destination,
    _In_ char Value
    );

char
InterlockedOr8 (
    _Inout_ _Interlocked_operand_ char volatile *Destination,
    _In_ char Value
    );

char
InterlockedXor8 (
    _Inout_ _Interlocked_operand_ char volatile *Destination,
    _In_ char Value
    );

SHORT
_InterlockedAnd16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

SHORT
InterlockedXor16(
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

SHORT
_InterlockedCompareExchange16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT ExChange,
    _In_ SHORT Comperand
    );

SHORT
_InterlockedOr16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

SHORT
_InterlockedIncrement16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination
    );

SHORT
_InterlockedDecrement16 (
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination
    );

#pragma intrinsic (_InterlockedExchangeAdd8)
#pragma intrinsic (_InterlockedAnd8)
#pragma intrinsic (_InterlockedOr8)
#pragma intrinsic (_InterlockedXor8)
#pragma intrinsic (_InterlockedAnd16)
#pragma intrinsic (_InterlockedOr16)
#pragma intrinsic (_InterlockedXor16)
#pragma intrinsic (_InterlockedCompareExchange16)
#pragma intrinsic (_InterlockedIncrement16)
#pragma intrinsic (_InterlockedDecrement16)

#endif  /* _MSC_FULL_VER >= 140040816 */

//
// Define 64-bit operations in terms of InterlockedCompareExchange64
//

#define InterlockedCompareExchange64 _InterlockedCompareExchange64

#if !defined(_WDMDDK_)
FORCEINLINE
LONG64
_InlineInterlockedAnd64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    )
{
    LONG64 Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old & Value,
                                          Old) != Old);

    return Old;
}

#define InterlockedAnd64 _InlineInterlockedAnd64
#define InterlockedAnd64Acquire _InlineInterlockedAnd64
#define InterlockedAnd64Release _InlineInterlockedAnd64
#define InterlockedAnd64NoFence _InlineInterlockedAnd64

FORCEINLINE
LONG64
_InlineInterlockedAdd64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Addend,
    _In_ LONG64 Value
    )
{
    LONG64 Old;

    do {
        Old = *Addend;
    } while (InterlockedCompareExchange64(Addend,
                                          Old + Value,
                                          Old) != Old);

    return Old + Value;
}
#endif // !defined(_WDMDDK_aaa)

#define InterlockedAdd64 _InlineInterlockedAdd64
#define InterlockedAddAcquire64 _InlineInterlockedAdd64
#define InterlockedAddRelease64 _InlineInterlockedAdd64
#define InterlockedAddNoFence64 _InlineInterlockedAdd64


#endif // !defined(_MANAGED)

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd((LONG *)a, b)
#define InterlockedExchangeAddSizeTAcquire(a, b) InterlockedExchangeAdd((LONG *)a, b)
#define InterlockedExchangeAddSizeTNoFence(a, b) InterlockedExchangeAdd((LONG *)a, b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement((LONG *)a)
#define InterlockedIncrementSizeTNoFence(a) InterlockedIncrement((LONG *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement((LONG *)a)
#define InterlockedDecrementSizeTNoFence(a) InterlockedDecrement((LONG *)a)

//
// Definitons below
//

LONG
_InterlockedXor (
    _Inout_ _Interlocked_operand_ LONG volatile *Target,
    _In_ LONG Set
    );

#pragma intrinsic(_InterlockedXor)

#define InterlockedXor _InterlockedXor

#if !defined(_MANAGED)

#if !defined(_WDMDDK_)
LONGLONG
FORCEINLINE
_InlineInterlockedOr64 (
    _Inout_ _Interlocked_operand_ LONGLONG volatile *Destination,
    _In_ LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old | Value,
                                          Old) != Old);

    return Old;
}

#define InterlockedOr64 _InlineInterlockedOr64

FORCEINLINE
LONG64
_InlineInterlockedXor64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    )
{
    LONG64 Old;

    do {
        Old = *Destination;
    } while (InterlockedCompareExchange64(Destination,
                                          Old ^ Value,
                                          Old) != Old);

    return Old;
}

#define InterlockedXor64 _InlineInterlockedXor64

LONGLONG
FORCEINLINE
_InlineInterlockedIncrement64 (
    _Inout_ _Interlocked_operand_ LONGLONG volatile *Addend
    )
{
    LONGLONG Old;

    do {
        Old = *Addend;
    } while (InterlockedCompareExchange64(Addend,
                                          Old + 1,
                                          Old) != Old);

    return Old + 1;
}

#define InterlockedIncrement64 _InlineInterlockedIncrement64
#define InterlockedIncrementAcquire64 InterlockedIncrement64
#define InterlockedIncrementRelease64 InterlockedIncrement64
#define InterlockedIncrementNoFence64 InterlockedIncrement64

FORCEINLINE
LONGLONG
_InlineInterlockedDecrement64 (
    _Inout_ _Interlocked_operand_ LONGLONG volatile *Addend
    )
{
    LONGLONG Old;

    do {
        Old = *Addend;
    } while (InterlockedCompareExchange64(Addend,
                                          Old - 1,
                                          Old) != Old);

    return Old - 1;
}

#define InterlockedDecrement64 _InlineInterlockedDecrement64
#define InterlockedDecrementAcquire64 InterlockedDecrement64
#define InterlockedDecrementRelease64 InterlockedDecrement64
#define InterlockedDecrementNoFence64 InterlockedDecrement64

FORCEINLINE
LONGLONG
_InlineInterlockedExchange64 (
    _Inout_ _Interlocked_operand_ LONGLONG volatile *Target,
    _In_ LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Target;
    } while (InterlockedCompareExchange64(Target,
                                          Value,
                                          Old) != Old);

    return Old;
}

#define InterlockedExchange64 _InlineInterlockedExchange64
#define InterlockedExchangeAcquire64 InterlockedExchange64
#define InterlockedExchangeNoFence64 _InlineInterlockedExchange64


FORCEINLINE
LONGLONG
_InlineInterlockedExchangeAdd64 (
    _Inout_ _Interlocked_operand_ LONGLONG volatile *Addend,
    _In_ LONGLONG Value
    )
{
    LONGLONG Old;

    do {
        Old = *Addend;
    } while (InterlockedCompareExchange64(Addend,
                                          Old + Value,
                                          Old) != Old);

    return Old;
}
#endif // !defined(_WDMDDK_aaa)

#define InterlockedExchangeAdd64 _InlineInterlockedExchangeAdd64
#define InterlockedExchangeAddNoFence64 _InlineInterlockedExchangeAdd64

//
// FS relative adds and increments.
//

VOID
__incfsbyte (
    _In_ DWORD Offset
    );

VOID
__addfsbyte (
    _In_ DWORD Offset,
    _In_ BYTE  Value
    );

VOID
__incfsword (
    _In_ DWORD Offset
    );

VOID
__addfsword (
    _In_ DWORD Offset,
    _In_ WORD   Value
    );

VOID
__incfsdword (
    _In_ DWORD Offset
    );

VOID
__addfsdword (
    _In_ DWORD Offset,
    _In_ DWORD Value
    );

#pragma intrinsic(__incfsbyte)
#pragma intrinsic(__addfsbyte)
#pragma intrinsic(__incfsword)
#pragma intrinsic(__addfsword)
#pragma intrinsic(__incfsdword)
#pragma intrinsic(__addfsdword)

#endif // !defined(_MANAGED)

#if !defined(_M_CEE_PURE)

// end_ntoshvp

#if _MSC_VER >= 1500

//
// Define extended CPUID intrinsic.
//

#define CpuIdEx __cpuidex

VOID
__cpuidex (
    int CPUInfo[4],
    int Function,
    int SubLeaf
    );

#pragma intrinsic(__cpuidex)

#endif

// begin_ntoshvp

//
// Define FS read/write intrinsics
//

BYTE 
__readfsbyte (
    _In_ DWORD Offset
    );

WORD  
__readfsword (
    _In_ DWORD Offset
    );

DWORD
__readfsdword (
    _In_ DWORD Offset
    );

VOID
__writefsbyte (
    _In_ DWORD Offset,
    _In_ BYTE  Data
    );

VOID
__writefsword (
    _In_ DWORD Offset,
    _In_ WORD   Data
    );

VOID
__writefsdword (
    _In_ DWORD Offset,
    _In_ DWORD Data
    );

#pragma intrinsic(__readfsbyte)
#pragma intrinsic(__readfsword)
#pragma intrinsic(__readfsdword)
#pragma intrinsic(__writefsbyte)
#pragma intrinsic(__writefsword)
#pragma intrinsic(__writefsdword)

#endif // !defined(_M_CEE_PURE)


#if !defined(_MANAGED) && !defined(_M_HYBRID_X86_ARM64)
VOID
_mm_pause (
    VOID
    );

#pragma intrinsic(_mm_pause)

#define YieldProcessor _mm_pause

#endif // !defined(_MANAGED) && !defined(_M_HYBRID_X86_ARM64)

#ifdef __cplusplus
}
#endif

#endif  /* !defined(MIDL_PASS) || defined(_M_IX86) */

// end_ntoshvp
// begin_ntoshvp
#if !defined(MIDL_PASS) && defined(_M_IX86)

#if !defined(_M_CEE_PURE) && !defined(_M_HYBRID_X86_ARM64)

#if !defined(_NTDDK_)
#pragma prefast(push)
#pragma warning(push)
#pragma prefast(disable: 6001 28113, "The barrier variable is accessed only to create a side effect.")
#pragma warning(disable: 4793)
FORCEINLINE
VOID
MemoryBarrier (
    VOID
    )
{
    LONG Barrier;

    InterlockedOr(&Barrier, 0);
    return;
}

#pragma warning(pop)
#pragma prefast(pop)
#endif // !defined(_NTDDK_)

#endif /* !_M_CEE_PURE || !_M_HYBRID_X86_ARM64*/

#if !defined(_M_HYBRID_X86_ARM64)

//
// Define constants for use with _mm_prefetch.
//

#define _MM_HINT_T0     1
#define _MM_HINT_T1     2
#define _MM_HINT_T2     3
#define _MM_HINT_NTA    0

VOID
_mm_prefetch (
    _In_ CHAR CONST *a,
    _In_ int sel
    );

#pragma intrinsic(_mm_prefetch)

//
// PreFetchCacheLine level defines.
//

#define PF_TEMPORAL_LEVEL_1 _MM_HINT_T0
#define PF_TEMPORAL_LEVEL_2 _MM_HINT_T1
#define PF_TEMPORAL_LEVEL_3 _MM_HINT_T2
#define PF_NON_TEMPORAL_LEVEL_ALL _MM_HINT_NTA

#define PreFetchCacheLine(l, a)  _mm_prefetch((CHAR CONST *) a, l)
#define PrefetchForWrite(p)
#define ReadForWriteAccess(p) (*(p))

#if !defined(_MANAGED)

//
// Define function to read the value of a performance counter.
//

#define ReadPMC __readpmc

DWORD64
__readpmc (
    _In_ DWORD Counter
    );

#pragma intrinsic(__readpmc)

//
// Define function to read the value of the time stamp counter
//

#define ReadTimeStampCounter() __rdtsc()

DWORD64
__rdtsc (
    VOID
    );

#pragma intrinsic(__rdtsc)

#endif // !defined(_MANAGED)

#endif // !defined(_M_HYBRID_X86_ARM64)

// end_ntoshvp
// end_ntddk

#if !defined(_MANAGED)

__inline PVOID GetFiberData( void )    { return *(PVOID *) (ULONG_PTR) __readfsdword (0x10);}
__inline PVOID GetCurrentFiber( void ) { return (PVOID) (ULONG_PTR) __readfsdword (0x10);}

#endif // !defined(_MANAGED)

// begin_ntddk
// begin_ntoshvp
#endif // !defined(MIDL_PASS) && defined(_M_IX86)
// end_ntoshvp
// end_ntddk

//
// The following values specify the type of failing access when the status is
// STATUS_ACCESS_VIOLATION and the first parameter in the execpetion record.
//

#define EXCEPTION_READ_FAULT          0 // Access violation was caused by a read
#define EXCEPTION_WRITE_FAULT         1 // Access violation was caused by a write
#define EXCEPTION_EXECUTE_FAULT       8 // Access violation was caused by an instruction fetch

// begin_wx86
// begin_ntddk
// begin_ntoshvp

//
//  Define the size of the 80387 save area, which is in the context frame.
//

#define SIZE_OF_80387_REGISTERS      80

//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_i386    0x00010000L    // this assumes that i386 and
#define CONTEXT_i486    0x00010000L    // i486 have identical context records

// end_wx86

#define CONTEXT_CONTROL         (CONTEXT_i386 | 0x00000001L) // SS:SP, CS:IP, FLAGS, BP
#define CONTEXT_INTEGER         (CONTEXT_i386 | 0x00000002L) // AX, BX, CX, DX, SI, DI
#define CONTEXT_SEGMENTS        (CONTEXT_i386 | 0x00000004L) // DS, ES, FS, GS
#define CONTEXT_FLOATING_POINT  (CONTEXT_i386 | 0x00000008L) // 387 state
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_i386 | 0x00000010L) // DB 0-3,6,7
#define CONTEXT_EXTENDED_REGISTERS  (CONTEXT_i386 | 0x00000020L) // cpu specific extensions

#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER |\
                      CONTEXT_SEGMENTS)

#define CONTEXT_ALL             (CONTEXT_CONTROL | CONTEXT_INTEGER | CONTEXT_SEGMENTS | \
                                 CONTEXT_FLOATING_POINT | CONTEXT_DEBUG_REGISTERS | \
                                 CONTEXT_EXTENDED_REGISTERS)

#define CONTEXT_XSTATE          (CONTEXT_i386 | 0x00000040L)

#define CONTEXT_EXCEPTION_ACTIVE    0x08000000L
#define CONTEXT_SERVICE_ACTIVE      0x10000000L
#define CONTEXT_EXCEPTION_REQUEST   0x40000000L
#define CONTEXT_EXCEPTION_REPORTING 0x80000000L

// begin_wx86

#endif // !defined(RC_INVOKED)

#if !defined(_NTDDK_)
typedef struct _FLOATING_SAVE_AREA {
    DWORD   ControlWord;
    DWORD   StatusWord;
    DWORD   TagWord;
    DWORD   ErrorOffset;
    DWORD   ErrorSelector;
    DWORD   DataOffset;
    DWORD   DataSelector;
    BYTE    RegisterArea[SIZE_OF_80387_REGISTERS];
    DWORD   Spare0;
} FLOATING_SAVE_AREA;

typedef FLOATING_SAVE_AREA *PFLOATING_SAVE_AREA;
#endif // !defined(_NTDDK_)

// end_ntddk
// begin_wdm begin_ntosp

#define MAXIMUM_SUPPORTED_EXTENSION     512

#if !defined(__midl) && !defined(MIDL_PASS)

C_ASSERT(sizeof(XSAVE_FORMAT) == MAXIMUM_SUPPORTED_EXTENSION);

#endif

// end_wdm end_ntosp

#if !defined(_NTDDK_)
// begin_ntddk

#include "pshpack4.h"

//
// Context Frame
//
//  This frame has a several purposes: 1) it is used as an argument to
//  NtContinue, 2) is is used to constuct a call frame for APC delivery,
//  and 3) it is used in the user level thread creation routines.
//
//  The layout of the record conforms to a standard call frame.
//

typedef struct DECLSPEC_NOINITALL _CONTEXT {

    //
    // The flags values within this flag control the contents of
    // a CONTEXT record.
    //
    // If the context record is used as an input parameter, then
    // for each portion of the context record controlled by a flag
    // whose value is set, it is assumed that that portion of the
    // context record contains valid context. If the context record
    // is being used to modify a threads context, then only that
    // portion of the threads context will be modified.
    //
    // If the context record is used as an IN OUT parameter to capture
    // the context of a thread, then only those portions of the thread's
    // context corresponding to set flags will be returned.
    //
    // The context record is never used as an OUT only parameter.
    //

    DWORD ContextFlags;

    //
    // This section is specified/returned if CONTEXT_DEBUG_REGISTERS is
    // set in ContextFlags.  Note that CONTEXT_DEBUG_REGISTERS is NOT
    // included in CONTEXT_FULL.
    //

    DWORD   Dr0;
    DWORD   Dr1;
    DWORD   Dr2;
    DWORD   Dr3;
    DWORD   Dr6;
    DWORD   Dr7;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_FLOATING_POINT.
    //

    FLOATING_SAVE_AREA FloatSave;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_SEGMENTS.
    //

    DWORD   SegGs;
    DWORD   SegFs;
    DWORD   SegEs;
    DWORD   SegDs;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_INTEGER.
    //

    DWORD   Edi;
    DWORD   Esi;
    DWORD   Ebx;
    DWORD   Edx;
    DWORD   Ecx;
    DWORD   Eax;

    //
    // This section is specified/returned if the
    // ContextFlags word contians the flag CONTEXT_CONTROL.
    //

    DWORD   Ebp;
    DWORD   Eip;
    DWORD   SegCs;              // MUST BE SANITIZED
    DWORD   EFlags;             // MUST BE SANITIZED
    DWORD   Esp;
    DWORD   SegSs;

    //
    // This section is specified/returned if the ContextFlags word
    // contains the flag CONTEXT_EXTENDED_REGISTERS.
    // The format and contexts are processor specific
    //

    BYTE    ExtendedRegisters[MAXIMUM_SUPPORTED_EXTENSION];

} CONTEXT;

typedef CONTEXT *PCONTEXT;

#include "poppack.h"
#endif // !defined(_NTDDK_)

// end_ntoshvp
// begin_ntminiport
#endif //_X86