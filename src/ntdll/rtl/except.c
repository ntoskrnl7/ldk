#include "../ntdll.h"
#include "../../kernel32/winbase.h"
#include "../../nt/zwapi.h"
#include <Ldk/winnt.h>

#if _LDK_DEFINE_RTL_RAISE_EXCEPTION
#if (defined(_M_X64) || defined(_M_ARM) || defined(_M_ARM64)) && !defined(_CHPE_X86_ARM64_EH_)
#define STACK_MISALIGNMENT ((1 << 3) - 1)
#else
#define STACK_MISALIGNMENT 3
#endif

NTKERNELAPI
VOID
NTAPI
ExRaiseException (
    _In_ PEXCEPTION_RECORD ExceptionRecord
    );

#if _AMD64_
BOOLEAN
RtlpIsFrameInBounds (
    IN OUT PULONG64 LowLimit,
    IN ULONG64 StackFrame,
    IN OUT PULONG64 HighLimit
    )

/*++

Routine Description:

    This function checks whether the specified frame address is properly
    aligned and within the specified limits. In kernel mode an additional
    check is made if the frame is not within the specified limits since
    the kernel stack can be expanded. For this case the next entry in the
    expansion list, if any, is checked. If the frame is within the next
    expansion extent, then the extent values are stored in the low and
    high limit before returning to the caller.

    N.B. It is assumed that the supplied high limit is the stack base.

Arguments:

    LowLimit - Supplies a pointer to a variable that contains the current
        lower stack limit.

    Frame - Supplies the frame address to check.

    HighLimit - Supplies a pointer to a variable that contains the current
        high stack limit.

Return Value:

    If the specified stack frame is within limits, then a value of TRUE is
    returned as the function value. Otherwise, a value of FALSE is returned.

--*/

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

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException (
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ ULONG64 EstablisherFrame,
    _Inout_ PCONTEXT ContextRecord,
    _Inout_ PDISPATCHER_CONTEXT DispatcherContext
    )
{
    return DispatcherContext->LanguageHandler(ExceptionRecord, (PVOID)EstablisherFrame, ContextRecord, DispatcherContext);
}

VOID
RtlpCopyContext (
    _Out_ PCONTEXT Destination,
    _In_ PCONTEXT Source
    )

/*++

Routine Description:

    This function copies the nonvolatile context required for exception
    dispatch and unwind from the specified source context record to the
    specified destination context record.

Arguments:

    Destination - Supplies a pointer to the destination context record.

    Source - Supplies a pointer to the source context record.

Return Value:

    None.

--*/

{

    //
    // Copy nonvolatile context required for exception dispatch and unwind.
    //

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

BOOLEAN
RtlDispatchException (
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord
    )
{
    BOOLEAN Completion = FALSE;
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

    //
    // Get current stack limits, copy the context record, get the initial
    // PC value, capture the exception flags, and set the nested exception
    // frame pointer.
    //

    IoGetStackLimits(&LowLimit, &HighLimit);
    RtlpCopyContext(&ContextRecord1, ContextRecord);
    ControlPc = (ULONG64)ExceptionRecord->ExceptionAddress;
    ExceptionFlags = ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE;
    NestedFrame = 0;

    //
    // Initialize the unwind history table.
    //

    HistoryTable = &UnwindTable;
    HistoryTable->Count = 0;
    HistoryTable->Search = UNWIND_HISTORY_TABLE_NONE;
    HistoryTable->LowAddress = (ULONG64)-1;
    HistoryTable->HighAddress = 0;

    //
    // Start with the frame specified by the context record and search
    // backwards through the call frame hierarchy attempting to find an
    // exception handler that will handle the exception.
    //

    do {

        //
        // Lookup the function table entry using the point at which control
        // left the procedure.
        //

        FunctionEntry = RtlLookupFunctionEntry(ControlPc,
                                               &ImageBase,
                                               HistoryTable);

        //
        // If there is a function table entry for the routine, then virtually
        // unwind to the caller of the current routine to obtain the virtual
        // frame pointer of the establisher and check if there is an exception
        // handler for the frame.
        //

        if (FunctionEntry != NULL) {
            ExceptionRoutine = RtlVirtualUnwind(UNW_FLAG_EHANDLER,
                                                ImageBase,
                                                ControlPc,
                                                FunctionEntry,
                                                &ContextRecord1,
                                                &HandlerData,
                                                &EstablisherFrame,
                                                NULL);

            //
            // If the establisher frame pointer is not within the specified
            // stack limits or the established frame pointer is unaligned,
            // then set the stack invalid flag in the exception record and
            // return exception not handled. Otherwise, check if the current
            // routine has an exception handler.
            //

            if (RtlpIsFrameInBounds(&LowLimit, EstablisherFrame, &HighLimit) == FALSE) {
                ExceptionFlags |= EXCEPTION_STACK_INVALID;
                break;

            } else if (ExceptionRoutine != NULL) {

                //
                // The frame has an exception handler.
                //
                // A linkage routine written in assembler is used to actually
                // call the actual exception handler. This is required by the
                // exception handler that is associated with the linkage
                // routine so it can have access to two sets of dispatcher
                // context when it is called.
                //
                // Call the language specific handler.
                //

                ScopeIndex = 0;
                do {

                    //
                    // Log the exception if exception logging is enabled.
                    //
    
                    ExceptionRecord->ExceptionFlags = ExceptionFlags;

                    //
                    // Clear repeat, set the dispatcher context, and call the
                    // exception handler.
                    //

                    Repeat = FALSE;
                    DispatcherContext.ControlPc = ControlPc;
                    DispatcherContext.ImageBase = ImageBase;
                    DispatcherContext.FunctionEntry = FunctionEntry;
                    DispatcherContext.EstablisherFrame = EstablisherFrame;
                    DispatcherContext.ContextRecord = &ContextRecord1;
                    DispatcherContext.LanguageHandler = ExceptionRoutine;
                    DispatcherContext.HandlerData = HandlerData;
                    DispatcherContext.HistoryTable = HistoryTable;
                    DispatcherContext.ScopeIndex = ScopeIndex;
                    Disposition =
                        RtlpExecuteHandlerForException(ExceptionRecord,
                                                       EstablisherFrame,
                                                       ContextRecord,
                                                       &DispatcherContext);

                    //
                    // Propagate noncontinuable exception flag.
                    //
    
                    ExceptionFlags |=
                        (ExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE);

                    //
                    // If the current scan is within a nested context and the
                    // frame just examined is the end of the nested region,
                    // then clear the nested context frame and the nested
                    // exception flag in the exception flags.
                    //
    
                    if (NestedFrame == EstablisherFrame) {
                        ExceptionFlags &= (~EXCEPTION_NESTED_CALL);
                        NestedFrame = 0;
                    }
    
                    //
                    // Case on the handler disposition.
                    //
    
                    switch (Disposition) {
    
                        //
                        // The disposition is to continue execution.
                        //
                        // If the exception is not continuable, then raise
                        // the exception STATUS_NONCONTINUABLE_EXCEPTION.
                        // Otherwise return exception handled.
                        //
    
                    case ExceptionContinueExecution :
                        if ((ExceptionFlags & EXCEPTION_NONCONTINUABLE) != 0) {
                            RtlRaiseStatus(STATUS_NONCONTINUABLE_EXCEPTION);
    
                        } else {
                            Completion = TRUE;
                            goto DispatchExit;
                        }
    
                        //
                        // The disposition is to continue the search.
                        //
                        // Get next frame address and continue the search.
                        //
    
                    case ExceptionContinueSearch :
                        break;
    
                        //
                        // The disposition is nested exception.
                        //
                        // Set the nested context frame to the establisher frame
                        // address and set the nested exception flag in the
                        // exception flags.
                        //
    
                    case ExceptionNestedException :
                        ExceptionFlags |= EXCEPTION_NESTED_CALL;
                        if (DispatcherContext.EstablisherFrame > NestedFrame) {
                            NestedFrame = DispatcherContext.EstablisherFrame;
                        }
    
                        break;

                        //
                        // The dispostion is collided unwind.
                        //
                        // A collided unwind occurs when an exception dispatch
                        // encounters a previous call to an unwind handler. In
                        // this case the previous unwound frames must be skipped.
                        //

                    case ExceptionCollidedUnwind:
                        ControlPc = DispatcherContext.ControlPc;
                        ImageBase = DispatcherContext.ImageBase;
                        FunctionEntry = DispatcherContext.FunctionEntry;
                        EstablisherFrame = DispatcherContext.EstablisherFrame;
                        RtlpCopyContext(&ContextRecord1,
                                        DispatcherContext.ContextRecord);

                        ContextRecord1.Rip = ControlPc;
                        ExceptionRoutine = DispatcherContext.LanguageHandler;
                        HandlerData = DispatcherContext.HandlerData;
                        HistoryTable = DispatcherContext.HistoryTable;
                        ScopeIndex = DispatcherContext.ScopeIndex;
                        Repeat = TRUE;
                        break;

                        //
                        // All other disposition values are invalid.
                        //
                        // Raise invalid disposition exception.
                        //
    
                    default :
                        RtlRaiseStatus(STATUS_INVALID_DISPOSITION);
                    }

                } while (Repeat != FALSE);
            }

        } else {

            //
            // If the old control PC is the same as the return address,
            // then no progress is being made and the function tables are
            // most likely malformed.
            //
    
            if (ControlPc == *(PULONG64)(ContextRecord1.Rsp)) {
                break;
            }
    
            //
            // Set the point where control left the current function by
            // obtaining the return address from the top of the stack.
            //

            ContextRecord1.Rip = *(PULONG64)(ContextRecord1.Rsp);
            ContextRecord1.Rsp += 8;
        }

        //
        // Set point at which control left the previous routine.
        //

        ControlPc = ContextRecord1.Rip;
    } while (RtlpIsFrameInBounds(&LowLimit, (ULONG64)ContextRecord1.Rsp, &HighLimit) == TRUE);

    //
    // Set final exception flags and return exception not handled.
    //

    ExceptionRecord->ExceptionFlags = ExceptionFlags;

    //
    // Call vectored continue handlers.
    //

DispatchExit:

    return Completion;
}
#elif _X86_
typedef struct _DISPATCHER_CONTEXT {
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

// ntpsapi.h
#define EXCEPTION_CHAIN_END ((struct _EXCEPTION_REGISTRATION_RECORD * POINTER_32)-1)

PEXCEPTION_REGISTRATION_RECORD
RtlpGetRegistrationHead (
    VOID
    )
{
    return (PEXCEPTION_REGISTRATION_RECORD)__readfsdword(0);
}

EXCEPTION_DISPOSITION
RtlpExecuteHandlerForException (
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID EstablisherFrame,
    _Inout_ PCONTEXT ContextRecord,
    _Inout_ PDISPATCHER_CONTEXT DispatcherContext,
    _In_ PEXCEPTION_ROUTINE ExceptionRoutine
    )
{
    return ExceptionRoutine(ExceptionRecord, EstablisherFrame, ContextRecord, DispatcherContext);
}

BOOLEAN
RtlDispatchException (
    _In_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PCONTEXT ContextRecord
    )
{
    BOOLEAN Completion = FALSE;
    DISPATCHER_CONTEXT DispatcherContext;
    EXCEPTION_DISPOSITION Disposition;
    PEXCEPTION_REGISTRATION_RECORD RegistrationPointer;
    PEXCEPTION_REGISTRATION_RECORD NestedRegistration;
    ULONG HighAddress;
    ULONG HighLimit;
    ULONG LowLimit;
    EXCEPTION_RECORD ExceptionRecord1;

    //
    // Get current stack limits.
    //
    IoGetStackLimits(&LowLimit, &HighLimit);

    //
    // Start with the frame specified by the context record and search
    // backwards through the call frame hierarchy attempting to find an
    // exception handler that will handler the exception.
    //

    RegistrationPointer = RtlpGetRegistrationHead();
    NestedRegistration = 0;

    while (RegistrationPointer != EXCEPTION_CHAIN_END) {

        //
        // If the call frame is not within the specified stack limits or the
        // call frame is unaligned, then set the stack invalid flag in the
        // exception record and return FALSE. Else check to determine if the
        // frame has an exception handler.
        //

        HighAddress = (ULONG)RegistrationPointer +
            sizeof(EXCEPTION_REGISTRATION_RECORD);

        if ( ((ULONG)RegistrationPointer < LowLimit) ||
             (HighAddress > HighLimit) ||
             (((ULONG)RegistrationPointer & 0x3) != 0) 
           ) {
            ExceptionRecord->ExceptionFlags |= EXCEPTION_STACK_INVALID;
            goto DispatchExit;
        }

        Disposition = RtlpExecuteHandlerForException(
            ExceptionRecord,
            (PVOID)RegistrationPointer,
            ContextRecord,
            (PVOID)&DispatcherContext,
            (PEXCEPTION_ROUTINE)RegistrationPointer->Handler);

        //
        // If the current scan is within a nested context and the frame
        // just examined is the end of the context region, then clear
        // the nested context frame and the nested exception in the
        // exception flags.
        //

        if (NestedRegistration == RegistrationPointer) {
            ExceptionRecord->ExceptionFlags &= (~EXCEPTION_NESTED_CALL);
            NestedRegistration = 0;
        }

        //
        // Case on the handler disposition.
        //

        switch (Disposition) {

            //
            // The disposition is to continue execution. If the
            // exception is not continuable, then raise the exception
            // STATUS_NONCONTINUABLE_EXCEPTION. Otherwise return
            // TRUE.
            //

        case ExceptionContinueExecution :
            if ((ExceptionRecord->ExceptionFlags &
               EXCEPTION_NONCONTINUABLE) != 0) {
                ExceptionRecord1.ExceptionCode =
                                        STATUS_NONCONTINUABLE_EXCEPTION;
                ExceptionRecord1.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
                ExceptionRecord1.ExceptionRecord = ExceptionRecord;
                ExceptionRecord1.NumberParameters = 0;
                RtlRaiseException(&ExceptionRecord1);

            } else {
                Completion = TRUE;
                goto DispatchExit;
            }

            //
            // The disposition is to continue the search. If the frame isn't
            // suspect/corrupt, get next frame address and continue the search 
            //

        case ExceptionContinueSearch :
            if (ExceptionRecord->ExceptionFlags & EXCEPTION_STACK_INVALID)
                goto DispatchExit;

            break;

            //
            // The disposition is nested exception. Set the nested
            // context frame to the establisher frame address and set
            // nested exception in the exception flags.
            //

        case ExceptionNestedException :
            ExceptionRecord->ExceptionFlags |= EXCEPTION_NESTED_CALL;
            if (DispatcherContext.RegistrationPointer > NestedRegistration) {
                NestedRegistration = DispatcherContext.RegistrationPointer;
            }

            break;

            //
            // All other disposition values are invalid. Raise
            // invalid disposition exception.
            //

        default :
            ExceptionRecord1.ExceptionCode = STATUS_INVALID_DISPOSITION;
            ExceptionRecord1.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
            ExceptionRecord1.ExceptionRecord = ExceptionRecord;
            ExceptionRecord1.NumberParameters = 0;
            RtlRaiseException(&ExceptionRecord1);
            break;
        }

        //
        // If chain goes in wrong direction or loops, report an
        // invalid exception stack, otherwise go on to the next one.
        //

        RegistrationPointer = RegistrationPointer->Next;
    }

    //
    // Call vectored continue handlers.
    //

DispatchExit:

    return Completion;
}
#endif

VOID
NTAPI
RtlRaiseException (
    _In_ PEXCEPTION_RECORD ExceptionRecord
    )
{
    CONTEXT ContextRecord;
    RtlCaptureContext(&ContextRecord);
    ContextRecord.ContextFlags = CONTEXT_FULL;

    if (RtlDispatchException(ExceptionRecord, &ContextRecord)) {
        return;
    }

    ExRaiseException(ExceptionRecord);
}
#endif // _LDK_DEFINE_RTL_RAISE_EXCEPTION