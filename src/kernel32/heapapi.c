#include "winbase.h"
#include "../ldk.h"

//
// 커널에서도 힙을 사용할수있으나 NtAllocateVirtualMemory루틴이 사용되었기때문에
// 프로세스 컨텍스트(PDE)에 영향을 받으므로 비페이징풀로 할당되도록 직접 구현했습니다.
// (힙 관련 루틴 : RtlCreateHeap, RtlDestroyHeap, RtlAllocateHeap, RtlFreeHeap 등)
// 

typedef struct _LDK_HEAP_IMP_HEADER {
	HANDLE HeapHandle;
	SIZE_T Size;
} LDK_HEAP_IMP_HEADER, *PLDK_HEAP_IMP_HEADER;

#define LDK_HEAP_IMP_HEADER_SIZE		    sizeof(LDK_HEAP_IMP_HEADER)

#define LDK_HEAP_IMP_GET_HEADER(H)		    ((PLDK_HEAP_IMP_HEADER)(((ULONG_PTR)(H)) - LDK_HEAP_IMP_HEADER_SIZE))
#define LDK_HEAP_IMP_GET_BUFFER(H)		    Add2Ptr((H), LDK_HEAP_IMP_HEADER_SIZE)
#define LDK_HEAP_IMP_GET_BUFFER_SIZE(H)	

#define LDK_HEAP_IMP_PROCESS_HEAP		    ((HANDLE)'paeH')

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
HeapCreate(
    _In_ DWORD flOptions,
    _In_ SIZE_T dwInitialSize,
    _In_ SIZE_T dwMaximumSize
    )
{
	UNREFERENCED_PARAMETER(flOptions);
	UNREFERENCED_PARAMETER(dwInitialSize);
	UNREFERENCED_PARAMETER(dwMaximumSize);
	return (HANDLE)PsGetCurrentThread();
}

WINBASEAPI
BOOL
WINAPI
HeapDestroy(
    _In_ HANDLE hHeap
    )
{
	UNREFERENCED_PARAMETER(hHeap);
	return TRUE;
}

WINBASEAPI
_Ret_maybenull_
_Post_writable_byte_size_(dwBytes)
LPVOID
WINAPI
HeapAlloc(
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ SIZE_T dwBytes
    )
{
	PLDK_HEAP_IMP_HEADER Buffer;
	
	Buffer = ExAllocatePoolWithTag( LdkpDefaultPoolType,
									dwBytes + LDK_HEAP_IMP_HEADER_SIZE,
									HandleToUlong(hHeap) );
	
	if (! Buffer) {
		return NULL;
	}
	
	Buffer->HeapHandle = hHeap;
	Buffer->Size = dwBytes;
	
	if (FlagOn(dwFlags, HEAP_ZERO_MEMORY)) {
		RtlZeroMemory(LDK_HEAP_IMP_GET_BUFFER(Buffer), dwBytes);
	}

	return LDK_HEAP_IMP_GET_BUFFER(Buffer);
}

WINBASEAPI
_Success_(return!=0)
_Ret_maybenull_
_Post_writable_byte_size_(dwBytes)
LPVOID
WINAPI
HeapReAlloc(
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _Frees_ptr_opt_ LPVOID lpMem,
    _In_ SIZE_T dwBytes
    )
{
	if (ARGUMENT_PRESENT(lpMem)) {

		PVOID NewBuffer;
		PLDK_HEAP_IMP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(lpMem);
		SIZE_T OldSize;

		if (Header->HeapHandle != hHeap) {
			return NULL;
		}

		NewBuffer = HeapAlloc(hHeap, 0, dwBytes);
		OldSize = Header->Size;

		if (OldSize > dwBytes) {
			RtlCopyMemory(NewBuffer, lpMem, dwBytes);
		} else {			
			RtlCopyMemory(NewBuffer, lpMem, OldSize);
			if (FlagOn(dwFlags, HEAP_ZERO_MEMORY)) {
				RtlZeroMemory(Add2Ptr(NewBuffer, OldSize), dwBytes - OldSize);
			}
		}

		HeapFree(hHeap, 0, lpMem);

		return NewBuffer;

	} else {

		return NULL;

	}
}

WINBASEAPI
BOOL
WINAPI
HeapFree(
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    __drv_freesMem(Mem) _Frees_ptr_opt_ LPVOID lpMem
    )
{
	UNREFERENCED_PARAMETER(dwFlags);
	if (lpMem) {
		ExFreePoolWithTag(LDK_HEAP_IMP_GET_HEADER(lpMem), HandleToUlong(hHeap));
	}
	return TRUE;
}

WINBASEAPI
SIZE_T
WINAPI
HeapSize(
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ LPCVOID lpMem
    )
{
	UNREFERENCED_PARAMETER(dwFlags);

	if (ARGUMENT_PRESENT(lpMem)) {

		PLDK_HEAP_IMP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(lpMem);		
		if (Header->HeapHandle != hHeap) {			
			SetLastError(ERROR_INVALID_PARAMETER);
			return (SIZE_T)-1;
		}
		return Header->Size;

	} else {

		SetLastError(ERROR_INVALID_PARAMETER);
		return (SIZE_T)-1;
	}
}

WINBASEAPI
BOOL
WINAPI
HeapValidate(
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_opt_ LPCVOID lpMem
    )
{
	UNREFERENCED_PARAMETER(dwFlags);

	if (ARGUMENT_PRESENT(lpMem)) {
		if (MmIsAddressValid((PVOID)lpMem)) {
			PLDK_HEAP_IMP_HEADER Header = LDK_HEAP_IMP_GET_HEADER(lpMem);
			return (Header->HeapHandle == hHeap);
		}
	}

	return FALSE;
}

WINBASEAPI
HANDLE
WINAPI
GetProcessHeap(
    VOID
    )
{
	return LDK_HEAP_IMP_PROCESS_HEAP;
}