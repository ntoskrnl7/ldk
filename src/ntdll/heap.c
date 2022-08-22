#include "ntdll.h"


NTSTATUS
LdkpInitializeHeapList (
	VOID
	);


#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeHeapList)
#endif



#ifndef LDK_ENABLE_HEAP_STACK_TRACE
#define LDK_ENABLE_HEAP_STACK_TRACE	0
#endif

ULONG NtdllBaseTag;



//
// 커널에서도 힙을 사용할 수 있으나 NtAllocateVirtualMemory루틴이 사용되었기때문에
// 프로세스 컨텍스트(PDE)에 영향을 받으므로 비페이징풀로 할당되도록 직접 구현했습니다.
// (힙 관련 루틴 : RtlCreateHeap, RtlDestroyHeap, RtlAllocateHeap, RtlFreeHeap 등)
// 

typedef struct _LDK_HEAP_HEADER {
	HANDLE HeapHandle;
	ULONG Tag;
	SIZE_T Size;
	LIST_ENTRY Links;
#if LDK_ENABLE_HEAP_STACK_TRACE
	CONTEXT ContextRecord;
	PVOID StackData[1024];
#endif
} LDK_HEAP_HEADER, *PLDK_HEAP_HEADER;

#define LDK_HEAP_HEADER_SIZE		    sizeof(LDK_HEAP_HEADER)

#define LDK_HEAP_IMP_GET_HEADER(H)		    ((PLDK_HEAP_HEADER)(((ULONG_PTR)(H)) - LDK_HEAP_HEADER_SIZE))
#define LDK_HEAP_IMP_GET_BUFFER(H)		    Add2Ptr((H), LDK_HEAP_HEADER_SIZE)
#define LDK_HEAP_IMP_GET_BUFFER_SIZE(H)	



EX_SPIN_LOCK LdkpHeapListLock;
LIST_ENTRY LdkpHeapListHead;

NTSTATUS
LdkpInitializeHeapList (
	VOID
	)
{
	PAGED_CODE();

	LdkpHeapListLock = 0;
	InitializeListHead( &LdkpHeapListHead );
	return STATUS_SUCCESS;
}


VOID
LdkpTerminateHeapList (
	VOID
	)
{
	PAGED_CODE();

	KIRQL OldIrql = ExAcquireSpinLockExclusive( &LdkpHeapListLock );

	PLIST_ENTRY Entry = RemoveTailList( &LdkpHeapListHead );
	PLDK_HEAP_HEADER Header;
	while (Entry != &LdkpHeapListHead) {
		Header = CONTAINING_RECORD(Entry, LDK_HEAP_HEADER, Links);
		Entry = RemoveHeadList( &LdkpHeapListHead );
		ExFreePoolWithTag( Header,
						   Header->Tag );
	}

	ExReleaseSpinLockExclusive( &LdkpHeapListLock,
								OldIrql );

}



LONG LdkpHeapHandle = 0;



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
    )
{
    UNREFERENCED_PARAMETER(Flags);
    UNREFERENCED_PARAMETER(HeapBase);
    UNREFERENCED_PARAMETER(ReserveSize);
    UNREFERENCED_PARAMETER(CommitSize);
    UNREFERENCED_PARAMETER(Lock);
    UNREFERENCED_PARAMETER(Parameters);
	return LongToHandle(InterlockedIncrement( &LdkpHeapHandle ));
}

PVOID
NTAPI
LdkDestroyHeap (
    _In_ _Post_invalid_ PVOID HeapHandle
    )
{
    UNREFERENCED_PARAMETER(HeapHandle);
    return NULL;
}

_Must_inspect_result_
_Ret_maybenull_
_Post_writable_byte_size_(Size)
PVOID
NTAPI
LdkAllocateHeap (
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _In_ SIZE_T Size
    )
{
	union {
		struct {
#pragma warning(disable:4214)
			ULONG Value0: 8;
			ULONG Value1: 8;
			ULONG Value2: 8;
			ULONG Value3: 8;
#pragma warning(default:4214)
		} Part;
		ULONG Value;
	} Tag;
	Tag.Value = HandleToUlong(HeapHandle);

#define TAG_CHAR_RANGE		('~' - 'A')
	while (Tag.Part.Value0 > TAG_CHAR_RANGE) {
		Tag.Part.Value0 -= TAG_CHAR_RANGE;
		Tag.Part.Value1++;
	}
	while (Tag.Part.Value1 > TAG_CHAR_RANGE) {
		Tag.Part.Value1 -= TAG_CHAR_RANGE;
		Tag.Part.Value2++;
	}
	while (Tag.Part.Value2 > TAG_CHAR_RANGE) {
		Tag.Part.Value2 -= TAG_CHAR_RANGE;
		Tag.Part.Value3++;
	}
	if (Tag.Part.Value3 > TAG_CHAR_RANGE) {
		KdBreakPoint();
		return NULL;
	}
#undef TAG_CHAR_RANGE
	Tag.Part.Value0 += 'A';
	Tag.Part.Value1 += 'A';
	Tag.Part.Value2 += 'A';
	Tag.Part.Value3 += 'A';
#pragma warning(disable:4996)
	PLDK_HEAP_HEADER Header = ExAllocatePoolWithTag( LdkpDefaultPoolType,
														 Size + LDK_HEAP_HEADER_SIZE,
														 Tag.Value );
#pragma warning(default:4996)
	if (! Header) {
		return NULL;
	}
	Header->HeapHandle = HeapHandle;
	Header->Tag = Tag.Value;
	Header->Size = Size;

#if LDK_ENABLE_HEAP_STACK_TRACE
	RtlCaptureContext( &Header->ContextRecord );

    ULONG_PTR Top;
    ULONG_PTR Bottom;
    IoGetStackLimits( &Bottom,
					  &Top );
	ULONG_PTR Sp
#if defined(_AMD64__)
		= (ULONG_PTR)Header->ContextRecord.Rsp;
#elif defined(_X86_)
		= (ULONG_PTR)Header->ContextRecord.Esp;
#elif defined(_ARM_) || defined(_ARM64_)
		= (ULONG_PTR)Header->ContextRecord.Sp;
#else
#error "Not Supported Target Architecture"
#endif
	SIZE_T Length = min((SIZE_T)(Top - Sp), sizeof(Header->StackData));
	RtlCopyMemory( Header->StackData,
				   (PVOID)Sp,
				   Length );
#endif // LDK_ENABLE_HEAP_STACK_TRACE

	if (FlagOn(Flags, HEAP_ZERO_MEMORY)) {
		RtlZeroMemory( LDK_HEAP_IMP_GET_BUFFER(Header),
					   Size );
	}

	KIRQL OldIrql = ExAcquireSpinLockExclusive( &LdkpHeapListLock );
	InsertTailList( &LdkpHeapListHead,
					&Header->Links );
	ExReleaseSpinLockExclusive( &LdkpHeapListLock,
								OldIrql );

	return LDK_HEAP_IMP_GET_BUFFER(Header);
}

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
    )
{
	if (! ARGUMENT_PRESENT(BaseAddress)) {
		return NULL;
	}

	PVOID NewBuffer;
	PLDK_HEAP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(BaseAddress);
	SIZE_T OldSize;

	if (Header->HeapHandle != HeapHandle) {
		return NULL;
	}

	NewBuffer = LdkAllocateHeap( HeapHandle,
						   		 Flags,
						   		 Size );
	if (NewBuffer == NULL) {
		return NULL;
	}

	OldSize = Header->Size;

	if (OldSize > Size) {
		RtlCopyMemory( NewBuffer,
						BaseAddress,
						Size );
	} else {			
		RtlCopyMemory( NewBuffer,
					   BaseAddress,
					   OldSize );
		if (FlagOn(Flags, HEAP_ZERO_MEMORY)) {
			RtlZeroMemory( Add2Ptr(NewBuffer, OldSize),
						   Size - OldSize );
		}
	}
	LdkFreeHeap( HeapHandle,
			     0,
			     BaseAddress );
	return NewBuffer;
}

_Success_(return != 0)
LOGICAL
NTAPI
LdkFreeHeap (
    _In_ PVOID HeapHandle,
    _In_opt_ ULONG Flags,
    _Frees_ptr_opt_ PVOID BaseAddress
    )
{
	UNREFERENCED_PARAMETER(HeapHandle);
	UNREFERENCED_PARAMETER(Flags);
	if (BaseAddress) {
		PLDK_HEAP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(BaseAddress);
		KIRQL OldIrql = ExAcquireSpinLockExclusive( &LdkpHeapListLock );
		RemoveEntryList( &Header->Links );
		ExReleaseSpinLockExclusive( &LdkpHeapListLock,
									OldIrql );
		ExFreePoolWithTag( Header,
						   Header->Tag );
	}
	return TRUE;
}

SIZE_T
NTAPI
LdkSizeHeap (
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ LPCVOID BaseAddress
    )
{
	UNREFERENCED_PARAMETER(Flags);
	if (ARGUMENT_PRESENT(BaseAddress)) {
		PLDK_HEAP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(BaseAddress);
		if (Header->HeapHandle != HeapHandle) {
			return (SIZE_T)-1;
		}
		return Header->Size;
	}

	return (SIZE_T)-1;
}

BOOLEAN
NTAPI
LdkValidateHeap (
    _In_ PVOID HeapHandle,
    _In_ ULONG Flags,
    _In_ LPCVOID BaseAddress
    )
{
	UNREFERENCED_PARAMETER(Flags);

	if (! ARGUMENT_PRESENT(BaseAddress)) {
		return FALSE;
	}

	if (MmIsAddressValid( (PVOID)BaseAddress) ) {
		PLDK_HEAP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(BaseAddress);
		return (Header->HeapHandle == HeapHandle);
	}

	return FALSE;
}