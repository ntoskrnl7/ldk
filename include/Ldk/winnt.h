#pragma once

#ifndef _WINNT_
#define _WINNT_

#include <ntifs.h>
#define _DEVIOCTL_

#include <ntimage.h>

EXTERN_C_START

#pragma warning(disable:4201 4214)

#define NtCurrentTeb			LdkCurrentTeb
#define NtCurrentPeb			LdkCurrentPeb



#define DLL_PROCESS_ATTACH   1    
#define DLL_THREAD_ATTACH    2    
#define DLL_THREAD_DETACH    3    
#define DLL_PROCESS_DETACH   0


#ifndef PROCESS_TERMINATE
#define PROCESS_TERMINATE                  (0x0001)
#endif
#ifndef PROCESS_CREATE_THREAD
#define PROCESS_CREATE_THREAD              (0x0002)
#endif
#ifndef PROCESS_SET_SESSIONID
#define PROCESS_SET_SESSIONID              (0x0004)
#endif
#ifndef PROCESS_VM_OPERATION
#define PROCESS_VM_OPERATION               (0x0008)
#endif
#ifndef PROCESS_VM_READ
#define PROCESS_VM_READ                    (0x0010)
#endif
#ifndef PROCESS_VM_WRITE
#define PROCESS_VM_WRITE                   (0x0020)
#endif
#ifndef PROCESS_DUP_HANDLE
#define PROCESS_DUP_HANDLE                 (0x0040)
#endif
#ifndef PROCESS_CREATE_PROCESS
#define PROCESS_CREATE_PROCESS             (0x0080)
#endif
#ifndef PROCESS_SET_QUOTA
#define PROCESS_SET_QUOTA                  (0x0100)
#endif
#ifndef PROCESS_SET_INFORMATION
#define PROCESS_SET_INFORMATION            (0x0200)
#endif
#ifndef PROCESS_QUERY_INFORMATION
#define PROCESS_QUERY_INFORMATION          (0x0400)
#endif
#ifndef PROCESS_SUSPEND_RESUME
#define PROCESS_SUSPEND_RESUME             (0x0800)
#endif
#ifndef PROCESS_QUERY_LIMITED_INFORMATION
#define PROCESS_QUERY_LIMITED_INFORMATION  (0x1000)
#endif
#ifndef PROCESS_SET_LIMITED_INFORMATION
#define PROCESS_SET_LIMITED_INFORMATION    (0x2000)
#endif
#ifndef PROCESS_ALL_ACCESS
#define PROCESS_ALL_ACCESS                 (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0xffff)
#endif

#ifndef DUPLICATE_CLOSE_SOURCE
#define DUPLICATE_CLOSE_SOURCE             0x00000001
#endif
#ifndef DUPLICATE_SAME_ACCESS
#define DUPLICATE_SAME_ACCESS              0x00000002
#endif



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

//
// Processor feature identifiers for IsProcessorFeaturePresent.
//
#ifndef PF_FLOATING_POINT_PRECISION_ERRATA
#define PF_FLOATING_POINT_PRECISION_ERRATA           0
#endif
#ifndef PF_FLOATING_POINT_EMULATED
#define PF_FLOATING_POINT_EMULATED                   1
#endif
#ifndef PF_COMPARE_EXCHANGE_DOUBLE
#define PF_COMPARE_EXCHANGE_DOUBLE                   2
#endif
#ifndef PF_MMX_INSTRUCTIONS_AVAILABLE
#define PF_MMX_INSTRUCTIONS_AVAILABLE                3
#endif
#ifndef PF_PPC_MOVEMEM_64BIT_OK
#define PF_PPC_MOVEMEM_64BIT_OK                      4
#endif
#ifndef PF_ALPHA_BYTE_INSTRUCTIONS
#define PF_ALPHA_BYTE_INSTRUCTIONS                   5
#endif
#ifndef PF_XMMI_INSTRUCTIONS_AVAILABLE
#define PF_XMMI_INSTRUCTIONS_AVAILABLE               6
#endif
#ifndef PF_3DNOW_INSTRUCTIONS_AVAILABLE
#define PF_3DNOW_INSTRUCTIONS_AVAILABLE              7
#endif
#ifndef PF_RDTSC_INSTRUCTION_AVAILABLE
#define PF_RDTSC_INSTRUCTION_AVAILABLE               8
#endif
#ifndef PF_PAE_ENABLED
#define PF_PAE_ENABLED                               9
#endif
#ifndef PF_XMMI64_INSTRUCTIONS_AVAILABLE
#define PF_XMMI64_INSTRUCTIONS_AVAILABLE            10
#endif
#ifndef PF_SSE_DAZ_MODE_AVAILABLE
#define PF_SSE_DAZ_MODE_AVAILABLE                   11
#endif
#ifndef PF_NX_ENABLED
#define PF_NX_ENABLED                               12
#endif
#ifndef PF_SSE3_INSTRUCTIONS_AVAILABLE
#define PF_SSE3_INSTRUCTIONS_AVAILABLE              13
#endif
#ifndef PF_COMPARE_EXCHANGE128
#define PF_COMPARE_EXCHANGE128                      14
#endif
#ifndef PF_COMPARE64_EXCHANGE128
#define PF_COMPARE64_EXCHANGE128                    15
#endif
#ifndef PF_CHANNELS_ENABLED
#define PF_CHANNELS_ENABLED                         16
#endif
#ifndef PF_XSAVE_ENABLED
#define PF_XSAVE_ENABLED                            17
#endif
#ifndef PF_ARM_VFP_32_REGISTERS_AVAILABLE
#define PF_ARM_VFP_32_REGISTERS_AVAILABLE           18
#endif
#ifndef PF_ARM_NEON_INSTRUCTIONS_AVAILABLE
#define PF_ARM_NEON_INSTRUCTIONS_AVAILABLE          19
#endif
#ifndef PF_SECOND_LEVEL_ADDRESS_TRANSLATION
#define PF_SECOND_LEVEL_ADDRESS_TRANSLATION         20
#endif
#ifndef PF_VIRT_FIRMWARE_ENABLED
#define PF_VIRT_FIRMWARE_ENABLED                    21
#endif
#ifndef PF_RDWRFSGSBASE_AVAILABLE
#define PF_RDWRFSGSBASE_AVAILABLE                   22
#endif
#ifndef PF_FASTFAIL_AVAILABLE
#define PF_FASTFAIL_AVAILABLE                       23
#endif
#ifndef PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE
#define PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE         24
#endif
#ifndef PF_ARM_64BIT_LOADSTORE_ATOMIC
#define PF_ARM_64BIT_LOADSTORE_ATOMIC               25
#endif
#ifndef PF_ARM_EXTERNAL_CACHE_AVAILABLE
#define PF_ARM_EXTERNAL_CACHE_AVAILABLE             26
#endif
#ifndef PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE
#define PF_ARM_FMAC_INSTRUCTIONS_AVAILABLE          27
#endif
#ifndef PF_RDRAND_INSTRUCTION_AVAILABLE
#define PF_RDRAND_INSTRUCTION_AVAILABLE             28
#endif
#ifndef PF_ARM_V8_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V8_INSTRUCTIONS_AVAILABLE            29
#endif
#ifndef PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V8_CRYPTO_INSTRUCTIONS_AVAILABLE     30
#endif
#ifndef PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V8_CRC32_INSTRUCTIONS_AVAILABLE      31
#endif
#ifndef PF_RDTSCP_INSTRUCTION_AVAILABLE
#define PF_RDTSCP_INSTRUCTION_AVAILABLE             32
#endif
#ifndef PF_RDPID_INSTRUCTION_AVAILABLE
#define PF_RDPID_INSTRUCTION_AVAILABLE              33
#endif
#ifndef PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V81_ATOMIC_INSTRUCTIONS_AVAILABLE    34
#endif
#ifndef PF_MONITORX_INSTRUCTION_AVAILABLE
#define PF_MONITORX_INSTRUCTION_AVAILABLE           35
#endif
#ifndef PF_SSSE3_INSTRUCTIONS_AVAILABLE
#define PF_SSSE3_INSTRUCTIONS_AVAILABLE             36
#endif
#ifndef PF_SSE4_1_INSTRUCTIONS_AVAILABLE
#define PF_SSE4_1_INSTRUCTIONS_AVAILABLE            37
#endif
#ifndef PF_SSE4_2_INSTRUCTIONS_AVAILABLE
#define PF_SSE4_2_INSTRUCTIONS_AVAILABLE            38
#endif
#ifndef PF_AVX_INSTRUCTIONS_AVAILABLE
#define PF_AVX_INSTRUCTIONS_AVAILABLE               39
#endif
#ifndef PF_AVX2_INSTRUCTIONS_AVAILABLE
#define PF_AVX2_INSTRUCTIONS_AVAILABLE              40
#endif
#ifndef PF_AVX512F_INSTRUCTIONS_AVAILABLE
#define PF_AVX512F_INSTRUCTIONS_AVAILABLE           41
#endif
#ifndef PF_ERMS_AVAILABLE
#define PF_ERMS_AVAILABLE                           42
#endif
#ifndef PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V82_DP_INSTRUCTIONS_AVAILABLE        43
#endif
#ifndef PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V83_JSCVT_INSTRUCTIONS_AVAILABLE     44
#endif
#ifndef PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V83_LRCPC_INSTRUCTIONS_AVAILABLE     45
#endif
#ifndef PF_ARM_SVE_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_INSTRUCTIONS_AVAILABLE           46
#endif
#ifndef PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE2_INSTRUCTIONS_AVAILABLE          47
#endif
#ifndef PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE2_1_INSTRUCTIONS_AVAILABLE        48
#endif
#ifndef PF_ARM_SVE_AES_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_AES_INSTRUCTIONS_AVAILABLE       49
#endif
#ifndef PF_ARM_SVE_PMULL128_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_PMULL128_INSTRUCTIONS_AVAILABLE  50
#endif
#ifndef PF_ARM_SVE_BITPERM_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_BITPERM_INSTRUCTIONS_AVAILABLE   51
#endif
#ifndef PF_ARM_SVE_BF16_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_BF16_INSTRUCTIONS_AVAILABLE      52
#endif
#ifndef PF_ARM_SVE_EBF16_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_EBF16_INSTRUCTIONS_AVAILABLE     53
#endif
#ifndef PF_ARM_SVE_B16B16_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_B16B16_INSTRUCTIONS_AVAILABLE    54
#endif
#ifndef PF_ARM_SVE_SHA3_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_SHA3_INSTRUCTIONS_AVAILABLE      55
#endif
#ifndef PF_ARM_SVE_SM4_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_SM4_INSTRUCTIONS_AVAILABLE       56
#endif
#ifndef PF_ARM_SVE_I8MM_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_I8MM_INSTRUCTIONS_AVAILABLE      57
#endif
#ifndef PF_ARM_SVE_F32MM_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_F32MM_INSTRUCTIONS_AVAILABLE     58
#endif
#ifndef PF_ARM_SVE_F64MM_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SVE_F64MM_INSTRUCTIONS_AVAILABLE     59
#endif
#ifndef PF_BMI2_INSTRUCTIONS_AVAILABLE
#define PF_BMI2_INSTRUCTIONS_AVAILABLE              60
#endif
#ifndef PF_MOVDIR64B_INSTRUCTION_AVAILABLE
#define PF_MOVDIR64B_INSTRUCTION_AVAILABLE          61
#endif
#ifndef PF_ARM_LSE2_AVAILABLE
#define PF_ARM_LSE2_AVAILABLE                       62
#endif
#ifndef PF_RESERVED_FEATURE
#define PF_RESERVED_FEATURE                         63
#endif
#ifndef PF_ARM_SHA3_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SHA3_INSTRUCTIONS_AVAILABLE          64
#endif
#ifndef PF_ARM_SHA512_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SHA512_INSTRUCTIONS_AVAILABLE        65
#endif
#ifndef PF_ARM_V82_I8MM_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V82_I8MM_INSTRUCTIONS_AVAILABLE      66
#endif
#ifndef PF_ARM_V82_FP16_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V82_FP16_INSTRUCTIONS_AVAILABLE      67
#endif
#ifndef PF_ARM_V86_BF16_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V86_BF16_INSTRUCTIONS_AVAILABLE      68
#endif
#ifndef PF_ARM_V86_EBF16_INSTRUCTIONS_AVAILABLE
#define PF_ARM_V86_EBF16_INSTRUCTIONS_AVAILABLE     69
#endif
#ifndef PF_ARM_SME_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_INSTRUCTIONS_AVAILABLE           70
#endif
#ifndef PF_ARM_SME2_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME2_INSTRUCTIONS_AVAILABLE          71
#endif
#ifndef PF_ARM_SME2_1_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME2_1_INSTRUCTIONS_AVAILABLE        72
#endif
#ifndef PF_ARM_SME2_2_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME2_2_INSTRUCTIONS_AVAILABLE        73
#endif
#ifndef PF_ARM_SME_AES_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_AES_INSTRUCTIONS_AVAILABLE       74
#endif
#ifndef PF_ARM_SME_SBITPERM_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_SBITPERM_INSTRUCTIONS_AVAILABLE  75
#endif
#ifndef PF_ARM_SME_SF8MM4_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_SF8MM4_INSTRUCTIONS_AVAILABLE    76
#endif
#ifndef PF_ARM_SME_SF8MM8_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_SF8MM8_INSTRUCTIONS_AVAILABLE    77
#endif
#ifndef PF_ARM_SME_SF8DP2_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_SF8DP2_INSTRUCTIONS_AVAILABLE    78
#endif
#ifndef PF_ARM_SME_SF8DP4_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_SF8DP4_INSTRUCTIONS_AVAILABLE    79
#endif
#ifndef PF_ARM_SME_SF8FMA_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_SF8FMA_INSTRUCTIONS_AVAILABLE    80
#endif
#ifndef PF_ARM_SME_F8F32_INSTRUCTIONS_AVAILABLE
#define PF_ARM_SME_F8F32_INSTRUCTIONS_AVAILABLE     81
#endif



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
typedef
VOID
(NTAPI *PAPCFUNC)(
    _In_ ULONG_PTR Parameter
    );

typedef LONG (NTAPI *PVECTORED_EXCEPTION_HANDLER)(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );

typedef enum _HEAP_INFORMATION_CLASS {

    HeapCompatibilityInformation = 0,
    HeapEnableTerminationOnCorruption = 1


#if ((NTDDI_VERSION > NTDDI_WINBLUE) || \
     (NTDDI_VERSION == NTDDI_WINBLUE && defined(WINBLUE_KBSPRING14)))
    ,

    HeapOptimizeResources = 3

#endif


    ,

    HeapTag = 7

} HEAP_INFORMATION_CLASS;



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
#if !defined(_LDK_NTURTL_)
#define WT_SET_MAX_THREADPOOL_THREADS(Flags, Limit)  ((Flags) |= (Limit)<<16)
typedef VOID (NTAPI * WORKERCALLBACKFUNC) (PVOID );        
typedef VOID (NTAPI * WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );
typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK;
#endif // !defined(_LDK_NTURTL_)
#define WT_EXECUTEINLONGTHREAD  0x00000010
#define WT_EXECUTEDELETEWAIT    0x00000008


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


#if !defined(_WDMDDK_)

//
// Define 128-bit 16-byte aligned xmm register type.
//

typedef struct DECLSPEC_ALIGN(16) _M128A {
    ULONGLONG Low;
    LONGLONG High;
} M128A, *PM128A;

//
// Format of data for (F)XSAVE/(F)XRSTOR instruction
//

typedef struct DECLSPEC_ALIGN(16) _XSAVE_FORMAT {
    WORD   ControlWord;
    WORD   StatusWord;
    BYTE  TagWord;
    BYTE  Reserved1;
    WORD   ErrorOpcode;
    DWORD ErrorOffset;
    WORD   ErrorSelector;
    WORD   Reserved2;
    DWORD DataOffset;
    WORD   DataSelector;
    WORD   Reserved3;
    DWORD MxCsr;
    DWORD MxCsr_Mask;
    M128A FloatRegisters[8];

#if defined(_WIN64)

    M128A XmmRegisters[16];
    BYTE  Reserved4[96];

#else

    M128A XmmRegisters[8];
    BYTE  Reserved4[224];

#endif

} XSAVE_FORMAT, *PXSAVE_FORMAT;
#endif //!defined(_WDMDDK_)



typedef struct _SCOPE_TABLE_AMD64 {
	DWORD Count;
	struct {
		DWORD BeginAddress;
		DWORD EndAddress;
		DWORD HandlerAddress;
		DWORD JumpTarget;
	} ScopeRecord[1];
} SCOPE_TABLE_AMD64, *PSCOPE_TABLE_AMD64;

#include "winnt_amd64.h"

//
//
// Scope table structure definition.
//

typedef struct _SCOPE_TABLE_ARM {
    DWORD Count;
    struct
    {
        DWORD BeginAddress;
        DWORD EndAddress;
        DWORD HandlerAddress;
        DWORD JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE_ARM, *PSCOPE_TABLE_ARM;

#include "winnt_arm.h"

//
// Scope table structure definition.
//

typedef struct _SCOPE_TABLE_ARM64 {
    DWORD Count;
    struct
    {
        DWORD BeginAddress;
        DWORD EndAddress;
        DWORD HandlerAddress;
        DWORD JumpTarget;
    } ScopeRecord[1];
} SCOPE_TABLE_ARM64, *PSCOPE_TABLE_ARM64;

#include "winnt_arm64.h"

#include "winnt_x86.h"

#if defined (_AMD64_) || defined(_ARM_) || defined(_ARM64_)

//
// Define unwind history table structure.
//

#define UNWIND_HISTORY_TABLE_SIZE 12

typedef struct _UNWIND_HISTORY_TABLE_ENTRY {
    ULONG_PTR ImageBase;
    PRUNTIME_FUNCTION FunctionEntry;
} UNWIND_HISTORY_TABLE_ENTRY, *PUNWIND_HISTORY_TABLE_ENTRY;

typedef struct _UNWIND_HISTORY_TABLE {
    DWORD Count;
    BYTE  LocalHint;
    BYTE  GlobalHint;
    BYTE  Search;
    BYTE  Once;
    ULONG_PTR LowAddress;
    ULONG_PTR HighAddress;
    UNWIND_HISTORY_TABLE_ENTRY Entry[UNWIND_HISTORY_TABLE_SIZE];
} UNWIND_HISTORY_TABLE, *PUNWIND_HISTORY_TABLE;

#endif

#if !defined(_LDK_NTXCAPI_)
#pragma region Application or OneCore Family or Games Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES)

NTSYSAPI
VOID
NTAPI
RtlUnwind(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES) */
#pragma endregion
#endif

#include "winnt_amd64_2.h"
#include "winnt_arm_2.h"
#include "winnt_arm64_2.h"
#include "winnt_x86_2.h"



typedef struct _JOBOBJECT_BASIC_ACCOUNTING_INFORMATION {
    LARGE_INTEGER TotalUserTime;
    LARGE_INTEGER TotalKernelTime;
    LARGE_INTEGER ThisPeriodTotalUserTime;
    LARGE_INTEGER ThisPeriodTotalKernelTime;
    DWORD TotalPageFaultCount;
    DWORD TotalProcesses;
    DWORD ActiveProcesses;
    DWORD TotalTerminatedProcesses;
} JOBOBJECT_BASIC_ACCOUNTING_INFORMATION, *PJOBOBJECT_BASIC_ACCOUNTING_INFORMATION;

//@[comment("MVI_tracked")]
typedef struct _JOBOBJECT_BASIC_LIMIT_INFORMATION {
    LARGE_INTEGER PerProcessUserTimeLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    DWORD LimitFlags;
    SIZE_T MinimumWorkingSetSize;
    SIZE_T MaximumWorkingSetSize;
    DWORD ActiveProcessLimit;
    ULONG_PTR Affinity;
    DWORD PriorityClass;
    DWORD SchedulingClass;
} JOBOBJECT_BASIC_LIMIT_INFORMATION, *PJOBOBJECT_BASIC_LIMIT_INFORMATION;

//@[comment("MVI_tracked")]
typedef struct _JOBOBJECT_EXTENDED_LIMIT_INFORMATION {
    JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
    IO_COUNTERS IoInfo;
    SIZE_T ProcessMemoryLimit;
    SIZE_T JobMemoryLimit;
    SIZE_T PeakProcessMemoryUsed;
    SIZE_T PeakJobMemoryUsed;
} JOBOBJECT_EXTENDED_LIMIT_INFORMATION, *PJOBOBJECT_EXTENDED_LIMIT_INFORMATION;

//
//

//@[comment("MVI_tracked")]
typedef struct _JOBOBJECT_BASIC_PROCESS_ID_LIST {
    DWORD NumberOfAssignedProcesses;
    DWORD NumberOfProcessIdsInList;
    ULONG_PTR ProcessIdList[1];
} JOBOBJECT_BASIC_PROCESS_ID_LIST, *PJOBOBJECT_BASIC_PROCESS_ID_LIST;

typedef struct _JOBOBJECT_BASIC_UI_RESTRICTIONS {
    DWORD UIRestrictionsClass;
} JOBOBJECT_BASIC_UI_RESTRICTIONS, *PJOBOBJECT_BASIC_UI_RESTRICTIONS;

//
// N.B. The JOBOBJECT_SECURITY_LIMIT_INFORMATION information class is no longer supported.
//

typedef struct _JOBOBJECT_SECURITY_LIMIT_INFORMATION {
    DWORD SecurityLimitFlags ;
    HANDLE JobToken ;
    PTOKEN_GROUPS SidsToDisable ;
    PTOKEN_PRIVILEGES PrivilegesToDelete ;
    PTOKEN_GROUPS RestrictedSids ;
} JOBOBJECT_SECURITY_LIMIT_INFORMATION, *PJOBOBJECT_SECURITY_LIMIT_INFORMATION ;

typedef struct _JOBOBJECT_END_OF_JOB_TIME_INFORMATION {
    DWORD EndOfJobTimeAction;
} JOBOBJECT_END_OF_JOB_TIME_INFORMATION, *PJOBOBJECT_END_OF_JOB_TIME_INFORMATION;

typedef struct _JOBOBJECT_ASSOCIATE_COMPLETION_PORT {
    PVOID CompletionKey;
    HANDLE CompletionPort;
} JOBOBJECT_ASSOCIATE_COMPLETION_PORT, *PJOBOBJECT_ASSOCIATE_COMPLETION_PORT;

typedef struct _JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION {
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION BasicInfo;
    IO_COUNTERS IoInfo;
} JOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION, *PJOBOBJECT_BASIC_AND_IO_ACCOUNTING_INFORMATION;

typedef struct _JOBOBJECT_JOBSET_INFORMATION {
    DWORD MemberLevel;
} JOBOBJECT_JOBSET_INFORMATION, *PJOBOBJECT_JOBSET_INFORMATION;

typedef enum _JOBOBJECT_RATE_CONTROL_TOLERANCE {
    ToleranceLow = 1,
    ToleranceMedium,
    ToleranceHigh
} JOBOBJECT_RATE_CONTROL_TOLERANCE, *PJOBOBJECT_RATE_CONTROL_TOLERANCE;

typedef enum _JOBOBJECT_RATE_CONTROL_TOLERANCE_INTERVAL {
    ToleranceIntervalShort = 1,
    ToleranceIntervalMedium,
    ToleranceIntervalLong
} JOBOBJECT_RATE_CONTROL_TOLERANCE_INTERVAL,
  *PJOBOBJECT_RATE_CONTROL_TOLERANCE_INTERVAL;

typedef struct _JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION {
    DWORD64 IoReadBytesLimit;
    DWORD64 IoWriteBytesLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    DWORD64 JobMemoryLimit;
    JOBOBJECT_RATE_CONTROL_TOLERANCE RateControlTolerance;
    JOBOBJECT_RATE_CONTROL_TOLERANCE_INTERVAL RateControlToleranceInterval;
    DWORD LimitFlags;
} JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION, *PJOBOBJECT_NOTIFICATION_LIMIT_INFORMATION;

typedef struct JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION_2 {
    DWORD64 IoReadBytesLimit;
    DWORD64 IoWriteBytesLimit;
    LARGE_INTEGER PerJobUserTimeLimit;
    union {
        DWORD64 JobHighMemoryLimit;
        DWORD64 JobMemoryLimit;
    } DUMMYUNIONNAME;

    union {
        JOBOBJECT_RATE_CONTROL_TOLERANCE RateControlTolerance;
        JOBOBJECT_RATE_CONTROL_TOLERANCE CpuRateControlTolerance;
    } DUMMYUNIONNAME2;

    union {
        JOBOBJECT_RATE_CONTROL_TOLERANCE_INTERVAL RateControlToleranceInterval;
        JOBOBJECT_RATE_CONTROL_TOLERANCE_INTERVAL
            CpuRateControlToleranceInterval;
    } DUMMYUNIONNAME3;

    DWORD LimitFlags;
    JOBOBJECT_RATE_CONTROL_TOLERANCE IoRateControlTolerance;
    DWORD64 JobLowMemoryLimit;
    JOBOBJECT_RATE_CONTROL_TOLERANCE_INTERVAL IoRateControlToleranceInterval;
    JOBOBJECT_RATE_CONTROL_TOLERANCE NetRateControlTolerance;
    JOBOBJECT_RATE_CONTROL_TOLERANCE_INTERVAL NetRateControlToleranceInterval;
} JOBOBJECT_NOTIFICATION_LIMIT_INFORMATION_2;

//
//

typedef struct _JOBOBJECT_LIMIT_VIOLATION_INFORMATION {
    DWORD LimitFlags;
    DWORD ViolationLimitFlags;
    DWORD64 IoReadBytes;
    DWORD64 IoReadBytesLimit;
    DWORD64 IoWriteBytes;
    DWORD64 IoWriteBytesLimit;
    LARGE_INTEGER PerJobUserTime;
    LARGE_INTEGER PerJobUserTimeLimit;
    DWORD64 JobMemory;
    DWORD64 JobMemoryLimit;
    JOBOBJECT_RATE_CONTROL_TOLERANCE RateControlTolerance;
    JOBOBJECT_RATE_CONTROL_TOLERANCE RateControlToleranceLimit;
} JOBOBJECT_LIMIT_VIOLATION_INFORMATION, *PJOBOBJECT_LIMIT_VIOLATION_INFORMATION;

typedef struct JOBOBJECT_LIMIT_VIOLATION_INFORMATION_2 {
    DWORD LimitFlags;
    DWORD ViolationLimitFlags;
    DWORD64 IoReadBytes;
    DWORD64 IoReadBytesLimit;
    DWORD64 IoWriteBytes;
    DWORD64 IoWriteBytesLimit;
    LARGE_INTEGER PerJobUserTime;
    LARGE_INTEGER PerJobUserTimeLimit;
    DWORD64 JobMemory;
    union {
        DWORD64 JobHighMemoryLimit;
        DWORD64 JobMemoryLimit;
    } DUMMYUNIONNAME;

    union {
        JOBOBJECT_RATE_CONTROL_TOLERANCE RateControlTolerance;
        JOBOBJECT_RATE_CONTROL_TOLERANCE CpuRateControlTolerance;
    } DUMMYUNIONNAME2;

    union {
        JOBOBJECT_RATE_CONTROL_TOLERANCE RateControlToleranceLimit;
        JOBOBJECT_RATE_CONTROL_TOLERANCE CpuRateControlToleranceLimit;
    } DUMMYUNIONNAME3;

    DWORD64 JobLowMemoryLimit;
    JOBOBJECT_RATE_CONTROL_TOLERANCE IoRateControlTolerance;
    JOBOBJECT_RATE_CONTROL_TOLERANCE IoRateControlToleranceLimit;
    JOBOBJECT_RATE_CONTROL_TOLERANCE NetRateControlTolerance;
    JOBOBJECT_RATE_CONTROL_TOLERANCE NetRateControlToleranceLimit;
} JOBOBJECT_LIMIT_VIOLATION_INFORMATION_2;

//
//

typedef struct _JOBOBJECT_CPU_RATE_CONTROL_INFORMATION {
    DWORD ControlFlags;
    union {
        DWORD CpuRate;
        DWORD Weight;
        struct {
            WORD   MinRate;
            WORD   MaxRate;
        } DUMMYSTRUCTNAME;
    } DUMMYUNIONNAME;
} JOBOBJECT_CPU_RATE_CONTROL_INFORMATION, *PJOBOBJECT_CPU_RATE_CONTROL_INFORMATION;

//
// Control flags for network rate control.
//

typedef enum JOB_OBJECT_NET_RATE_CONTROL_FLAGS {
    JOB_OBJECT_NET_RATE_CONTROL_ENABLE = 0x1,
    JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH = 0x2,
    JOB_OBJECT_NET_RATE_CONTROL_DSCP_TAG = 0x4,
    JOB_OBJECT_NET_RATE_CONTROL_VALID_FLAGS = 0x7
} JOB_OBJECT_NET_RATE_CONTROL_FLAGS;

#if !defined(SORTPP_PASS) && !defined(MIDL_PASS) && !defined(RC_INVOKED)

DEFINE_ENUM_FLAG_OPERATORS(JOB_OBJECT_NET_RATE_CONTROL_FLAGS)
C_ASSERT(JOB_OBJECT_NET_RATE_CONTROL_VALID_FLAGS ==
             (JOB_OBJECT_NET_RATE_CONTROL_ENABLE +
              JOB_OBJECT_NET_RATE_CONTROL_MAX_BANDWIDTH +
              JOB_OBJECT_NET_RATE_CONTROL_DSCP_TAG));

#endif

#define JOB_OBJECT_NET_RATE_CONTROL_MAX_DSCP_TAG 64

typedef struct JOBOBJECT_NET_RATE_CONTROL_INFORMATION {
    DWORD64 MaxBandwidth;
    JOB_OBJECT_NET_RATE_CONTROL_FLAGS ControlFlags;
    BYTE  DscpTag;
} JOBOBJECT_NET_RATE_CONTROL_INFORMATION;

//
//

//
// Control flags for IO rate control.
//

typedef enum JOB_OBJECT_IO_RATE_CONTROL_FLAGS {
    JOB_OBJECT_IO_RATE_CONTROL_ENABLE = 0x1,
    JOB_OBJECT_IO_RATE_CONTROL_STANDALONE_VOLUME = 0x2,
    JOB_OBJECT_IO_RATE_CONTROL_FORCE_UNIT_ACCESS_ALL = 0x4,
    JOB_OBJECT_IO_RATE_CONTROL_FORCE_UNIT_ACCESS_ON_SOFT_CAP = 0x8,
    JOB_OBJECT_IO_RATE_CONTROL_VALID_FLAGS = JOB_OBJECT_IO_RATE_CONTROL_ENABLE |
                                             JOB_OBJECT_IO_RATE_CONTROL_STANDALONE_VOLUME |
                                             JOB_OBJECT_IO_RATE_CONTROL_FORCE_UNIT_ACCESS_ALL |
                                             JOB_OBJECT_IO_RATE_CONTROL_FORCE_UNIT_ACCESS_ON_SOFT_CAP
} JOB_OBJECT_IO_RATE_CONTROL_FLAGS;

#if !defined(SORTPP_PASS) && !defined(MIDL_PASS) && !defined(RC_INVOKED)

DEFINE_ENUM_FLAG_OPERATORS(JOB_OBJECT_IO_RATE_CONTROL_FLAGS)

#endif

typedef struct JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE {
    LONG64 MaxIops;
    LONG64 MaxBandwidth;
    LONG64 ReservationIops;
    PWSTR VolumeName;
    DWORD BaseIoSize;
    JOB_OBJECT_IO_RATE_CONTROL_FLAGS ControlFlags;
    WORD   VolumeNameLength;
} JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE;

typedef JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE
    JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V1;

typedef struct JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2 {
    LONG64 MaxIops;
    LONG64 MaxBandwidth;
    LONG64 ReservationIops;
    PWSTR VolumeName;
    DWORD BaseIoSize;
    JOB_OBJECT_IO_RATE_CONTROL_FLAGS ControlFlags;
    WORD   VolumeNameLength;
    LONG64 CriticalReservationIops;
    LONG64 ReservationBandwidth;
    LONG64 CriticalReservationBandwidth;
    LONG64 MaxTimePercent;
    LONG64 ReservationTimePercent;
    LONG64 CriticalReservationTimePercent;
} JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V2;

typedef struct JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V3 {
    LONG64 MaxIops;
    LONG64 MaxBandwidth;
    LONG64 ReservationIops;
    PWSTR VolumeName;
    DWORD BaseIoSize;
    JOB_OBJECT_IO_RATE_CONTROL_FLAGS ControlFlags;
    WORD   VolumeNameLength;
    LONG64 CriticalReservationIops;
    LONG64 ReservationBandwidth;
    LONG64 CriticalReservationBandwidth;
    LONG64 MaxTimePercent;
    LONG64 ReservationTimePercent;
    LONG64 CriticalReservationTimePercent;
    LONG64 SoftMaxIops;
    LONG64 SoftMaxBandwidth;
    LONG64 SoftMaxTimePercent;
    LONG64 LimitExcessNotifyIops;
    LONG64 LimitExcessNotifyBandwidth;
    LONG64 LimitExcessNotifyTimePercent;
} JOBOBJECT_IO_RATE_CONTROL_INFORMATION_NATIVE_V3;

//
//

typedef enum JOBOBJECT_IO_ATTRIBUTION_CONTROL_FLAGS {
    JOBOBJECT_IO_ATTRIBUTION_CONTROL_ENABLE = 0x1,
    JOBOBJECT_IO_ATTRIBUTION_CONTROL_DISABLE = 0x2,
    JOBOBJECT_IO_ATTRIBUTION_CONTROL_VALID_FLAGS = 0x3
} JOBOBJECT_IO_ATTRIBUTION_CONTROL_FLAGS;

typedef struct _JOBOBJECT_IO_ATTRIBUTION_STATS {

    ULONG_PTR IoCount;
    ULONGLONG TotalNonOverlappedQueueTime;
    ULONGLONG TotalNonOverlappedServiceTime;
    ULONGLONG TotalSize;

} JOBOBJECT_IO_ATTRIBUTION_STATS, *PJOBOBJECT_IO_ATTRIBUTION_STATS;

typedef struct _JOBOBJECT_IO_ATTRIBUTION_INFORMATION {
    DWORD ControlFlags;

    JOBOBJECT_IO_ATTRIBUTION_STATS ReadStats;
    JOBOBJECT_IO_ATTRIBUTION_STATS WriteStats;

} JOBOBJECT_IO_ATTRIBUTION_INFORMATION, *PJOBOBJECT_IO_ATTRIBUTION_INFORMATION;

//
//

#define JOB_OBJECT_TERMINATE_AT_END_OF_JOB  0
#define JOB_OBJECT_POST_AT_END_OF_JOB       1

//
// Completion Port Messages for job objects
//
// These values are returned via the lpNumberOfBytesTransferred parameter
//

#define JOB_OBJECT_MSG_END_OF_JOB_TIME          1
#define JOB_OBJECT_MSG_END_OF_PROCESS_TIME      2
#define JOB_OBJECT_MSG_ACTIVE_PROCESS_LIMIT     3
#define JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO      4
#define JOB_OBJECT_MSG_NEW_PROCESS              6
#define JOB_OBJECT_MSG_EXIT_PROCESS             7
#define JOB_OBJECT_MSG_ABNORMAL_EXIT_PROCESS    8
#define JOB_OBJECT_MSG_PROCESS_MEMORY_LIMIT     9
#define JOB_OBJECT_MSG_JOB_MEMORY_LIMIT         10
#define JOB_OBJECT_MSG_NOTIFICATION_LIMIT       11
#define JOB_OBJECT_MSG_JOB_CYCLE_TIME_LIMIT     12
#define JOB_OBJECT_MSG_SILO_TERMINATED          13

//
// Define the valid notification filter values.
//

#define JOB_OBJECT_MSG_MINIMUM 1
#define JOB_OBJECT_MSG_MAXIMUM 13

#define JOB_OBJECT_VALID_COMPLETION_FILTER \
    (((1UL << (JOB_OBJECT_MSG_MAXIMUM + 1)) - 1) - \
     ((1UL << JOB_OBJECT_MSG_MINIMUM) - 1))

//
// Basic Limits
//
#define JOB_OBJECT_LIMIT_WORKINGSET                 0x00000001
#define JOB_OBJECT_LIMIT_PROCESS_TIME               0x00000002
#define JOB_OBJECT_LIMIT_JOB_TIME                   0x00000004
#define JOB_OBJECT_LIMIT_ACTIVE_PROCESS             0x00000008
#define JOB_OBJECT_LIMIT_AFFINITY                   0x00000010
#define JOB_OBJECT_LIMIT_PRIORITY_CLASS             0x00000020
#define JOB_OBJECT_LIMIT_PRESERVE_JOB_TIME          0x00000040
#define JOB_OBJECT_LIMIT_SCHEDULING_CLASS           0x00000080

//
// Extended Limits
//
#define JOB_OBJECT_LIMIT_PROCESS_MEMORY             0x00000100
#define JOB_OBJECT_LIMIT_JOB_MEMORY                 0x00000200
#define JOB_OBJECT_LIMIT_JOB_MEMORY_HIGH            JOB_OBJECT_LIMIT_JOB_MEMORY
#define JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION 0x00000400
#define JOB_OBJECT_LIMIT_BREAKAWAY_OK               0x00000800
#define JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK        0x00001000
#define JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE          0x00002000
#define JOB_OBJECT_LIMIT_SUBSET_AFFINITY            0x00004000
#define JOB_OBJECT_LIMIT_JOB_MEMORY_LOW             0x00008000

//
// Notification Limits
//

#define JOB_OBJECT_LIMIT_JOB_READ_BYTES             0x00010000
#define JOB_OBJECT_LIMIT_JOB_WRITE_BYTES            0x00020000
#define JOB_OBJECT_LIMIT_RATE_CONTROL               0x00040000
#define JOB_OBJECT_LIMIT_CPU_RATE_CONTROL           JOB_OBJECT_LIMIT_RATE_CONTROL
#define JOB_OBJECT_LIMIT_IO_RATE_CONTROL            0x00080000
#define JOB_OBJECT_LIMIT_NET_RATE_CONTROL           0x00100000

//
//

//
// Valid Job Object Limits
//

#define JOB_OBJECT_LIMIT_VALID_FLAGS                 0x0007ffff
#define JOB_OBJECT_BASIC_LIMIT_VALID_FLAGS           0x000000ff
#define JOB_OBJECT_EXTENDED_LIMIT_VALID_FLAGS        0x00007fff
#define JOB_OBJECT_NOTIFICATION_LIMIT_VALID_FLAGS \
    (JOB_OBJECT_LIMIT_JOB_READ_BYTES | \
     JOB_OBJECT_LIMIT_JOB_WRITE_BYTES | \
     JOB_OBJECT_LIMIT_JOB_TIME | \
     JOB_OBJECT_LIMIT_JOB_MEMORY_LOW | \
     JOB_OBJECT_LIMIT_JOB_MEMORY_HIGH | \
     JOB_OBJECT_LIMIT_CPU_RATE_CONTROL | \
     JOB_OBJECT_LIMIT_IO_RATE_CONTROL | \
     JOB_OBJECT_LIMIT_NET_RATE_CONTROL)

//
//

//
// UI restrictions for jobs
//

#define JOB_OBJECT_UILIMIT_NONE             0x00000000

#define JOB_OBJECT_UILIMIT_HANDLES          0x00000001
#define JOB_OBJECT_UILIMIT_READCLIPBOARD    0x00000002
#define JOB_OBJECT_UILIMIT_WRITECLIPBOARD   0x00000004
#define JOB_OBJECT_UILIMIT_SYSTEMPARAMETERS 0x00000008
#define JOB_OBJECT_UILIMIT_DISPLAYSETTINGS  0x00000010
#define JOB_OBJECT_UILIMIT_GLOBALATOMS      0x00000020
#define JOB_OBJECT_UILIMIT_DESKTOP          0x00000040
#define JOB_OBJECT_UILIMIT_EXITWINDOWS      0x00000080

#define JOB_OBJECT_UILIMIT_ALL              0x000000FF

#define JOB_OBJECT_UI_VALID_FLAGS           0x000000FF

#define JOB_OBJECT_SECURITY_NO_ADMIN            0x00000001
#define JOB_OBJECT_SECURITY_RESTRICTED_TOKEN    0x00000002
#define JOB_OBJECT_SECURITY_ONLY_TOKEN          0x00000004
#define JOB_OBJECT_SECURITY_FILTER_TOKENS       0x00000008

#define JOB_OBJECT_SECURITY_VALID_FLAGS         0x0000000f

//
// Control flags for CPU rate control.
//

#define JOB_OBJECT_CPU_RATE_CONTROL_ENABLE 0x1
#define JOB_OBJECT_CPU_RATE_CONTROL_WEIGHT_BASED 0x2
#define JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP 0x4
#define JOB_OBJECT_CPU_RATE_CONTROL_NOTIFY 0x8
#define JOB_OBJECT_CPU_RATE_CONTROL_MIN_MAX_RATE 0x10
#define JOB_OBJECT_CPU_RATE_CONTROL_VALID_FLAGS 0x1f

//
//

//@[comment("MVI_tracked")]
typedef enum _JOBOBJECTINFOCLASS {
    JobObjectBasicAccountingInformation = 1,
    JobObjectBasicLimitInformation,
    JobObjectBasicProcessIdList,
    JobObjectBasicUIRestrictions,
    JobObjectSecurityLimitInformation,  // deprecated
    JobObjectEndOfJobTimeInformation,
    JobObjectAssociateCompletionPortInformation,
    JobObjectBasicAndIoAccountingInformation,
    JobObjectExtendedLimitInformation,
    JobObjectJobSetInformation,
    JobObjectGroupInformation,
    JobObjectNotificationLimitInformation,
    JobObjectLimitViolationInformation,
    JobObjectGroupInformationEx,
    JobObjectCpuRateControlInformation,
    JobObjectCompletionFilter,
    JobObjectCompletionCounter,

//
//

    JobObjectReserved1Information = 18,
    JobObjectReserved2Information,
    JobObjectReserved3Information,
    JobObjectReserved4Information,
    JobObjectReserved5Information,
    JobObjectReserved6Information,
    JobObjectReserved7Information,
    JobObjectReserved8Information,
    JobObjectReserved9Information,
    JobObjectReserved10Information,
    JobObjectReserved11Information,
    JobObjectReserved12Information,
    JobObjectReserved13Information,
    JobObjectReserved14Information = 31,
    JobObjectNetRateControlInformation,
    JobObjectNotificationLimitInformation2,
    JobObjectLimitViolationInformation2,
    JobObjectCreateSilo,
    JobObjectSiloBasicInformation,
    JobObjectReserved15Information = 37,
    JobObjectReserved16Information = 38,
    JobObjectReserved17Information = 39,
    JobObjectReserved18Information = 40,
    JobObjectReserved19Information = 41,
    JobObjectReserved20Information = 42,
    JobObjectReserved21Information = 43,
    JobObjectReserved22Information = 44,
    JobObjectReserved23Information = 45,
    JobObjectReserved24Information = 46,
    JobObjectReserved25Information = 47,
    MaxJobObjectInfoClass
} JOBOBJECTINFOCLASS;



#ifndef MEM_COMMIT
#define MEM_COMMIT                  0x00001000
#endif
#ifndef MEM_RESERVE
#define MEM_RESERVE                 0x00002000
#endif
#ifndef MEM_DECOMMIT
#define MEM_DECOMMIT                0x00004000
#endif
#ifndef MEM_RELEASE
#define MEM_RELEASE                 0x00008000
#endif
#ifndef MEM_FREE
#define MEM_FREE                    0x00010000
#endif
#define MEM_PRIVATE                 0x00020000  
#define MEM_MAPPED                  0x00040000  
#define MEM_IMAGE                   0x01000000  
#define WRITE_WATCH_FLAG_RESET  0x01    



#define THREAD_BASE_PRIORITY_LOWRT  15  // value that gets a thread to LowRealtime-1
#define THREAD_BASE_PRIORITY_MAX    2   // maximum thread base priority boost
#define THREAD_BASE_PRIORITY_MIN    (-2)  // minimum thread base priority boost
#define THREAD_BASE_PRIORITY_IDLE   (-15) // value that gets a thread to idle

#pragma warning(default:4201 4214)

EXTERN_C_END

#endif // _WINNT_
