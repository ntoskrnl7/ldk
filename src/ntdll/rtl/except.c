#include "../ntdll.h"
#include "../../kernel32/winbase.h"
#include "../../nt/zwapi.h"
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

VOID
LdkpTerminateDispatchExceptionStackVariables (
	VOID
	)
{
    PAGED_CODE();
	ExDeleteNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside );
}
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

NPAGED_LOOKASIDE_LIST LdkpDispatchExceptionStackVariablesLookaside;



NTSTATUS
LdkpInitializeDispatchExceptionStackVariables (
	VOID
	)
{
    PAGED_CODE();

	ExInitializeNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_DISPATCH_EXCEPTION_STACK_VARIABLES),
									 'xEdL',
									 0 );
	return STATUS_SUCCESS;
}



BOOLEAN
RtlDispatchException (
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord
    )
{
	PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES variables = ExAllocateFromNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside );
	if (!variables) {
		ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
	}

	variables->Completion = FALSE;

    IoGetStackLimits( &variables->LowLimit,
                      &variables->HighLimit );

    RtlpCopyContext(&variables->ContextRecord1, ContextRecord);
	variables->ControlPc = (ULONG64)ExceptionRecord->ExceptionAddress;
	variables->ExceptionFlags = FlagOn(ExceptionRecord->ExceptionFlags, EXCEPTION_NONCONTINUABLE);
	variables->NestedFrame = 0;

	variables->HistoryTable = &variables->UnwindTable;
	variables->HistoryTable->Count = 0;
	variables->HistoryTable->Search = UNWIND_HISTORY_TABLE_NONE;
	variables->HistoryTable->LowAddress = (ULONG64)-1;
	variables->HistoryTable->HighAddress = 0;

    do {

		variables->FunctionEntry = RtlLookupFunctionEntry( variables->ControlPc,
                                                           &variables->ImageBase,
                                                           variables->HistoryTable );

        if (variables->FunctionEntry) {
			variables->ExceptionRoutine = RtlVirtualUnwind( UNW_FLAG_EHANDLER,
														    variables->ImageBase,
                                                            variables->ControlPc,
                                                            variables->FunctionEntry,
                                                            &variables->ContextRecord1,
                                                            &variables->HandlerData,
                                                            &variables->EstablisherFrame,
                                                            NULL );

            if (! RtlpIsFrameInBounds( &variables->LowLimit,
                                       variables->EstablisherFrame,
                                       &variables->HighLimit )) {

				SetFlag(variables->ExceptionFlags, EXCEPTION_STACK_INVALID);
                break;

            } else if (variables->ExceptionRoutine) {

				variables->ScopeIndex = 0;

                do {
                    ExceptionRecord->ExceptionFlags = variables->ExceptionFlags;

					variables->Repeat = FALSE;
					variables->DispatcherContext.ControlPc = variables->ControlPc;
					variables->DispatcherContext.ImageBase = variables->ImageBase;
					variables->DispatcherContext.FunctionEntry = variables->FunctionEntry;
					variables->DispatcherContext.EstablisherFrame = variables->EstablisherFrame;
					variables->DispatcherContext.ContextRecord = &variables->ContextRecord1;
					variables->DispatcherContext.LanguageHandler = variables->ExceptionRoutine;
					variables->DispatcherContext.HandlerData = variables->HandlerData;
					variables->DispatcherContext.HistoryTable = variables->HistoryTable;
					variables->DispatcherContext.ScopeIndex = variables->ScopeIndex;

					variables->Disposition = variables->DispatcherContext.LanguageHandler( ExceptionRecord,
                                                                                           (PVOID)variables->EstablisherFrame,
                                                                                           ContextRecord,
                                                                                           &variables->DispatcherContext );

					SetFlag(variables->ExceptionFlags, FlagOn(ExceptionRecord->ExceptionFlags, EXCEPTION_NONCONTINUABLE));

                    if (variables->NestedFrame == variables->EstablisherFrame) {
						ClearFlag(variables->ExceptionFlags, EXCEPTION_NESTED_CALL);
						variables->NestedFrame = 0;
                    }
    
                    switch (variables->Disposition) {
                    case ExceptionContinueExecution:
                        if (FlagOn(variables->ExceptionFlags, EXCEPTION_NONCONTINUABLE)) {
                            ExRaiseStatus( STATUS_NONCONTINUABLE_EXCEPTION );
                        } else {
							variables->Completion = TRUE;
                            goto DispatchExit;
                        }
    
                    case ExceptionContinueSearch:
                        break;

                    case ExceptionNestedException:
                        SetFlag(variables->ExceptionFlags, EXCEPTION_NESTED_CALL);
                        if (variables->DispatcherContext.EstablisherFrame > variables->NestedFrame) {
							variables->NestedFrame = variables->DispatcherContext.EstablisherFrame;
                        }
                        break;

                    case ExceptionCollidedUnwind:
						variables->ControlPc = variables->DispatcherContext.ControlPc;
						variables->ImageBase = variables->DispatcherContext.ImageBase;
						variables->FunctionEntry = variables->DispatcherContext.FunctionEntry;
						variables->EstablisherFrame = variables->DispatcherContext.EstablisherFrame;
                        RtlpCopyContext(&variables->ContextRecord1, variables->DispatcherContext.ContextRecord);
						variables->ContextRecord1.Rip = variables->ControlPc;
						variables->ExceptionRoutine = variables->DispatcherContext.LanguageHandler;
						variables->HandlerData = variables->DispatcherContext.HandlerData;
						variables->HistoryTable = variables->DispatcherContext.HistoryTable;
						variables->ScopeIndex = variables->DispatcherContext.ScopeIndex;
						variables->Repeat = TRUE;
                        break;

                    default:
                        ExRaiseStatus(STATUS_INVALID_DISPOSITION);
                    }

                } while (variables->Repeat);
            }

        } else {

            if (variables->ControlPc == *(PULONG64)(variables->ContextRecord1.Rsp)) {
                break;
            }
    
			variables->ContextRecord1.Rip = *(PULONG64)(variables->ContextRecord1.Rsp);
			variables->ContextRecord1.Rsp += sizeof(ULONG64);
        }

		variables->ControlPc = variables->ContextRecord1.Rip;

    } while (RtlpIsFrameInBounds( &variables->LowLimit,
								  (ULONG64)variables->ContextRecord1.Rsp,
								  &variables->HighLimit));

    ExceptionRecord->ExceptionFlags = variables->ExceptionFlags;

DispatchExit:
	{
		BOOLEAN Completion = variables->Completion;
		ExFreeToNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
                                    variables );
		return Completion;
	}
}
#elif _X86_
typedef struct _DISPATCHER_CONTEXT {
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

// ntpsapi.h
#define EXCEPTION_CHAIN_END ((struct _EXCEPTION_REGISTRATION_RECORD * POINTER_32)-1)



typedef struct _LDK_DISPATCH_EXCEPTION_STACK_VARIABLES {
    BOOLEAN Completion;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
    PEXCEPTION_REGISTRATION_RECORD NestedRegistration;
    ULONG HighAddress;
    ULONG HighLimit;
    ULONG LowLimit;
} LDK_DISPATCH_EXCEPTION_STACK_VARIABLES, *PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES;



NTSTATUS
LdkpInitializeDispatchExceptionStackVariables (
	VOID
	)
{
    PAGED_CODE();

	ExInitializeNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
									 NULL,
									 NULL,
									 0,
									 sizeof(LDK_DISPATCH_EXCEPTION_STACK_VARIABLES),
									 'xEdL',
									 0 );
	return STATUS_SUCCESS;
}



BOOLEAN
RtlDispatchException (
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord
    )
{
	PLDK_DISPATCH_EXCEPTION_STACK_VARIABLES variables = ExAllocateFromNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside );
	if (!variables) {
		ExRaiseStatus( STATUS_INSUFFICIENT_RESOURCES );
	}

    variables->Completion = FALSE;
    
    IoGetStackLimits( &variables->LowLimit,
                      &variables->HighLimit );

    variables->RegistrationPointer = (PEXCEPTION_REGISTRATION_RECORD)__readfsdword(0);
    variables->NestedRegistration = 0;

    while (variables->RegistrationPointer != EXCEPTION_CHAIN_END) {

        if (variables->RegistrationPointer == variables->RegistrationPointer->Next) {
            variables->Completion = TRUE;
            goto DispatchExit;
        }

        variables->HighAddress = (ULONG)variables->RegistrationPointer + sizeof(EXCEPTION_REGISTRATION_RECORD);

        if (((ULONG)variables->RegistrationPointer < variables->LowLimit) ||
            (variables->HighAddress > variables->HighLimit) || (((ULONG)variables->RegistrationPointer & 0x3) != 0)) {
            ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            goto DispatchExit;
        }

        variables->Disposition = variables->RegistrationPointer->Handler( ExceptionRecord,
                                                                          variables->RegistrationPointer,
                                                                          ContextRecord,
                                                                          &variables->DispatcherContext );

        if (variables->NestedRegistration == variables->RegistrationPointer) {
            ClearFlag(ExceptionRecord->ExceptionFlags, EXCEPTION_NESTED_CALL);
            variables->NestedRegistration = 0;
        }

        switch (variables->Disposition) {
        case ExceptionContinueExecution:
            if (FlagOn(ExceptionRecord->ExceptionFlags, EXCEPTION_NONCONTINUABLE)) {
                ExFreeToNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
                                            variables );
                EXCEPTION_RECORD ExceptionRecord1;
                ExceptionRecord1.ExceptionCode = STATUS_NONCONTINUABLE_EXCEPTION;
                ExceptionRecord1.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord1.ExceptionRecord = ExceptionRecord;
                ExceptionRecord1.NumberParameters = 0;
                ExRaiseException( &ExceptionRecord1 );
            } else {
                variables->Completion = TRUE;
                goto DispatchExit;
            }

        case ExceptionContinueSearch :
            if (FlagOn(ExceptionRecord->ExceptionFlags, EXCEPTION_STACK_INVALID))
                goto DispatchExit;
            break;

        case ExceptionNestedException :
            ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;
            if (variables->DispatcherContext.RegistrationPointer > variables->NestedRegistration) {
                variables->NestedRegistration = variables->DispatcherContext.RegistrationPointer;
            }
            break;

        default : {
            ExFreeToNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
                                         variables );
            EXCEPTION_RECORD ExceptionRecord1;
            ExceptionRecord1.ExceptionCode = STATUS_INVALID_DISPOSITION;
            ExceptionRecord1.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord1.ExceptionRecord = ExceptionRecord;
            ExceptionRecord1.NumberParameters = 0;
            ExRaiseException( &ExceptionRecord1 );
            break;
        }
        }
        variables->RegistrationPointer = variables->RegistrationPointer->Next;
    }

DispatchExit:
	{
		BOOLEAN Completion = variables->Completion;
		ExFreeToNPagedLookasideList( &LdkpDispatchExceptionStackVariablesLookaside,
                                     variables );
		return Completion;
	}
}
#endif
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