#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
HeapApiTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HeapApiTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <stdio.h>
#define PAGED_CODE()
#endif

BOOLEAN
HeapApiTest (
    VOID
    )
{
    BOOLEAN Result = TRUE;
    HANDLE Heap;
    PVOID BlockA;
    PVOID BlockB;
    PROCESS_HEAP_ENTRY Entry;
    ULONG Compatibility;
    SIZE_T ReturnLength;
    ULONG SeenA = 0;
    ULONG SeenB = 0;
    ULONG WalkCount = 0;

    PAGED_CODE();

    printf("Heap API Test\n");

    Heap = HeapCreate( 0,
                       0,
                       0 );
    if (Heap == NULL) {
        fprintf(stderr,
                "[Failed] HeapCreate ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    BlockA = HeapAlloc( Heap,
                        HEAP_ZERO_MEMORY,
                        32 );
    BlockB = HeapAlloc( Heap,
                        0,
                        64 );
    if (BlockA == NULL ||
        BlockB == NULL) {
        fprintf(stderr,
                "[Failed] HeapAlloc ErrorCode = %lu\n",
                GetLastError() );
        Result = FALSE;
        goto Exit;
    }

    Compatibility = 0xffffffffu;
    ReturnLength = 0;
    if (! HeapQueryInformation( Heap,
                                HeapCompatibilityInformation,
                                &Compatibility,
                                sizeof(Compatibility),
                                &ReturnLength ) ||
        Compatibility != 0 ||
        ReturnLength != sizeof(ULONG)) {
        fprintf(stderr,
                "[Failed] HeapQueryInformation ErrorCode = %lu Compatibility = %lu ReturnLength = %Iu\n",
                GetLastError(),
                Compatibility,
                ReturnLength );
        Result = FALSE;
    }

    RtlZeroMemory( &Entry,
                   sizeof(Entry) );
    while (HeapWalk( Heap,
                     &Entry )) {
        WalkCount++;
        if (Entry.lpData == BlockA) {
            SeenA++;
            if (Entry.cbData != 32 ||
                ! FlagOn(Entry.wFlags, PROCESS_HEAP_ENTRY_BUSY)) {
                fprintf(stderr,
                        "[Failed] HeapWalk BlockA cbData = %lu Flags = 0x%04x\n",
                        Entry.cbData,
                        Entry.wFlags );
                Result = FALSE;
            }
        } else if (Entry.lpData == BlockB) {
            SeenB++;
            if (Entry.cbData != 64 ||
                ! FlagOn(Entry.wFlags, PROCESS_HEAP_ENTRY_BUSY)) {
                fprintf(stderr,
                        "[Failed] HeapWalk BlockB cbData = %lu Flags = 0x%04x\n",
                        Entry.cbData,
                        Entry.wFlags );
                Result = FALSE;
            }
        }

        if (WalkCount > 16) {
            fprintf(stderr,
                    "[Failed] HeapWalk did not terminate\n" );
            Result = FALSE;
            break;
        }
    }

    if (GetLastError() != ERROR_NO_MORE_ITEMS ||
        SeenA != 1 ||
        SeenB != 1) {
        fprintf(stderr,
                "[Failed] HeapWalk enumeration ErrorCode = %lu SeenA = %lu SeenB = %lu Count = %lu\n",
                GetLastError(),
                SeenA,
                SeenB,
                WalkCount );
        Result = FALSE;
    }

    if (HeapCompact( Heap,
                     0 ) != 0) {
#if _KERNEL_MODE
        fprintf(stderr,
                "[Failed] HeapCompact returned nonzero\n" );
        Result = FALSE;
#endif
    }

Exit:
    if (BlockA != NULL &&
        ! HeapFree( Heap,
                    0,
                    BlockA )) {
        fprintf(stderr,
                "[Failed] HeapFree(BlockA) ErrorCode = %lu\n",
                GetLastError() );
        Result = FALSE;
    }

    if (BlockB != NULL &&
        ! HeapFree( Heap,
                    0,
                    BlockB )) {
        fprintf(stderr,
                "[Failed] HeapFree(BlockB) ErrorCode = %lu\n",
                GetLastError() );
        Result = FALSE;
    }

    if (! HeapDestroy( Heap )) {
        fprintf(stderr,
                "[Failed] HeapDestroy ErrorCode = %lu\n",
                GetLastError() );
        Result = FALSE;
    }

    if (Result) {
        printf("[Success] Heap API Test\n\n");
    } else {
        printf("[Failed] Heap API Test\n\n");
    }

    return Result;
}
