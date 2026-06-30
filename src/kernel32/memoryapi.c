#include "winbase.h"
#include "../ldk.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, VirtualQuery)
#pragma alloc_text(PAGE, VirtualProtect)
#pragma alloc_text(PAGE, VirtualFree)
#endif

WINBASEAPI
SIZE_T
WINAPI
VirtualQuery (
    _In_opt_ LPCVOID lpAddress,
    _Out_writes_bytes_to_(dwLength, return) PMEMORY_BASIC_INFORMATION lpBuffer,
    _In_ SIZE_T dwLength
    )
{
    NTSTATUS Status;
    SIZE_T ReturnLength = 0;

    PAGED_CODE();

    if (lpBuffer == NULL ||
        dwLength < sizeof(MEMORY_BASIC_INFORMATION)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    EXIT_WHEN_DPC_WITH_RETURN(0);

    Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                   (PVOID)lpAddress,
                                   MemoryBasicInformation,
                                   lpBuffer,
                                   dwLength,
                                   &ReturnLength );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return 0;
    }

    return ReturnLength;
}

WINBASEAPI
BOOL
WINAPI
VirtualProtect (
    _In_ LPVOID lpAddress,
    _In_ SIZE_T dwSize,
    _In_ DWORD flNewProtect,
    _Out_ PDWORD lpflOldProtect
    )
{
    NTSTATUS Status;
    PVOID BaseAddress = lpAddress;
    SIZE_T RegionSize = dwSize;

    PAGED_CODE();

    if (lpAddress == NULL ||
        dwSize == 0 ||
        lpflOldProtect == NULL) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    EXIT_WHEN_DPC_WITH_RETURN(FALSE);

    Status = ZwProtectVirtualMemory( NtCurrentProcess(),
                                     &BaseAddress,
                                     &RegionSize,
                                     flNewProtect,
                                     lpflOldProtect );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    return TRUE;
}

WINBASEAPI
BOOL
WINAPI
VirtualFree (
    _Pre_notnull_ _When_(dwFreeType == MEM_DECOMMIT, _Post_invalid_) _When_(dwFreeType == MEM_RELEASE, _Post_ptr_invalid_) LPVOID lpAddress,
    _In_ SIZE_T dwSize,
    _In_ DWORD dwFreeType
    )
{
    NTSTATUS Status;
    PVOID BaseAddress = lpAddress;
    SIZE_T RegionSize = dwSize;
    DWORD Operation;

    PAGED_CODE();

    Operation = dwFreeType & (MEM_DECOMMIT | MEM_RELEASE);
    if (lpAddress == NULL) {
        SetLastError( ERROR_INVALID_ADDRESS );
        return FALSE;
    }

    if ((dwFreeType & ~(MEM_DECOMMIT | MEM_RELEASE)) != 0 ||
        Operation == 0 ||
        Operation == (MEM_DECOMMIT | MEM_RELEASE) ||
        (Operation == MEM_RELEASE && dwSize != 0)) {
        SetLastError( ERROR_INVALID_PARAMETER );
        return FALSE;
    }

    EXIT_WHEN_DPC_WITH_RETURN(FALSE);

    Status = ZwFreeVirtualMemory( NtCurrentProcess(),
                                  &BaseAddress,
                                  &RegionSize,
                                  dwFreeType );
    if (! NT_SUCCESS(Status)) {
        LdkSetLastNTError( Status );
        return FALSE;
    }

    return TRUE;
}
