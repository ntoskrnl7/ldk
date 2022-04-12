#pragma once

#include <ntimage.h>

EXTERN_C_START

#pragma warning(disable:4201 4214)

#define NtCurrentTeb			LdkCurrentTeb
#define NtCurrentPeb			LdkCurrentPeb



#define DLL_PROCESS_ATTACH   1    
#define DLL_THREAD_ATTACH    2    
#define DLL_THREAD_DETACH    3    
#define DLL_PROCESS_DETACH   0



#define FLS_MAXIMUM_AVAILABLE 128
#define TLS_MINIMUM_AVAILABLE 64

typedef
VOID
(NTAPI *PFLS_CALLBACK_FUNCTION) (
	IN PVOID lpFlsData
	);



#define PROCESSOR_ARCHITECTURE_INTEL            0
#define PROCESSOR_ARCHITECTURE_MIPS             1
#define PROCESSOR_ARCHITECTURE_ALPHA            2
#define PROCESSOR_ARCHITECTURE_PPC              3
#define PROCESSOR_ARCHITECTURE_SHX              4
#define PROCESSOR_ARCHITECTURE_ARM              5
#define PROCESSOR_ARCHITECTURE_IA64             6
#define PROCESSOR_ARCHITECTURE_ALPHA64          7
#define PROCESSOR_ARCHITECTURE_MSIL             8
#define PROCESSOR_ARCHITECTURE_AMD64            9
#define PROCESSOR_ARCHITECTURE_IA32_ON_WIN64    10
#define PROCESSOR_ARCHITECTURE_NEUTRAL          11

#define PROCESSOR_INTEL_386     386
#define PROCESSOR_INTEL_486     486
#define PROCESSOR_INTEL_PENTIUM 586
#define PROCESSOR_INTEL_IA64    2200
#define PROCESSOR_AMD_X8664     8664
#define PROCESSOR_MIPS_R4000    4000    // incl R4101 & R3910 for Windows CE
#define PROCESSOR_ALPHA_21064   21064
#define PROCESSOR_PPC_601       601
#define PROCESSOR_PPC_603       603
#define PROCESSOR_PPC_604       604
#define PROCESSOR_PPC_620       620
#define PROCESSOR_HITACHI_SH3   10003   // Windows CE
#define PROCESSOR_HITACHI_SH3E  10004   // Windows CE
#define PROCESSOR_HITACHI_SH4   10005   // Windows CE
#define PROCESSOR_MOTOROLA_821  821     // Windows CE
#define PROCESSOR_SHx_SH3       103     // Windows CE
#define PROCESSOR_SHx_SH4       104     // Windows CE
#define PROCESSOR_STRONGARM     2577    // Windows CE - 0xA11
#define PROCESSOR_ARM720        1824    // Windows CE - 0x720
#define PROCESSOR_ARM820        2080    // Windows CE - 0x820
#define PROCESSOR_ARM920        2336    // Windows CE - 0x920
#define PROCESSOR_ARM_7TDMI     70001   // Windows CE
#define PROCESSOR_OPTIL         0x494f  // MSIL



NTSYSAPI
PVOID
NTAPI
RtlPcToFileHeader(
    _In_ PVOID PcValue,
    _Out_ PVOID* BaseOfImage
    );



typedef struct _RTL_CRITICAL_SECTION_DEBUG {
    WORD   Type;
    WORD   CreatorBackTraceIndex;
    struct _RTL_CRITICAL_SECTION* CriticalSection;
    LIST_ENTRY ProcessLocksList;
    DWORD EntryCount;
    DWORD ContentionCount;
    DWORD Flags;
    WORD   CreatorBackTraceIndexHigh;
    WORD   SpareWORD;
} RTL_CRITICAL_SECTION_DEBUG, * PRTL_CRITICAL_SECTION_DEBUG, RTL_RESOURCE_DEBUG, * PRTL_RESOURCE_DEBUG;

//
// These flags define the upper byte of the critical section SpinCount field
//
#define RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO         0x01000000
#define RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN          0x02000000
#define RTL_CRITICAL_SECTION_FLAG_STATIC_INIT           0x04000000
#define RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE         0x08000000
#define RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO      0x10000000
#define RTL_CRITICAL_SECTION_ALL_FLAG_BITS              0xFF000000
#define RTL_CRITICAL_SECTION_FLAG_RESERVED              (RTL_CRITICAL_SECTION_ALL_FLAG_BITS & (~(RTL_CRITICAL_SECTION_FLAG_NO_DEBUG_INFO | RTL_CRITICAL_SECTION_FLAG_DYNAMIC_SPIN | RTL_CRITICAL_SECTION_FLAG_STATIC_INIT | RTL_CRITICAL_SECTION_FLAG_RESOURCE_TYPE | RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO)))

//
// These flags define possible values stored in the Flags field of a critsec debuginfo.
//
#define RTL_CRITICAL_SECTION_DEBUG_FLAG_STATIC_INIT     0x00000001

#pragma pack(push, 8)

typedef struct _RTL_CRITICAL_SECTION {
    PRTL_CRITICAL_SECTION_DEBUG DebugInfo;

    //
    //  The following three fields control entering and exiting the critical
    //  section for the resource
    //

    LONG LockCount;
    LONG RecursionCount;
    HANDLE OwningThread;        // from the thread's ClientId->UniqueThread
    HANDLE LockSemaphore;
    ULONG_PTR SpinCount;        // force size on 64-bit systems when packed
} RTL_CRITICAL_SECTION, * PRTL_CRITICAL_SECTION;

#pragma pack(pop)

typedef struct _RTL_SRWLOCK {
    PVOID Ptr;
} RTL_SRWLOCK, * PRTL_SRWLOCK;
#define RTL_SRWLOCK_INIT {0}                            
typedef struct _RTL_CONDITION_VARIABLE {
    PVOID Ptr;
} RTL_CONDITION_VARIABLE, * PRTL_CONDITION_VARIABLE;
#define RTL_CONDITION_VARIABLE_INIT {0}                 
#define RTL_CONDITION_VARIABLE_LOCKMODE_SHARED  0x1     



#define WT_EXECUTEDEFAULT       0x00000000                           
#define WT_EXECUTEINIOTHREAD    0x00000001                           
#define WT_EXECUTEINUITHREAD    0x00000002                           
#define WT_EXECUTEINWAITTHREAD  0x00000004                           
#define WT_EXECUTEONLYONCE      0x00000008                           
#define WT_EXECUTEINTIMERTHREAD 0x00000020                           
#define WT_EXECUTELONGFUNCTION  0x00000010                           
#define WT_EXECUTEINPERSISTENTIOTHREAD  0x00000040                   
#define WT_EXECUTEINPERSISTENTTHREAD 0x00000080                      
#define WT_TRANSFER_IMPERSONATION 0x00000100                         
#define WT_SET_MAX_THREADPOOL_THREADS(Flags, Limit)  ((Flags) |= (Limit)<<16) 



typedef DWORD TP_VERSION, *PTP_VERSION; 

typedef struct _TP_CALLBACK_INSTANCE TP_CALLBACK_INSTANCE, *PTP_CALLBACK_INSTANCE;

typedef VOID (NTAPI *PTP_SIMPLE_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context
    );

typedef struct _TP_POOL TP_POOL, *PTP_POOL; 

typedef enum _TP_CALLBACK_PRIORITY {
    TP_CALLBACK_PRIORITY_HIGH,
    TP_CALLBACK_PRIORITY_NORMAL,
    TP_CALLBACK_PRIORITY_LOW,
    TP_CALLBACK_PRIORITY_INVALID,
    TP_CALLBACK_PRIORITY_COUNT = TP_CALLBACK_PRIORITY_INVALID
} TP_CALLBACK_PRIORITY;

typedef struct _TP_POOL_STACK_INFORMATION {
    SIZE_T StackReserve;
    SIZE_T StackCommit;
}TP_POOL_STACK_INFORMATION, *PTP_POOL_STACK_INFORMATION;

typedef struct _TP_CLEANUP_GROUP TP_CLEANUP_GROUP, *PTP_CLEANUP_GROUP; 

typedef VOID (NTAPI *PTP_CLEANUP_GROUP_CANCEL_CALLBACK)(
    _Inout_opt_ PVOID ObjectContext,
    _Inout_opt_ PVOID CleanupContext
    );

//
// Do not manipulate this structure directly!  Allocate space for it
// and use the inline interfaces below.
//

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)
typedef struct _TP_CALLBACK_ENVIRON_V3 {
    TP_VERSION                         Version;
    PTP_POOL                           Pool;
    PTP_CLEANUP_GROUP                  CleanupGroup;
    PTP_CLEANUP_GROUP_CANCEL_CALLBACK  CleanupGroupCancelCallback;
    PVOID                              RaceDll;
    struct _ACTIVATION_CONTEXT        *ActivationContext;
    PTP_SIMPLE_CALLBACK                FinalizationCallback;
    union {
        DWORD                          Flags;
        struct {
            DWORD                      LongFunction :  1;
            DWORD                      Persistent   :  1;
            DWORD                      Private      : 30;
        } s;
    } u;
    TP_CALLBACK_PRIORITY               CallbackPriority;
    DWORD                              Size;
} TP_CALLBACK_ENVIRON_V3;

typedef TP_CALLBACK_ENVIRON_V3 TP_CALLBACK_ENVIRON, *PTP_CALLBACK_ENVIRON;

#else

typedef struct _TP_CALLBACK_ENVIRON_V1 {
    TP_VERSION                         Version;
    PTP_POOL                           Pool;
    PTP_CLEANUP_GROUP                  CleanupGroup;
    PTP_CLEANUP_GROUP_CANCEL_CALLBACK  CleanupGroupCancelCallback;
    PVOID                              RaceDll;
    struct _ACTIVATION_CONTEXT        *ActivationContext;
    PTP_SIMPLE_CALLBACK                FinalizationCallback;
    union {
        DWORD                          Flags;
        struct {
            DWORD                      LongFunction :  1;
            DWORD                      Persistent   :  1;
            DWORD                      Private      : 30;
        } s;
    } u;
} TP_CALLBACK_ENVIRON_V1;

typedef TP_CALLBACK_ENVIRON_V1 TP_CALLBACK_ENVIRON, *PTP_CALLBACK_ENVIRON;

#endif

#if !defined(MIDL_PASS)

FORCEINLINE
VOID
TpInitializeCallbackEnviron(
    _Out_ PTP_CALLBACK_ENVIRON CallbackEnviron
    )
{

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)

    CallbackEnviron->Version = 3;

#else

    CallbackEnviron->Version = 1;

#endif

    CallbackEnviron->Pool = NULL;
    CallbackEnviron->CleanupGroup = NULL;
    CallbackEnviron->CleanupGroupCancelCallback = NULL;
    CallbackEnviron->RaceDll = NULL;
    CallbackEnviron->ActivationContext = NULL;
    CallbackEnviron->FinalizationCallback = NULL;
    CallbackEnviron->u.Flags = 0;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)

    CallbackEnviron->CallbackPriority = TP_CALLBACK_PRIORITY_NORMAL;
    CallbackEnviron->Size = sizeof(TP_CALLBACK_ENVIRON);

#endif

}

FORCEINLINE
VOID
TpSetCallbackThreadpool(
    _Inout_ PTP_CALLBACK_ENVIRON CallbackEnviron,
    _In_    PTP_POOL             Pool
    )
{
    CallbackEnviron->Pool = Pool;
}

FORCEINLINE
VOID
TpSetCallbackCleanupGroup(
    _Inout_  PTP_CALLBACK_ENVIRON              CallbackEnviron,
    _In_     PTP_CLEANUP_GROUP                 CleanupGroup,
    _In_opt_ PTP_CLEANUP_GROUP_CANCEL_CALLBACK CleanupGroupCancelCallback
    )
{
    CallbackEnviron->CleanupGroup = CleanupGroup;
    CallbackEnviron->CleanupGroupCancelCallback = CleanupGroupCancelCallback;
}

FORCEINLINE
VOID
TpSetCallbackActivationContext(
    _Inout_  PTP_CALLBACK_ENVIRON CallbackEnviron,
    _In_opt_ struct _ACTIVATION_CONTEXT *ActivationContext
    )
{
    CallbackEnviron->ActivationContext = ActivationContext;
}

FORCEINLINE
VOID
TpSetCallbackNoActivationContext(
    _Inout_ PTP_CALLBACK_ENVIRON CallbackEnviron
    )
{
    CallbackEnviron->ActivationContext = (struct _ACTIVATION_CONTEXT *)(LONG_PTR) -1; // INVALID_ACTIVATION_CONTEXT
}

FORCEINLINE
VOID
TpSetCallbackLongFunction(
    _Inout_ PTP_CALLBACK_ENVIRON CallbackEnviron
    )
{
    CallbackEnviron->u.s.LongFunction = 1;
}

FORCEINLINE
VOID
TpSetCallbackRaceWithDll(
    _Inout_ PTP_CALLBACK_ENVIRON CallbackEnviron,
    _In_    PVOID                DllHandle
    )
{
    CallbackEnviron->RaceDll = DllHandle;
}

FORCEINLINE
VOID
TpSetCallbackFinalizationCallback(
    _Inout_ PTP_CALLBACK_ENVIRON CallbackEnviron,
    _In_    PTP_SIMPLE_CALLBACK  FinalizationCallback
    )
{
    CallbackEnviron->FinalizationCallback = FinalizationCallback;
}

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN7)

FORCEINLINE
VOID
TpSetCallbackPriority(
    _Inout_ PTP_CALLBACK_ENVIRON CallbackEnviron,
    _In_    TP_CALLBACK_PRIORITY Priority
    )
{
    CallbackEnviron->CallbackPriority = Priority;
}

#endif

FORCEINLINE
VOID
TpSetCallbackPersistent(
    _Inout_ PTP_CALLBACK_ENVIRON CallbackEnviron
    )
{
    CallbackEnviron->u.s.Persistent = 1;
}


FORCEINLINE
VOID
TpDestroyCallbackEnviron(
    _In_ PTP_CALLBACK_ENVIRON CallbackEnviron
    )
{
    //
    // For the current version of the callback environment, no actions
    // need to be taken to tear down an initialized structure.  This
    // may change in a future release.
    //

    UNREFERENCED_PARAMETER(CallbackEnviron);
}

#endif // !defined(MIDL_PASS)


typedef struct _TP_WORK TP_WORK, *PTP_WORK;

typedef VOID (NTAPI *PTP_WORK_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WORK              Work
    );

typedef struct _TP_TIMER TP_TIMER, *PTP_TIMER;

typedef VOID (NTAPI *PTP_TIMER_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_TIMER             Timer
    );

typedef DWORD    TP_WAIT_RESULT;

typedef struct _TP_WAIT TP_WAIT, *PTP_WAIT;

typedef VOID (NTAPI *PTP_WAIT_CALLBACK)(
    _Inout_     PTP_CALLBACK_INSTANCE Instance,
    _Inout_opt_ PVOID                 Context,
    _Inout_     PTP_WAIT              Wait,
    _In_        TP_WAIT_RESULT        WaitResult
    );

typedef struct _TP_IO TP_IO, *PTP_IO;




















typedef struct _SCOPE_TABLE_AMD64 {
	DWORD Count;
	struct {
		DWORD BeginAddress;
		DWORD EndAddress;
		DWORD HandlerAddress;
		DWORD JumpTarget;
	} ScopeRecord[1];
} SCOPE_TABLE_AMD64, *PSCOPE_TABLE_AMD64;

#ifdef _AMD64_

#if defined(_M_AMD64) && !defined(RC_INVOKED) && !defined(MIDL_PASS)

//
// Define bit test intrinsics.
//

#ifdef __cplusplus
extern "C" {
#endif

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

#define BitTest64 _bittest64
#define BitTestAndComplement64 _bittestandcomplement64
#define BitTestAndSet64 _bittestandset64
#define BitTestAndReset64 _bittestandreset64
#define InterlockedBitTestAndSet64 _interlockedbittestandset64
#define InterlockedBitTestAndSet64Acquire _interlockedbittestandset64
#define InterlockedBitTestAndSet64Release _interlockedbittestandset64
#define InterlockedBitTestAndSet64NoFence _interlockedbittestandset64
#define InterlockedBitTestAndReset64 _interlockedbittestandreset64
#define InterlockedBitTestAndReset64Acquire _interlockedbittestandreset64
#define InterlockedBitTestAndReset64Release _interlockedbittestandreset64
#define InterlockedBitTestAndReset64NoFence _interlockedbittestandreset64

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

BOOLEAN
_bittest64 (
    _In_reads_bytes_((Offset/8)+1) LONG64 const *Base,
    _In_range_(>=,0) LONG64 Offset
    );

BOOLEAN
_bittestandcomplement64 (
    _Inout_updates_bytes_((Offset/8)+1) LONG64 *Base,
    _In_range_(>=,0) LONG64 Offset
    );

BOOLEAN
_bittestandset64 (
    _Inout_updates_bytes_((Offset/8)+1) LONG64 *Base,
    _In_range_(>=,0) LONG64 Offset
    );

BOOLEAN
_bittestandreset64 (
    _Inout_updates_bytes_((Offset/8)+1) LONG64 *Base,
    _In_range_(>=,0) LONG64 Offset
    );

BOOLEAN
_interlockedbittestandset64 (
    _Inout_updates_bytes_((Offset/8)+1) _Interlocked_operand_ LONG64 volatile *Base,
    _In_range_(>=,0) LONG64 Offset
    );

BOOLEAN
_interlockedbittestandreset64 (
    _Inout_updates_bytes_((Offset/8)+1) _Interlocked_operand_ LONG64 volatile *Base,
    _In_range_(>=,0) LONG64 Offset
    );

#pragma intrinsic(_bittest)
#pragma intrinsic(_bittestandcomplement)
#pragma intrinsic(_bittestandset)
#pragma intrinsic(_bittestandreset)
#pragma intrinsic(_interlockedbittestandset)
#pragma intrinsic(_interlockedbittestandreset)

#pragma intrinsic(_bittest64)
#pragma intrinsic(_bittestandcomplement64)
#pragma intrinsic(_bittestandset64)
#pragma intrinsic(_bittestandreset64)
#pragma intrinsic(_interlockedbittestandset64)
#pragma intrinsic(_interlockedbittestandreset64)

//
// Define bit scan intrinsics.
//

#define BitScanForward _BitScanForward
#define BitScanReverse _BitScanReverse
#define BitScanForward64 _BitScanForward64
#define BitScanReverse64 _BitScanReverse64

_Success_(return!=0)
BOOLEAN
_BitScanForward (
    _Out_ DWORD *Index,
    _In_ DWORD Mask
    );

_Success_(return!=0)
BOOLEAN
_BitScanReverse (
    _Out_ DWORD *Index,
    _In_ DWORD Mask
    );

_Success_(return!=0)
BOOLEAN
_BitScanForward64 (
    _Out_ DWORD *Index,
    _In_ DWORD64 Mask
    );

_Success_(return!=0)
BOOLEAN
_BitScanReverse64 (
    _Out_ DWORD *Index,
    _In_ DWORD64 Mask
    );

#pragma intrinsic(_BitScanForward)
#pragma intrinsic(_BitScanReverse)
#pragma intrinsic(_BitScanForward64)
#pragma intrinsic(_BitScanReverse64)

//
// Interlocked intrinsic functions.
//

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

#define InterlockedAnd64 _InterlockedAnd64
#define InterlockedAnd64Acquire _InterlockedAnd64
#define InterlockedAnd64Release _InterlockedAnd64
#define InterlockedAnd64NoFence _InterlockedAnd64
#define InterlockedAndAffinity InterlockedAnd64
#define InterlockedOr64 _InterlockedOr64
#define InterlockedOr64Acquire _InterlockedOr64
#define InterlockedOr64Release _InterlockedOr64
#define InterlockedOr64NoFence _InterlockedOr64
#define InterlockedOrAffinity InterlockedOr64
#define InterlockedXor64 _InterlockedXor64
#define InterlockedXor64Acquire _InterlockedXor64
#define InterlockedXor64Release _InterlockedXor64
#define InterlockedXor64NoFence _InterlockedXor64
#define InterlockedIncrement64 _InterlockedIncrement64
#define InterlockedIncrementAcquire64 _InterlockedIncrement64
#define InterlockedIncrementRelease64 _InterlockedIncrement64
#define InterlockedIncrementNoFence64 _InterlockedIncrement64
#define InterlockedDecrement64 _InterlockedDecrement64
#define InterlockedDecrementAcquire64 _InterlockedDecrement64
#define InterlockedDecrementRelease64 _InterlockedDecrement64
#define InterlockedDecrementNoFence64 _InterlockedDecrement64
#define InterlockedAdd64 _InlineInterlockedAdd64
#define InterlockedAddAcquire64 _InlineInterlockedAdd64
#define InterlockedAddRelease64 _InlineInterlockedAdd64
#define InterlockedAddNoFence64 _InlineInterlockedAdd64
#define InterlockedExchange64 _InterlockedExchange64
#define InterlockedExchangeAcquire64 InterlockedExchange64
#define InterlockedExchangeNoFence64 InterlockedExchange64
#define InterlockedExchangeAdd64 _InterlockedExchangeAdd64
#define InterlockedExchangeAddAcquire64 _InterlockedExchangeAdd64
#define InterlockedExchangeAddRelease64 _InterlockedExchangeAdd64
#define InterlockedExchangeAddNoFence64 _InterlockedExchangeAdd64
#define InterlockedCompareExchange64 _InterlockedCompareExchange64
#define InterlockedCompareExchangeAcquire64 InterlockedCompareExchange64
#define InterlockedCompareExchangeRelease64 InterlockedCompareExchange64
#define InterlockedCompareExchangeNoFence64 InterlockedCompareExchange64
#define InterlockedCompareExchange128 _InterlockedCompareExchange128

#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedExchangePointerNoFence _InterlockedExchangePointer
#define InterlockedExchangePointerAcquire _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerAcquire _InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerRelease _InterlockedCompareExchangePointer
#define InterlockedCompareExchangePointerNoFence _InterlockedCompareExchangePointer

#define InterlockedExchangeAddSizeT(a, b) InterlockedExchangeAdd64((LONG64 *)a, b)
#define InterlockedExchangeAddSizeTAcquire(a, b) InterlockedExchangeAdd64((LONG64 *)a, b)
#define InterlockedExchangeAddSizeTNoFence(a, b) InterlockedExchangeAdd64((LONG64 *)a, b)
#define InterlockedIncrementSizeT(a) InterlockedIncrement64((LONG64 *)a)
#define InterlockedIncrementSizeTNoFence(a) InterlockedIncrement64((LONG64 *)a)
#define InterlockedDecrementSizeT(a) InterlockedDecrement64((LONG64 *)a)
#define InterlockedDecrementSizeTNoFence(a) InterlockedDecrement64((LONG64 *)a)

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

LONG64
InterlockedAnd64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedOr64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG64
InterlockedXor64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 Value
    );

LONG
InterlockedIncrement (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend
    );

LONG
InterlockedDecrement (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend
    );

LONG
InterlockedExchange (
    _Inout_ _Interlocked_operand_ LONG volatile *Target,
    _In_ LONG Value
    );

LONG
InterlockedExchangeAdd (
    _Inout_ _Interlocked_operand_ LONG volatile *Addend,
    _In_ LONG Value
    );

//#if !defined(_X86AMD64_)
//
//__forceinline
//LONG
//InterlockedAdd (
//    _Inout_ _Interlocked_operand_ LONG volatile *Addend,
//    _In_ LONG Value
//    )
//
//{
//    return InterlockedExchangeAdd(Addend, Value) + Value;
//}
//
//#endif

LONG
InterlockedCompareExchange (
    _Inout_ _Interlocked_operand_ LONG volatile *Destination,
    _In_ LONG ExChange,
    _In_ LONG Comperand
    );

LONG64
InterlockedIncrement64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Addend
    );

LONG64
InterlockedDecrement64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Addend
    );

LONG64
InterlockedExchange64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Target,
    _In_ LONG64 Value
    );

LONG64
InterlockedExchangeAdd64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Addend,
    _In_ LONG64 Value
    );

//#if !defined(_X86AMD64_)
//
//__forceinline
//LONG64
//_InlineInterlockedAdd64 (
//    _Inout_ _Interlocked_operand_ LONG64 volatile *Addend,
//    _In_ LONG64 Value
//    )
//
//{
//    return InterlockedExchangeAdd64(Addend, Value) + Value;
//}
//
//#endif

LONG64
InterlockedCompareExchange64 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 ExChange,
    _In_ LONG64 Comperand
    );

BOOLEAN
InterlockedCompareExchange128 (
    _Inout_ _Interlocked_operand_ LONG64 volatile *Destination,
    _In_ LONG64 ExchangeHigh,
    _In_ LONG64 ExchangeLow,
    _Inout_ LONG64 *ComparandResult
    );

_Ret_writes_(_Inexpressible_(Unknown)) PVOID
InterlockedCompareExchangePointer (
    _Inout_ _At_(*Destination,
        _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
    _Interlocked_operand_ PVOID volatile *Destination,
    _In_opt_ PVOID Exchange,
    _In_opt_ PVOID Comperand
    );

_Ret_writes_(_Inexpressible_(Unknown)) PVOID
InterlockedExchangePointer(
    _Inout_ _At_(*Target,
        _Pre_writable_byte_size_(_Inexpressible_(Unknown))
        _Post_writable_byte_size_(_Inexpressible_(Unknown)))
    _Interlocked_operand_ PVOID volatile *Target,
    _In_opt_ PVOID Value
    );

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
#pragma intrinsic(_InterlockedExchangeAdd64)
#pragma intrinsic(_InterlockedCompareExchange64)

#if _MSC_VER >= 1500

#pragma intrinsic(_InterlockedCompareExchange128)

#endif

#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)

#if (_MSC_VER >= 1600)

#define InterlockedExchange8 _InterlockedExchange8
#define InterlockedExchange16 _InterlockedExchange16

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

#endif /* _MSC_VER >= 1600 */

#if _MSC_FULL_VER >= 140041204

#define InterlockedExchangeAdd8 _InterlockedExchangeAdd8
#define InterlockedAnd8 _InterlockedAnd8
#define InterlockedOr8 _InterlockedOr8
#define InterlockedXor8 _InterlockedXor8
#define InterlockedAnd16 _InterlockedAnd16
#define InterlockedOr16 _InterlockedOr16
#define InterlockedXor16 _InterlockedXor16

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
InterlockedAnd16(
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

SHORT
InterlockedOr16(
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

SHORT
InterlockedXor16(
    _Inout_ _Interlocked_operand_ SHORT volatile *Destination,
    _In_ SHORT Value
    );

#pragma intrinsic (_InterlockedExchangeAdd8)
#pragma intrinsic (_InterlockedAnd8)
#pragma intrinsic (_InterlockedOr8)
#pragma intrinsic (_InterlockedXor8)
#pragma intrinsic (_InterlockedAnd16)
#pragma intrinsic (_InterlockedOr16)
#pragma intrinsic (_InterlockedXor16)

#endif

// end_ntoshvp

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

// begin_ntoshvp

//
// Define function to flush a cache line.
//

#define CacheLineFlush(Address) _mm_clflush(Address)

VOID
_mm_clflush (
    _In_ VOID const *Address
    );

#pragma intrinsic(_mm_clflush)

// begin_sdfwdm
// begin_wudfpwdm

VOID
_ReadWriteBarrier (
    VOID
    );

#pragma intrinsic(_ReadWriteBarrier)

//
// Define memory fence intrinsics
//

#define FastFence __faststorefence

// end_wudfpwdm
// end_sdfwdm

#define LoadFence _mm_lfence
#define MemoryFence _mm_mfence
#define StoreFence _mm_sfence
#define SpeculationFence LoadFence

// begin_sdfwdm
// begin_wudfpwdm

VOID
__faststorefence (
    VOID
    );

// end_wudfpwdm
// end_sdfwdm

VOID
_mm_lfence (
    VOID
    );

VOID
_mm_mfence (
    VOID
    );

VOID
_mm_sfence (
    VOID
    );

VOID
_mm_pause (
    VOID
    );

VOID
_mm_prefetch (
    _In_ CHAR CONST *a,
    _In_ int sel
    );

VOID
_m_prefetchw (
    _In_ volatile CONST VOID *Source
    );

//
// Define constants for use with _mm_prefetch.
//

#define _MM_HINT_T0     1
#define _MM_HINT_T1     2
#define _MM_HINT_T2     3
#define _MM_HINT_NTA    0

// begin_sdfwdm
// begin_wudfpwdm

#pragma intrinsic(__faststorefence)

// end_wudfpwdm
// end_sdfwdm

#pragma intrinsic(_mm_pause)
#pragma intrinsic(_mm_prefetch)
#pragma intrinsic(_mm_lfence)
#pragma intrinsic(_mm_mfence)
#pragma intrinsic(_mm_sfence)
#pragma intrinsic(_m_prefetchw)

#define YieldProcessor _mm_pause
#define MemoryBarrier __faststorefence
#define PreFetchCacheLine(l, a)  _mm_prefetch((CHAR CONST *) a, l)
#define PrefetchForWrite(p) _m_prefetchw(p)
#define ReadForWriteAccess(p) (_m_prefetchw(p), *(p))

//
// PreFetchCacheLine level defines.
//

#define PF_TEMPORAL_LEVEL_1 _MM_HINT_T0
#define PF_TEMPORAL_LEVEL_2 _MM_HINT_T1
#define PF_TEMPORAL_LEVEL_3 _MM_HINT_T2
#define PF_NON_TEMPORAL_LEVEL_ALL _MM_HINT_NTA

//
// Define get/set MXCSR intrinsics.
//

#define ReadMxCsr _mm_getcsr
#define WriteMxCsr _mm_setcsr

unsigned int
_mm_getcsr (
    VOID
    );

VOID
_mm_setcsr (
    _In_ unsigned int MxCsr
    );

#pragma intrinsic(_mm_getcsr)
#pragma intrinsic(_mm_setcsr)

//
// Define function to get the caller's EFLAGs value.
//

#define GetCallersEflags() __getcallerseflags()

unsigned __int32
__getcallerseflags (
    VOID
    );

#pragma intrinsic(__getcallerseflags)

//
// Define function to get segment limit.
//

#define GetSegmentLimit __segmentlimit

DWORD
__segmentlimit (
    _In_ DWORD Selector
    );

#pragma intrinsic(__segmentlimit)

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

//
// Define functions to move strings as bytes, words, dwords, and qwords.
//

VOID
__movsb (
    _Out_writes_all_(Count) PBYTE  Destination,
    _In_reads_(Count) BYTE  const *Source,
    _In_ SIZE_T Count
    );

VOID
__movsw (
    _Out_writes_all_(Count) PWORD   Destination,
    _In_reads_(Count) WORD   const *Source,
    _In_ SIZE_T Count
    );

VOID
__movsd (
    _Out_writes_all_(Count) PDWORD Destination,
    _In_reads_(Count) DWORD const *Source,
    _In_ SIZE_T Count
    );

VOID
__movsq (
    _Out_writes_all_(Count) PDWORD64 Destination,
    _In_reads_(Count) DWORD64 const *Source,
    _In_ SIZE_T Count
    );

#pragma intrinsic(__movsb)
#pragma intrinsic(__movsw)
#pragma intrinsic(__movsd)
#pragma intrinsic(__movsq)

//
// Define functions to store strings as bytes, words, dwords, and qwords.
//

VOID
__stosb (
    _Out_writes_all_(Count) PBYTE  Destination,
    _In_ BYTE  Value,
    _In_ SIZE_T Count
    );

VOID
__stosw (
    _Out_writes_all_(Count) PWORD   Destination,
    _In_ WORD   Value,
    _In_ SIZE_T Count
    );

VOID
__stosd (
    _Out_writes_all_(Count) PDWORD Destination,
    _In_ DWORD Value,
    _In_ SIZE_T Count
    );

VOID
__stosq (
    _Out_writes_all_(Count) PDWORD64 Destination,
    _In_ DWORD64 Value,
    _In_ SIZE_T Count
    );

#pragma intrinsic(__stosb)
#pragma intrinsic(__stosw)
#pragma intrinsic(__stosd)
#pragma intrinsic(__stosq)

//
// Define functions to capture the high 64-bits of a 128-bit multiply.
//

#define MultiplyHigh __mulh
#define UnsignedMultiplyHigh __umulh

LONGLONG
MultiplyHigh (
    _In_ LONG64 Multiplier,
    _In_ LONG64 Multiplicand
    );

ULONGLONG
UnsignedMultiplyHigh (
    _In_ DWORD64 Multiplier,
    _In_ DWORD64 Multiplicand
    );

#pragma intrinsic(__mulh)
#pragma intrinsic(__umulh)

//
// Define population count intrinsic.
//

#define PopulationCount64 __popcnt64

DWORD64
PopulationCount64 (
    _In_ DWORD64 operand
    );

#if _MSC_VER >= 1500

#pragma intrinsic(__popcnt64)

#endif

//
// Define functions to perform 128-bit shifts
//

#define ShiftLeft128 __shiftleft128
#define ShiftRight128 __shiftright128

DWORD64
ShiftLeft128 (
    _In_ DWORD64 LowPart,
    _In_ DWORD64 HighPart,
    _In_ BYTE  Shift
    );

DWORD64
ShiftRight128 (
    _In_ DWORD64 LowPart,
    _In_ DWORD64 HighPart,
    _In_ BYTE  Shift
    );

#pragma intrinsic(__shiftleft128)
#pragma intrinsic(__shiftright128)

//
// Define functions to perform 128-bit multiplies.
//

#define Multiply128 _mul128

LONG64
Multiply128 (
    _In_ LONG64 Multiplier,
    _In_ LONG64 Multiplicand,
    _Out_ LONG64 *HighProduct
    );

#pragma intrinsic(_mul128)

#ifndef UnsignedMultiply128

#define UnsignedMultiply128 _umul128

DWORD64
UnsignedMultiply128 (
    _In_ DWORD64 Multiplier,
    _In_ DWORD64 Multiplicand,
    _Out_ DWORD64 *HighProduct
    );

#pragma intrinsic(_umul128)

#endif

//__forceinline
//LONG64
//MultiplyExtract128 (
//    _In_ LONG64 Multiplier,
//    _In_ LONG64 Multiplicand,
//    _In_ BYTE  Shift
//    )
//
//{
//
//    LONG64 extractedProduct;
//    LONG64 highProduct;
//    LONG64 lowProduct;
//    BOOLEAN negate;
//    DWORD64 uhighProduct;
//    DWORD64 ulowProduct;
//
//    lowProduct = Multiply128(Multiplier, Multiplicand, &highProduct);
//    negate = FALSE;
//    uhighProduct = (DWORD64)highProduct;
//    ulowProduct = (DWORD64)lowProduct;
//    if (highProduct < 0) {
//        negate = TRUE;
//        uhighProduct = (DWORD64)(-highProduct);
//        ulowProduct = (DWORD64)(-lowProduct);
//        if (ulowProduct != 0) {
//            uhighProduct -= 1;
//        }
//    }
//
//    extractedProduct = (LONG64)ShiftRight128(ulowProduct, uhighProduct, Shift);
//    if (negate != FALSE) {
//        extractedProduct = -extractedProduct;
//    }
//
//    return extractedProduct;
//}
//
//__forceinline
//DWORD64
//UnsignedMultiplyExtract128 (
//    _In_ DWORD64 Multiplier,
//    _In_ DWORD64 Multiplicand,
//    _In_ BYTE  Shift
//    )
//
//{
//
//    DWORD64 extractedProduct;
//    DWORD64 highProduct;
//    DWORD64 lowProduct;
//
//    lowProduct = UnsignedMultiply128(Multiplier, Multiplicand, &highProduct);
//    extractedProduct = ShiftRight128(lowProduct, highProduct, Shift);
//    return extractedProduct;
//}

//
// Define functions to read and write the uer TEB and the system PCR/PRCB.
//

BYTE 
__readgsbyte (
    _In_ DWORD Offset
    );

WORD  
__readgsword (
    _In_ DWORD Offset
    );

DWORD
__readgsdword (
    _In_ DWORD Offset
    );

DWORD64
__readgsqword (
    _In_ DWORD Offset
    );

VOID
__writegsbyte (
    _In_ DWORD Offset,
    _In_ BYTE  Data
    );

VOID
__writegsword (
    _In_ DWORD Offset,
    _In_ WORD   Data
    );

VOID
__writegsdword (
    _In_ DWORD Offset,
    _In_ DWORD Data
    );

VOID
__writegsqword (
    _In_ DWORD Offset,
    _In_ DWORD64 Data
    );

#pragma intrinsic(__readgsbyte)
#pragma intrinsic(__readgsword)
#pragma intrinsic(__readgsdword)
#pragma intrinsic(__readgsqword)
#pragma intrinsic(__writegsbyte)
#pragma intrinsic(__writegsword)
#pragma intrinsic(__writegsdword)
#pragma intrinsic(__writegsqword)

#if !defined(_MANAGED)

VOID
__incgsbyte (
    _In_ DWORD Offset
    );

VOID
__addgsbyte (
    _In_ DWORD Offset,
    _In_ BYTE  Value
    );

VOID
__incgsword (
    _In_ DWORD Offset
    );

VOID
__addgsword (
    _In_ DWORD Offset,
    _In_ WORD   Value
    );

VOID
__incgsdword (
    _In_ DWORD Offset
    );

VOID
__addgsdword (
    _In_ DWORD Offset,
    _In_ DWORD Value
    );

VOID
__incgsqword (
    _In_ DWORD Offset
    );

VOID
__addgsqword (
    _In_ DWORD Offset,
    _In_ DWORD64 Value
    );

#if 0
#pragma intrinsic(__incgsbyte)
#pragma intrinsic(__addgsbyte)
#pragma intrinsic(__incgsword)
#pragma intrinsic(__addgsword)
#pragma intrinsic(__incgsdword)
#pragma intrinsic(__addgsdword)
#pragma intrinsic(__incgsqword)
#pragma intrinsic(__addgsqword)
#endif

#endif // !defined(_MANAGED)


#ifdef __cplusplus
}
#endif

#endif // defined(_M_AMD64) && !defined(RC_INVOKED) && !defined(MIDL_PASS)

// end_ntoshvp
//
// The following values specify the type of access in the first parameter
// of the exception record whan the exception code specifies an access
// violation.
//

#define EXCEPTION_READ_FAULT 0          // exception caused by a read
#define EXCEPTION_WRITE_FAULT 1         // exception caused by a write
#define EXCEPTION_EXECUTE_FAULT 8       // exception caused by an instruction fetch

// begin_wx86
//
// The following flags control the contents of the CONTEXT structure.
//

#if !defined(RC_INVOKED)

#define CONTEXT_AMD64   0x00100000L

// end_wx86

#define CONTEXT_CONTROL         (CONTEXT_AMD64 | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_AMD64 | 0x00000002L)
#define CONTEXT_SEGMENTS        (CONTEXT_AMD64 | 0x00000004L)
#define CONTEXT_FLOATING_POINT  (CONTEXT_AMD64 | 0x00000008L)
#define CONTEXT_DEBUG_REGISTERS (CONTEXT_AMD64 | 0x00000010L)

#define CONTEXT_FULL            (CONTEXT_CONTROL | CONTEXT_INTEGER | \
                                 CONTEXT_FLOATING_POINT)

#define CONTEXT_ALL             (CONTEXT_CONTROL | CONTEXT_INTEGER | \
                                 CONTEXT_SEGMENTS | CONTEXT_FLOATING_POINT | \
                                 CONTEXT_DEBUG_REGISTERS)

#define CONTEXT_XSTATE          (CONTEXT_AMD64 | 0x00000040L)

#if defined(XBOX_SYSTEMOS)

#define CONTEXT_KERNEL_DEBUGGER     0x04000000L

#endif

#define CONTEXT_EXCEPTION_ACTIVE    0x08000000L
#define CONTEXT_SERVICE_ACTIVE      0x10000000L
#define CONTEXT_EXCEPTION_REQUEST   0x40000000L
#define CONTEXT_EXCEPTION_REPORTING 0x80000000L

// begin_wx86

#endif // !defined(RC_INVOKED)

//
// Define initial MxCsr and FpCsr control.
//

#define INITIAL_MXCSR 0x1f80            // initial MXCSR value
#define INITIAL_FPCSR 0x027f            // initial FPCSR value

// end_ntddk
// begin_wdm begin_ntosp
// begin_ntoshvp

typedef XSAVE_FORMAT XMM_SAVE_AREA32, *PXMM_SAVE_AREA32;

// end_wdm end_ntosp
// begin_ntddk

////
//// Context Frame
////
////  This frame has a several purposes: 1) it is used as an argument to
////  NtContinue, 2) it is used to constuct a call frame for APC delivery,
////  and 3) it is used in the user level thread creation routines.
////
////
//// The flags field within this record controls the contents of a CONTEXT
//// record.
////
//// If the context record is used as an input parameter, then for each
//// portion of the context record controlled by a flag whose value is
//// set, it is assumed that that portion of the context record contains
//// valid context. If the context record is being used to modify a threads
//// context, then only that portion of the threads context is modified.
////
//// If the context record is used as an output parameter to capture the
//// context of a thread, then only those portions of the thread's context
//// corresponding to set flags will be returned.
////
//// CONTEXT_CONTROL specifies SegSs, Rsp, SegCs, Rip, and EFlags.
////
//// CONTEXT_INTEGER specifies Rax, Rcx, Rdx, Rbx, Rbp, Rsi, Rdi, and R8-R15.
////
//// CONTEXT_SEGMENTS specifies SegDs, SegEs, SegFs, and SegGs.
////
//// CONTEXT_FLOATING_POINT specifies Xmm0-Xmm15.
////
//// CONTEXT_DEBUG_REGISTERS specifies Dr0-Dr3 and Dr6-Dr7.
////
//
//typedef struct DECLSPEC_ALIGN(16) DECLSPEC_NOINITALL _CONTEXT {
//
//    //
//    // Register parameter home addresses.
//    //
//    // N.B. These fields are for convience - they could be used to extend the
//    //      context record in the future.
//    //
//
//    DWORD64 P1Home;
//    DWORD64 P2Home;
//    DWORD64 P3Home;
//    DWORD64 P4Home;
//    DWORD64 P5Home;
//    DWORD64 P6Home;
//
//    //
//    // Control flags.
//    //
//
//    DWORD ContextFlags;
//    DWORD MxCsr;
//
//    //
//    // Segment Registers and processor flags.
//    //
//
//    WORD   SegCs;
//    WORD   SegDs;
//    WORD   SegEs;
//    WORD   SegFs;
//    WORD   SegGs;
//    WORD   SegSs;
//    DWORD EFlags;
//
//    //
//    // Debug registers
//    //
//
//    DWORD64 Dr0;
//    DWORD64 Dr1;
//    DWORD64 Dr2;
//    DWORD64 Dr3;
//    DWORD64 Dr6;
//    DWORD64 Dr7;
//
//    //
//    // Integer registers.
//    //
//
//    DWORD64 Rax;
//    DWORD64 Rcx;
//    DWORD64 Rdx;
//    DWORD64 Rbx;
//    DWORD64 Rsp;
//    DWORD64 Rbp;
//    DWORD64 Rsi;
//    DWORD64 Rdi;
//    DWORD64 R8;
//    DWORD64 R9;
//    DWORD64 R10;
//    DWORD64 R11;
//    DWORD64 R12;
//    DWORD64 R13;
//    DWORD64 R14;
//    DWORD64 R15;
//
//    //
//    // Program counter.
//    //
//
//    DWORD64 Rip;
//
//    //
//    // Floating point state.
//    //
//
//    union {
//        XMM_SAVE_AREA32 FltSave;
//        struct {
//            M128A Header[2];
//            M128A Legacy[8];
//            M128A Xmm0;
//            M128A Xmm1;
//            M128A Xmm2;
//            M128A Xmm3;
//            M128A Xmm4;
//            M128A Xmm5;
//            M128A Xmm6;
//            M128A Xmm7;
//            M128A Xmm8;
//            M128A Xmm9;
//            M128A Xmm10;
//            M128A Xmm11;
//            M128A Xmm12;
//            M128A Xmm13;
//            M128A Xmm14;
//            M128A Xmm15;
//        } DUMMYSTRUCTNAME;
//    } DUMMYUNIONNAME;
//
//    //
//    // Vector registers.
//    //
//
//    M128A VectorRegister[26];
//    DWORD64 VectorControl;
//
//    //
//    // Special debug control registers.
//    //
//
//    DWORD64 DebugControl;
//    DWORD64 LastBranchToRip;
//    DWORD64 LastBranchFromRip;
//    DWORD64 LastExceptionToRip;
//    DWORD64 LastExceptionFromRip;
//} CONTEXT, *PCONTEXT;

// end_ntoshvp
//
// Select platform-specific definitions
//

typedef struct _IMAGE_RUNTIME_FUNCTION_ENTRY RUNTIME_FUNCTION, *PRUNTIME_FUNCTION;
typedef SCOPE_TABLE_AMD64 SCOPE_TABLE, *PSCOPE_TABLE;

#define RUNTIME_FUNCTION_INDIRECT 0x1

//
// Define unwind information flags.
//

#define UNW_FLAG_NHANDLER       0x0
#define UNW_FLAG_EHANDLER       0x1
#define UNW_FLAG_UHANDLER       0x2
#define UNW_FLAG_CHAININFO      0x4

#define UNW_FLAG_NO_EPILOGUE    0x80000000UL    // Software only flag

//
// Define unwind history table structure.
//

#define UNWIND_HISTORY_TABLE_SIZE 12

typedef struct _UNWIND_HISTORY_TABLE_ENTRY {
    DWORD64 ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
} UNWIND_HISTORY_TABLE_ENTRY, *PUNWIND_HISTORY_TABLE_ENTRY;

typedef struct _UNWIND_HISTORY_TABLE {
    DWORD Count;
    BYTE  LocalHint;
    BYTE  GlobalHint;
    BYTE  Search;
    BYTE  Once;
    DWORD64 LowAddress;
    DWORD64 HighAddress;
    UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
} UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;

//
// Define dynamic function table entry.
//

typedef
_Function_class_(GET_RUNTIME_FUNCTION_CALLBACK)
PRUNTIME_FUNCTION
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
    _Out_ PRUNTIME_FUNCTION* Functions
    );
typedef OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK *POUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK;

#define OUT_OF_PROCESS_FUNCTION_TABLE_CALLBACK_EXPORT_NAME \
    "OutOfProcessFunctionTableCallback"

//
// Define exception dispatch context structure.
//

typedef struct _DISPATCHER_CONTEXT {
    DWORD64 ControlPc;
    DWORD64 ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
    DWORD64 EstablisherFrame;
    DWORD64 TargetIp;
    PCONTEXT ContextRecord;
    PEXCEPTION_ROUTINE LanguageHandler;
    PVOID HandlerData;
    PUNWIND_HISTORY_TABLE HistoryTable;
    DWORD ScopeIndex;
    DWORD Fill0;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

//
// Define exception filter and termination handler function types.
//

struct _EXCEPTION_POINTERS;
typedef
LONG
(*PEXCEPTION_FILTER) (
    struct _EXCEPTION_POINTERS *ExceptionPointers,
    PVOID EstablisherFrame
    );

typedef
VOID
(*PTERMINATION_HANDLER) (
    BOOLEAN AbnormalTermination,
    PVOID EstablisherFrame
    );


//
// Nonvolatile context pointer record.
//

typedef struct _KNONVOLATILE_CONTEXT_POINTERS {
    union {
        PM128A FloatingContext[16];
        struct {
            PM128A Xmm0;
            PM128A Xmm1;
            PM128A Xmm2;
            PM128A Xmm3;
            PM128A Xmm4;
            PM128A Xmm5;
            PM128A Xmm6;
            PM128A Xmm7;
            PM128A Xmm8;
            PM128A Xmm9;
            PM128A Xmm10;
            PM128A Xmm11;
            PM128A Xmm12;
            PM128A Xmm13;
            PM128A Xmm14;
            PM128A Xmm15;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;

    union {
        PDWORD64 IntegerContext[16];
        struct {
            PDWORD64 Rax;
            PDWORD64 Rcx;
            PDWORD64 Rdx;
            PDWORD64 Rbx;
            PDWORD64 Rsp;
            PDWORD64 Rbp;
            PDWORD64 Rsi;
            PDWORD64 Rdi;
            PDWORD64 R8;
            PDWORD64 R9;
            PDWORD64 R10;
            PDWORD64 R11;
            PDWORD64 R12;
            PDWORD64 R13;
            PDWORD64 R14;
            PDWORD64 R15;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME2;

} KNONVOLATILE_CONTEXT_POINTERS, *PKNONVOLATILE_CONTEXT_POINTERS;

#endif // _AMD64_



NTSYSAPI
VOID
NTAPI
RtlUnwindEx(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue,
    _In_ PCONTEXT ContextRecord,
    _In_opt_ PUNWIND_HISTORY_TABLE HistoryTable
    );



NTSYSAPI
PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionEntry(
    _In_ DWORD64 ControlPc,
    _Out_ PDWORD64 ImageBase,
    _Inout_opt_ PUNWIND_HISTORY_TABLE HistoryTable
    );



NTSYSAPI
PEXCEPTION_ROUTINE
NTAPI
RtlVirtualUnwind(
    _In_ DWORD HandlerType,
    _In_ DWORD64 ImageBase,
    _In_ DWORD64 ControlPc,
    _In_ PRUNTIME_FUNCTION FunctionEntry,
    _Inout_ PCONTEXT ContextRecord,
    _Out_ PVOID* HandlerData,
    _Out_ PDWORD64 EstablisherFrame,
    _Inout_opt_ PKNONVOLATILE_CONTEXT_POINTERS ContextPointers
    );



#pragma warning(default:4201 4214)
EXTERN_C_END
