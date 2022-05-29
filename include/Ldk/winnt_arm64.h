#pragma once

#if defined(_ARM64_) || defined(_CHPE_X86_ARM64_) || defined(_ARM64EC_)

//
//

#if !defined(_M_CEE_PURE)
#if !defined(RC_INVOKED) && !defined(MIDL_PASS)

#include <intrin.h>

#if defined(_M_ARM64) || defined(_M_ARM64EC)

#pragma intrinsic(__readx18byte)
#pragma intrinsic(__readx18word)
#pragma intrinsic(__readx18dword)
#pragma intrinsic(__readx18qword)

#pragma intrinsic(__writex18byte)
#pragma intrinsic(__writex18word)
#pragma intrinsic(__writex18dword)
#pragma intrinsic(__writex18qword)

#pragma intrinsic(__addx18byte)
#pragma intrinsic(__addx18word)
#pragma intrinsic(__addx18dword)
#pragma intrinsic(__addx18qword)

#pragma intrinsic(__incx18byte)
#pragma intrinsic(__incx18word)
#pragma intrinsic(__incx18dword)
#pragma intrinsic(__incx18qword)

//
// Define bit test intrinsics.
//

#define BitTest _bittest
#define BitTestAndComplement _bittestandcomplement
#define BitTestAndSet _bittestandset
#define BitTestAndReset _bittestandreset
#define InterlockedBitTestAndSet _interlockedbittestandset
#define InterlockedBitTestAndSetAcquire _interlockedbittestandset_acq
#define InterlockedBitTestAndSetRelease _interlockedbittestandset_rel
#define InterlockedBitTestAndSetNoFence _interlockedbittestandset_nf
#define InterlockedBitTestAndReset _interlockedbittestandreset
#define InterlockedBitTestAndResetAcquire _interlockedbittestandreset_acq
#define InterlockedBitTestAndResetRelease _interlockedbittestandreset_rel
#define InterlockedBitTestAndResetNoFence _interlockedbittestandreset_nf

//
// Temporary workaround for C++ bug: 64-bit bit test intrinsics are
// not honoring the full 64-bit wide index, so pre-process the index
// down to a qword base and a bit index 0-63 before calling through 
// to the true intrinsic.
//
#define __ARM64_COMPILER_BITTEST64_WORKAROUND

#if !defined(__ARM64_COMPILER_BITTEST64_WORKAROUND)
#define BitTest64 _bittest64
#define BitTestAndComplement64 _bittestandcomplement64
#define BitTestAndSet64 _bittestandset64
#define BitTestAndReset64 _bittestandreset64
#else
#undef BitTest64
#undef BitTestAndComplement64
#undef BitTestAndSet64
#undef BitTestAndReset64

#if !defined(_WDMDDK_)
FORCEINLINE
unsigned char
_BitTest64(__int64 const *Base, __int64 Index)
{
    return _bittest64(Base + (Index >> 6), Index & 63);
}

FORCEINLINE
unsigned char
_BitTestAndComplement64(__int64 *Base, __int64 Index)
{
    return _bittestandcomplement64(Base + (Index >> 6), Index & 63);
}

FORCEINLINE
unsigned char
_BitTestAndReset64(__int64 *Base, __int64 Index)
{
    return _bittestandreset64(Base + (Index >> 6), Index & 63);
}

FORCEINLINE
unsigned char
_BitTestAndSet64(__int64 *Base, __int64 Index)
{
    return _bittestandset64(Base + (Index >> 6), Index & 63);
}
#endif // !defined(_WDMDDK_)
#define BitTest64 _BitTest64
#define BitTestAndComplement64 _BitTestAndComplement64
#define BitTestAndSet64 _BitTestAndSet64
#define BitTestAndReset64 _BitTestAndReset64
#endif

//
// N.B. The above is not needed for the interlocked variants because they
//      are now generated as calls to glue code which already contain
//      fixes for this oversight.
//
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#define InterlockedBitTestAndSet64Acquire _interlockedbittestandset64_acq
#define InterlockedBitTestAndSet64Release _interlockedbittestandset64_rel
#define InterlockedBitTestAndSet64NoFence _interlockedbittestandset64_nf
#define InterlockedBitTestAndReset64 _interlockedbittestandreset64
#define InterlockedBitTestAndReset64Acquire _interlockedbittestandreset64_acq
#define InterlockedBitTestAndReset64Release _interlockedbittestandreset64_rel
#define InterlockedBitTestAndReset64NoFence _interlockedbittestandreset64_nf

#pragma intrinsic(_bittest)
#pragma intrinsic(_bittestandcomplement)
#pragma intrinsic(_bittestandset)
#pragma intrinsic(_bittestandreset)

#if !defined(ARM64X_INTRINSICS_FUNCTIONS)
#pragma intrinsic(_interlockedbittestandset)
#pragma intrinsic(_interlockedbittestandset_acq)
#pragma intrinsic(_interlockedbittestandset_rel)
#pragma intrinsic(_interlockedbittestandset_nf)
#pragma intrinsic(_interlockedbittestandreset)
#pragma intrinsic(_interlockedbittestandreset_acq)
#pragma intrinsic(_interlockedbittestandreset_rel)
#pragma intrinsic(_interlockedbittestandreset_nf)
#endif

#if !defined(__ARM64_COMPILER_BITTEST64_WORKAROUND)
#pragma intrinsic(_bittest64)
#pragma intrinsic(_bittestandcomplement64)
#pragma intrinsic(_bittestandset64)
#pragma intrinsic(_bittestandreset64)
#endif

#if !defined(ARM64X_INTRINSICS_FUNCTIONS)
#pragma intrinsic(_interlockedbittestandset64)
#pragma intrinsic(_interlockedbittestandset64_acq)
#pragma intrinsic(_interlockedbittestandset64_rel)
#pragma intrinsic(_interlockedbittestandset64_nf)
#pragma intrinsic(_interlockedbittestandreset64)
#pragma intrinsic(_interlockedbittestandreset64_acq)
#pragma intrinsic(_interlockedbittestandreset64_rel)
#pragma intrinsic(_interlockedbittestandreset64_nf)
#endif

//
// Define bit scan functions
//

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse
#define BitScanForward64 _BitScanForward64
#define BitScanReverse64 _BitScanReverse64

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

#if !defined(ARM64X_INTRINSICS_FUNCTIONS)
//
// Interlocked intrinsic functions.
//

#pragma intrinsic(_InterlockedAnd8)
#pragma intrinsic(_InterlockedOr8)
#pragma intrinsic(_InterlockedXor8)
#pragma intrinsic(_InterlockedExchangeAdd8)

#pragma intrinsic(_InterlockedAnd16)
#pragma intrinsic(_InterlockedOr16)
#pragma intrinsic(_InterlockedXor16)
#pragma intrinsic(_InterlockedIncrement16)
#pragma intrinsic(_InterlockedDecrement16)
#pragma intrinsic(_InterlockedCompareExchange16)

#pragma intrinsic(_InterlockedAnd)
#pragma intrinsic(_InterlockedOr)
#pragma intrinsic(_InterlockedXor)
#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)

#pragma intrinsic(_InterlockedAnd64)
#pragma intrinsic(_InterlockedOr64)
#pragma intrinsic(_InterlockedXor64)
#pragma intrinsic(_InterlockedIncrement64)
#pragma intrinsic(_InterlockedDecrement64)
#pragma intrinsic(_InterlockedExchange64)
#pragma intrinsic(_InterlockedCompareExchange64)

#pragma intrinsic(_InterlockedCompareExchange128)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)
#endif

#define InterlockedAnd8 _InterlockedAnd8
#define InterlockedOr8 _InterlockedOr8
#define InterlockedXor8 _InterlockedXor8
#define InterlockedExchangeAdd8 _InterlockedExchangeAdd8

#define InterlockedAnd16 _InterlockedAnd16
#define InterlockedOr16 _InterlockedOr16
#define InterlockedXor16 _InterlockedXor16
#define InterlockedIncrement16 _InterlockedIncrement16
#define InterlockedDecrement16 _InterlockedDecrement16
#define InterlockedCompareExchange16 _InterlockedCompareExchange16

#define InterlockedAnd _InterlockedAnd
#define InterlockedOr _InterlockedOr
#define InterlockedXor _InterlockedXor
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedAdd _InterlockedAdd
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange

#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedAndAffinity InterlockedAnd64
#define InterlockedOr64 _InterlockedOr64
#define InterlockedOrAffinity InterlockedOr64
#define InterlockedXor64 _InterlockedXor64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedAdd64 _InterlockedAdd64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64

#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

#if !defined(ARM64X_INTRINSICS_FUNCTIONS)
#pragma intrinsic(_InterlockedExchange8)
#pragma intrinsic(_InterlockedExchange16)
#endif
#define InterlockedExchange16 _InterlockedExchange16
#define InterlockedExchange8 _InterlockedExchange8

#if !defined(ARM64X_INTRINSICS_FUNCTIONS)
#pragma intrinsic(_InterlockedAnd8_acq)
#pragma intrinsic(_InterlockedAnd8_rel)
#pragma intrinsic(_InterlockedAnd8_nf)
#pragma intrinsic(_InterlockedOr8_acq)
#pragma intrinsic(_InterlockedOr8_rel)
#pragma intrinsic(_InterlockedOr8_nf)
#pragma intrinsic(_InterlockedXor8_acq)
#pragma intrinsic(_InterlockedXor8_rel)
#pragma intrinsic(_InterlockedXor8_nf)
#pragma intrinsic(_InterlockedExchange8_acq)
#pragma intrinsic(_InterlockedExchange8_nf)

#pragma intrinsic(_InterlockedAnd16_acq)
#pragma intrinsic(_InterlockedAnd16_rel)
#pragma intrinsic(_InterlockedAnd16_nf)
#pragma intrinsic(_InterlockedOr16_acq)
#pragma intrinsic(_InterlockedOr16_rel)
#pragma intrinsic(_InterlockedOr16_nf)
#pragma intrinsic(_InterlockedXor16_acq)
#pragma intrinsic(_InterlockedXor16_rel)
#pragma intrinsic(_InterlockedXor16_nf)
#pragma intrinsic(_InterlockedIncrement16_acq)
#pragma intrinsic(_InterlockedIncrement16_rel)
#pragma intrinsic(_InterlockedIncrement16_nf)
#pragma intrinsic(_InterlockedDecrement16_acq)
#pragma intrinsic(_InterlockedDecrement16_rel)
#pragma intrinsic(_InterlockedDecrement16_nf)
#pragma intrinsic(_InterlockedExchange16_acq)
#pragma intrinsic(_InterlockedExchange16_nf)
#pragma intrinsic(_InterlockedCompareExchange16_acq)
#pragma intrinsic(_InterlockedCompareExchange16_rel)
#pragma intrinsic(_InterlockedCompareExchange16_nf)

#pragma intrinsic(_InterlockedAnd_acq)
#pragma intrinsic(_InterlockedAnd_rel)
#pragma intrinsic(_InterlockedAnd_nf)
#pragma intrinsic(_InterlockedOr_acq)
#pragma intrinsic(_InterlockedOr_rel)
#pragma intrinsic(_InterlockedOr_nf)
#pragma intrinsic(_InterlockedXor_acq)
#pragma intrinsic(_InterlockedXor_rel)
#pragma intrinsic(_InterlockedXor_nf)
#pragma intrinsic(_InterlockedIncrement_acq)
#pragma intrinsic(_InterlockedIncrement_rel)
#pragma intrinsic(_InterlockedIncrement_nf)
#pragma intrinsic(_InterlockedDecrement_acq)
#pragma intrinsic(_InterlockedDecrement_rel)
#pragma intrinsic(_InterlockedDecrement_nf)
#pragma intrinsic(_InterlockedExchange_acq)
#pragma intrinsic(_InterlockedExchange_nf)
#pragma intrinsic(_InterlockedExchangeAdd_acq)
#pragma intrinsic(_InterlockedExchangeAdd_rel)
#pragma intrinsic(_InterlockedExchangeAdd_nf)
#pragma intrinsic(_InterlockedCompareExchange_rel)
#pragma intrinsic(_InterlockedCompareExchange_nf)

#pragma intrinsic(_InterlockedAnd64_acq)
#pragma intrinsic(_InterlockedAnd64_rel)
#pragma intrinsic(_InterlockedAnd64_nf)
#pragma intrinsic(_InterlockedOr64_acq)
#pragma intrinsic(_InterlockedOr64_rel)
#pragma intrinsic(_InterlockedOr64_nf)
#pragma intrinsic(_InterlockedXor64_acq)
#pragma intrinsic(_InterlockedXor64_rel)
#pragma intrinsic(_InterlockedXor64_nf)
#pragma intrinsic(_InterlockedIncrement64_acq)
#pragma intrinsic(_InterlockedIncrement64_rel)
#pragma intrinsic(_InterlockedIncrement64_nf)
#pragma intrinsic(_InterlockedDecrement64_acq)
#pragma intrinsic(_InterlockedDecrement64_rel)
#pragma intrinsic(_InterlockedDecrement64_nf)
#pragma intrinsic(_InterlockedExchange64_acq)
#pragma intrinsic(_InterlockedExchange64_nf)
#pragma intrinsic(_InterlockedCompareExchange64_acq)
#pragma intrinsic(_InterlockedCompareExchange64_rel)
#pragma intrinsic(_InterlockedCompareExchange64_nf)

#pragma intrinsic(_InterlockedExchangePointer_acq)
#pragma intrinsic(_InterlockedExchangePointer_nf)
#pragma intrinsic(_InterlockedCompareExchangePointer_acq)
#pragma intrinsic(_InterlockedCompareExchangePointer_rel)
#pragma intrinsic(_InterlockedCompareExchangePointer_nf)
#endif

#define InterlockedAndAcquire8 _InterlockedAnd8_acq
#define InterlockedAndRelease8 _InterlockedAnd8_rel
#define InterlockedAndNoFence8 _InterlockedAnd8_nf
#define InterlockedOrAcquire8 _InterlockedOr8_acq
#define InterlockedOrRelease8 _InterlockedOr8_rel
#define InterlockedOrNoFence8 _InterlockedOr8_nf
#define InterlockedXorAcquire8 _InterlockedXor8_acq
#define InterlockedXorRelease8 _InterlockedXor8_rel
#define InterlockedXorNoFence8 _InterlockedXor8_nf
#define InterlockedExchangeNoFence8 _InterlockedExchange8_nf
#define InterlockedExchangeAcquire8 _InterlockedExchange8_acq

#define InterlockedAndAcquire16 _InterlockedAnd16_acq
#define InterlockedAndRelease16 _InterlockedAnd16_rel
#define InterlockedAndNoFence16 _InterlockedAnd16_nf
#define InterlockedOrAcquire16 _InterlockedOr16_acq
#define InterlockedOrRelease16 _InterlockedOr16_rel
#define InterlockedOrNoFence16 _InterlockedOr16_nf
#define InterlockedXorAcquire16 _InterlockedXor16_acq
#define InterlockedXorRelease16 _InterlockedXor16_rel
#define InterlockedXorNoFence16 _InterlockedXor16_nf
#define InterlockedIncrementAcquire16 _InterlockedIncrement16_acq
#define InterlockedIncrementRelease16 _InterlockedIncrement16_rel
#define InterlockedIncrementNoFence16 _InterlockedIncrement16_nf
#define InterlockedDecrementAcquire16 _InterlockedDecrement16_acq
#define InterlockedDecrementRelease16 _InterlockedDecrement16_rel
#define InterlockedDecrementNoFence16 _InterlockedDecrement16_nf
#define InterlockedExchangeAcquire16 _InterlockedExchange16_acq
#define InterlockedExchangeNoFence16 _InterlockedExchange16_nf
#define InterlockedCompareExchangeAcquire16 _InterlockedCompareExchange16_acq
#define InterlockedCompareExchangeRelease16 _InterlockedCompareExchange16_rel
#define InterlockedCompareExchangeNoFence16 _InterlockedCompareExchange16_nf

#define InterlockedAndAcquire _InterlockedAnd_acq
#define InterlockedAndRelease _InterlockedAnd_rel
#define InterlockedAndNoFence _InterlockedAnd_nf
#define InterlockedOrAcquire _InterlockedOr_acq
#define InterlockedOrRelease _InterlockedOr_rel
#define InterlockedOrNoFence _InterlockedOr_nf
#define InterlockedXorAcquire _InterlockedXor_acq
#define InterlockedXorRelease _InterlockedXor_rel
#define InterlockedXorNoFence _InterlockedXor_nf
#define InterlockedIncrementAcquire _InterlockedIncrement_acq
#define InterlockedIncrementRelease _InterlockedIncrement_rel
#define InterlockedIncrementNoFence _InterlockedIncrement_nf
#define InterlockedDecrementAcquire _InterlockedDecrement_acq
#define InterlockedDecrementRelease _InterlockedDecrement_rel
#define InterlockedDecrementNoFence _InterlockedDecrement_nf
#define InterlockedAddAcquire _InterlockedAdd_acq
#define InterlockedAddRelease _InterlockedAdd_rel
#define InterlockedAddNoFence _InterlockedAdd_nf
#define InterlockedExchangeAcquire _InterlockedExchange_acq
#define InterlockedExchangeNoFence _InterlockedExchange_nf
#define InterlockedExchangeAddAcquire _InterlockedExchangeAdd_acq
#define InterlockedExchangeAddRelease _InterlockedExchangeAdd_rel
#define InterlockedExchangeAddNoFence _InterlockedExchangeAdd_nf
#define InterlockedCompareExchangeAcquire _InterlockedCompareExchange_acq
#define InterlockedCompareExchangeRelease _InterlockedCompareExchange_rel
#define InterlockedCompareExchangeNoFence _InterlockedCompareExchange_nf

#define InterlockedAndAcquire64 _InterlockedAnd64_acq
#define InterlockedAndRelease64 _InterlockedAnd64_rel
#define InterlockedAndNoFence64 _InterlockedAnd64_nf
#define InterlockedOrAcquire64 _InterlockedOr64_acq
#define InterlockedOrRelease64 _InterlockedOr64_rel
#define InterlockedOrNoFence64 _InterlockedOr64_nf
#define InterlockedXorAcquire64 _InterlockedXor64_acq
#define InterlockedXorRelease64 _InterlockedXor64_rel
#define InterlockedXorNoFence64 _InterlockedXor64_nf
#define InterlockedIncrementAcquire64 _InterlockedIncrement64_acq
#define InterlockedIncrementRelease64 _InterlockedIncrement64_rel
#define InterlockedIncrementNoFence64 _InterlockedIncrement64_nf
#define InterlockedDecrementAcquire64 _InterlockedDecrement64_acq
#define InterlockedDecrementRelease64 _InterlockedDecrement64_rel
#define InterlockedDecrementNoFence64 _InterlockedDecrement64_nf
#define InterlockedAddAcquire64 _InterlockedAdd64_acq
#define InterlockedAddRelease64 _InterlockedAdd64_rel
#define InterlockedAddNoFence64 _InterlockedAdd64_nf
#define InterlockedExchangeAcquire64 _InterlockedExchange64_acq
#define InterlockedExchangeNoFence64 _InterlockedExchange64_nf
#define InterlockedExchangeAddAcquire64 _InterlockedExchangeAdd64_acq
#define InterlockedExchangeAddRelease64 _InterlockedExchangeAdd64_rel
#define InterlockedExchangeAddNoFence64 _InterlockedExchangeAdd64_nf
#define InterlockedCompareExchangeAcquire64 _InterlockedCompareExchange64_acq
#define InterlockedCompareExchangeRelease64 _InterlockedCompareExchange64_rel
#define InterlockedCompareExchangeNoFence64 _InterlockedCompareExchange64_nf
#define InterlockedCompareExchange128 _InterlockedCompareExchange128

// AMD64_WORKITEM : these are redundant but necessary for AMD64 compatibility
#define InterlockedAnd64Acquire _InterlockedAnd64_acq
#define InterlockedAnd64Release _InterlockedAnd64_rel
#define InterlockedAnd64NoFence _InterlockedAnd64_nf
#define InterlockedOr64Acquire _InterlockedOr64_acq
#define InterlockedOr64Release _InterlockedOr64_rel
#define InterlockedOr64NoFence _InterlockedOr64_nf
#define InterlockedXor64Acquire _InterlockedXor64_acq
#define InterlockedXor64Release _InterlockedXor64_rel
#define InterlockedXor64NoFence _InterlockedXor64_nf

#define InterlockedExchangePointerAcquire _InterlockedExchangePointer_acq
#define InterlockedExchangePointerNoFence _InterlockedExchangePointer_nf
#define InterlockedCompareExchangePointerAcquire _InterlockedCompareExchangePointer_acq
#define InterlockedCompareExchangePointerRelease _InterlockedCompareExchangePointer_rel
#define InterlockedCompareExchangePointerNoFence _InterlockedCompareExchangePointer_nf

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd64((LONG64 *)a, b)
#define InterlockedExchangeAddSizeTAcquire(a, b) InterlockedExchangeAddAcquire64((LONG64 *)a, b)
#define InterlockedExchangeAddSizeTNoFence(a, b) InterlockedExchangeAddNoFence64((LONG64 *)a, b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement64((LONG64 *)a)
#define InterlockedIncrementSizeTNoFence(a) InterlockedIncrementNoFence64((LONG64 *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement64((LONG64 *)a)
#define InterlockedDecrementSizeTNoFence(a) InterlockedDecrementNoFence64((LONG64 *)a)

#endif // defined(_M_ARM64) || defined(_M_ARM64EC)

#if defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)

#pragma intrinsic(__getReg)
#pragma intrinsic(__getCallerReg)
#pragma intrinsic(__getRegFp)
#pragma intrinsic(__getCallerRegFp)

#pragma intrinsic(__setReg)
#pragma intrinsic(__setCallerReg)
#pragma intrinsic(__setRegFp)
#pragma intrinsic(__setCallerRegFp)

//
// Memory barriers and prefetch intrinsics.
//

#pragma intrinsic(__yield)
#pragma intrinsic(__prefetch)
#pragma intrinsic(__prefetch2)

#define ARM64_PREFETCH_PLD  (0 << 3)
#define ARM64_PREFETCH_PLI  (1 << 3)
#define ARM64_PREFETCH_PST  (2 << 3)

#define ARM64_PREFETCH_L1   (0 << 1)
#define ARM64_PREFETCH_L2   (1 << 1)
#define ARM64_PREFETCH_L3   (2 << 1)

#define ARM64_PREFETCH_KEEP (0 << 0)
#define ARM64_PREFETCH_STRM (1 << 0)

#define ARM64_PREFETCH(a,b,c) (ARM64_PREFETCH_##a | ARM64_PREFETCH_##b | ARM64_PREFETCH_##c)

#pragma intrinsic(__dmb)
#pragma intrinsic(__dsb)
#pragma intrinsic(__isb)

#pragma intrinsic(_ReadWriteBarrier)
#pragma intrinsic(_WriteBarrier)

#define MemoryBarrier()             __dmb(_ARM64_BARRIER_SY)
#define PreFetchCacheLine(l,a)      __prefetch2((const void *) (a), ARM64_PREFETCH(PLD, L1, KEEP))
#define PrefetchForWrite(p)         __prefetch2((const void *) (p), ARM64_PREFETCH(PST, L1, KEEP))
#define ReadForWriteAccess(p)       (__prefetch2((const void *) (p), ARM64_PREFETCH(PST, L1, KEEP)), *(p))

#define _DataSynchronizationBarrier()        __dsb(_ARM64_BARRIER_SY)
#define _InstructionSynchronizationBarrier() __isb(_ARM64_BARRIER_SY)

#if !defined(_WDMDDK_)
FORCEINLINE
VOID
YieldProcessor (
    VOID
    )
{
    __dmb(_ARM64_BARRIER_ISHST);
    __yield();
}
#endif // !defined(_WDMDDK_)

//
// Define accessors for volatile loads and stores.
//

#pragma intrinsic(__iso_volatile_load8)
#pragma intrinsic(__iso_volatile_load16)
#pragma intrinsic(__iso_volatile_load32)
#pragma intrinsic(__iso_volatile_load64)
#pragma intrinsic(__iso_volatile_store8)
#pragma intrinsic(__iso_volatile_store16)
#pragma intrinsic(__iso_volatile_store32)
#pragma intrinsic(__iso_volatile_store64)

//
//

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)
#if !defined(_WDMDDK_)
//
//

FORCEINLINE
CHAR
ReadAcquire8 (
    _In_ _Interlocked_operand_ CHAR const volatile *Source
    )

{

    CHAR Value;

    Value = __iso_volatile_load8(Source);
    __dmb(_ARM64_BARRIER_ISH);
    return Value;
}

FORCEINLINE
CHAR
ReadNoFence8 (
    _In_ _Interlocked_operand_ CHAR const volatile *Source
    )

{

    CHAR Value;

    Value = __iso_volatile_load8(Source);
    return Value;
}

FORCEINLINE
VOID
WriteRelease8 (
    _Out_ _Interlocked_operand_ CHAR volatile *Destination,
    _In_ CHAR Value
    )

{

    __dmb(_ARM64_BARRIER_ISH);
    __iso_volatile_store8(Destination, Value);
    return;
}

FORCEINLINE
VOID
WriteNoFence8 (
    _Out_ _Interlocked_operand_ CHAR volatile *Destination,
    _In_ CHAR Value
    )

{

    __iso_volatile_store8(Destination, Value);
    return;
}

FORCEINLINE
SHORT
ReadAcquire16 (
    _In_ _Interlocked_operand_ SHORT const volatile *Source
    )

{

    SHORT Value;

    Value = __iso_volatile_load16(Source);
    __dmb(_ARM64_BARRIER_ISH);
    return Value;
}

FORCEINLINE
SHORT
ReadNoFence16 (
    _In_ _Interlocked_operand_ SHORT const volatile *Source
    )

{

    SHORT Value;

    Value = __iso_volatile_load16(Source);
    return Value;
}

FORCEINLINE
VOID
WriteRelease16 (
    _Out_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT Value
    )

{

    __dmb(_ARM64_BARRIER_ISH);
    __iso_volatile_store16(Destination, Value);
    return;
}

FORCEINLINE
VOID
WriteNoFence16 (
    _Out_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT Value
    )

{

    __iso_volatile_store16(Destination, Value);
    return;
}

FORCEINLINE
LONG
ReadAcquire (
    _In_ _Interlocked_operand_ LONG const volatile *Source
    )

{

    LONG Value;

    Value = __iso_volatile_load32((int *)Source);
    __dmb(_ARM64_BARRIER_ISH);
    return Value;
}

FORCEINLINE
LONG
ReadNoFence (
    _In_ _Interlocked_operand_ LONG const volatile *Source
    )

{

    LONG Value;

    Value = __iso_volatile_load32((int *)Source);
    return Value;
}

FORCEINLINE
VOID
WriteRelease (
    _Out_ _Interlocked_operand_ LONG volatile *Destination,
    _In_ LONG Value
    )

{

    __dmb(_ARM64_BARRIER_ISH);
    __iso_volatile_store32((int *)Destination, Value);
    return;
}

FORCEINLINE
VOID
WriteNoFence (
    _Out_ _Interlocked_operand_ LONG volatile *Destination,
    _In_ LONG Value
    )

{

    __iso_volatile_store32((int *)Destination, Value);
    return;
}

FORCEINLINE
LONG64
ReadAcquire64 (
    _In_ _Interlocked_operand_ LONG64 const volatile *Source
    )

{

    LONG64 Value;

    Value = __iso_volatile_load64(Source);
    __dmb(_ARM64_BARRIER_ISH);
    return Value;
}

FORCEINLINE
LONG64
ReadNoFence64 (
    _In_ _Interlocked_operand_ LONG64 const volatile *Source
    )

{

    LONG64 Value;

    Value = __iso_volatile_load64(Source);
    return Value;
}

FORCEINLINE
VOID
WriteRelease64 (
    _Out_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    )

{

    __dmb(_ARM64_BARRIER_ISH);
    __iso_volatile_store64(Destination, Value);
    return;
}

FORCEINLINE
VOID
WriteNoFence64 (
    _Out_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    )

{

    __iso_volatile_store64(Destination, Value);
    return;
}
#endif // !defined(_WDMDDK_)

//
//

#endif // WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

//
//

//
// Define coprocessor access intrinsics.  Coprocessor 15 contains
// registers for the MMU, cache, TLB, feature bits, core
// identification and performance counters.
//

// op0=2/3 encodings, use with _Read/WriteStatusReg
#define ARM64_SYSREG(op0, op1, crn, crm, op2) \
        ( ((op0 & 1) << 14) | \
          ((op1 & 7) << 11) | \
          ((crn & 15) << 7) | \
          ((crm & 15) << 3) | \
          ((op2 & 7) << 0) )

// op0=1 encodings, use with __sys
#define ARM64_SYSINSTR(op0, op1, crn, crm, op2) \
        ( ((op1 & 7) << 11) | \
          ((crn & 15) << 7) | \
          ((crm & 15) << 3) | \
          ((op2 & 7) << 0) )

#define ARM64_SYSREG_OP1(_Reg_) (((_Reg_) >> 11) & 7)
#define ARM64_SYSREG_CRN(_Reg_) (((_Reg_) >> 7) & 15)
#define ARM64_SYSREG_CRM(_Reg_) (((_Reg_) >> 3) & 15)
#define ARM64_SYSREG_OP2(_Reg_) ((_Reg_) & 7)

#define ARM64_CNTVCT            ARM64_SYSREG(3,3,14, 0,2)  // Generic Timer counter register
#define ARM64_PMCCNTR_EL0       ARM64_SYSREG(3,3, 9,13,0)  // Cycle Count Register [CP15_PMCCNTR]
#define ARM64_PMSELR_EL0        ARM64_SYSREG(3,3, 9,12,5)  // Event Counter Selection Register [CP15_PMSELR]
#define ARM64_PMXEVCNTR_EL0     ARM64_SYSREG(3,3, 9,13,2)  // Event Count Register [CP15_PMXEVCNTR]
#define ARM64_PMXEVCNTRn_EL0(n) ARM64_SYSREG(3,3,14, 8+((n)/8), (n)%8)    // Direct Event Count Register [n/a]
#define ARM64_TPIDR_EL0         ARM64_SYSREG(3,3,13, 0,2)  // Thread ID Register, User Read/Write [CP15_TPIDRURW]
#define ARM64_TPIDRRO_EL0       ARM64_SYSREG(3,3,13, 0,3)  // Thread ID Register, User Read Only [CP15_TPIDRURO]
#define ARM64_TPIDR_EL1         ARM64_SYSREG(3,0,13, 0,4)  // Thread ID Register, Privileged Only [CP15_TPIDRPRW]



#pragma intrinsic(_WriteStatusReg)
#pragma intrinsic(_ReadStatusReg)

//
// PreFetchCacheLine level defines.
//

#define PF_TEMPORAL_LEVEL_1         0
#define PF_TEMPORAL_LEVEL_2         1
#define PF_TEMPORAL_LEVEL_3         2
#define PF_NON_TEMPORAL_LEVEL_ALL   3

#if defined(_M_HYBRID_X86_ARM64)

extern DWORD64 (*_os_wowa64_rdtsc) (VOID);

#endif // defined(_M_HYBRID_X86_ARM64)

//
// Define function to read the value of the time stamp counter.
//

#if defined(_M_HYBRID_X86_ARM64)

DECLSPEC_GUARDNOCF

#endif // defined(_M_HYBRID_X86_ARM64)



#if !defined(_WDMDDK_)
FORCEINLINE
DWORD64
ReadTimeStampCounter(
    VOID
    )
{

#if defined(_M_HYBRID_X86_ARM64)

    //
    // Call into the emulator to return the same value as the x86 RDTSC
    // instruction.
    //

    return (*_os_wowa64_rdtsc)();

#else // defined(_M_HYBRID_X86_ARM64)

    return (DWORD64)_ReadStatusReg(ARM64_PMCCNTR_EL0);

#endif // defined(_M_HYBRID_X86_ARM64)

}

FORCEINLINE
DWORD64
ReadPMC (
    _In_ DWORD Counter
    )
{
    // ARM64_WORKITEM: These can be directly accessed, but
    // given our usage, it that any benefit? We need to know
    // the register index at compile time, though atomicity
    // benefits would still be good if needed, even if we
    // went with a big switch statement.
    _WriteStatusReg(ARM64_PMSELR_EL0, Counter);
    return (DWORD64)_ReadStatusReg(ARM64_PMXEVCNTR_EL0);
}
#endif // !defined(_WDMDDK_)


//
// Define functions to capture the high 64-bits of a 128-bit multiply.
//

#define MultiplyHigh __mulh
#define UnsignedMultiplyHigh __umulh

#pragma intrinsic(__mulh)
#pragma intrinsic(__umulh)

#endif // defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)


//
// Define population count intrinsic.
//

#if !defined(_WDMDDK_)
#if !defined(PopulationCount64)

__forceinline
DWORD64
PopulationCount64 (
    _In_ DWORD64 operand
    )
{
    // log(n) population count

    DWORD64 highBits = (operand & 0xAAAAAAAAAAAAAAAA) >> 1;
    DWORD64 lowBits = operand & 0x5555555555555555;
    DWORD64 bitSum = highBits + lowBits;

    highBits = (bitSum & 0xCCCCCCCCCCCCCCCC) >> 2;
    lowBits = bitSum & 0x3333333333333333;
    bitSum = highBits + lowBits;

    highBits = (bitSum & 0xF0F0F0F0F0F0F0F0) >> 4;
    lowBits = bitSum & 0x0F0F0F0F0F0F0F0F;
    bitSum = highBits + lowBits;

    highBits = (bitSum & 0xFF00FF00FF00FF00) >> 8;
    lowBits = bitSum & 0x00FF00FF00FF00FF;
    bitSum = highBits + lowBits;

    highBits = (bitSum & 0xFFFF0000FFFF0000) >> 16;
    lowBits = bitSum & 0x0000FFFF0000FFFF;
    bitSum = highBits + lowBits;

    highBits = (bitSum & 0xFFFFFFFF00000000) >> 32;
    lowBits = bitSum & 0x00000000FFFFFFFF;
    bitSum = highBits + lowBits;

    return bitSum;
}

#endif // !defined(PopulationCount64)

//
// Define functions to perform 128-bit shifts
//

#if !defined(ShiftLeft128)

#define __shiftleft128   ShiftLeft128
#define __shiftright128  ShiftRight128

__forceinline
DWORD64
ShiftLeft128 (
    _In_ DWORD64 LowPart,
    _In_ DWORD64 HighPart,
    _In_ BYTE  Shift
    )
{
    Shift &= 63;

    if (Shift == 0) {
        return HighPart;
    }

    return (HighPart << Shift) | (LowPart >> (64 - Shift));
}

__forceinline
DWORD64
ShiftRight128 (
    _In_ DWORD64 LowPart,
    _In_ DWORD64 HighPart,
    _In_ BYTE  Shift
    )
{
    Shift &= 63;

    if (Shift == 0) {
        return LowPart;
    }

    return (LowPart >> Shift) | (HighPart << (64 - Shift));
}

#endif // !defined(ShiftLeft128)

//
// Define functions to perform 128-bit multiplies.
//

// Most ARM64/AArch64 intrinsics available in MSVC are not currently available in other compilers.
// clang-cl defines _M_ARM64, so we need a better escape hatch to avoid using these intrinsics,
// as well as allow us to re-enable them in the event clang-cl starts supporting the msvc intrinsics.
#if !defined(ARM64_MULT_INTRINSICS_SUPPORTED)
    #if defined(_MSC_VER) && !defined(__clang__)
    #define ARM64_MULT_INTRINSICS_SUPPORTED 1
    #else
    #define ARM64_MULT_INTRINSICS_SUPPORTED 0
    #endif
#endif

#if !defined(UnsignedMultiply128)

__forceinline
DWORD64
UnsignedMultiply128 (
    _In_ DWORD64 Multiplier,
    _In_ DWORD64 Multiplicand,
    _Out_ DWORD64 *HighProduct
    )

/*++

Routine Description:

    Calculates the 128 bit product of two 64 bit integers.

Arguments:

    Multiplier -

    Multiplicand -

    HighProduct - Receives the high 64 bits of the product.

Return Value:

    Low 64 bits of the product.

--*/

{

#if (defined(_M_ARM64) || defined(_M_HYBRID_X86_ARM64) || defined(_M_ARM64EC)) && ARM64_MULT_INTRINSICS_SUPPORTED

    *HighProduct = UnsignedMultiplyHigh(Multiplier, Multiplicand);
    return Multiplier * Multiplicand;

#else

    DWORD64 HiMultiplier = Multiplier >> 32;
    DWORD64 LoMultiplier = Multiplier & 0xFFFFFFFF;
    DWORD64 HiMultiplicand = Multiplicand >> 32;
    DWORD64 LoMultiplicand = Multiplicand & 0xFFFFFFFF;
    DWORD64 CrossTerm1 = (HiMultiplier * LoMultiplicand);
    DWORD64 CrossTerm2 = (LoMultiplier * HiMultiplicand);

    DWORD64 ResultLo = (LoMultiplier * LoMultiplicand);
    DWORD64 ResultHi = (HiMultiplier * HiMultiplicand);

    // Add the cross-terms and propagate carries across all 128 bits.

    ResultLo += (CrossTerm1 << 32);
    ResultHi += (CrossTerm1 >> 32) + (ResultLo < (CrossTerm1 << 32));

    ResultLo += (CrossTerm2 << 32);
    ResultHi += (CrossTerm2 >> 32) + (ResultLo < (CrossTerm2 << 32));

    *HighProduct = ResultHi;

    return ResultLo;

#endif

}

__forceinline
LONG64
Multiply128 (
    _In_ LONG64 Multiplier,
    _In_ LONG64 Multiplicand,
    _Out_ LONG64 *HighProduct
    )
{
    LONG64 Result = (LONG64)UnsignedMultiply128((DWORD64)Multiplier, (DWORD64)Multiplicand, (DWORD64 *)HighProduct);

    *HighProduct -= (Multiplier >> 63) * Multiplicand;
    *HighProduct -= Multiplier * (Multiplicand >> 63);

    return Result;
}

#endif // !defined(UnsignedMultiply128)

#if !defined(_M_ARM64EC)

__forceinline
LONG64
MultiplyExtract128 (
    _In_ LONG64 Multiplier,
    _In_ LONG64 Multiplicand,
    _In_ BYTE  Shift
    )

{

    LONG64 extractedProduct;
    LONG64 highProduct;
    LONG64 lowProduct;
    BOOLEAN negate;
    DWORD64 uhighProduct;
    DWORD64 ulowProduct;

    lowProduct = Multiply128(Multiplier, Multiplicand, &highProduct);
    negate = FALSE;
    uhighProduct = (DWORD64)highProduct;
    ulowProduct = (DWORD64)lowProduct;
    if (highProduct < 0) {
        negate = TRUE;
        uhighProduct = (DWORD64)(-highProduct);
        ulowProduct = (DWORD64)(-lowProduct);
        if (ulowProduct != 0) {
            uhighProduct -= 1;
        }
    }

    extractedProduct = (LONG64)ShiftRight128(ulowProduct, uhighProduct, Shift);
    if (negate != FALSE) {
        extractedProduct = -extractedProduct;
    }

    return extractedProduct;
}

__forceinline
DWORD64
UnsignedMultiplyExtract128 (
    _In_ DWORD64 Multiplier,
    _In_ DWORD64 Multiplicand,
    _In_ BYTE  Shift
    )

{

    DWORD64 extractedProduct;
    DWORD64 highProduct;
    DWORD64 lowProduct;

    lowProduct = UnsignedMultiply128(Multiplier, Multiplicand, &highProduct);
    extractedProduct = ShiftRight128(lowProduct, highProduct, Shift);
    return extractedProduct;
}

#endif // !defined(_M_ARM64EC)

#endif // !defined(_WDMDDK_)


#if defined(_M_ARM64EC)

unsigned int
_mm_getcsr (
    VOID
    );

VOID
_mm_setcsr (
    _In_ unsigned int MxCsr
    );

#endif // defined(_M_ARM64EC)

#endif // !defined(RC_INVOKED) && !defined(MIDL_PASS)

#else // defined(_M_CEE_PURE)

#include <intrin.h>

#endif // !defined(_M_CEE_PURE)

#if defined(_M_CEE_PURE)
FORCEINLINE
VOID
YieldProcessor (
    VOID
    )
{
}
#endif // defined(_M_CEE_PURE)

//
//

//
// The following values specify the type of access in the first parameter
// of the exception record whan the exception code specifies an access
// violation.
//

#define EXCEPTION_READ_FAULT 0          // exception caused by a read
#define EXCEPTION_WRITE_FAULT 1         // exception caused by a write
#define EXCEPTION_EXECUTE_FAULT 8       // exception caused by an instruction fetch

//
// Define initial Cpsr/Fpscr value
//

#define INITIAL_CPSR 0x10
#define INITIAL_FPSCR 0

//
//

#endif // defined(_ARM64_) || defined(_CHPE_X86_ARM64_) || defined(_ARM64EC_)







//
// Select platform-specific definitions
//

#if defined(_ARM64_) || defined(_CHPE_X86_ARM64_)

typedef SCOPE_TABLE_ARM64 SCOPE_TABLE, *PSCOPE_TABLE;

#endif //  defined(_ARM64_) || defined(_CHPE_X86_ARM64_)

typedef struct _IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY ARM64_RUNTIME_FUNCTION, *PARM64_RUNTIME_FUNCTION;

#if defined(_ARM64_)

typedef struct _IMAGE_ARM64_RUNTIME_FUNCTION_ENTRY RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;

#endif // defined(_ARM64_)

//
// Define unwind information flags.
//

#define UNW_FLAG_NHANDLER       0x0             /* any handler */
#define UNW_FLAG_EHANDLER       0x1             /* filter handler */
#define UNW_FLAG_UHANDLER       0x2             /* unwind handler */

#if defined(_ARM64_) || defined(_CHPE_X86_ARM64_)

#pragma push_macro("_DISPATCHER_CONTEXT_ARM64")
#undef _DISPATCHER_CONTEXT_ARM64
#define _DISPATCHER_CONTEXT_ARM64 _DISPATCHER_CONTEXT

#endif // defined(_ARM64_) || defined(_CHPE_X86_ARM64_)

//
// Define exception dispatch context structure.
//

#define NONVOL_INT_NUMREG_ARM64 (11)
#define NONVOL_FP_NUMREG_ARM64  (8)

#define NONVOL_INT_SIZE_ARM64 (NONVOL_INT_NUMREG_ARM64 * sizeof(DWORD64))
#define NONVOL_FP_SIZE_ARM64  (NONVOL_FP_NUMREG_ARM64 * sizeof(double))

typedef union _DISPATCHER_CONTEXT_NONVOLREG_ARM64 {
    BYTE  Buffer[NONVOL_INT_SIZE_ARM64 + NONVOL_FP_SIZE_ARM64];

    struct {
        DWORD64 GpNvRegs[NONVOL_INT_NUMREG_ARM64];      // [x19, x29(Fp)]
        double FpNvRegs [NONVOL_FP_NUMREG_ARM64];       // [V8d0, v15d0]
    } DUMMYSTRUCTNAME;
} DISPATCHER_CONTEXT_NONVOLREG_ARM64;

#if !defined(RC_INVOKED) && !defined(MIDL_PASS)

C_ASSERT(sizeof(DISPATCHER_CONTEXT_NONVOLREG_ARM64) == (NONVOL_INT_SIZE_ARM64 + NONVOL_FP_SIZE_ARM64));

#endif // !defined(RC_INVOKED) && !defined(MIDL_PASS)

typedef struct _DISPATCHER_CONTEXT_ARM64 {
    ULONG_PTR ControlPc;
    ULONG_PTR ImageBase;
    PARM64_RUNTIME_FUNCTION FunctionEntry;
    ULONG_PTR EstablisherFrame;
    ULONG_PTR TargetPc;
    PARM64_NT_CONTEXT ContextRecord;
    PEXCEPTION_ROUTINE LanguageHandler;
    PVOID HandlerData;
    struct _UNWIND_HISTORY_TABLE *HistoryTable;
    DWORD ScopeIndex;
    BOOLEAN ControlPcIsUnwound;
    PBYTE  NonVolatileRegisters;
} DISPATCHER_CONTEXT_ARM64, *PDISPATCHER_CONTEXT_ARM64;

#if defined(_ARM64_) || defined(_CHPE_X86_ARM64_)

typedef DISPATCHER_CONTEXT_ARM64 DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

#undef _DISPATCHER_CONTEXT_ARM64
#pragma pop_macro("_DISPATCHER_CONTEXT_ARM64")

#endif // defined(_ARM64_) || defined(_CHPE_X86_ARM64_)


#if defined(_ARM64_) || defined(_CHPE_X86_ARM64_)

//
// Define exception filter and termination handler function types.
// N.B. These functions use a custom calling convention.
//

struct _EXCEPTION_POINTERS;
typedef
LONG
(*PEXCEPTION_FILTER) (
    struct _EXCEPTION_POINTERS *ExceptionPointers,
    DWORD64 EstablisherFrame
    );

typedef
VOID
(*PTERMINATION_HANDLER) (
    BOOLEAN AbnormalTermination,
    DWORD64 EstablisherFrame
    );

#endif // defined(_ARM64_) || defined(_CHPE_X86_ARM64_)

#if defined(_ARM64_)

//
// Define dynamic function table entry.
//

typedef
_Function_class_(GET_RUNTIME_FUNCTION_CALLBACK)
PARM64_RUNTIME_FUNCTION
GET_RUNTIME_FUNCTION_CALLBACK (
    _In_ DWORD64 ControlPc,
    _In_opt_ PVOID Context
    );
typedef GET_RUNTIME_FUNCTION_CALLBACK *PGET_RUNTIME_FUNCTION_CALLBACK;

typedef
_Function_class_(OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK)
DWORD   
OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK (
    _In_ HANDLE Process,
    _In_ PVOID TableAddress,
    _Out_ PDWORD Entries,
    _Out_ PARM64_RUNTIME_FUNCTION* Functions
    );
typedef OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK *POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK;

#define OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK_EXPORT_NAME \
    "OutOfProcessFunctionTableCallback"

#endif // defined(_ARM64_)

//
// Nonvolatile context pointer record.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS_ARM64 {

    PDWORD64 X19;
    PDWORD64 X20;
    PDWORD64 X21;
    PDWORD64 X22;
    PDWORD64 X23;
    PDWORD64 X24;
    PDWORD64 X25;
    PDWORD64 X26;
    PDWORD64 X27;
    PDWORD64 X28;
    PDWORD64 Fp;
    PDWORD64 Lr;

    PDWORD64 D8;
    PDWORD64 D9;
    PDWORD64 D10;
    PDWORD64 D11;
    PDWORD64 D12;
    PDWORD64 D13;
    PDWORD64 D14;
    PDWORD64 D15;

} KNONVOLATILE_CONTEXT_POINTERS_ARM64, *PKNONVOLATILE_CONTEXT_POINTERS_ARM64;

#if defined(_ARM64_)

typedef KNONVOLATILE_CONTEXT_POINTERS_ARM64 KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

#endif // defined(_ARM64_)