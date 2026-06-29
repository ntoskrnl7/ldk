#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
UtilityApiTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, UtilityApiTest)
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

typedef
PVOID
(WINAPI *PLDK_ENCODE_POINTER_ROUTINE) (
    _In_opt_ PVOID Ptr
    );

typedef
PVOID
(WINAPI *PLDK_DECODE_POINTER_ROUTINE) (
    _In_opt_ PVOID Ptr
    );

static
BOOLEAN
LdkpTestPointerCodec (
    _In_z_ PCSTR Name,
    _In_ PLDK_ENCODE_POINTER_ROUTINE EncodeRoutine,
    _In_ PLDK_DECODE_POINTER_ROUTINE DecodeRoutine
    )
{
    ULONG LocalMarker = 0x12345678;
    PVOID Values[] = {
        NULL,
        (PVOID)(ULONG_PTR)1,
        (PVOID)(ULONG_PTR)0x12345678,
        &LocalMarker
    };

    for (ULONG Index = 0; Index < RTL_NUMBER_OF(Values); Index++) {
        PVOID Value = Values[Index];
        PVOID Encoded = EncodeRoutine( Value );
        PVOID EncodedAgain = EncodeRoutine( Value );
        PVOID Decoded = DecodeRoutine( Encoded );

        if (Encoded != EncodedAgain) {
            fprintf(stderr,
                    "[Failed] %s was not stable Value = %p Encoded = %p EncodedAgain = %p\n",
                    Name,
                    Value,
                    Encoded,
                    EncodedAgain );
            return FALSE;
        }

        if (Decoded != Value) {
            fprintf(stderr,
                    "[Failed] %s round-trip Value = %p Encoded = %p Decoded = %p\n",
                    Name,
                    Value,
                    Encoded,
                    Decoded );
            return FALSE;
        }

        if (Encoded == Value) {
            fprintf(stderr,
                    "[Failed] %s returned the original pointer Value = %p\n",
                    Name,
                    Value );
            return FALSE;
        }
    }

    return TRUE;
}

BOOLEAN
UtilityApiTest (
    VOID
    )
{
    PAGED_CODE();

    printf("Utility API Test\n");

    if (! LdkpTestPointerCodec( "EncodePointer/DecodePointer",
                                EncodePointer,
                                DecodePointer )) {
        return FALSE;
    }

    if (! LdkpTestPointerCodec( "EncodeSystemPointer/DecodeSystemPointer",
                                EncodeSystemPointer,
                                DecodeSystemPointer )) {
        return FALSE;
    }

    printf("[Success] Utility API Test\n\n");
    return TRUE;
}
