#include <Ldk/ntsecapi.h>
#include "../ldk.h"
#include <bcrypt.h>

WINBASEAPI
BOOLEAN
WINAPI
SystemFunction036 (
    _Out_writes_bytes_(RandomBufferLength) PVOID RandomBuffer,
    _In_ ULONG RandomBufferLength
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    if (RandomBufferLength == 0) {
        return TRUE;
    }

    if (RandomBuffer == NULL) {
        return FALSE;
    }

    Status = BCryptGenRandom( NULL,
                              (PUCHAR)RandomBuffer,
                              RandomBufferLength,
                              BCRYPT_USE_SYSTEM_PREFERRED_RNG );
    return NT_SUCCESS( Status );
}

#pragma warning(disable:4152)
LDK_FUNCTION_REGISTRATION LdkpAdvapi32FunctionRegistration[] = {
    { "SystemFunction036", SystemFunction036 },
    { NULL, NULL }
};
#pragma warning(default:4152)

LDK_MODULE LdkpAdvapi32Module = {
    RTL_CONSTANT_STRING("ADVAPI32.DLL"),
    RTL_CONSTANT_STRING("\\SystemRoot\\system32\\ADVAPI32.DLL"),
    LdkpAdvapi32FunctionRegistration,
    NULL
};
