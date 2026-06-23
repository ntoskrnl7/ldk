#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
FormatMessageTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FormatMessageTest)
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

static
BOOLEAN
ContainsStringA (
    _In_z_ PCSTR Haystack,
    _In_z_ PCSTR Needle
    )
{
    SIZE_T Index;
    SIZE_T NeedleIndex;

    for (Index = 0; Haystack[Index] != ANSI_NULL; Index++) {
        for (NeedleIndex = 0; Needle[NeedleIndex] != ANSI_NULL; NeedleIndex++) {
            if (Haystack[Index + NeedleIndex] != Needle[NeedleIndex]) {
                break;
            }
        }

        if (Needle[NeedleIndex] == ANSI_NULL) {
            return TRUE;
        }
    }

    return FALSE;
}

static
BOOLEAN
ContainsStringW (
    _In_z_ PCWSTR Haystack,
    _In_z_ PCWSTR Needle
    )
{
    SIZE_T Index;
    SIZE_T NeedleIndex;

    for (Index = 0; Haystack[Index] != UNICODE_NULL; Index++) {
        for (NeedleIndex = 0; Needle[NeedleIndex] != UNICODE_NULL; NeedleIndex++) {
            if (Haystack[Index + NeedleIndex] != Needle[NeedleIndex]) {
                break;
            }
        }

        if (Needle[NeedleIndex] == UNICODE_NULL) {
            return TRUE;
        }
    }

    return FALSE;
}

BOOLEAN
FormatMessageTest (
    VOID
    )
{
    BOOLEAN Result = TRUE;
    CHAR StackBuffer[64] = { 0 };
    WCHAR WideBuffer[64] = { 0 };
    LPSTR AllocatedBuffer = NULL;
    DWORD Chars;

    PAGED_CODE();

    printf("FormatMessage Test\n");

    Chars = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            ERROR_ACCESS_DENIED,
                            0x0409,
                            StackBuffer,
                            sizeof(StackBuffer),
                            NULL );
    if ((Chars == 0) ||
        ! ContainsStringA( StackBuffer,
                           "Access is denied." )) {
        fprintf(stderr,
                "[Failed] FormatMessageA stack buffer ErrorCode = %d Message = %s\n",
                GetLastError(),
                StackBuffer );
        Result = FALSE;
    }

    Chars = FormatMessageA( FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS |
                                FORMAT_MESSAGE_ALLOCATE_BUFFER,
                            NULL,
                            ERROR_ACCESS_DENIED,
                            0x0409,
                            (LPSTR)&AllocatedBuffer,
                            0,
                            NULL );
    if ((Chars == 0) ||
        (AllocatedBuffer == NULL) ||
        ! ContainsStringA( AllocatedBuffer,
                           "Access is denied." )) {
        fprintf(stderr,
                "[Failed] FormatMessageA allocated buffer ErrorCode = %d Message = %s\n",
                GetLastError(),
                AllocatedBuffer != NULL ? AllocatedBuffer : "" );
        Result = FALSE;
    }

    if (AllocatedBuffer != NULL) {
        if (LocalFree( AllocatedBuffer ) != NULL) {
            fprintf(stderr, "[Failed] LocalFree(FormatMessageA buffer)\n");
            Result = FALSE;
        }
    }

    Chars = FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL,
                            ERROR_ACCESS_DENIED,
                            0x0409,
                            WideBuffer,
                            RTL_NUMBER_OF(WideBuffer),
                            NULL );
    if ((Chars == 0) ||
        ! ContainsStringW( WideBuffer,
                           L"Access is denied." )) {
        fprintf(stderr,
                "[Failed] FormatMessageW stack buffer ErrorCode = %d\n",
                GetLastError() );
        Result = FALSE;
    }

#if _KERNEL_MODE
    RtlZeroMemory( StackBuffer,
                   sizeof(StackBuffer) );
    Chars = FormatMessageA( FORMAT_MESSAGE_FROM_HMODULE |
                                FORMAT_MESSAGE_FROM_SYSTEM |
                                FORMAT_MESSAGE_IGNORE_INSERTS,
                            GetModuleHandleW( NULL ),
                            ERROR_ACCESS_DENIED,
                            0x0409,
                            StackBuffer,
                            sizeof(StackBuffer),
                            NULL );
    if ((Chars == 0) ||
        ! ContainsStringA( StackBuffer,
                           "Access is denied." )) {
        fprintf(stderr,
                "[Failed] FormatMessageA module/system fallback ErrorCode = %d Message = %s\n",
                GetLastError(),
                StackBuffer );
        Result = FALSE;
    }
#endif

    if (Result) {
#if _KERNEL_MODE
        printf("[Success] FormatMessage(ERROR_ACCESS_DENIED) => %s\n\n",
               StackBuffer );
#else
        printf("[Success] FormatMessage(ERROR_ACCESS_DENIED)\n\n");
#endif
    } else {
        printf("[Failed] FormatMessage Test\n\n");
    }

    return Result;
}
