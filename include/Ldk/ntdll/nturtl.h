#pragma once

EXTERN_C_START

VOID
NTAPI
RtlInitializeSRWLock(
    _Out_ PRTL_SRWLOCK SRWLock
    );

VOID
NTAPI
RtlAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

VOID
NTAPI
RtlReleaseSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

BOOLEAN
NTAPI
RtlTryAcquireSRWLockShared(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

VOID
NTAPI
RtlAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

VOID
NTAPI
RtlReleaseSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );

BOOLEAN
NTAPI
RtlTryAcquireSRWLockExclusive(
    _Inout_ PRTL_SRWLOCK SRWLock
    );



NTSTATUS
NTAPI
RtlInitializeCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSTATUS
NTAPI
RtlInitializeCriticalSectionAndSpinCount(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount
    );

NTSTATUS
NTAPI
RtlInitializeCriticalSectionEx(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_ ULONG SpinCount,
    _In_ ULONG Flags
    );

NTSTATUS
NTAPI
RtlDeleteCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

LONG
NTAPI
RtlGetCriticalSectionRecursionCount(
    _In_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSTATUS
NTAPI
RtlEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

NTSTATUS
NTAPI
RtlLeaveCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );

BOOLEAN
NTAPI
RtlTryEnterCriticalSection(
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection
    );


VOID
NTAPI
RtlInitializeConditionVariable(
    _Out_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

VOID
NTAPI
RtlWakeConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

VOID
NTAPI
RtlWakeAllConditionVariable(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable
    );

NTSTATUS
NTAPI
RtlSleepConditionVariableCS(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_CRITICAL_SECTION CriticalSection,
    _In_opt_ const LARGE_INTEGER* TimeOut
    );

NTSTATUS
NTAPI
RtlSleepConditionVariableSRW(
    _Inout_ PRTL_CONDITION_VARIABLE ConditionVariable,
    _Inout_ PRTL_SRWLOCK SRWLock,
    _In_opt_ const LARGE_INTEGER* TimeOut,
    _In_ ULONG Flags
    );


#define RTL_HEAP_MAKE_TAG HEAP_MAKE_TAG_FLAGS

#define RtlProcessHeap() (NtCurrentPeb()->ProcessHeap)



#define WT_EXECUTEDEFAULT       0x00000000                           // winnt
#define WT_EXECUTEINIOTHREAD    0x00000001                           // winnt
#define WT_EXECUTEINUITHREAD    0x00000002                           // winnt
#define WT_EXECUTEINWAITTHREAD  0x00000004                           // winnt
#define WT_EXECUTEONLYONCE      0x00000008                           // winnt
#define WT_EXECUTEINTIMERTHREAD 0x00000020                           // winnt
#define WT_EXECUTELONGFUNCTION  0x00000010                           // winnt
#define WT_EXECUTEINPERSISTENTIOTHREAD  0x00000040                   // winnt
#define WT_EXECUTEINPERSISTENTTHREAD 0x00000080                      // winnt
#define WT_TRANSFER_IMPERSONATION 0x00000100                         // winnt
#define WT_SET_MAX_THREADPOOL_THREADS(Flags, Limit)  ((Flags) |= (Limit)<<16) // winnt

typedef VOID (NTAPI * WORKERCALLBACKFUNC) (PVOID );                 // winnt

NTSTATUS
NTAPI
RtlQueueWorkItem (
    _In_ WORKERCALLBACKFUNC Function,
    _In_ PVOID Context,
    _In_ ULONG Flags
    );



typedef struct _RTLP_CURDIR_REF *PRTLP_CURDIR_REF;

typedef struct _RTL_RELATIVE_NAME_U {
    UNICODE_STRING RelativeName;
    HANDLE ContainingDirectory;
    PRTLP_CURDIR_REF CurDirRef;
} RTL_RELATIVE_NAME_U, *PRTL_RELATIVE_NAME_U;

VOID
NTAPI
RtlReleaseRelativeName (
    _In_ PRTL_RELATIVE_NAME_U RelativeName
    );



typedef enum _RTL_PATH_TYPE {
    RtlPathTypeUnknown,         // 0
    RtlPathTypeUncAbsolute,     // 1
    RtlPathTypeDriveAbsolute,   // 2
    RtlPathTypeDriveRelative,   // 3
    RtlPathTypeRooted,          // 4
    RtlPathTypeRelative,        // 5
    RtlPathTypeLocalDevice,     // 6
    RtlPathTypeRootLocalDevice  // 7
} RTL_PATH_TYPE;

RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U (
    _In_ PCWSTR DosFileName
    );

ULONG
NTAPI
RtlIsDosDeviceName_U (
    _In_ PCWSTR DosFileName
    );

ULONG
NTAPI
RtlGetFullPathName_U (
    _In_ PCWSTR lpFileName,
    _In_ ULONG nBufferLength,
    _Out_bytecapcount_(nBufferLength) PWSTR lpBuffer,
    _Out_opt_ PWSTR *lpFilePart
    );

ULONG
NTAPI
RtlGetCurrentDirectory_U (
    _In_ ULONG nBufferLength,
    _Out_bytecapcount_(nBufferLength) PWSTR lpBuffer
    );

NTSTATUS
NTAPI
RtlSetCurrentDirectory_U (
    _In_ PCUNICODE_STRING PathName
    );

BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U (
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Reserved_ PVOID Reserved
    );

BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_U (
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_ PRTL_RELATIVE_NAME_U RelativeName
    );

BOOLEAN
NTAPI
RtlDoesFileExists_U (
    _In_ PCWSTR FileName
    );

ULONG
NTAPI
RtlDosSearchPath_U (
    _In_ PCWSTR lpPath,
    _In_ PCWSTR lpFileName,
    _In_opt_ PCWSTR lpExtension,
    _In_ ULONG nBufferLength,
    _Out_ PWSTR lpBuffer,
    _Out_ PWSTR *lpFilePart
    );



#ifndef RTL_ERRORMODE_FAILCRITICALERRORS
#define RTL_ERRORMODE_FAILCRITICALERRORS (0x0010)
#endif
#define RTL_ERRORMODE_NOGPFAULTERRORBOX  (0x0020)
#define RTL_ERRORMODE_NOOPENFILEERRORBOX (0x0040)

ULONG
NTAPI
RtlGetThreadErrorMode (
    VOID
    );

NTSTATUS
NTAPI
RtlSetThreadErrorMode (
    _In_  ULONG  NewMode,
    _Out_opt_ PULONG OldMode
    );

EXTERN_C_END