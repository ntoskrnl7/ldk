#pragma once

EXTERN_C_START

#define PROCESS_HARDERROR_ALIGNMENT_BIT 0x0004
#if defined(_AMD64_)
#define PROCESS_HARDERROR_DEFAULT (1 | PROCESS_HARDERROR_ALIGNMENT_BIT)
#else
#define PROCESS_HARDERROR_DEFAULT 1
#endif

NTKERNELAPI
BOOLEAN
PsIsProcessBeingDebugged (
    __in PEPROCESS Process
    );



typedef struct _THREAD_BASIC_INFORMATION {
    NTSTATUS ExitStatus;
    PTEB TebBaseAddress;
    CLIENT_ID ClientId;
    KAFFINITY AffinityMask;
    KPRIORITY Priority;
    LONG BasePriority;
} THREAD_BASIC_INFORMATION;
typedef THREAD_BASIC_INFORMATION* PTHREAD_BASIC_INFORMATION;

NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationThread (
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_ PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength
    );



NTSYSAPI
NTSTATUS
NTAPI
ZwQueryInformationProcess (
	_In_ HANDLE ProcessHandle,
	_In_ PROCESSINFOCLASS ProcessInformationClass,
	_Out_ PVOID ProcessInformation,
	_In_ ULONG ProcessInformationLength,
	_Out_opt_ PULONG ReturnLength
    );

NTSYSAPI
NTSTATUS
NTAPI
ZwSetInformationProcess (
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_bytecount_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength
    );

EXTERN_C_END