#pragma once

#include <ntimage.h>

EXTERN_C_START

#pragma warning(disable:4214)

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

#pragma warning(default:4214)
EXTERN_C_END
