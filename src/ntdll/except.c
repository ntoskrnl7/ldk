#include "ntdll.h"
#include "../kernel32/winbase.h"
#include <Ldk/winnt.h>

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
NTKERNELAPI
VOID
NTAPI
ExRaiseException (
    _In_ PEXCEPTION_RECORD ExceptionRecord
    );



LDK_INITIALIZE_COMPONENT LdkpInitializeDispatchExceptionStackVariables;
LDK_TERMINATE_COMPONENT LdkpTerminateDispatchExceptionStackVariables;



#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, LdkpInitializeDispatchExceptionStackVariables)
#pragma alloc_text(PAGE, LdkpTerminateDispatchExceptionStackVariables)
#endif



NPAGED_LOOKASIDE_LIST LdkpDispatchExceptionStackVariablesLookaside;

KSPIN_LOCK LdkpDispatchExceptionStackVariablesListLock;

LIST_ENTRY LdkpDispatchExceptionStackVariablesListHead;



VOID
LdkFreeDispatchExceptionStackVariables (
    _In_ PVOID Variables
    );



#if _AMD64_
FORCEINLINE
BOOLEAN
RtlpIsFrameInBounds (
    _Inout_ PULONG64 LowLimit,
    _In_ ULONG64 StackFrame,
    _Inout_ PULONG64 HighLimit
    )
{
    if ((StackFrame & 0x7) != 0) {
        return FALSE;
    }

    if ((StackFrame < *LowLimit) ||
        (StackFrame >= *HighLimit)) {
        return FALSE;

    } else {
        return TRUE;
    }
}

FORCEINLINE
VOID
RtlpCopyContext (
    _Out_ PCONTEXT Destination,
    _In_ PCONTEXT Source
    )
{
    Destination->Rip = Source->Rip;
    Destination->Rbx = Source->Rbx;
    Destination->Rsp = Source->Rsp;
    Destination->Rbp = Source->Rbp;
    Destination->Rsi = Source->Rsi;
    Destination->Rdi = Source->Rdi;
    Destination->R12 = Source->R12;
    Destination->R13 = Source->R13;
    Destination->R14 = Source->R14;
    Destination->R15 = Source->R15;
    Destination->Xmm6 = Source->Xmm6;
    Destination->Xmm7 = Source->Xmm7;
    Destination->Xmm8 = Source->Xmm8;
    Destination->Xmm9 = Source->Xmm9;
    Destination->Xmm10 = Source->Xmm10;
    Destination->Xmm11 = Source->Xmm11;
    Destination->Xmm12 = Source->Xmm12;
    Destination->Xmm13 = Source->Xmm13;
    Destination->Xmm14 = Source->Xmm14;
    Destination->Xmm15 = Source->Xmm15;
    Destination->SegCs = Source->SegCs;
    Destination->SegSs = Source->SegSs;
    Destination->MxCsr = Source->MxCsr;
    Destination->EFlags = Source->EFlags;
    return;
}

typedef struct _LDK_DISPATCH_EXCEPTION_STACK_VARIABLES {
    LIST_ENTRY Links;
	BOOLEAN Completion;
	CONTEXT ContextRecord1;
	ULONG64 ControlPc;
	DISPATCHER_CONTEXT DispatcherContext;
	EXCEPTION_DISPOSITION Disposition;
	ULONG64 EstablisherFrame;
	ULONG ExceptionFlags;
	PEXCEPTION_ROUTINE ExceptionRoutine;
	PRUNTIME_FUNCTION FunctionEntry;
	PVOID HandlerData;
	ULONG64 HighLimit;
	PUNWIND_HISTORY_TABLE HistoryTable;
	ULONG64 ImageBase;
	ULONG64 LowLimit;
	ULONG64 NestedFrame;
	BOOLEAN Repeat;
	ULONG ScopeIndex;
	UNWIND_HISTORY_TABLE UnwindTable;
} LDK_DISPATCH_EXCEPTION_STACK_VARIABLES, *PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES;

BOOLEAN
RtlDispatchException (
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord
    )
{
    PTEB Teb = NtCurrentTeb();
    PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES OldVariables = Teb->OldDispatchExceptionStackVariables;
    if (OldVariables) {
        LdkFreeDispatchExceptionStackVariables( OldVariables );
        Teb->OldDispatchExceptionStackVariables = NULL;
    }

	PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES Variables = ExAllocateFromNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside );
	if (! Variables) {
		ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
	}
    Teb->OldDispatchExceptionStackVariables = Variables;

    ExInterlockedInsertHeadList( &LdkpDispatchExceptionStackVariablesListHead,
                                 &Variables->Links,
                                 &LdkpDispatchExceptionStackVariablesListLock );

	Variables->Completion = FALSE;

    IoGetStackLimits( &Variables->LowLimit,
                      &Variables->HighLimit );

    RtlpCopyContext(&Variables->ContextRecord1, ContextRecord);
	Variables->ControlPc = (ULONG64)ExceptionRecord->ExceptionAddress;
	Variables->ExceptionFlags = FlagOn(ExceptionRecord->ExceptionFlags, EXCEPTION_NONCONTINUABLE);
	Variables->NestedFrame = 0;

	Variables->HistoryTable = &Variables->UnwindTable;
	Variables->HistoryTable->Count = 0;
	Variables->HistoryTable->Search = UNWIND_HISTORY_TABLE_NONE;
	Variables->HistoryTable->LowAddress = (ULONG64)-1;
	Variables->HistoryTable->HighAddress = 0;

    do {

		Variables->FunctionEntry = RtlLookupFunctionEntry( Variables->ControlPc,
                                                           &Variables->ImageBase,
                                                           Variables->HistoryTable );

        if (Variables->FunctionEntry) {
			Variables->ExceptionRoutine = RtlVirtualUnwind( UNW_FLAG_EHANDLER,
														    Variables->ImageBase,
                                                            Variables->ControlPc,
                                                            Variables->FunctionEntry,
                                                            &Variables->ContextRecord1,
                                                            &Variables->HandlerData,
                                                            &Variables->EstablisherFrame,
                                                            NULL );

            if (! RtlpIsFrameInBounds( &Variables->LowLimit,
                                       Variables->EstablisherFrame,
                                       &Variables->HighLimit )) {

				SetFlag(Variables->ExceptionFlags, EXCEPTION_STACK_INVALID);
                break;

            } else if (Variables->ExceptionRoutine) {

				Variables->ScopeIndex = 0;

                do {
                    ExceptionRecord->ExceptionFlags = Variables->ExceptionFlags;

					Variables->Repeat = FALSE;
					Variables->DispatcherContext.ControlPc = Variables->ControlPc;
					Variables->DispatcherContext.ImageBase = Variables->ImageBase;
					Variables->DispatcherContext.FunctionEntry = Variables->FunctionEntry;
					Variables->DispatcherContext.EstablisherFrame = Variables->EstablisherFrame;
					Variables->DispatcherContext.ContextRecord = &Variables->ContextRecord1;
					Variables->DispatcherContext.LanguageHandler = Variables->ExceptionRoutine;
					Variables->DispatcherContext.HandlerData = Variables->HandlerData;
					Variables->DispatcherContext.HistoryTable = Variables->HistoryTable;
					Variables->DispatcherContext.ScopeIndex = Variables->ScopeIndex;

					Variables->Disposition = Variables->DispatcherContext.LanguageHandler( ExceptionRecord,
                                                                                           (PVOID)Variables->EstablisherFrame,
                                                                                           ContextRecord,
                                                                                           &Variables->DispatcherContext );

					SetFlag(Variables->ExceptionFlags, FlagOn(ExceptionRecord->ExceptionFlags, EXCEPTION_NONCONTINUABLE));

                    if (Variables->NestedFrame == Variables->EstablisherFrame) {
						ClearFlag(Variables->ExceptionFlags, EXCEPTION_NESTED_CALL);
						Variables->NestedFrame = 0;
                    }
    
                    switch (Variables->Disposition) {
                    case ExceptionContinueExecution:
                        if (FlagOn(Variables->ExceptionFlags, EXCEPTION_NONCONTINUABLE)) {
                            LdkFreeDispatchExceptionStackVariables( Variables );
                            Variables = NULL;
                            ExRaiseStatus( STATUS_NONCONTINUABLE_EXCEPTION );
                        } else {
							Variables->Completion = TRUE;
                            goto DispatchExit;
                        }
                        break;

                    case ExceptionContinueSearch:
                        break;

                    case ExceptionNestedException:
                        SetFlag(Variables->ExceptionFlags, EXCEPTION_NESTED_CALL);
                        if (Variables->DispatcherContext.EstablisherFrame > Variables->NestedFrame) {
							Variables->NestedFrame = Variables->DispatcherContext.EstablisherFrame;
                        }
                        break;

                    case ExceptionCollidedUnwind:
						Variables->ControlPc = Variables->DispatcherContext.ControlPc;
						Variables->ImageBase = Variables->DispatcherContext.ImageBase;
						Variables->FunctionEntry = Variables->DispatcherContext.FunctionEntry;
						Variables->EstablisherFrame = Variables->DispatcherContext.EstablisherFrame;
                        RtlpCopyContext(&Variables->ContextRecord1, Variables->DispatcherContext.ContextRecord);
						Variables->ContextRecord1.Rip = Variables->ControlPc;
						Variables->ExceptionRoutine = Variables->DispatcherContext.LanguageHandler;
						Variables->HandlerData = Variables->DispatcherContext.HandlerData;
						Variables->HistoryTable = Variables->DispatcherContext.HistoryTable;
						Variables->ScopeIndex = Variables->DispatcherContext.ScopeIndex;
						Variables->Repeat = TRUE;
                        break;

                    default: {
                        LdkFreeDispatchExceptionStackVariables( Variables );
                        Variables = NULL;
                        ExRaiseStatus(STATUS_INVALID_DISPOSITION);
                        break;
                    }
                    }

                } while (Variables->Repeat);
            }

        } else {

            if (Variables->ControlPc == *(PULONG64)(Variables->ContextRecord1.Rsp)) {
                break;
            }
    
			Variables->ContextRecord1.Rip = *(PULONG64)(Variables->ContextRecord1.Rsp);
			Variables->ContextRecord1.Rsp += sizeof(ULONG64);
        }

		Variables->ControlPc = Variables->ContextRecord1.Rip;

    } while (RtlpIsFrameInBounds( &Variables->LowLimit,
								  (ULONG64)Variables->ContextRecord1.Rsp,
								  &Variables->HighLimit));

    ExceptionRecord->ExceptionFlags = Variables->ExceptionFlags;

DispatchExit:
	if (Variables) {
		BOOLEAN Completion = Variables->Completion;
        Teb->OldDispatchExceptionStackVariables = NULL;
        LdkFreeDispatchExceptionStackVariables( Variables );
		return Completion;
	}
    return FALSE;
}
#elif _X86_
typedef struct _DISPATCHER_CONTEXT {
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

// ntpsapi.h
#define EXCEPTION_CHAIN_END ((struct _EXCEPTION_REGISTRATION_RECORD * POINTER_32)-1)

typedef struct _LDK_DISPATCH_EXCEPTION_STACK_VARIABLES {
    LIST_ENTRY Links;
    BOOLEAN Completion;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
    PEXCEPTION_REGISTRATION_RECORD NestedRegistration;
    ULONG HighAddress;
    ULONG HighLimit;
    ULONG LowLimit;
} LDK_DISPATCH_EXCEPTION_STACK_VARIABLES, *PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES;

BOOLEAN
RtlDispatchException (
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord
    )
{
	PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES Variables = ExAllocateFromNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside );
	if (! Variables) {
		ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
	}

    ExInterlockedInsertHeadList( &LdkpDispatchExceptionStackVariablesListHead,
                                 &Variables->Links,
                                 &LdkpDispatchExceptionStackVariablesListLock );

    Variables->Completion = FALSE;
    
    IoGetStackLimits( &Variables->LowLimit,
                      &Variables->HighLimit );

    Variables->RegistrationPointer = (PEXCEPTION_REGISTRATION_RECORD)__readfsdword(0);
    Variables->NestedRegistration = 0;

    while (Variables->RegistrationPointer != EXCEPTION_CHAIN_END) {

        if (Variables->RegistrationPointer == Variables->RegistrationPointer->Next) {
            Variables->Completion = TRUE;
            goto DispatchExit;
        }

        Variables->HighAddress = (ULONG)Variables->RegistrationPointer + sizeof(EXCEPTION_REGISTRATION_RECORD);

        if (((ULONG)Variables->RegistrationPointer < Variables->LowLimit) ||
            (Variables->HighAddress > Variables->HighLimit) || (((ULONG)Variables->RegistrationPointer & 0x3) != 0)) {
            ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            goto DispatchExit;
        }

        Variables->Disposition = Variables->RegistrationPointer->Handler( ExceptionRecord,
                                                                          Variables->RegistrationPointer,
                                                                          ContextRecord,
                                                                          &Variables->DispatcherContext );

        if (Variables->NestedRegistration == Variables->RegistrationPointer) {
            ClearFlag(ExceptionRecord->ExceptionFlags, EXCEPTION_NESTED_CALL);
            Variables->NestedRegistration = 0;
        }

        switch (Variables->Disposition) {
        case ExceptionContinueExecution:
            if (FlagOn(ExceptionRecord->ExceptionFlags, EXCEPTION_NONCONTINUABLE)) {
                LdkFreeDispatchExceptionStackVariables( Variables );
                Variables = NULL;
                EXCEPTION_RECORD ExceptionRecord1;
                ExceptionRecord1.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
                ExceptionRecord1.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord1.ExceptionRecord = ExceptionRecord;
                ExceptionRecord1.NumberParameters = 0;
                ExRaiseException( &ExceptionRecord1 );
            } else {
                Variables->Completion = TRUE;
                goto DispatchExit;
            }
            break;

        case ExceptionContinueSearch:
            if (FlagOn(ExceptionRecord->ExceptionFlags, EXCEPTION_STACK_INVALID))
                goto DispatchExit;
            break;

        case ExceptionNestedException:
            ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;
            if (Variables->DispatcherContext.RegistrationPointer > Variables->NestedRegistration) {
                Variables->NestedRegistration = Variables->DispatcherContext.RegistrationPointer;
            }
            break;

        default:
            LdkFreeDispatchExceptionStackVariables( Variables );
            Variables = NULL;
            EXCEPTION_RECORD ExceptionRecord1;
            ExceptionRecord1.ExceptionCode = STATUS_INVALID_DISPOSITION;
            ExceptionRecord1.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord1.ExceptionRecord = ExceptionRecord;
            ExceptionRecord1.NumberParameters = 0;
            ExRaiseException( &ExceptionRecord1 );
            break;
        }
        Variables->RegistrationPointer = Variables->RegistrationPointer->Next;
    }

DispatchExit:
    if (Variables) {
        BOOLEAN Completion = Variables->Completion;
        LdkFreeDispatchExceptionStackVariables( Variables );
        Variables = NULL;
        return Completion;
    }
    return FALSE;
}
#endif



NTSTATUS
LdkpInitializeDispatchExceptionStackVariables (
	VOID
	)
{
    PAGED_CODE();

    KeInitializeSpinLock( &LdkpDispatchExceptionStackVariablesListLock );

    InitializeListHead( &LdkpDispatchExceptionStackVariablesListHead );

	ExInitializeNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_DISPATCH_EXCEPTION_STACK_VARIABLES),
									 'xEdL',
									 0 );
	return STATUS_SUCCESS;
}

VOID
LdkpTerminateDispatchExceptionStackVariables (
	VOID
	)
{
    PAGED_CODE();

    PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES Variables;
	PLIST_ENTRY Entry = RemoveHeadList( &LdkpDispatchExceptionStackVariablesListHead );
	while (Entry != &LdkpDispatchExceptionStackVariablesListHead) {
		Variables = CONTAINING_RECORD(Entry, LDK_DISPATCH_EXCEPTION_STACK_VARIABLES, Links);
		Entry = RemoveHeadList( &LdkpDispatchExceptionStackVariablesListHead );
        ExFreeToNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
                                     Variables );
	}
	ExDeleteNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside );
}

VOID
LdkFreeDispatchExceptionStackVariables (
    _In_ PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES Variables
    )
{
    KIRQL OldIrql;
    KeAcquireSpinLock( &LdkpDispatchExceptionStackVariablesListLock,
                       &OldIrql );
    RemoveEntryList( &Variables->Links );
    KeReleaseSpinLock( &LdkpDispatchExceptionStackVariablesListLock,
                       OldIrql );
    ExFreeToNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
                                 Variables );
}

VOID
NTAPI
RtlRaiseException (
    _In_ PEXCEPTION_RECORD ExceptionRecord
    )
{
	CONTEXT ContextRecord;
    RtlCaptureContext( &ContextRecord );
	ContextRecord.ContextFlags = CONTEXT_FULL;
#if _AMD64_
	ExceptionRecord->ExceptionAddress = (PVOID)ContextRecord.Rip;
#elif _X86_
	ExceptionRecord->ExceptionAddress = _ReturnAddress();
#endif
    if (RtlDispatchException( ExceptionRecord,
							  &ContextRecord )) {
        return;
    }

    ExRaiseException(ExceptionRecord);
}
#endif // _LDK_DEFINE_RTL_RAISE_EXCEPTION