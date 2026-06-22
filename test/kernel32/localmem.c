#if _KERNEL_MODE
#include <Ldk/Windows.h>
#else
#include <windows.h>
#include <stdio.h>
#define PAGED_CODE()
#define DbgPrintEx(_id_, _level_, ...) printf(__VA_ARGS__)
#define DPFLTR_IHVDRIVER_ID 0
#define DPFLTR_INFO_LEVEL 0
#define DPFLTR_ERROR_LEVEL 1
#endif

BOOLEAN
LocalMemoryTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, LocalMemoryTest)
#endif

#define printf(...)         DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_INFO_LEVEL, __VA_ARGS__)
#define fprintf(_f_, ...)   DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__)
#define stderr              DPFLTR_ERROR_LEVEL

static
BOOLEAN
LocalMemoryExpect (
    _In_ BOOL Condition,
    _In_ PCSTR Message
    )
{
    if (! Condition) {
        fprintf(stderr, "[Failed] %s (LastError = %lu)\n", Message, GetLastError());
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
LocalMemoryIsFilled (
    _In_reads_bytes_(Length) const UCHAR *Buffer,
    _In_ SIZE_T Length,
    _In_ UCHAR Value
    )
{
    for (SIZE_T Index = 0; Index < Length; Index++) {
        if (Buffer[Index] != Value) {
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
LocalMemoryTest (
    VOID
    )
{
    HLOCAL Memory;
    UCHAR *Bytes;
    BOOL Result = TRUE;

    PAGED_CODE();

    printf("Local Memory Test\n");

    Memory = LocalAlloc( LMEM_ZEROINIT,
                         32 );
    Result &= LocalMemoryExpect( Memory != NULL,
                                 "LocalAlloc(LMEM_ZEROINIT)" );
    if (Memory == NULL) {
        return FALSE;
    }

    Bytes = (UCHAR *)LocalLock( Memory );
    Result &= LocalMemoryExpect( Bytes != NULL,
                                 "LocalLock(LocalAlloc result)" );
    Result &= LocalMemoryExpect( LocalSize( Memory ) >= 32,
                                 "LocalSize(LocalAlloc result)" );
    Result &= LocalMemoryExpect( LocalMemoryIsFilled( Bytes,
                                                     32,
                                                     0 ),
                                 "LocalAlloc(LMEM_ZEROINIT) zeroes allocation" );

    for (SIZE_T Index = 0; Index < 32; Index++) {
        Bytes[Index] = 0x7f;
    }

    Memory = LocalReAlloc( Memory,
                           64,
                           LMEM_ZEROINIT );
    Result &= LocalMemoryExpect( Memory != NULL,
                                 "LocalReAlloc(LMEM_ZEROINIT)" );
    if (Memory == NULL) {
        return FALSE;
    }

    Bytes = (UCHAR *)LocalLock( Memory );
    Result &= LocalMemoryExpect( Bytes != NULL,
                                 "LocalLock(LocalReAlloc result)" );
    Result &= LocalMemoryExpect( LocalSize( Memory ) >= 64,
                                 "LocalSize(LocalReAlloc result)" );
    Result &= LocalMemoryExpect( LocalMemoryIsFilled( Bytes,
                                                     32,
                                                     0x7f ),
                                 "LocalReAlloc preserves existing bytes" );
    Result &= LocalMemoryExpect( LocalMemoryIsFilled( Bytes + 32,
                                                     32,
                                                     0 ),
                                 "LocalReAlloc(LMEM_ZEROINIT) zeroes new bytes" );

    Result &= LocalMemoryExpect( LocalHandle( Bytes ) == Memory,
                                 "LocalHandle(LocalLock result)" );
    Result &= LocalMemoryExpect( LocalFree( Memory ) == NULL,
                                 "LocalFree(LocalAlloc result)" );

    if (Result) {
        printf("[Success] Local Memory Test\n\n");
    } else {
        printf("[Failed] Local Memory Test\n\n");
    }

    return Result ? TRUE : FALSE;
}
