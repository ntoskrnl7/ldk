#pragma once

#include <Ldk/ntdll.h>
#include "../ldk.h"

extern ULONG NtdllBaseTag;

#define TMP_TAG 0
#define MAKE_TAG( t ) (RTL_HEAP_MAKE_TAG( NtdllBaseTag, t ))

_Must_inspect_result_
PVOID
NTAPI
LdkCreateHeap (
    _In_     ULONG Flags,
    _In_opt_ PVOID HeapBase,
    _In_opt_ SIZE_T ReserveSize,
    _In_opt_ SIZE_T CommitSize,
    _In_opt_ PVOID Lock,
    _When_((Flags & 0x100) != 0,
           _In_reads_bytes_opt_(sizeof(RTL_SEGMENT_HEAP_PARAMETERS)))
    _When_((Flags & 0x100) == 0,
           _In_reads_bytes_opt_(sizeof(RTL_HEAP_PARAMETERS)))
    PRTL_HEAP_PARAMETERS Parameters
    );

PVOID
NTAPI
LdkDestroyHeap (
    _In_ _Post_invalid_ PVOID HeapHandle
    );

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
NTAPI
LdkAllocateHeap (
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size
    );

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
NTAPI
LdkReAllocateHeap (
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ PVOID BaseAddress,
    _In_ SIZE_T Size
    );

_Success_(return != 0)
LOGICAL
NTAPI
LdkFreeHeap (
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress
    );

BOOLEAN
NTAPI
LdkValidateHeap (
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ LPCVOID BaseAddress
    );

SIZE_T
NTAPI
LdkSizeHeap (
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ LPCVOID BaseAddress
    );

#define RtlCreateHeap               LdkCreateHeap
#define RtlAllocateHeap             LdkAllocateHeap
#define RtlReAllocateHeap		    LdkReAllocateHeap
#define RtlValidateHeap             LdkValidateHeap
#define RtlFreeHeap		            LdkFreeHeap
#define RtlSizeHeap                 LdkSizeHeap
#define RtlDestroyHeap              LdkDestroyHeap
#define ZwCreateNamedPipeFile       NtCreateNamedPipeFile