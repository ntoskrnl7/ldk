#include "ntdll.h"
#include "../ldk.h"

#include <limits.h>
#include <Ldk/ntdll/ntrtlstringandbuffer.h>

NTSTATUS
NTAPI
RtlpEnsureBufferSize (
    _In_ ULONG Flags,
    _Inout_ PRTL_BUFFER Buffer,
    _In_ SIZE_T Size
    )
{
    if ((Flags & ~(RTL_ENSURE_BUFFER_SIZE_NO_COPY)) != 0) {
        return STATUS_INVALID_PARAMETER;
    }
    if (Buffer == NULL) {
        return STATUS_INVALID_PARAMETER;
    }

    if (Size <= Buffer->Size) {
        return STATUS_SUCCESS;
    }

    if (Buffer->Buffer == Buffer->StaticBuffer && Size <= Buffer->StaticSize) {
        Buffer->Size = Size;
        return STATUS_SUCCESS;
    }

    PUCHAR Temp = (PUCHAR)RtlAllocateStringRoutine( Size );
    if (Temp == NULL) {
        return STATUS_NO_MEMORY;
    }

    if ((Flags & RTL_ENSURE_BUFFER_SIZE_NO_COPY) == 0) {
        RtlCopyMemory( Temp,
                       Buffer->Buffer,
                       Buffer->Size );
    }

    if (RTLP_BUFFER_IS_HEAP_ALLOCATED(Buffer)) {
        RtlFreeStringRoutine( Buffer->Buffer );
        Buffer->Buffer = NULL;
    }
    ASSERT(Temp);
    Buffer->Buffer = Temp;
    Buffer->Size = Size;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
RtlMultiAppendUnicodeStringBuffer (
    _Out_ PRTL_UNICODE_STRING_BUFFER Destination,
    _In_ ULONG NumberOfSources,
    _In_ PCUNICODE_STRING SourceArray
    )
{
    const SIZE_T CharSize = sizeof(*Destination->String.Buffer);
    const ULONG OriginalDestinationLength = Destination->String.Length;
    SIZE_T Length = OriginalDestinationLength;
    for (ULONG i = 0 ; i != NumberOfSources; ++i) {
        Length += SourceArray[i].Length;
        if (Length > MAX_UNICODE_STRING_MAXLENGTH) {
            return STATUS_NAME_TOO_LONG;
        }
    }
    Length += CharSize;
    if (Length > MAX_UNICODE_STRING_MAXLENGTH) {
        return STATUS_NAME_TOO_LONG;
    }
    NTSTATUS Status = RtlEnsureBufferSize(0, &Destination->ByteBuffer, Length);
    if (! NT_SUCCESS(Status)) {
        return Status;
    }
    Destination->String.MaximumLength = (USHORT)Length;
    Destination->String.Length = (USHORT)(Length - CharSize);
    Destination->String.Buffer = (PWSTR)Destination->ByteBuffer.Buffer;
    Length = OriginalDestinationLength;
    for (ULONG i = 0 ; i != NumberOfSources; ++i) {
        RtlMoveMemory( Destination->String.Buffer + Length / CharSize,
                       SourceArray[i].Buffer,
                       SourceArray[i].Length );
        Length += SourceArray[i].Length;
    }
    Destination->String.Buffer[Length / CharSize] = 0;
    return STATUS_SUCCESS;
}