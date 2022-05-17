#include "winbase.h"
#include "../ldk.h"



LPTOP_LEVEL_EXCEPTION_FILTER LdkpCurrentTopLevelFilter = NULL;



#if _LDK_DEFINE_RAISE_EXCEPTION
WINBASEAPI
__analysis_noreturn
VOID
WINAPI
RaiseException (
    _In_ DWORD dwExceptionCode,
    _In_ DWORD dwExceptionFlags,
    _In_ DWORD nNumberOfArguments,
    _In_reads_opt_(nNumberOfArguments) CONST ULONG_PTR* lpArguments
	)
{
    EXCEPTION_RECORD ExceptionRecord;
    ExceptionRecord.ExceptionCode = (DWORD)dwExceptionCode;
    ExceptionRecord.ExceptionFlags = dwExceptionFlags & EXCEPTION_NONCONTINUABLE;
    ExceptionRecord.ExceptionRecord = NULL;
    ExceptionRecord.ExceptionAddress = (PVOID)_ReturnAddress();
    if (ARGUMENT_PRESENT(lpArguments)) {
        if (nNumberOfArguments > EXCEPTION_MAXIMUM_PARAMETERS) {
            nNumberOfArguments = EXCEPTION_MAXIMUM_PARAMETERS;
        }
        ExceptionRecord.NumberParameters = nNumberOfArguments;
        RtlCopyMemory( ExceptionRecord.ExceptionInformation,
					   lpArguments,
					   nNumberOfArguments * sizeof(ULONG_PTR) );
    } else {
        ExceptionRecord.NumberParameters = 0;
    }

    RtlRaiseException( &ExceptionRecord );
}
#endif


LONG
CheckForReadOnlyResource (
    _In_ PEXCEPTION_POINTERS ExceptionInfo
    )
{
	NTSTATUS Status;
	MEMORY_BASIC_INFORMATION MemInfo;
	PIMAGE_RESOURCE_DIRECTORY ResourceDirectory;
	ULONG ResourceSize;
	LONG ReturnValue;
	PEXCEPTION_RECORD ExceptionRecord = ExceptionInfo->ExceptionRecord;
	PVOID VirtualAddress = &ExceptionRecord->ExceptionInformation[1];

	if (ExceptionRecord->ExceptionCode != STATUS_ACCESS_VIOLATION) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	if (ExceptionRecord->ExceptionInformation[0] != 1) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (ExceptionRecord->NumberParameters < 2) {
		return EXCEPTION_CONTINUE_SEARCH;
	}
	
	EXIT_WHEN_DPC_WITH_RETURN(EXCEPTION_CONTINUE_SEARCH);

	Status = ZwQueryVirtualMemory( NtCurrentProcess(),
                                   VirtualAddress,
                                   MemoryBasicInformation,
                                   (PVOID)&MemInfo,
                                   sizeof(MemInfo),
                                   NULL );
	if (! NT_SUCCESS(Status)) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (! ((MemInfo.Protect == PAGE_READONLY) && (MemInfo.Type == MEM_IMAGE))){
		return EXCEPTION_CONTINUE_SEARCH;
	}

	ReturnValue = EXCEPTION_CONTINUE_SEARCH;

	try {
		ResourceDirectory = RtlImageDirectoryEntryToData( MemInfo.AllocationBase,
                                                          TRUE,
                                                          IMAGE_DIRECTORY_ENTRY_RESOURCE,
                                                          &ResourceSize );
		if (ResourceDirectory &&
			(ULONG_PTR)VirtualAddress >= (ULONG_PTR)ResourceDirectory &&
			VirtualAddress < Add2Ptr(ResourceDirectory, ResourceSize)
           ) {
            SIZE_T RegionSize = 1;
            ULONG OldProtect;
            Status = ZwProtectVirtualMemory( NtCurrentProcess(),
                                             &VirtualAddress,
                                             &RegionSize,
                                             PAGE_READWRITE,
                                             &OldProtect );
            if (NT_SUCCESS(Status)) {
                ReturnValue = EXCEPTION_CONTINUE_EXECUTION;
            }
		}
	} except (EXCEPTION_EXECUTE_HANDLER) {
	}

	return ReturnValue;
}

__callback
WINBASEAPI
LONG
WINAPI
UnhandledExceptionFilter (
    _In_ struct _EXCEPTION_POINTERS* ExceptionInfo
    )
{
	NTSTATUS Status;
	LONG FilterReturn;
	JOBOBJECT_BASIC_LIMIT_INFORMATION JobBasicLimitInfo;
	ULONG_PTR Parameters[4];
	ULONG ResponseFlag = OptionOk;
	ULONG Response;

	PEXCEPTION_RECORD ExceptionRecord = ExceptionInfo->ExceptionRecord;

	KdBreakPoint();

	FilterReturn = CheckForReadOnlyResource(ExceptionInfo);

	if (FilterReturn == EXCEPTION_CONTINUE_EXECUTION) {
		return FilterReturn;
	}

	if (PsIsProcessBeingDebugged(PsGetCurrentProcess())) {
		return EXCEPTION_CONTINUE_SEARCH;
	}

	if (LdkpCurrentTopLevelFilter) {
		FilterReturn = (LdkpCurrentTopLevelFilter)(ExceptionInfo);
		if (FilterReturn == EXCEPTION_EXECUTE_HANDLER || FilterReturn == EXCEPTION_CONTINUE_EXECUTION ) {
			return FilterReturn;
		}
	}
	
	if (FlagOn(GetErrorMode(), SEM_NOGPFAULTERRORBOX)) {
		return EXCEPTION_EXECUTE_HANDLER;
	}
	
	EXIT_WHEN_DPC_WITH_RETURN(EXCEPTION_CONTINUE_SEARCH);

	Status = ZwQueryInformationJobObject( NULL,
                                          JobObjectBasicLimitInformation,
                                          &JobBasicLimitInfo,
                                          sizeof(JOBOBJECT_BASIC_LIMIT_INFORMATION),
                                          NULL );

	if (NT_SUCCESS(Status) &&
		(JobBasicLimitInfo.LimitFlags & JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION)) {
		return EXCEPTION_EXECUTE_HANDLER;
	}

	Parameters[0] = (ULONG_PTR)ExceptionRecord->ExceptionCode;
	Parameters[1] = (ULONG_PTR)ExceptionRecord->ExceptionAddress;

	if (ExceptionRecord->ExceptionCode == STATUS_IN_PAGE_ERROR) {
		Parameters[2] = ExceptionRecord->ExceptionInformation[2];
	} else {
		Parameters[2] = ExceptionRecord->ExceptionInformation[0];
	}

	Parameters[3] = ExceptionRecord->ExceptionInformation[1];

	Status = ExRaiseHardError( STATUS_UNHANDLED_EXCEPTION | HARDERROR_OVERRIDE_ERRORMODE,
							4,
							0,
							Parameters,
							ResponseFlag,
							&Response );

	return EXCEPTION_EXECUTE_HANDLER;
}

WINBASEAPI
LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter (
    _In_opt_ LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    )
{
    LPTOP_LEVEL_EXCEPTION_FILTER PreviousTopLevelFilter;

    PreviousTopLevelFilter = LdkpCurrentTopLevelFilter;
    LdkpCurrentTopLevelFilter = lpTopLevelExceptionFilter;

    return PreviousTopLevelFilter;
}

WINBASEAPI
_Check_return_ _Post_equals_last_error_
DWORD
WINAPI
GetLastError (
    VOID
    )
{
	return NtCurrentTeb()->LastErrorValue;
}

WINBASEAPI
VOID
WINAPI
SetLastError (
    _In_ DWORD dwErrCode
    )
{
	NtCurrentTeb()->LastErrorValue = (LONG)dwErrCode;
}

WINBASEAPI
UINT
WINAPI
GetErrorMode (
    VOID
    )
{
	NTSTATUS Status;
	UINT PreviousMode;

	KdBreakPoint();
	EXIT_WHEN_DPC_WITH_RETURN(0);

	Status = ZwQueryInformationProcess( NtCurrentProcess(),
                                        ProcessDefaultHardErrorMode,
                                        (PVOID)&PreviousMode,
                                        sizeof(PreviousMode),
                                        NULL );

	if (! NT_SUCCESS(Status)) {
		LdkSetLastNTError(Status);
		return 0;
	}

	if (PreviousMode & PROCESS_HARDERROR_DEFAULT) {
		PreviousMode &= ~SEM_FAILCRITICALERRORS;
	} else {
		PreviousMode |= SEM_FAILCRITICALERRORS;
	}
	return PreviousMode;
}

WINBASEAPI
UINT
WINAPI
SetErrorMode (
    _In_ UINT uMode
    )
{
    UINT PreviousMode;
    UINT NewMode;

	KdBreakPoint();
    EXIT_WHEN_DPC_WITH_RETURN(0);

    PreviousMode = GetErrorMode();

    NewMode = uMode;
    if (NewMode & SEM_FAILCRITICALERRORS ) {
        NewMode &= ~SEM_FAILCRITICALERRORS;
    } else {
        NewMode |= SEM_FAILCRITICALERRORS;
    }

    NTSTATUS Status = ZwSetInformationProcess( NtCurrentProcess(),
                                               ProcessDefaultHardErrorMode,
                                               (PVOID)&NewMode,
                                               sizeof(NewMode) );

    if (NT_SUCCESS(Status)){
        return PreviousMode;
    }
    LdkSetLastNTError( Status );
    return 0;
}