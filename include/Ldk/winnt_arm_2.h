#pragma once

#if defined(_ARM_)

#pragma region Application Family or OneCore Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM)

NTSYSAPI
BOOLEAN
__cdecl
RtlAddFunctionTable(
    _In_reads_(EntryCount) PRUNTIME_FUNCTION FunctionTable,
    _In_ DWORD EntryCount,
    _In_ DWORD BaseAddress
    );

NTSYSAPI
BOOLEAN
__cdecl
RtlDeleteFunctionTable(
    _In_ PRUNTIME_FUNCTION FunctionTable
    );

NTSYSAPI
BOOLEAN
__cdecl
RtlInstallFunctionTableCallback(
    _In_ DWORD TableIdentifier,
    _In_ DWORD BaseAddress,
    _In_ DWORD Length,
    _In_ PGET_RUNTIME_FUNCTION_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_opt_ PCWSTR OutOfProcessCallbackDll
    );

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM) */
#pragma endregion

#pragma region Desktop Family or OneCore Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM)

#if (NTDDI_VERSION >= NTDDI_WIN8)

NTSYSAPI
DWORD   
NTAPI
RtlAddGrowableFunctionTable(
    _Out_ PVOID* DynamicTable,
    _In_reads_(MaximumEntryCount) PRUNTIME_FUNCTION FunctionTable,
    _In_ DWORD EntryCount,
    _In_ DWORD MaximumEntryCount,
    _In_ ULONG_PTR RangeBase,
    _In_ ULONG_PTR RangeEnd
    );

NTSYSAPI
VOID
NTAPI
RtlGrowFunctionTable(
    _Inout_ PVOID DynamicTable,
    _In_ DWORD NewEntryCount
    );

NTSYSAPI
VOID
NTAPI
RtlDeleteGrowableFunctionTable(
    _In_ PVOID DynamicTable
    );

#endif // (NTDDI_VERSION >= NTDDI_WIN8)

#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP | WINAPI_PARTITION_SYSTEM) */
#pragma endregion

#pragma region Application or OneCore Family or Games Family
#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES)

NTSYSAPI
PRUNTIME_FUNCTION
NTAPI
RtlLookupFunctionEntry(
    _In_ ULONG_PTR ControlPc,
    _Out_ PDWORD ImageBase,
    _Inout_opt_ PUNWIND_HISTORY_TABLE HistoryTable
    );

NTSYSAPI
VOID
__cdecl
RtlRestoreContext(
    _In_ PCONTEXT ContextRecord,
    _In_opt_ struct _EXCEPTION_RECORD* ExceptionRecord
    );

NTSYSAPI
VOID
NTAPI
RtlUnwindEx(
    _In_opt_ PVOID TargetFrame,
    _In_opt_ PVOID TargetIp,
    _In_opt_ PEXCEPTION_RECORD ExceptionRecord,
    _In_ PVOID ReturnValue,
    _In_ PCONTEXT ContextRecord,
    _In_opt_ PUNWIND_HISTORY_TABLE HistoryTable
    );

NTSYSAPI
PEXCEPTION_ROUTINE
NTAPI
RtlVirtualUnwind(
    _In_ DWORD HandlerType,
    _In_ DWORD ImageBase,
    _In_ DWORD ControlPc,
    _In_ PRUNTIME_FUNCTION FunctionEntry,
    _Inout_ PCONTEXT ContextRecord,
    _Out_ PVOID* HandlerData,
    _Out_ PDWORD EstablisherFrame,
    _Inout_opt_ PKNONVOLATILE_CONTEXT_POINTERS ContextPointers
    );


#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP | WINAPI_PARTITION_SYSTEM | WINAPI_PARTITION_GAMES) */
#pragma endregion

#endif // defined(_ARM_)
