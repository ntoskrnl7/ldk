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
    UNREFERENCED_PARAMETER(hHeap);
    UNREFERENCED_PARAMETER(dwFlags);

    PAGED_CODE();

    KdBreakPoint();

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );

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
    UNREFERENCED_PARAMETER(hHeap);
    UNREFERENCED_PARAMETER(lpEntry);

    PAGED_CODE();

    KdBreakPoint();

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );

    return FALSE;
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
    UNREFERENCED_PARAMETER(HeapInformationClass);
    UNREFERENCED_PARAMETER(HeapInformation);
    UNREFERENCED_PARAMETER(HeapInformationLength);
    UNREFERENCED_PARAMETER(ReturnLength);

    PAGED_CODE();

    KdBreakPoint();

    SetLastError( ERROR_CALL_NOT_IMPLEMENTED );

    return FALSE;
}