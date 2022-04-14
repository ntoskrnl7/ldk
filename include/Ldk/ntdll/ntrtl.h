#pragma once

EXTERN_C_START

VOID
NTAPI
RtlAcquirePebLock (
    VOID
    );

VOID
NTAPI
RtlReleasePebLock (
    VOID
    );

EXTERN_C_END