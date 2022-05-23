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



NTKERNELAPI
LOGICAL
KeIsExecutingDpc (
	VOID
	);



#define EXIT_WHEN_DPC_WITH_NO_RETURN()	\
	if (KeIsExecutingDpc()) {	\
		KdBreakPoint();	\
		LdkSetLastNTError(STATUS_NOT_SUPPORTED);	\
		return;	\
	}

#define EXIT_WHEN_DPC_WITH_RETURN(ReturnValue)	\
	if (KeIsExecutingDpc()) {	\
		KdBreakPoint();	\
		LdkSetLastNTError(STATUS_NOT_SUPPORTED);	\
		return ReturnValue;	\
	}
