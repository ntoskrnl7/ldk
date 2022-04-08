#include "winbase.h"
#include "../ldk.h"
#include "../nt/ntexapi.h"



WINBASEAPI
_Check_return_ _Post_equals_last_error_
DWORD
WINAPI
GetLastError(
    VOID
    )
{
	return NtCurrentTeb()->LastErrorValue;
}

WINBASEAPI
VOID
WINAPI
SetLastError(
    _In_ DWORD dwErrCode
    )
{
	NtCurrentTeb()->LastErrorValue = (LONG)dwErrCode;
}



#if _LDK_DEFINE_KERNEL32_RAISE_EXCEPTION
WINBASEAPI
__analysis_noreturn
VOID
WINAPI
RaiseException(
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
	ExceptionRecord.ExceptionAddress = (PVOID)RaiseException;
	if (ARGUMENT_PRESENT(lpArguments)) {
		ULONG n = nNumberOfArguments;
		PULONG_PTR s, d;
		if (n > EXCEPTION_MAXIMUM_PARAMETERS) {
			n = EXCEPTION_MAXIMUM_PARAMETERS;
		}
		ExceptionRecord.NumberParameters = n;
		s = (PULONG_PTR)lpArguments;
		d = ExceptionRecord.ExceptionInformation;
		while (n--) {
			*d++ = *s++;
		}
	}
	else {
		ExceptionRecord.NumberParameters = 0;
	}
	RtlRaiseException(&ExceptionRecord);
}
#endif