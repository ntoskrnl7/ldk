#include <windows.h>

#if defined(_MSC_VER)
#pragma runtime_checks("", off)
#endif

#define TLS_EVENT_TLS_PROCESS_ATTACH 0x101
#define TLS_EVENT_DLL_PROCESS_ATTACH 0x102
#define TLS_EVENT_TLS_THREAD_ATTACH  0x201
#define TLS_EVENT_TLS_THREAD_DETACH  0x301
#define TLS_EVENT_TLS_PROCESS_DETACH 0x401
#define TLS_EVENT_DLL_PROCESS_DETACH 0x402
#define TLS_EVENT_CAPACITY           64
#define TLS_STATIC_INITIAL_VALUE     7

#if defined(_M_IX86)
#pragma comment(linker, "/include:__tls_used")
#pragma comment(linker, "/export:TlsCallbackReset=_TlsCallbackReset@0")
#pragma comment(linker, "/export:TlsCallbackSetLog=_TlsCallbackSetLog@12")
#pragma comment(linker, "/export:TlsCallbackGetCount=_TlsCallbackGetCount@0")
#pragma comment(linker, "/export:TlsCallbackGetEvent=_TlsCallbackGetEvent@4")
#pragma comment(linker, "/export:TlsCallbackSetStaticTlsAccessor=_TlsCallbackSetStaticTlsAccessor@4")
#pragma comment(linker, "/export:TlsCallbackGetStaticIndex=_TlsCallbackGetStaticIndex@0")
#pragma comment(linker, "/export:TlsCallbackGetStaticValue=_TlsCallbackGetStaticValue@0")
#pragma comment(linker, "/export:TlsCallbackSetStaticValue=_TlsCallbackSetStaticValue@4")
#else
#pragma comment(linker, "/include:_tls_used")
#pragma comment(linker, "/export:TlsCallbackReset")
#pragma comment(linker, "/export:TlsCallbackSetLog")
#pragma comment(linker, "/export:TlsCallbackGetCount")
#pragma comment(linker, "/export:TlsCallbackGetEvent")
#pragma comment(linker, "/export:TlsCallbackSetStaticTlsAccessor")
#pragma comment(linker, "/export:TlsCallbackGetStaticIndex")
#pragma comment(linker, "/export:TlsCallbackGetStaticValue")
#pragma comment(linker, "/export:TlsCallbackSetStaticValue")
#endif

typedef PVOID(__stdcall *TLS_STATIC_TLS_ACCESSOR)(ULONG Index);

typedef struct _TLS_STATIC_DATA {
    LONG Value;
    LONG Guard;
} TLS_STATIC_DATA, *PTLS_STATIC_DATA;

static LONG LdkpTlsCallbackCount;
static LONG LdkpTlsCallbackEvents[TLS_EVENT_CAPACITY];
static LONG *LdkpTlsCallbackExternalCount;
static LONG *LdkpTlsCallbackExternalEvents;
static LONG LdkpTlsCallbackExternalCapacity;
ULONG LdkpTlsIndex;
static TLS_STATIC_TLS_ACCESSOR LdkpTlsStaticTlsAccessor;

#pragma section(".tls$AAA", long, read, write)
#pragma section(".tls$ZZZ", long, read, write)
__declspec(allocate(".tls$AAA")) TLS_STATIC_DATA LdkpTlsStart = {
    TLS_STATIC_INITIAL_VALUE,
    0x12345678
};
__declspec(allocate(".tls$ZZZ")) CHAR LdkpTlsEnd = 0;

static
VOID
LdkpTlsCallbackRecord (
    _In_ LONG Event
    )
{
    LONG Index;

    Index = LdkpTlsCallbackCount;
    if (Index < TLS_EVENT_CAPACITY) {
        LdkpTlsCallbackEvents[Index] = Event;
    }
    LdkpTlsCallbackCount = Index + 1;

    if (LdkpTlsCallbackExternalCount &&
        LdkpTlsCallbackExternalEvents &&
        LdkpTlsCallbackExternalCapacity > 0) {
        Index = *LdkpTlsCallbackExternalCount;
        if (Index < LdkpTlsCallbackExternalCapacity) {
            LdkpTlsCallbackExternalEvents[Index] = Event;
        }
        *LdkpTlsCallbackExternalCount = Index + 1;
    }
}

static
PTLS_STATIC_DATA
LdkpTlsGetStaticData (
    VOID
    )
{
    if (!LdkpTlsStaticTlsAccessor) {
        return NULL;
    }

    return (PTLS_STATIC_DATA)LdkpTlsStaticTlsAccessor( LdkpTlsIndex );
}

VOID
__stdcall
TlsCallbackReset (
    VOID
    )
{
    LdkpTlsCallbackCount = 0;
    if (LdkpTlsCallbackExternalCount) {
        *LdkpTlsCallbackExternalCount = 0;
    }
}

VOID
__stdcall
TlsCallbackSetLog (
    _Inout_opt_ LONG *Count,
    _Out_writes_opt_(Capacity) LONG *Events,
    _In_ LONG Capacity
    )
{
    LdkpTlsCallbackExternalCount = Count;
    LdkpTlsCallbackExternalEvents = Events;
    LdkpTlsCallbackExternalCapacity = Capacity;

    if (Count) {
        *Count = 0;
    }
}

LONG
__stdcall
TlsCallbackGetCount (
    VOID
    )
{
    return LdkpTlsCallbackCount;
}

LONG
__stdcall
TlsCallbackGetEvent (
    _In_ LONG Index
    )
{
    if (Index < 0 ||
        Index >= LdkpTlsCallbackCount ||
        Index >= TLS_EVENT_CAPACITY) {
        return 0;
    }

    return LdkpTlsCallbackEvents[Index];
}

VOID
__stdcall
TlsCallbackSetStaticTlsAccessor (
    _In_opt_ TLS_STATIC_TLS_ACCESSOR Accessor
    )
{
    LdkpTlsStaticTlsAccessor = Accessor;
}

ULONG
__stdcall
TlsCallbackGetStaticIndex (
    VOID
    )
{
    return LdkpTlsIndex;
}

LONG
__stdcall
TlsCallbackGetStaticValue (
    VOID
    )
{
    PTLS_STATIC_DATA Data = LdkpTlsGetStaticData();
    if (!Data) {
        return -1;
    }

    if (Data->Guard != 0x12345678) {
        return -2;
    }

    return Data->Value;
}

VOID
__stdcall
TlsCallbackSetStaticValue (
    _In_ LONG Value
    )
{
    PTLS_STATIC_DATA Data = LdkpTlsGetStaticData();
    if (Data) {
        Data->Value = Value;
    }
}

VOID
NTAPI
LdkpTlsCallback (
    _In_ PVOID Module,
    _In_ DWORD Reason,
    _In_opt_ PVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Module );
    UNREFERENCED_PARAMETER( Reserved );

    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        LdkpTlsCallbackRecord( TLS_EVENT_TLS_PROCESS_ATTACH );
        break;
    case DLL_THREAD_ATTACH:
        LdkpTlsCallbackRecord( TLS_EVENT_TLS_THREAD_ATTACH );
        break;
    case DLL_THREAD_DETACH:
        LdkpTlsCallbackRecord( TLS_EVENT_TLS_THREAD_DETACH );
        break;
    case DLL_PROCESS_DETACH:
        LdkpTlsCallbackRecord( TLS_EVENT_TLS_PROCESS_DETACH );
        break;
    default:
        break;
    }
}

#pragma section(".rdata$T", long, read)
__declspec(allocate(".rdata$T"))
PIMAGE_TLS_CALLBACK const LdkpTlsCallbacks[] = {
    LdkpTlsCallback,
    NULL
};

__declspec(allocate(".rdata$T"))
#if defined(_WIN64)
const IMAGE_TLS_DIRECTORY _tls_used = {
    (ULONGLONG)&LdkpTlsStart,
    (ULONGLONG)&LdkpTlsEnd,
    (ULONGLONG)&LdkpTlsIndex,
    (ULONGLONG)LdkpTlsCallbacks,
    0,
    0
};
#else
const IMAGE_TLS_DIRECTORY _tls_used = {
    (ULONG)(ULONG_PTR)&LdkpTlsStart,
    (ULONG)(ULONG_PTR)&LdkpTlsEnd,
    (ULONG)(ULONG_PTR)&LdkpTlsIndex,
    (ULONG)(ULONG_PTR)LdkpTlsCallbacks,
    0,
    0
};
#endif

BOOL
WINAPI
DllMain (
    _In_ HINSTANCE Module,
    _In_ DWORD Reason,
    _In_opt_ LPVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Module );
    UNREFERENCED_PARAMETER( Reserved );

    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        LdkpTlsCallbackRecord( TLS_EVENT_DLL_PROCESS_ATTACH );
        break;
    case DLL_PROCESS_DETACH:
        LdkpTlsCallbackRecord( TLS_EVENT_DLL_PROCESS_DETACH );
        break;
    default:
        break;
    }

    return TRUE;
}

BOOL
WINAPI
TlsCallbackDllMain (
    _In_ HINSTANCE Module,
    _In_ DWORD Reason,
    _In_opt_ LPVOID Reserved
    )
{
    return DllMain( Module,
                    Reason,
                    Reserved );
}
