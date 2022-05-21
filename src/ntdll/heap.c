#include "ntdll.h"

ULONG NtdllBaseTag;

//
// 커널에서도 힙을 사용할 수 있으나 NtAllocateVirtualMemory루틴이 사용되었기때문에
// 프로세스 컨텍스트(PDE)에 영향을 받으므로 비페이징풀로 할당되도록 직접 구현했습니다.
// (힙 관련 루틴 : RtlCreateHeap, RtlDestroyHeap, RtlAllocateHeap, RtlFreeHeap 등)
// 

typedef struct _LDK_HEAP_IMP_HEADER {
	HANDLE HeapHandle;
	ULONG Tag;
	SIZE_T Size;
} LDK_HEAP_IMP_HEADER, *PLDK_HEAP_IMP_HEADER;

#define LDK_HEAP_IMP_HEADER_SIZE		    sizeof(LDK_HEAP_IMP_HEADER)

#define LDK_HEAP_IMP_GET_HEADER(H)		    ((PLDK_HEAP_IMP_HEADER)(((ULONG_PTR)(H)) - LDK_HEAP_IMP_HEADER_SIZE))
#define LDK_HEAP_IMP_GET_BUFFER(H)		    Add2Ptr((H), LDK_HEAP_IMP_HEADER_SIZE)
#define LDK_HEAP_IMP_GET_BUFFER_SIZE(H)	


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
	PLDK_HEAP_IMP_HEADER Buffer;
	
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
	Buffer = ExAllocatePoolWithTag( LdkpDefaultPoolType,
									Size + LDK_HEAP_IMP_HEADER_SIZE,
									Tag.Value );
	if (! Buffer) {
		return NULL;
	}

	Buffer->HeapHandle = HeapHandle;
	Buffer->Tag = Tag.Value;
	Buffer->Size = Size;
	
	if (FlagOn(Flags, HEAP_ZERO_MEMORY)) {
		RtlZeroMemory(LDK_HEAP_IMP_GET_BUFFER(Buffer), Size);
	}

	return LDK_HEAP_IMP_GET_BUFFER(Buffer);
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
	PLDK_HEAP_IMP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(BaseAddress);
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
		PLDK_HEAP_IMP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(BaseAddress);
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
		PLDK_HEAP_IMP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(BaseAddress);
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
		PLDK_HEAP_IMP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(BaseAddress);
		return (Header->HeapHandle == HeapHandle);
	}

	return FALSE;
}