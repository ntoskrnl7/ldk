#if _KERNEL_MODE
#include <Ldk/Windows.h>
#include <Ldk/ntdll.h>

BOOLEAN
NtdllEcCodeCompatibilityTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, NtdllEcCodeCompatibilityTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <stdio.h>
#define PAGED_CODE()

typedef BOOLEAN (NTAPI *PRTL_IS_EC_CODE) (
    _In_ ULONG64 CodeAddress
    );
#endif

BOOLEAN
NtdllEcCodeCompatibilityTest (
    VOID
    )
{
    BOOLEAN Result;

    PAGED_CODE();

    printf("NTDLL EC code compatibility test\n");

#if _KERNEL_MODE
    Result = RtlIsEcCode( (ULONG64)(ULONG_PTR)NtdllEcCodeCompatibilityTest );
#else
    {
        HMODULE Ntdll;
        PRTL_IS_EC_CODE RtlIsEcCodeRoutine;

        Ntdll = GetModuleHandleW( L"ntdll.dll" );
        if (Ntdll == NULL) {
            fprintf(stderr,
                    "[Failed] GetModuleHandleW(ntdll.dll) ErrorCode = %lu\n",
                    GetLastError() );
            return FALSE;
        }

        RtlIsEcCodeRoutine = (PRTL_IS_EC_CODE)GetProcAddress( Ntdll,
                                                              "RtlIsEcCode" );
        if (RtlIsEcCodeRoutine == NULL) {
            printf("[Skipped] RtlIsEcCode is not exported by this ntdll.dll\n\n");
            return TRUE;
        }

        Result = RtlIsEcCodeRoutine( (ULONG64)(ULONG_PTR)NtdllEcCodeCompatibilityTest );
    }
#endif

    if (Result != FALSE) {
        fprintf(stderr,
                "[Failed] RtlIsEcCode returned TRUE for native test code\n" );
        return FALSE;
    }

    printf("[Success] NTDLL EC code compatibility test\n\n");
    return TRUE;
}
