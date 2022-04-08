#pragma once

EXTERN_C_START

WINBASEAPI
_Ret_maybenull_
PVOID
WINAPI
EncodePointer(
    _In_opt_ PVOID Ptr
    );


WINBASEAPI
_Ret_maybenull_
PVOID
WINAPI
DecodePointer(
    _In_opt_ PVOID Ptr
    );

EXTERN_C_END
