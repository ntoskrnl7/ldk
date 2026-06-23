#include "winbase.h"
#include "../ldk.h"
#include "../ntdll/ntdll.h"

WINBASEAPI
_Ret_maybenull_
HANDLE
WINAPI
HeapCreate (
    _In_ DWORD flOptions,
    _In_ SIZE_T dwInitialSize,
    _In_ SIZE_T dwMaximumSize
    )
{
	SYSTEM_INFO sytemInfo;

	PAGED_CODE();

	GetSystemInfo( &sytemInfo );

    ULONG Flags = (flOptions & (HEAP_GENERATE_EXCEPTIONS | HEAP_NO_SERIALIZE)) | HEAP_CLASS_1;
    ULONG GrowthThreshold = 0;

    if (dwMaximumSize < sytemInfo.dwPageSize) {
        if (dwMaximumSize == 0) {
            GrowthThreshold = sytemInfo.dwPageSize * 16;
            Flags |= HEAP_GROWABLE;
        } else {
			dwMaximumSize = sytemInfo.dwPageSize;
		}
	}
    if (GrowthThreshold == 0 && dwInitialSize > dwMaximumSize) {
        dwMaximumSize = dwInitialSize;
    }
    HANDLE hHeap = (HANDLE)RtlCreateHeap( Flags,
                                		  NULL,
                                   		  dwMaximumSize,
                                   		  dwInitialSize,
                                   		  0,
                                   		  NULL );
    if (hHeap) {
		return hHeap;
	}
	SetLastError( ERROR_NOT_ENOUGH_MEMORY );
    return NULL;
}

WINBASEAPI
BOOL
WINAPI
HeapDestroy (
    _In_ HANDLE hHeap
    )
{
    if (RtlDestroyHeap( (PVOID)hHeap )) {
        SetLastError( ERROR_INVALID_HANDLE );
		return FALSE;
	}
	return TRUE;
}

WINBASEAPI
_Ret_maybenull_
_Post_writable_byte_size_(dwBytes)
LPVOID
WINAPI
HeapAlloc (
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ SIZE_T dwBytes
    )
{
	return RtlAllocateHeap( hHeap,
							dwFlags,
							dwBytes );
}

WINBASEAPI
_Success_(return!=0)
_Ret_maybenull_
_Post_writable_byte_size_(dwBytes)
LPVOID
WINAPI
HeapReAlloc (
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _Frees_ptr_opt_ LPVOID lpMem,
    _In_ SIZE_T dwBytes
    )
{
	return RtlReAllocateHeap( hHeap,
							  dwFlags,
							  lpMem,
							  dwBytes );
}

WINBASEAPI
BOOL
WINAPI
HeapFree (
    _Inout_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    __drv_freesMem(Mem) _Frees_ptr_opt_ LPVOID lpMem
    )
{
	return RtlFreeHeap( hHeap,
						dwFlags,
						lpMem );
}

WINBASEAPI
SIZE_T
WINAPI
HeapSize (
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_ LPCVOID lpMem
    )
{
	return RtlSizeHeap( hHeap,
						dwFlags,
						lpMem );
}

WINBASEAPI
BOOL
WINAPI
HeapValidate (
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags,
    _In_opt_ LPCVOID lpMem
    )
{
	return RtlValidateHeap( hHeap,
							dwFlags,
							lpMem );
}

WINBASEAPI
HANDLE
WINAPI
GetProcessHeap (
    VOID
    )
{
	return RtlProcessHeap();
}

WINBASEAPI
SIZE_T
WINAPI
HeapCompact (
    _In_ HANDLE hHeap,
    _In_ DWORD dwFlags
    )
{
    UNREFERENCED_PARAMETER(dwFlags);

    PAGED_CODE();

    if (hHeap == NULL) {
        SetLastError( ERROR_INVALID_HANDLE );
        return 0;
    }

    //
    // LDK heaps are backed by nonpaged pool allocations, so there is no
    // committed heap segment to compact. A zero result is a valid "nothing was
    // compacted" result for this heap model.
    //
    return 0;
}

WINBASEAPI
BOOL
WINAPI
HeapWalk (
    _In_ HANDLE hHeap,
    _Inout_ LPPROCESS_HEAP_ENTRY lpEntry
    )
{
    LDK_HEAP_WALK_ENTRY WalkEntry;
    PVOID PreviousData;
    NTSTATUS Status;

    PAGED_CODE();

    if (hHeap == NULL ||
        lpEntry == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    PreviousData = lpEntry->lpData;
    Status = LdkWalkHeap( hHeap,
                          PreviousData,
                          &WalkEntry );
    if (! NT_SUCCESS(Status)) {
        if (Status == STATUS_NO_MORE_ENTRIES) {
            SetLastError( ERROR_NO_MORE_ITEMS );
        } else {
            LdkSetLastNTError( Status );
        }
        return FALSE;
    }

    RtlZeroMemory( lpEntry,
                   sizeof(*lpEntry) );
    lpEntry->lpData = WalkEntry.Data;
    lpEntry->cbData = WalkEntry.Size > 0xffffffffu ? 0xffffffffu : (DWORD)WalkEntry.Size;
    lpEntry->cbOverhead = WalkEntry.Overhead;
    lpEntry->iRegionIndex = WalkEntry.RegionIndex;
    lpEntry->wFlags = PROCESS_HEAP_ENTRY_BUSY;

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
HeapQueryInformation (
    _In_opt_ HANDLE HeapHandle,
    _In_ HEAP_INFORMATION_CLASS HeapInformationClass,
    _Out_writes_bytes_to_opt_(HeapInformationLength,*ReturnLength) PVOID HeapInformation,
    _In_ SIZE_T HeapInformationLength,
    _Out_opt_ PSIZE_T ReturnLength
    )
{
    UNREFERENCED_PARAMETER(HeapHandle);

    PAGED_CODE();

    if (ReturnLength != NULL) {
        *ReturnLength = 0;
    }

    if (HeapInformationClass == HeapCompatibilityInformation) {
        if (ReturnLength != NULL) {
            *ReturnLength = sizeof(ULONG);
        }

        if (HeapInformation == NULL ||
            HeapInformationLength < sizeof(ULONG)) {
            SetLastError( ERROR_INSUFFICIENT_BUFFER );
            return FALSE;
        }

        *(PULONG)HeapInformation = 0;
        return TRUE;
    }

    SetLastError( ERROR_INVALID_PARAMETER );
    return FALSE;
}
