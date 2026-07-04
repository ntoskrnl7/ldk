#include "ntdll.h"
#include "../ldk.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlQueryEnvironmentVariable_U)
#pragma alloc_text(PAGE, RtlSetEnvironmentVariable)
#endif



#define TAG_ENV     'EkdL'

#pragma warning(disable:4701 4706)

const UNICODE_STRING RtlDosPathSeparatorsString = RTL_CONSTANT_STRING(L"\\/");



BOOLEAN RtlpEnvironCacheValid = FALSE;
UNICODE_STRING RtlpEnvironCacheName;
UNICODE_STRING RtlpEnvironCacheValue;



NTSTATUS
NTAPI
RtlQueryEnvironmentVariable_U (
    _Inout_opt_ PVOID Environment,
    _In_ PCUNICODE_STRING Name,
    _Inout_ PUNICODE_STRING Value
    )
{
    NTSTATUS Status = STATUS_VARIABLE_NOT_FOUND;
    BOOLEAN PebLockLocked = FALSE;

    PAGED_CODE();

    try {
        PWSTR p;
        PPEB Peb = NtCurrentPeb();
        PRTL_USER_PROCESS_PARAMETERS ProcessParameters = Peb->ProcessParameters;

        if (ARGUMENT_PRESENT(Environment)) {
            p = Environment;
            if (*p == UNICODE_NULL) {
                leave;
            }
        } else {
            PebLockLocked = TRUE;
            RtlAcquirePebLock();
            p = ProcessParameters->Environment;
        }
        if (RtlpEnvironCacheValid && p == ProcessParameters->Environment) {
            if (RtlEqualUnicodeString( Name,
                                       &RtlpEnvironCacheName,
                                       TRUE )) {
                Value->Length = RtlpEnvironCacheValue.Length;
                if (Value->MaximumLength >= RtlpEnvironCacheValue.Length) {
                    RtlCopyMemory (Value->Buffer,
                                   RtlpEnvironCacheValue.Buffer,
                                   RtlpEnvironCacheValue.Length);
                    if (Value->MaximumLength > RtlpEnvironCacheValue.Length) {
                        Value->Buffer[RtlpEnvironCacheValue.Length / sizeof(WCHAR)] = UNICODE_NULL;
                    }
                    Status = STATUS_SUCCESS;
                } else {
                    Status = STATUS_BUFFER_TOO_SMALL;
                }
                leave;
            }
        }

        UNICODE_STRING CurrentName;
        UNICODE_STRING CurrentValue;
        SIZE_T NameLength = Name->Length;
        SIZE_T NameChars = NameLength / sizeof (WCHAR);        
        SIZE_T len;
        PWSTR q;
        if (p) {
            for (;;) {
                len = wcslen( p );
                if (len == 0) {
                    break;
                }
                if (NameChars < len) {
                    q = &p[NameChars];
                    if (*q == L'=') {                    
                        CurrentName.Length = (USHORT) NameLength;
                        CurrentName.Buffer = p;
                        if (RtlEqualUnicodeString( Name,
                                                   &CurrentName,
                                                   TRUE ) &&
                            (wcschr( p + 1,
                                     L'=' ) == q)) {
                            CurrentValue.Buffer = q+1;
                            CurrentValue.Length = (USHORT) ((len - 1) * sizeof (WCHAR) - NameLength);
                            Value->Length = CurrentValue.Length;
                            if (Value->MaximumLength >= CurrentValue.Length) {
                                RtlCopyMemory( Value->Buffer,
                                               CurrentValue.Buffer,
                                               CurrentValue.Length );
                                if (Value->MaximumLength > CurrentValue.Length) {
                                    Value->Buffer[ CurrentValue.Length/sizeof(WCHAR) ] = UNICODE_NULL;
                                }
                                if (Environment == ProcessParameters->Environment) {
                                    RtlpEnvironCacheValid = TRUE;
                                    RtlpEnvironCacheName = CurrentName;
                                    RtlpEnvironCacheValue = CurrentValue;
                                }
                                Status = STATUS_SUCCESS;
                            } else {
                                Status = STATUS_BUFFER_TOO_SMALL;
                            }
                            break;
                        }
                    }
                }
                p += len + 1;
            }
        }

        if (Status == STATUS_VARIABLE_NOT_FOUND) {
            static const UNICODE_STRING CurrentWorkingDirectoryPseudoVariable = RTL_CONSTANT_STRING(L"__CD__");
            static const UNICODE_STRING ApplicationDirectoryPseudoVariable = RTL_CONSTANT_STRING(L"__APPDIR__");
            if (RtlEqualUnicodeString( Name,
                                       &CurrentWorkingDirectoryPseudoVariable,
                                       TRUE )) {
                if (! PebLockLocked) {
                    PebLockLocked = TRUE;
                    RtlAcquirePebLock();
                }
                CurrentValue = ProcessParameters->CurrentDirectory.DosPath;
                Status = STATUS_SUCCESS;
            } else if (RtlEqualUnicodeString( Name,
                                              &ApplicationDirectoryPseudoVariable,
                                              TRUE )) {
                USHORT PrefixLength = 0;
                if (!PebLockLocked) {
                    PebLockLocked = TRUE;
                    RtlAcquirePebLock();
                }
                CurrentValue = ProcessParameters->ImagePathName;
                Status = RtlFindCharInUnicodeString( RTL_FIND_CHAR_IN_UNICODE_STRING_START_AT_END,
                                                     &CurrentValue,
                                                     &RtlDosPathSeparatorsString,
                                                     &PrefixLength );
                if (NT_SUCCESS(Status)) {
                    CurrentValue.Length = PrefixLength + sizeof( WCHAR );
                } else if (Status == STATUS_NOT_FOUND) {
                    Status = STATUS_SUCCESS;
                }
            }
            if (NT_SUCCESS(Status)) {
                Value->Length = CurrentValue.Length;
                if (Value->MaximumLength >= CurrentValue.Length) {
                    RtlCopyMemory( Value->Buffer,
                                   CurrentValue.Buffer,
                                   CurrentValue.Length );
                    if (Value->MaximumLength > CurrentValue.Length) {
                        Value->Buffer[ CurrentValue.Length / sizeof(WCHAR) ] = UNICODE_NULL;
                    }
                }
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_ACCESS_VIOLATION;
    }
    if (PebLockLocked) {
        RtlReleasePebLock();
    }
    return Status;
}


NTSTATUS
NTAPI
RtlSetEnvironmentVariable (
    _Inout_opt_ PVOID *Environment,
    _In_ PCUNICODE_STRING Name,
    _In_opt_ PCUNICODE_STRING Value
    )
{
    PPEB Peb;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters;
    PVOID OldEnvironment;
    PVOID NewEnvironment;
    PWSTR OldBlock;
    PWSTR OldFinal;
    PWSTR Scan;
    PWSTR EntryStart;
    PWSTR EntryEnd;
    PWSTR EditStart;
    PWSTR EditEnd;
    PWSTR InsertBefore;
    PWSTR Out;
    ULONG NameChars;
    SIZE_T OldSize;
    SIZE_T RemovedSize;
    SIZE_T NewEntrySize;
    SIZE_T NewSize;
    SIZE_T PrefixSize;
    SIZE_T SuffixSize;
    BOOLEAN PebLockLocked;
    BOOLEAN Found;
    NTSTATUS Status;

    PAGED_CODE();

    Peb = NtCurrentPeb();
    ProcessParameters = Peb->ProcessParameters;
    OldEnvironment = NULL;
    NewEnvironment = NULL;
    OldBlock = NULL;
    OldFinal = NULL;
    Scan = NULL;
    EditStart = NULL;
    EditEnd = NULL;
    InsertBefore = NULL;
    PebLockLocked = FALSE;
    Found = FALSE;
    Status = STATUS_VARIABLE_NOT_FOUND;

    try {
        if (!Name ||
            !Name->Buffer ||
            Name->Length == 0 ||
            (Name->Length & (sizeof(WCHAR) - 1))) {
            return STATUS_INVALID_PARAMETER;
        }

        NameChars = Name->Length / sizeof(WCHAR);
        for (ULONG Index = 1; Index < NameChars; Index++) {
            if (Name->Buffer[Index] == L'=') {
                return STATUS_INVALID_PARAMETER;
            }
        }

        if (ARGUMENT_PRESENT(Value) &&
            Value->Length != 0 &&
            !Value->Buffer) {
            return STATUS_INVALID_PARAMETER;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    RtlpEnvironCacheValid = FALSE;

    try {
        try {
            if (ARGUMENT_PRESENT(Environment)) {
                OldEnvironment = *Environment;
            } else {
                PebLockLocked = TRUE;
                RtlAcquirePebLock();
                OldEnvironment = ProcessParameters->Environment;
            }

        OldBlock = (PWSTR)OldEnvironment;
        if (OldBlock) {
            Scan = OldBlock;
            while (*Scan) {
                UNICODE_STRING CurrentName;
                LONG CompareResult;
                PWSTR Delimiter;

                EntryStart = Scan;
                while (*Scan) {
                    Scan++;
                }
                EntryEnd = Scan + 1;

                Delimiter = EntryStart + 1;
                while (*Delimiter && *Delimiter != L'=') {
                    Delimiter++;
                }

                if (*Delimiter == L'=') {
                    CurrentName.Buffer = EntryStart;
                    CurrentName.Length = (USHORT)((Delimiter - EntryStart) * sizeof(WCHAR));
                    CurrentName.MaximumLength = CurrentName.Length;

                    CompareResult = RtlCompareUnicodeString( Name,
                                                             &CurrentName,
                                                             TRUE );
                    if (CompareResult == 0) {
                        Found = TRUE;
                        EditStart = EntryStart;
                        EditEnd = EntryEnd;
                        Scan = EntryEnd;
                        break;
                    }
                    if (CompareResult < 0 && !InsertBefore) {
                        InsertBefore = EntryStart;
                    }
                }

                Scan = EntryEnd;
            }
            OldFinal = Scan;
        }

        if (!ARGUMENT_PRESENT(Value) && !Found) {
            //
            // Windows treats deleting a missing environment variable as a
            // successful no-op.  MSVC UCRT depends on this for
            // _putenv_s(name, "") / _wputenv_s(name, "").
            //
            Status = STATUS_SUCCESS;
            leave;
        }

        if (!OldBlock) {
            OldSize = sizeof(WCHAR);
        } else {
            while (*Scan) {
                while (*Scan++) {
                }
            }
            OldFinal = Scan;
            OldSize = (OldFinal - OldBlock + 1) * sizeof(WCHAR);
        }

        if (!Found) {
            EditStart = InsertBefore ? InsertBefore : OldFinal;
            EditEnd = EditStart;
        }

        RemovedSize = Found ? ((EditEnd - EditStart) * sizeof(WCHAR)) : 0;
        NewEntrySize = ARGUMENT_PRESENT(Value) ?
            (Name->Length + sizeof(WCHAR) + Value->Length + sizeof(WCHAR)) :
            0;
        NewSize = OldSize - RemovedSize + NewEntrySize;
        if (NewSize < sizeof(WCHAR)) {
            NewSize = sizeof(WCHAR);
        }

        NewEnvironment = RtlAllocateHeap( RtlProcessHeap(),
                                          HEAP_ZERO_MEMORY,
                                          NewSize );
        if (!NewEnvironment) {
            Status = STATUS_INSUFFICIENT_RESOURCES;
            leave;
        }

        Out = (PWSTR)NewEnvironment;
        if (OldBlock && EditStart > OldBlock) {
            PrefixSize = (EditStart - OldBlock) * sizeof(WCHAR);
            RtlCopyMemory( Out,
                           OldBlock,
                           PrefixSize );
            Out += PrefixSize / sizeof(WCHAR);
        }

        if (ARGUMENT_PRESENT(Value)) {
            RtlCopyMemory( Out,
                           Name->Buffer,
                           Name->Length );
            Out += Name->Length / sizeof(WCHAR);
            *Out++ = L'=';
            if (Value->Length != 0) {
                RtlCopyMemory( Out,
                               Value->Buffer,
                               Value->Length );
                Out += Value->Length / sizeof(WCHAR);
            }
            *Out++ = UNICODE_NULL;
        }

        if (OldBlock) {
            SuffixSize = (OldFinal - EditEnd + 1) * sizeof(WCHAR);
            if (SuffixSize != 0) {
                RtlCopyMemory( Out,
                               EditEnd,
                               SuffixSize );
            }
        }

        if (ARGUMENT_PRESENT(Environment)) {
            *Environment = NewEnvironment;
        } else {
            ProcessParameters->Environment = NewEnvironment;
            ProcessParameters->EnvironmentSize = (ULONG)NewSize;
            Peb->EnvironmentUpdateCount += 1;
        }

        if (OldEnvironment) {
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         OldEnvironment );
        }

            NewEnvironment = NULL;
            Status = STATUS_SUCCESS;
        } except(EXCEPTION_EXECUTE_HANDLER) {
            Status = STATUS_ACCESS_VIOLATION;
        }
    } finally {
        if (!NT_SUCCESS(Status) && NewEnvironment) {
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         NewEnvironment );
        }
        if (PebLockLocked) {
            RtlReleasePebLock();
        }
    }
    return Status;
}
