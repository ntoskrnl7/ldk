
#include "ntdll.h"
#include "../ldk.h"

NTSTATUS
NTAPI
RtlFindCharInUnicodeString (
    _In_ ULONG Flags,
    _In_ PCUNICODE_STRING StringToSearch,
    _In_ PCUNICODE_STRING CharSet,
    _Out_ USHORT *NonInclusivePrefixLength
    )
{
    NTSTATUS Status;
    USHORT PrefixLengthFound = 0;
    USHORT CharsToSearch = 0;
    int MovementDirection = 0;
    PCWSTR Cursor = NULL;
    USHORT CharSetChars = 0;
    PCWSTR CharSetBuffer = NULL;
    USHORT i;

    if (NonInclusivePrefixLength != 0) {
        *NonInclusivePrefixLength = 0;
    }

    if (((Flags & ~(RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END |
                    RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET |
                    RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE)) != 0) ||
        (NonInclusivePrefixLength == NULL)) {
        Status = STATUS_INVALID_PARAMETER;
        goto Exit;
    }

    Status = RtlValidateUnicodeString(0, StringToSearch);
    if (! NT_SUCCESS(Status)) {
        goto Exit;
    }

    Status = RtlValidateUnicodeString(0, CharSet);
    if (! NT_SUCCESS(Status)) {
        goto Exit;
    }

    CharsToSearch = StringToSearch->Length / sizeof(WCHAR);
    CharSetChars = CharSet->Length / sizeof(WCHAR);
    CharSetBuffer = CharSet->Buffer;

    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END) {
        MovementDirection = -1;
        Cursor = StringToSearch->Buffer + CharsToSearch - 1;
    } else {
        MovementDirection = 1;
        Cursor = StringToSearch->Buffer;
    }
    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_CASE_INSENSITIVE) {
        WCHAR CharSetStackBuffer[32];
        if (CharSetChars <= RTL_NUMBER_OF(CharSetStackBuffer)) {
            for (i=0; i<CharSetChars; i++) {
                CharSetStackBuffer[i] = RtlDowncaseUnicodeChar( CharSetBuffer[i] );
            }
            while (CharsToSearch != 0) {
                CONST WCHAR wch = RtlDowncaseUnicodeChar( *Cursor );
                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetStackBuffer[i]) {
                            break;
                        }
                    }
                    if (i == CharSetChars) {
                        break;
                    }
                } else {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetStackBuffer[i]) {
                            break;
                        }
                    }

                    if (i != CharSetChars) {
                        break;
                    }
                }
                CharsToSearch--;
                Cursor += MovementDirection;
            }
        } else {
            while (CharsToSearch != 0) {
                const WCHAR wch = RtlDowncaseUnicodeChar( *Cursor );

                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i = 0; i < CharSetChars; i++) {
                        if (wch == RtlDowncaseUnicodeChar( CharSetBuffer[i]) ) {
                            break;
                        }
                    }

                    if (i == CharSetChars)
                        break;
                } else {
                    for (i = 0; i < CharSetChars; i++) {
                        if (wch == RtlDowncaseUnicodeChar( CharSetBuffer[i] )) {
                            break;
                        }
                    }
                    if (i != CharSetChars) {
                        break;
                    }
                }
                CharsToSearch--;
                Cursor += MovementDirection;
            }
        }
    } else {
        if (CharSetChars == 1) {
            const WCHAR wchSearchChar = CharSetBuffer[0];
            if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                while (CharsToSearch != 0) {
                    if (*Cursor != wchSearchChar) {
                        break;
                    }
                    CharsToSearch--;
                    Cursor += MovementDirection;
                }
            } else {
                while (CharsToSearch != 0) {
                    if (*Cursor == wchSearchChar) {
                        break;
                    }
                    CharsToSearch--;
                    Cursor += MovementDirection;
                }
            }
        } else {
            while (CharsToSearch != 0) {
                CONST WCHAR wch = *Cursor;
                if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_COMPLEMENT_CHAR_SET) {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetBuffer[i]) {
                            break;
                        }
                    }
                    if (i == CharSetChars) {
                        break;
                    }
                } else {
                    for (i=0; i<CharSetChars; i++) {
                        if (wch == CharSetBuffer[i]) {
                            break;
                        }
                    }
                    if (i != CharSetChars) {
                        break;
                    }
                }
                CharsToSearch--;
                Cursor += MovementDirection;
            }
        }
    }

    if (CharsToSearch == 0) {
        Status = STATUS_NOT_FOUND;
        goto Exit;
    }

    CharsToSearch--;

    if (Flags & RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END) {
        PrefixLengthFound = (USHORT) (CharsToSearch * sizeof(WCHAR));
    } else {
        PrefixLengthFound = (USHORT) (StringToSearch->Length - (CharsToSearch * sizeof(WCHAR)));
    }

    *NonInclusivePrefixLength = PrefixLengthFound;
    Status = STATUS_SUCCESS;

Exit:
    return Status;
}
