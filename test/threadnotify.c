#include <windows.h>

#if defined(_MSC_VER)
#pragma runtime_checks("", off)
#endif

#define THREAD_NOTIFY_EVENT_PROCESS_ATTACH 0x111
#define THREAD_NOTIFY_EVENT_THREAD_ATTACH  0x211
#define THREAD_NOTIFY_EVENT_THREAD_DETACH  0x311
#define THREAD_NOTIFY_EVENT_PROCESS_DETACH 0x411
#define THREAD_NOTIFY_EVENT_CAPACITY       64

#if defined(_M_IX86)
#pragma comment(linker, "/export:ThreadNotifyReset=_ThreadNotifyReset@0")
#pragma comment(linker, "/export:ThreadNotifySetLog=_ThreadNotifySetLog@12")
#pragma comment(linker, "/export:ThreadNotifyGetCount=_ThreadNotifyGetCount@0")
#pragma comment(linker, "/export:ThreadNotifyGetEvent=_ThreadNotifyGetEvent@4")
#else
#pragma comment(linker, "/export:ThreadNotifyReset")
#pragma comment(linker, "/export:ThreadNotifySetLog")
#pragma comment(linker, "/export:ThreadNotifyGetCount")
#pragma comment(linker, "/export:ThreadNotifyGetEvent")
#endif

static LONG LdkpThreadNotifyCount;
static LONG LdkpThreadNotifyEvents[THREAD_NOTIFY_EVENT_CAPACITY];
static LONG *LdkpThreadNotifyExternalCount;
static LONG *LdkpThreadNotifyExternalEvents;
static LONG LdkpThreadNotifyExternalCapacity;

static
VOID
LdkpThreadNotifyRecord (
    _In_ LONG Event
    )
{
    LONG Index;

    Index = LdkpThreadNotifyCount;
    if (Index < THREAD_NOTIFY_EVENT_CAPACITY) {
        LdkpThreadNotifyEvents[Index] = Event;
    }
    LdkpThreadNotifyCount = Index + 1;

    if (LdkpThreadNotifyExternalCount &&
        LdkpThreadNotifyExternalEvents &&
        LdkpThreadNotifyExternalCapacity > 0) {
        Index = *LdkpThreadNotifyExternalCount;
        if (Index < LdkpThreadNotifyExternalCapacity) {
            LdkpThreadNotifyExternalEvents[Index] = Event;
        }
        *LdkpThreadNotifyExternalCount = Index + 1;
    }
}

VOID
__stdcall
ThreadNotifyReset (
    VOID
    )
{
    LdkpThreadNotifyCount = 0;
    if (LdkpThreadNotifyExternalCount) {
        *LdkpThreadNotifyExternalCount = 0;
    }
}

VOID
__stdcall
ThreadNotifySetLog (
    _Inout_opt_ LONG *Count,
    _Out_writes_opt_(Capacity) LONG *Events,
    _In_ LONG Capacity
    )
{
    LdkpThreadNotifyExternalCount = Count;
    LdkpThreadNotifyExternalEvents = Events;
    LdkpThreadNotifyExternalCapacity = Capacity;

    if (Count) {
        *Count = 0;
    }
}

LONG
__stdcall
ThreadNotifyGetCount (
    VOID
    )
{
    return LdkpThreadNotifyCount;
}

LONG
__stdcall
ThreadNotifyGetEvent (
    _In_ LONG Index
    )
{
    if (Index < 0 ||
        Index >= LdkpThreadNotifyCount ||
        Index >= THREAD_NOTIFY_EVENT_CAPACITY) {
        return 0;
    }

    return LdkpThreadNotifyEvents[Index];
}

BOOL
WINAPI
ThreadNotifyDllMain (
    _In_ HINSTANCE Module,
    _In_ DWORD Reason,
    _In_opt_ LPVOID Reserved
    )
{
    UNREFERENCED_PARAMETER( Module );
    UNREFERENCED_PARAMETER( Reserved );

    switch (Reason) {
    case DLL_PROCESS_ATTACH:
        LdkpThreadNotifyRecord( THREAD_NOTIFY_EVENT_PROCESS_ATTACH );
        break;
    case DLL_THREAD_ATTACH:
        LdkpThreadNotifyRecord( THREAD_NOTIFY_EVENT_THREAD_ATTACH );
        break;
    case DLL_THREAD_DETACH:
        LdkpThreadNotifyRecord( THREAD_NOTIFY_EVENT_THREAD_DETACH );
        break;
    case DLL_PROCESS_DETACH:
        LdkpThreadNotifyRecord( THREAD_NOTIFY_EVENT_PROCESS_DETACH );
        break;
    default:
        break;
    }

    return TRUE;
}
