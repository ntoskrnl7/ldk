
#pragma once

EXTERN_C_START

//
// These work for both UNICODE_STRING and STRING.
// That's why "plain" 0 and sizeof(Buffer[0]) is used.
//

// odd but correct use of RTL_STRING_IS_PUT_AT_SAFE instead of RTL_STRING_IS_GET_AT_SAFE,
// we are reaching past the Length
#define RTL_STRING_IS_NUL_TERMINATED(s)    (RTL_STRING_IS_PUT_AT_SAFE(s, RTL_STRING_GET_LENGTH_CHARS(s), 0) \
                                           && RTL_STRING_GET_AT_UNSAFE(s, RTL_STRING_GET_LENGTH_CHARS(s)) == 0)

#define RTL_STRING_NUL_TERMINATE(s)        ((VOID)(ASSERT(RTL_STRING_IS_PUT_AT_SAFE(s, RTL_STRING_GET_LENGTH_CHARS(s), 0)), \
                                           ((s)->Buffer[RTL_STRING_GET_LENGTH_CHARS(s)] = 0)))

#define RTL_NUL_TERMINATE_STRING(s)        (RTL_STRING_NUL_TERMINATE(s)) /* compatibility */

#define RTL_STRING_MAKE_LENGTH_INCLUDE_TERMINAL_NUL(s) ((VOID)(ASSERT(RTL_STRING_IS_NUL_TERMINATED(s)), \
                                                       ((s)->Length += sizeof((s)->Buffer[0]))))

#define RTL_STRING_IS_EMPTY(s)             ((s)->Length == 0)

#define RTL_STRING_GET_LAST_CHAR(s)        (RTL_STRING_GET_AT((s), RTL_STRING_GET_LENGTH_CHARS(s) - 1))

#define RTL_STRING_GET_LENGTH_CHARS(s)     ((s)->Length / sizeof((s)->Buffer[0]))
#define RTL_STRING_GET_MAX_LENGTH_CHARS(s) ((s)->MaximumLength / sizeof((s)->Buffer[0]))
#define RTL_STRING_GET_LENGTH_BYTES(s)     ((s)->Length)

#define RTL_STRING_SET_LENGTH_CHARS_UNSAFE(s,n) ((s)->Length = (RTL_STRING_LENGTH_TYPE)(((n) * sizeof(s)->Buffer[0])))

//
// We don't provide an explicit/retail RTL_STRING_GET_AT_SAFE because it'd
// need a return value distinct from all values of c. -1? NTSTATUS? Seems too heavy.
//
// For consistency then, we also don't provide RTL_STRING_PUT_AT_SAFE.
//

#define RTL_STRING_IS_GET_AT_SAFE(s,n)   ((n) < RTL_STRING_GET_LENGTH_CHARS(s))
#define RTL_STRING_GET_AT_UNSAFE(s,n)    ((s)->Buffer[n])
#define RTLP_STRING_GET_AT_SAFE(s,n)     (RTL_STRING_IS_GET_AT_SAFE(s,n) ? RTL_STRING_GET_AT_UNSAFE(s,n) : 0)

#define RTL_STRING_IS_PUT_AT_SAFE(s,n,c) ((n) < RTL_STRING_GET_MAX_LENGTH_CHARS(s))
#define RTL_STRING_PUT_AT_UNSAFE(s,n,c)  ((s)->Buffer[n] = (c))
#define RTLP_STRING_PUT_AT_SAFE(s,n,c)   ((void)(RTL_STRING_IS_PUT_AT_SAFE(s,n,c) ? RTL_STRING_PUT_AT_UNSAFE(s,n,c) : 0))

#if defined(RTL_STRING_RANGE_CHECKED)
#define RTL_STRING_GET_AT(s,n)         (ASSERT(RTL_STRING_IS_GET_AT_SAFE(s,n)), \
                                       RTL_STRING_GET_AT_UNSAFE(s,n))
#else
#define RTL_STRING_GET_AT(s,n)         (RTL_STRING_GET_AT_UNSAFE(s,n))
#endif

#if defined(RTL_STRING_RANGE_CHECKED)
#define RTL_STRING_PUT_AT(s,n,c)       (ASSERT(RTL_STRING_IS_PUT_AT_SAFE(s,n,c)), \
                                       RTL_STRING_PUT_AT_UNSAFE(s,n,c))
#else
#define RTL_STRING_PUT_AT(s,n,c)       (RTL_STRING_PUT_AT_UNSAFE(s,n,c))
#endif

//
// preallocated heap-growable buffers
//
struct _RTL_BUFFER;

#if !defined(RTL_BUFFER)
// This is duplicated in ntldr.h.

#define RTL_BUFFER RTL_BUFFER

typedef struct _RTL_BUFFER {
    PUCHAR    Buffer;
    PUCHAR    StaticBuffer;
    SIZE_T    Size;
    SIZE_T    StaticSize;
    SIZE_T    ReservedForAllocatedSize; // for future doubling
    PVOID     ReservedForIMalloc; // for future pluggable growth
} RTL_BUFFER, *PRTL_BUFFER;

#endif

#define RTLP_BUFFER_IS_HEAP_ALLOCATED(b) ((b)->Buffer != (b)->StaticBuffer)

//++
//
// NTSTATUS
// RtlInitBuffer(
//     OUT PRTL_BUFFER Buffer,
//     IN  PUCHAR      StaticBuffer,
//     IN  SIZE_T      StaticSize
//     );
//
// Routine Description:
//
// Initialize a preallocated heap-growable buffer.
//
// Arguments:
//
//     Buffer - "this"
//     StaticBuffer - preallocated storage for Buffer to use until/unless more than StaticSize is needed
//     StaticSize - the size of StaticBuffer in bytes
//
// Return Value:
//
//     STATUS_SUCCESS
//
//--
#define RtlInitBuffer(Buff, StatBuff, StatSize) \
    do {                                        \
        (Buff)->Buffer       = (StatBuff);      \
        (Buff)->Size         = (StatSize);      \
        (Buff)->StaticBuffer = (StatBuff);      \
        (Buff)->StaticSize   = (StatSize);      \
    } while (0)

#define RTL_ENSURE_BUFFER_SIZE_NO_COPY (0x00000001)

NTSTATUS
NTAPI
RtlpEnsureBufferSize (
    _In_ ULONG Flags,
    _Inout_ PRTL_BUFFER Buffer,
    _In_ SIZE_T Size
    );

//++
//
// NTSTATUS
// RtlEnsureBufferSize(
//      IN OUT PRTL_BUFFER Buffer,
//      IN     SIZE_T      NewSizeBytes
//      );
//
// Routine Description:
//
// If Buffer is smaller than NewSize, grow it to NewSize, using the static buffer if it
// is large enough, else heap allocating
//
// Arguments:
//
//     Flags -
//               RTL_ENSURE_BUFFER_SIZE_NO_COPY
//     Buffer -
//     NewSizeBytes -
//
// Return Value:
//
//     STATUS_SUCCESS
//     STATUS_NO_MEMORY
//
//--
#define RtlEnsureBufferSize(Flags, Buff, NewSizeBytes) \
    (   ((Buff) != NULL && (NewSizeBytes) <= (Buff)->Size) \
        ? STATUS_SUCCESS \
        : RtlpEnsureBufferSize((Flags), (Buff), (NewSizeBytes)) \
    )

//++
//
// VOID
// RtlFreeBuffer(
//     IN OUT PRTL_BUFFER Buffer,
//     );
//
//
// Routine Description:
//
// Free any heap allocated storage associated with Buffer.
// Notes:
// - RtlFreeBuffer returns a buffer to the state it was in after
//   calling RtlInitBuffer, so you may reuse it.
// - If you want to shrink the buffer without freeing it, just poke Buffer->Size down.
//     This is safe regardless of if the buffer has gone heap allocated or not.
// - You may RtlFreeBuffer an RTL_BUFFER that is all zeros. You do not need to track if you
//     called RtlInitBuffer if you know you filled it with zeros.
// - You may RtlFreeBuffer an RTL_BUFFER repeatedly.
//
// Arguments:
//
//     Buffer -
//
// Return Value:
//
//     none, unconditional success
//
//--
#define RtlFreeBuffer(Buff)                              \
    do {                                                 \
        if ((Buff) != NULL && (Buff)->Buffer != NULL) {  \
            if (RTLP_BUFFER_IS_HEAP_ALLOCATED(Buff)) {   \
                UNICODE_STRING UnicodeString;            \
                UnicodeString.Buffer = (PWSTR)(PVOID)(Buff)->Buffer; \
                RtlFreeUnicodeString(&UnicodeString);    \
            }                                            \
            (Buff)->Buffer = (Buff)->StaticBuffer;       \
            (Buff)->Size = (Buff)->StaticSize;           \
        }                                                \
    } while (0)

//
// a preallocated buffer that is "tied" to a UNICODE_STRING
//
struct _RTL_UNICODE_STRING_BUFFER;

typedef struct _RTL_UNICODE_STRING_BUFFER {
    UNICODE_STRING String;
    RTL_BUFFER     ByteBuffer;
    UCHAR          MinimumStaticBufferForTerminalNul[sizeof(WCHAR)];
} RTL_UNICODE_STRING_BUFFER, *PRTL_UNICODE_STRING_BUFFER;

//
// MAX_UNICODE_STRING_MAXLENGTH is the maximum allowed value for UNICODE_STRING::MaximumLength.
// MAX_UNICODE_STRING_LENGTH is the maximum allowed value for UNICODE_STRING::Length, allowing
//   room for a terminal nul.
//
// Explanation of MAX_UNICODE_STRING_MAXLENGTH implementation
//   ~0 is all bits set, maximum value in two's complement arithmetic, which C guarantees for unsigned types
//   << shifts out the number of bits that fit in UNICODE_STRING::Length
//   ~ and now we have all bits set that fit in UNICODE_STRING::Length,
//     like if UNICODE_STRING::Length is 16 bits, we have 0xFFFF
//   then mask so it is even multiple of whatever UNICODE_STRING::Buffer points to.
//   If Length is changed to ULONG or SIZE_T, this macro is still correct.
//   If Buffer pointed to CHAR or "WIDER_CHAR" or something else, this macro is still correct.
//
#define MAX_UNICODE_STRING_MAXLENGTH  ((~((~(SIZE_T)0) << (RTL_FIELD_SIZE(UNICODE_STRING, Length) * CHAR_BIT))) & ~(sizeof(((PCUNICODE_STRING)0)->Buffer[0]) - 1))
#define MAX_UNICODE_STRING_LENGTH     (MAX_UNICODE_STRING_MAXLENGTH - sizeof(((PCUNICODE_STRING)0)->Buffer[0]))

//++
//
// NTSTATUS
// RtlInitUnicodeStringBuffer(
//     OUT PRTL_UNICODE_STRING_BUFFER Buffer,
//     IN  PUCHAR                     StaticBuffer,
//     IN  SIZE_T                     StaticSize
//     );
//
// Routine Description:
//
//
// Arguments:
//
//     Buffer -
//     StaticBuffer - can be NULL, but generally is not
//     StaticSize - should be at least sizeof(WCHAR), but can be zero
//                  ought to be an even multiple of sizeof(WCHAR)
//                  gets rounded to down to an even multiple of sizeof(WCHAR)
//                  gets clamped to MAX_UNICODE_STRING_MAXLENGTH (64k - 2)
//
// RTL_UNICODE_STRING_BUFFER contains room for the terminal nul for the
// case of StaticBuffer == NULL or StaticSize < sizeof(WCHAR), or, more likely,
// for RtlTakeRemainingStaticBuffer leaving it with no static buffer.
//
// Return Value:
//
//     STATUS_SUCCESS
//
//--
#define RtlInitUnicodeStringBuffer(Buff, StatBuff, StatSize)      \
    do {                                                          \
        SIZE_T TempStaticSize = (StatSize);                       \
        PUCHAR TempStaticBuff = (StatBuff);                       \
        TempStaticSize &= ~(sizeof((Buff)->String.Buffer[0]) - 1);  \
        if (TempStaticSize > UNICODE_STRING_MAX_BYTES) {          \
            TempStaticSize = UNICODE_STRING_MAX_BYTES;            \
        }                                                         \
        if (TempStaticSize < sizeof(WCHAR)) {                     \
            TempStaticBuff = (Buff)->MinimumStaticBufferForTerminalNul; \
            TempStaticSize = sizeof(WCHAR);                       \
        }                                                         \
        RtlInitBuffer(&(Buff)->ByteBuffer, TempStaticBuff, TempStaticSize); \
        (Buff)->String.Buffer = (WCHAR*)TempStaticBuff;           \
        if ((Buff)->String.Buffer != NULL)                        \
            (Buff)->String.Buffer[0] = 0;                         \
        (Buff)->String.Length = 0;                                \
        (Buff)->String.MaximumLength = (RTL_STRING_LENGTH_TYPE)TempStaticSize;    \
    } while (0)

//++
//
// NTSTATUS
// RtlSyncStringToBuffer(
//      IN OUT PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer
//      );
//
// Routine Description:
//
// After carefully modifying the underlying RTL_BUFFER, this updates
// dependent fields in the underlying UNICODE_STRING.
//
// For example, use this after you grow the buffer with RtlEnsureBufferSize,
// but that example you don't need, use RtlEnsureUnicodeStringBufferSizeChars
// or RtlEnsureStringBufferSizeBytes.
//
// Arguments:
//
//     UnicodeStringBuffer - 
//
// Return Value:
//
//     STATUS_SUCCESS       - hooray
//
//--
#define RtlSyncStringToBuffer(x)                                            \
    (                                                                       \
      ( ASSERT((x)->String.Length < (x)->ByteBuffer.Size)                ), \
      ( ASSERT((x)->String.MaximumLength >= (x)->String.Length)          ), \
      ( ASSERT((x)->String.MaximumLength <= (x)->ByteBuffer.Size)        ), \
      ( (x)->String.Buffer        = (PWSTR)(x)->ByteBuffer.Buffer        ), \
      ( (x)->String.MaximumLength = (RTL_STRING_LENGTH_TYPE)((x)->ByteBuffer.Size) ), \
      ( ASSERT(RTL_STRING_IS_NUL_TERMINATED(&(x)->String))               ), \
      ( STATUS_SUCCESS                                                   )  \
    )

//++
//
// NTSTATUS
// RtlSyncBufferToString(
//      IN OUT PRTL_UNICODE_STRING_BUFFER UnicodeStringBuffer
//      );
//
// Routine Description:
//
// After carefully modifying the underlying UNICODE_STRING, this updates
// dependent fields in the underlying RTL_BUFFER.
//
// For example, use this after you the alloc the buffer with RtlAnsiStringToUnicodeString.
// This is possible because RTL_BUFFER deliberately uses the same memory allocator
// as RtlAnsiStringToUnicodeString.
//
// Arguments:
//
//     UnicodeStringBuffer - 
//
// Return Value:
//
//     STATUS_SUCCESS       - hooray
//
//--
#define RtlSyncBufferToString(Buff_)                                         \
    (                                                                        \
      ( (Buff_)->ByteBuffer.Buffer        = (Buff_)->String.Buffer        ), \
      ( (Buff_)->ByteBuffer.Buffer.Size   = (Buff_)->String.MaximumLength ), \
      ( STATUS_SUCCESS )                                                     \
    )

//++
//
// NTSTATUS
// RtlEnsureUnicodeStringBufferSizeChars(
//      IN OUT PRTL_BUFFER Buffer,
//      IN     USHORT      NewSizeChars
//      );
//
// NTSTATUS
// RtlEnsureUnicodeStringBufferSizeBytes(
//      IN OUT PRTL_BUFFER Buffer,
//      IN     USHORT      NewSizeBytes
//      );
//
// Routine Description:
//
// Optionally multiply cch to go from count of character to count of bytes.
// +1 or +2 for you to account for the terminal nul.
// Delegate to underlying RtlEnsureBufferSize.
// Keep String.Buffer, .MaximumLength, and terminal nul in sync.
//
// Arguments:
//
//     Buffer -
//     NewSizeChars -
//     NewSizeBytes - must be a multiple of sizeof(WCHAR), and we don't presently
//                    verify that.
//
// Return Value:
//
//     STATUS_SUCCESS       - hooray
//     STATUS_NO_MEMORY     - out of memory
//     STATUS_NAME_TOO_LONG - (NewSizeChars + 1) * sizeof(WCHAR) > UNICODE_STRING_MAX_BYTES (USHORT)
//
//--
#define RtlEnsureUnicodeStringBufferSizeBytes(Buff_, NewSizeBytes_)                            \
    (     ( ((NewSizeBytes_) + sizeof((Buff_)->String.Buffer[0])) > UNICODE_STRING_MAX_BYTES ) \
        ? STATUS_NAME_TOO_LONG                                                                 \
        : !NT_SUCCESS(RtlEnsureBufferSize(0, &(Buff_)->ByteBuffer, ((NewSizeBytes_) + sizeof((Buff_)->String.Buffer[0])))) \
        ? STATUS_NO_MEMORY                                                                      \
        : (RtlSyncStringToBuffer(Buff_))                                                       \
    )

#define RtlEnsureUnicodeStringBufferSizeChars(Buff_, NewSizeChars_) \
    (RtlEnsureUnicodeStringBufferSizeBytes((Buff_), (NewSizeChars_) * sizeof((Buff_)->String.Buffer[0])))

NTSTATUS
NTAPI
RtlMultiAppendUnicodeStringBuffer (
    _Out_ PRTL_UNICODE_STRING_BUFFER Destination,
    _In_ ULONG NumberOfSources,
    _In_ PCUNICODE_STRING SourceArray
    );

EXTERN_C_END