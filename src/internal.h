#pragma once



#define LDK_TLS_SLOTS_SIZE			1024
#define LDK_FLS_SLOTS_SIZE			1024



#ifndef Add2Ptr
#define Add2Ptr(P,I) ((PVOID)((PUCHAR)(P) + (I)))
#endif

#ifndef LIST_FOR_EACH_SAFE
#define LIST_FOR_EACH_SAFE(curr, n, head) \
        for (curr = (head)->Flink , n = curr->Flink ; curr != (head); \
             curr = n, n = curr->Flink )
#endif

#ifndef LDK_ENABLE_DIAGNOSTIC_BREAKPOINTS
#define LDK_ENABLE_DIAGNOSTIC_BREAKPOINTS 0
#endif

#if LDK_ENABLE_DIAGNOSTIC_BREAKPOINTS
#define LDK_DIAGNOSTIC_BREAK() KdBreakPoint()
#else
#define LDK_DIAGNOSTIC_BREAK() ((void)0)
#endif

#ifndef LDK_ENABLE_RUNTIME_DIAGNOSTICS
#if defined(DBG) && DBG
#define LDK_ENABLE_RUNTIME_DIAGNOSTICS 1
#else
#define LDK_ENABLE_RUNTIME_DIAGNOSTICS 0
#endif
#endif



NTKERNELAPI
LOGICAL
KeIsExecutingDpc (
	VOID
	);



#define EXIT_WHEN_DPC_WITH_NO_RETURN()	\
	if (KeIsExecutingDpc()) {	\
		LDK_DIAGNOSTIC_BREAK();	\
		LdkSetLastNTError(STATUS_NOT_SUPPORTED);	\
		return;	\
	}

#define EXIT_WHEN_DPC_WITH_RETURN(ReturnValue)	\
	if (KeIsExecutingDpc()) {	\
		LDK_DIAGNOSTIC_BREAK();	\
		LdkSetLastNTError(STATUS_NOT_SUPPORTED);	\
		return ReturnValue;	\
	}
