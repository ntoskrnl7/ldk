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
    PAGED_CODE();

    PWSTR p;
    ULONG n = Name->Length / sizeof(WCHAR);
    if (n == 0) {
        return STATUS_INVALID_PARAMETER;
    }

    try {
        p = Name->Buffer;
        while (--n) {
            if (*++p == L'=') {
                return STATUS_INVALID_PARAMETER;
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    PVOID pOld;
    PPEB Peb = NtCurrentPeb();    
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters = Peb->ProcessParameters;
    if (ARGUMENT_PRESENT(Environment)) {
        pOld = *Environment;
    } else {
        RtlAcquirePebLock();
        pOld = ProcessParameters->Environment;
    }

    MEMORY_BASIC_INFORMATION MemoryInformation;
    UNICODE_STRING CurrentName;
    UNICODE_STRING CurrentValue;
    ULONG Size;
    SIZE_T NewSize;
    LONG CompareResult;
    PWSTR pStart, pEnd;
    NTSTATUS Status = STATUS_VARIABLE_NOT_FOUND;
    PVOID pNew = NULL;
    PWSTR InsertionPoint = NULL;
    RtlpEnvironCacheValid = FALSE;
    try {
        try {
            p = pOld;
            pEnd = NULL;
            if (p) {
                while (*p) {
                    CurrentName.Buffer = p;
                    CurrentName.Length = 0;
                    CurrentName.MaximumLength = 0;
                    while (*p) {
                        if (*p == L'=' && p != CurrentName.Buffer) {
                            CurrentName.Length = (USHORT)(p - CurrentName.Buffer) * sizeof(WCHAR);
                            CurrentName.MaximumLength = (USHORT)(CurrentName.Length + sizeof(WCHAR));
                            CurrentValue.Buffer = ++p;
                            while(*p) {
                                p++;
                            }
                            CurrentValue.Length = (USHORT)(p - CurrentValue.Buffer) * sizeof(WCHAR);
                            CurrentValue.MaximumLength = (USHORT)(CurrentValue.Length + sizeof(WCHAR));
                            break;
                        } else {
                            p++;
                        }
                    }
                    p++;
                    if (! (CompareResult = RtlCompareUnicodeString( Name,
                                                                    &CurrentName,
                                                                    TRUE ))) {
                        pEnd = p;
                        while (*pEnd) {
                            while (*pEnd++) {}
                        }
                        pEnd++;

                        if (! ARGUMENT_PRESENT(Value)) {
                            RtlMoveMemory( CurrentName.Buffer,
                                           p,
                                           (ULONG)((pEnd - p) * sizeof(WCHAR)));
                            Status = STATUS_SUCCESS;
                        } else if (Value->Length <= CurrentValue.Length) {
                            pStart = CurrentValue.Buffer;
                            RtlMoveMemory( pStart,
                                           Value->Buffer,
                                           Value->Length );
                            pStart += Value->Length / sizeof(WCHAR);
                            *pStart++ = UNICODE_NULL;
                            RtlMoveMemory( pStart,
                                           p,
                                           (ULONG)((pEnd - p) * sizeof(WCHAR)) );
                            Status = STATUS_SUCCESS;
                        } else {
                            RtlZeroMemory( &MemoryInformation,
                                           sizeof(MemoryInformation) );
                            MemoryInformation.BaseAddress = pOld;
                            MemoryInformation.AllocationBase = pOld;
                            MemoryInformation.AllocationProtect = PAGE_READWRITE;
                            MemoryInformation.RegionSize = RtlSizeHeap( RtlProcessHeap(),
                                                                        0,
                                                                        pOld );
                            MemoryInformation.State = MEM_COMMIT;
                            MemoryInformation.Protect = PAGE_READWRITE;
                            MemoryInformation.Type = MEM_PRIVATE;
                            Status = STATUS_SUCCESS;

                            NewSize = (pEnd - (PWSTR)pOld) * sizeof(WCHAR) + Value->Length - CurrentValue.Length;
                            if (NewSize >= MemoryInformation.RegionSize) {
                                pNew = RtlAllocateHeap( RtlProcessHeap(),
                                                        0,
                                                        NewSize );
                                if (! pNew) {
                                    Status = STATUS_INSUFFICIENT_RESOURCES;
                                }
                                if (! NT_SUCCESS(Status)) {
                                    leave;
                                }
                                Size = (ULONG)(CurrentValue.Buffer - (PWSTR)pOld);
                                RtlMoveMemory( pNew,
                                               pOld,
                                               Size*sizeof(WCHAR) );
                                pStart = (PWSTR)pNew + Size;
                                RtlMoveMemory( pStart,
                                               Value->Buffer,
                                               Value->Length );
                                pStart += Value->Length/sizeof(WCHAR);
                                *pStart++ = UNICODE_NULL;
                                RtlMoveMemory( pStart,
                                               p,
                                               (ULONG)((pEnd - p) * sizeof(WCHAR)) );
                                if (ARGUMENT_PRESENT(Environment)) {
                                    *Environment = pNew;
                                } else {
                                    ProcessParameters->Environment = pNew;
                                    Peb->EnvironmentUpdateCount += 1;
                                }
                                RtlFreeHeap( RtlProcessHeap(),
                                            0,
                                            pOld );
                                pOld = NULL;

                                // untested :-(
                                KdBreakPoint();
                                pNew = pOld;
                            } else {
                                pStart = CurrentValue.Buffer + Value->Length/sizeof(WCHAR) + 1;
                                RtlMoveMemory( pStart,
                                               p,
                                               (ULONG)((pEnd - p)*sizeof(WCHAR)) );
                                *--pStart = UNICODE_NULL;
                                RtlMoveMemory( pStart - Value->Length/sizeof(WCHAR),
                                               Value->Buffer,
                                               Value->Length );
                            }
                        }
                        break;
                    } else if (CompareResult < 0) {
                        if (InsertionPoint == NULL) {
                            InsertionPoint = CurrentName.Buffer;
                        }
                    }
                }
            }
            if (InsertionPoint) {
                p = InsertionPoint;
            }

            if (pEnd == NULL && ARGUMENT_PRESENT(Value)) {
                if (p) {
                    pEnd = p;
                    while (*pEnd) {
                        while (*pEnd++) { }
                    }
                    pEnd++;
                    RtlZeroMemory( &MemoryInformation,
                                    sizeof(MemoryInformation) );
                    MemoryInformation.BaseAddress = pOld;
                    MemoryInformation.AllocationBase = pOld;
                    MemoryInformation.AllocationProtect = PAGE_READWRITE;
                    MemoryInformation.RegionSize = RtlSizeHeap( RtlProcessHeap(),
                                                                0,
                                                                pOld );
                    MemoryInformation.State = MEM_COMMIT;
                    MemoryInformation.Protect = PAGE_READWRITE;
                    MemoryInformation.Type = MEM_PRIVATE;
                    Status = STATUS_SUCCESS;

                    if (! NT_SUCCESS(Status)) {
                        leave;
                    }
                    NewSize = (pEnd - (PWSTR)pOld) * sizeof(WCHAR) + Name->Length + sizeof(WCHAR) + Value->Length + sizeof(WCHAR);
                } else {
                    NewSize = Name->Length + sizeof(WCHAR) + Value->Length + sizeof(WCHAR);
                    MemoryInformation.RegionSize = 0;
                }

                if (NewSize >= MemoryInformation.RegionSize) {
                    pNew = RtlAllocateHeap( RtlProcessHeap(),
                                            0,
                                            NewSize );
                    if (! pNew) {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }
                    if (! NT_SUCCESS(Status)) {
                        leave;
                    }
                    if (p) {
                        Size = (ULONG)(p - (PWSTR)pOld);
                        RtlMoveMemory( pNew,
                                       pOld,
                                       Size*sizeof(WCHAR) );
                    } else {
                        Size = 0;
                    }
                    pStart = (PWSTR)pNew + Size;
                    RtlMoveMemory( pStart,
                                   Name->Buffer,
                                   Name->Length );
                    pStart += Name->Length/sizeof(WCHAR);
                    *pStart++ = L'=';
                    RtlMoveMemory( pStart,
                                   Value->Buffer,
                                   Value->Length );
                    pStart += Value->Length/sizeof(WCHAR);
                    *pStart++ = UNICODE_NULL;
                    if (p) {
                        RtlMoveMemory( pStart,
                                       p,
                                       (ULONG)((pEnd - p)*sizeof(WCHAR)) );
                    }
                    if (ARGUMENT_PRESENT(Environment)) {
    		            *Environment = pNew;
                    } else {
    		            ProcessParameters->Environment = pNew;
                        Peb->EnvironmentUpdateCount += 1;
                    }
                    RtlFreeHeap( RtlProcessHeap(),
                                 0,
                                 pOld);
                } else {
                    pStart = p + Name->Length/sizeof(WCHAR) + 1 + Value->Length/sizeof(WCHAR) + 1;
                    RtlMoveMemory( pStart,
                                   p,
                                   (ULONG)((pEnd - p)*sizeof(WCHAR)) );
                    RtlMoveMemory( p,
                                   Name->Buffer,
                                   Name->Length );
                    p += Name->Length / sizeof(WCHAR);
                    *p++ = L'=';
                    RtlMoveMemory( p,
                                   Value->Buffer,
                                   Value->Length );
                    p += Value->Length/sizeof(WCHAR);
                    *p++ = UNICODE_NULL;
                }
            }
        } except(EXCEPTION_EXECUTE_HANDLER) {
              Status = STATUS_ACCESS_VIOLATION;
        }
    } finally {
        if (! ARGUMENT_PRESENT(Environment)) {
            RtlReleasePebLock();
        }
    }
    return Status;
}