#include "../ldk.h"
#include <Ldk/combaseapi.h>

#ifndef LDK_S_OK
#define LDK_S_OK ((HRESULT)0L)
#endif

#ifndef LDK_S_FALSE
#define LDK_S_FALSE ((HRESULT)1L)
#endif

#ifndef LDK_E_POINTER
#define LDK_E_POINTER ((HRESULT)0x80004003L)
#endif

#ifndef LDK_E_INVALIDARG
#define LDK_E_INVALIDARG ((HRESULT)0x80070057L)
#endif

#ifndef LDK_E_OUTOFMEMORY
#define LDK_E_OUTOFMEMORY ((HRESULT)0x8007000EL)
#endif

#ifndef LDK_E_NOTIMPL
#define LDK_E_NOTIMPL ((HRESULT)0x80004001L)
#endif

#ifndef LDK_E_NOINTERFACE
#define LDK_E_NOINTERFACE ((HRESULT)0x80004002L)
#endif

#ifndef LDK_CO_E_NOTINITIALIZED
#define LDK_CO_E_NOTINITIALIZED ((HRESULT)0x800401F0L)
#endif

#ifndef LDK_REGDB_E_CLASSNOTREG
#define LDK_REGDB_E_CLASSNOTREG ((HRESULT)0x80040154L)
#endif

#ifndef LDK_CLASS_E_NOAGGREGATION
#define LDK_CLASS_E_NOAGGREGATION ((HRESULT)0x80040110L)
#endif

#ifndef LDK_RPC_E_CHANGED_MODE
#define LDK_RPC_E_CHANGED_MODE ((HRESULT)0x80010106L)
#endif

#define LDK_HSTRING_SIGNATURE 0x6748534cUL
#define LDK_HSTRING_FLAG_REFERENCE 0x00000001UL
#define LDK_APARTMENT_SHUTDOWN_COOKIE_SIGNATURE 0x6353414cUL

#ifndef STATUS_FAIL_FAST_EXCEPTION
#define STATUS_FAIL_FAST_EXCEPTION ((NTSTATUS)0xC0000602L)
#endif

typedef struct _LDK_HSTRING {
    ULONG Signature;
    ULONG Flags;
    UINT32 Length;
    BOOL HasEmbeddedNull;
    PCWSTR Buffer;
    WCHAR InlineBuffer[1];
} LDK_HSTRING, *PLDK_HSTRING;

C_ASSERT(FIELD_OFFSET(LDK_HSTRING, InlineBuffer) == sizeof(HSTRING_HEADER));

typedef struct _LDK_APARTMENT_SHUTDOWN_REGISTRATION {
    ULONG Signature;
    UINT64 ApartmentIdentifier;
    IApartmentShutdown *CallbackObject;
} LDK_APARTMENT_SHUTDOWN_REGISTRATION, *PLDK_APARTMENT_SHUTDOWN_REGISTRATION;

static LONG LdkpCombaseMtaUsageCookie;
static volatile LONG LdkpCombaseApartmentIdentifierSeed;

#define LDK_HRESULT_FAILED(_hr) (((HRESULT)(_hr)) < 0)

static
BOOLEAN
LdkpCombaseHasApartmentState (
    VOID
    )
{
    PLDK_TEB Teb;

    Teb = LdkCurrentTeb();
    return Teb->CombaseApartmentInitCount > 0 ||
           Teb->CombaseMtaUsageCount > 0;
}

static
UINT64
LdkpCombaseGetApartmentIdentifier (
    VOID
    )
{
    PLDK_TEB Teb;
    LONG NewIdentifier;

    Teb = LdkCurrentTeb();
    if (Teb->CombaseApartmentIdentifier == 0) {
        NewIdentifier = InterlockedIncrement( &LdkpCombaseApartmentIdentifierSeed );
        Teb->CombaseApartmentIdentifier = (UINT64)(ULONG)NewIdentifier;
    }

    return Teb->CombaseApartmentIdentifier;
}

static
BOOL
LdkpCombaseStringHasEmbeddedNull (
    _In_reads_(Length) PCWSTR String,
    _In_ UINT32 Length
    )
{
    UINT32 Index;

    for (Index = 0; Index < Length; Index++) {
        if (String[Index] == L'\0') {
            return TRUE;
        }
    }

    return FALSE;
}

static
HRESULT
LdkpCombaseReferenceHstring (
    _In_ HSTRING String,
    _Out_ PLDK_HSTRING *Record
    )
{
    PLDK_HSTRING LocalRecord;

    LocalRecord = (PLDK_HSTRING)String;
    if (LocalRecord->Signature != LDK_HSTRING_SIGNATURE) {
        *Record = NULL;
        return LDK_E_INVALIDARG;
    }

    *Record = LocalRecord;
    return LDK_S_OK;
}

WINOLEAPI_(LPVOID)
CoTaskMemAlloc (
    _In_ SIZE_T cb
    )
{
    return RtlAllocateHeap( RtlProcessHeap(),
                            0,
                            cb );
}

WINOLEAPI_(VOID)
CoTaskMemFree (
    _Frees_ptr_opt_ LPVOID pv
    )
{
    if (pv != NULL) {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     pv );
    }
}

WINOLEAPI
CoCreateInstance (
    _In_ REFCLSID rclsid,
    _In_opt_ LPUNKNOWN pUnkOuter,
    _In_ DWORD dwClsContext,
    _In_ REFIID riid,
    _COM_Outptr_ LPVOID *ppv
    )
{
    UNREFERENCED_PARAMETER( rclsid );
    UNREFERENCED_PARAMETER( dwClsContext );
    UNREFERENCED_PARAMETER( riid );

    if (ppv == NULL) {
        return LDK_E_POINTER;
    }

    *ppv = NULL;

    if (pUnkOuter != NULL) {
        return LDK_CLASS_E_NOAGGREGATION;
    }

    if (!LdkpCombaseHasApartmentState()) {
        return LDK_CO_E_NOTINITIALIZED;
    }

    return LDK_REGDB_E_CLASSNOTREG;
}

WINOLEAPI
CoCreateFreeThreadedMarshaler (
    _In_opt_ LPUNKNOWN punkOuter,
    _Outptr_ LPUNKNOWN *ppunkMarshal
    )
{
    UNREFERENCED_PARAMETER( punkOuter );

    if (ppunkMarshal == NULL) {
        return LDK_E_POINTER;
    }

    *ppunkMarshal = NULL;

    if (!LdkpCombaseHasApartmentState()) {
        return LDK_CO_E_NOTINITIALIZED;
    }

    return LDK_E_NOTIMPL;
}

WINOLEAPI
CoGetContextToken (
    _Out_ ULONG_PTR *pToken
    )
{
    if (pToken == NULL) {
        return LDK_E_POINTER;
    }

    *pToken = (ULONG_PTR)LdkpCombaseGetApartmentIdentifier();

    return LDK_S_OK;
}

WINOLEAPI
CoGetObjectContext (
    _In_ REFIID riid,
    _COM_Outptr_ LPVOID *ppv
    )
{
    UNREFERENCED_PARAMETER( riid );

    if (ppv == NULL) {
        return LDK_E_POINTER;
    }

    *ppv = NULL;

    if (!LdkpCombaseHasApartmentState()) {
        return LDK_CO_E_NOTINITIALIZED;
    }

    return LDK_E_NOINTERFACE;
}

WINOLEAPI
CoMarshalInterThreadInterfaceInStream (
    _In_ REFIID riid,
    _In_ LPUNKNOWN pUnk,
    _Outptr_ LPSTREAM *ppStm
    )
{
    UNREFERENCED_PARAMETER( riid );

    if (ppStm == NULL) {
        return LDK_E_POINTER;
    }

    *ppStm = NULL;

    if (pUnk == NULL) {
        return LDK_E_INVALIDARG;
    }

    if (!LdkpCombaseHasApartmentState()) {
        return LDK_CO_E_NOTINITIALIZED;
    }

    return LDK_E_NOTIMPL;
}

WINOLEAPI
CoGetInterfaceAndReleaseStream (
    _In_ LPSTREAM pStm,
    _In_ REFIID iid,
    _COM_Outptr_ LPVOID *ppv
    )
{
    UNREFERENCED_PARAMETER( iid );

    if (ppv == NULL) {
        return LDK_E_POINTER;
    }

    *ppv = NULL;

    if (pStm == NULL) {
        return LDK_E_INVALIDARG;
    }

    if (!LdkpCombaseHasApartmentState()) {
        return LDK_CO_E_NOTINITIALIZED;
    }

    return LDK_E_NOTIMPL;
}

WINOLEAPI
RoRegisterForApartmentShutdown (
    _In_ IApartmentShutdown *callbackObject,
    _Out_ UINT64 *apartmentIdentifier,
    _Out_ APARTMENT_SHUTDOWN_REGISTRATION_COOKIE *regCookie
    )
{
    PLDK_APARTMENT_SHUTDOWN_REGISTRATION Registration;
    UINT64 Identifier;

    if (callbackObject == NULL ||
        apartmentIdentifier == NULL ||
        regCookie == NULL) {
        return LDK_E_INVALIDARG;
    }

    *regCookie = NULL;

    Registration = RtlAllocateHeap( RtlProcessHeap(),
                                    HEAP_ZERO_MEMORY,
                                    sizeof(*Registration) );
    if (Registration == NULL) {
        return LDK_E_OUTOFMEMORY;
    }

    Identifier = LdkpCombaseGetApartmentIdentifier();
    Registration->Signature = LDK_APARTMENT_SHUTDOWN_COOKIE_SIGNATURE;
    Registration->ApartmentIdentifier = Identifier;
    Registration->CallbackObject = callbackObject;

    *apartmentIdentifier = Identifier;
    *regCookie = (APARTMENT_SHUTDOWN_REGISTRATION_COOKIE)Registration;

    return LDK_S_OK;
}

WINOLEAPI
RoUnregisterForApartmentShutdown (
    _In_ APARTMENT_SHUTDOWN_REGISTRATION_COOKIE regCookie
    )
{
    PLDK_APARTMENT_SHUTDOWN_REGISTRATION Registration;

    if (regCookie == NULL) {
        return LDK_E_INVALIDARG;
    }

    Registration = (PLDK_APARTMENT_SHUTDOWN_REGISTRATION)regCookie;
    if (Registration->Signature != LDK_APARTMENT_SHUTDOWN_COOKIE_SIGNATURE) {
        return LDK_E_INVALIDARG;
    }

    Registration->Signature = 0;
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 Registration );

    return LDK_S_OK;
}

WINOLEAPI
RoGetApartmentIdentifier (
    _Out_ UINT64 *apartmentIdentifier
    )
{
    if (apartmentIdentifier == NULL) {
        return LDK_E_POINTER;
    }

    *apartmentIdentifier = LdkpCombaseGetApartmentIdentifier();

    return LDK_S_OK;
}

WINOLEAPI
WindowsCreateString (
    _In_reads_opt_(length) PCNZWCH sourceString,
    _In_ UINT32 length,
    _Outptr_result_maybenull_ HSTRING *string
    )
{
    PLDK_HSTRING Record;
    SIZE_T AllocationSize;
    SIZE_T HeaderSize;

    if (string == NULL) {
        return LDK_E_POINTER;
    }

    *string = NULL;

    if (length == 0) {
        return LDK_S_OK;
    }

    if (sourceString == NULL) {
        return LDK_E_POINTER;
    }

    HeaderSize = FIELD_OFFSET(LDK_HSTRING, InlineBuffer);
    if ((SIZE_T)length > (((SIZE_T)-1 - HeaderSize) / sizeof(WCHAR)) - 1) {
        return LDK_E_OUTOFMEMORY;
    }

    AllocationSize = HeaderSize + ((SIZE_T)length + 1) * sizeof(WCHAR);
    Record = RtlAllocateHeap( RtlProcessHeap(),
                              0,
                              AllocationSize );
    if (Record == NULL) {
        return LDK_E_OUTOFMEMORY;
    }

    Record->Signature = LDK_HSTRING_SIGNATURE;
    Record->Flags = 0;
    Record->Length = length;
    Record->HasEmbeddedNull = LdkpCombaseStringHasEmbeddedNull( sourceString,
                                                                length );
    Record->Buffer = Record->InlineBuffer;

    RtlCopyMemory( Record->InlineBuffer,
                   sourceString,
                   (SIZE_T)length * sizeof(WCHAR) );
    Record->InlineBuffer[length] = L'\0';

    *string = (HSTRING)Record;
    return LDK_S_OK;
}

WINOLEAPI
WindowsCreateStringReference (
    _In_reads_opt_(length + 1) PCWSTR sourceString,
    _In_ UINT32 length,
    _Out_ HSTRING_HEADER *hstringHeader,
    _Outptr_result_maybenull_ HSTRING *string
    )
{
    PLDK_HSTRING Record;

    if (hstringHeader == NULL ||
        string == NULL) {
        return LDK_E_POINTER;
    }

    RtlZeroMemory( hstringHeader,
                   sizeof(*hstringHeader) );
    *string = NULL;

    if (length == 0) {
        return LDK_S_OK;
    }

    if (sourceString == NULL) {
        return LDK_E_POINTER;
    }

    if (sourceString[length] != L'\0') {
        return LDK_E_INVALIDARG;
    }

    Record = (PLDK_HSTRING)hstringHeader;
    Record->Signature = LDK_HSTRING_SIGNATURE;
    Record->Flags = LDK_HSTRING_FLAG_REFERENCE;
    Record->Length = length;
    Record->HasEmbeddedNull = LdkpCombaseStringHasEmbeddedNull( sourceString,
                                                                length );
    Record->Buffer = sourceString;

    *string = (HSTRING)Record;
    return LDK_S_OK;
}

WINOLEAPI
WindowsDeleteString (
    _In_opt_ HSTRING string
    )
{
    PLDK_HSTRING Record;
    HRESULT Result;

    if (string == NULL) {
        return LDK_S_OK;
    }

    Result = LdkpCombaseReferenceHstring( string,
                                          &Record );
    if (Result != LDK_S_OK) {
        return Result;
    }

    if ((Record->Flags & LDK_HSTRING_FLAG_REFERENCE) == 0) {
        Record->Signature = 0;
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     Record );
    }

    return LDK_S_OK;
}

WINOLEAPI
WindowsStringHasEmbeddedNull (
    _In_opt_ HSTRING string,
    _Out_ BOOL *hasEmbedNull
    )
{
    PLDK_HSTRING Record;
    HRESULT Result;

    if (hasEmbedNull == NULL) {
        return LDK_E_POINTER;
    }

    *hasEmbedNull = FALSE;

    if (string == NULL) {
        return LDK_S_OK;
    }

    Result = LdkpCombaseReferenceHstring( string,
                                          &Record );
    if (Result != LDK_S_OK) {
        return Result;
    }

    *hasEmbedNull = Record->HasEmbeddedNull;
    return LDK_S_OK;
}

WINOLEAPI
SetRestrictedErrorInfo (
    _In_opt_ IRestrictedErrorInfo *pRestrictedErrorInfo
    )
{
    UNREFERENCED_PARAMETER( pRestrictedErrorInfo );

    return LDK_S_OK;
}

WINOLEAPI
GetRestrictedErrorInfo (
    _Outptr_result_maybenull_ IRestrictedErrorInfo **ppRestrictedErrorInfo
    )
{
    if (ppRestrictedErrorInfo == NULL) {
        return LDK_E_POINTER;
    }

    *ppRestrictedErrorInfo = NULL;
    return LDK_S_FALSE;
}

WINOLEAPI_(BOOL)
RoOriginateErrorW (
    _In_ HRESULT error,
    _In_ UINT cchMax,
    _In_reads_or_z_opt_(cchMax) PCWSTR message
    )
{
    UNREFERENCED_PARAMETER( cchMax );
    UNREFERENCED_PARAMETER( message );

    return LDK_HRESULT_FAILED( error );
}

WINOLEAPI_(BOOL)
RoOriginateError (
    _In_ HRESULT error,
    _In_opt_ HSTRING message
    )
{
    UNREFERENCED_PARAMETER( message );

    return LDK_HRESULT_FAILED( error );
}

WINOLEAPI_(BOOL)
RoTransformErrorW (
    _In_ HRESULT oldError,
    _In_ HRESULT newError,
    _In_ UINT cchMax,
    _In_reads_or_z_opt_(cchMax) PCWSTR message
    )
{
    UNREFERENCED_PARAMETER( cchMax );
    UNREFERENCED_PARAMETER( message );

    if (oldError == newError) {
        return FALSE;
    }

    if (!LDK_HRESULT_FAILED( oldError ) &&
        !LDK_HRESULT_FAILED( newError )) {
        return FALSE;
    }

    return TRUE;
}

WINOLEAPI_(BOOL)
RoTransformError (
    _In_ HRESULT oldError,
    _In_ HRESULT newError,
    _In_opt_ HSTRING message
    )
{
    UNREFERENCED_PARAMETER( message );

    return RoTransformErrorW( oldError,
                              newError,
                              0,
                              NULL );
}

WINOLEAPI
RoCaptureErrorContext (
    _In_ HRESULT hr
    )
{
    UNREFERENCED_PARAMETER( hr );

    return LDK_S_OK;
}

WINOLEAPI_(VOID)
RoFailFastWithErrorContext (
    _In_ HRESULT hrError
    )
{
    EXCEPTION_RECORD ExceptionRecord;

    RtlZeroMemory( &ExceptionRecord,
                   sizeof(ExceptionRecord) );
    ExceptionRecord.ExceptionCode = STATUS_FAIL_FAST_EXCEPTION;
    ExceptionRecord.ExceptionFlags = EXCEPTION_NONCONTINUABLE;
    ExceptionRecord.ExceptionAddress = (PVOID)_ReturnAddress();
    ExceptionRecord.NumberParameters = 1;
    ExceptionRecord.ExceptionInformation[0] = (ULONG_PTR)hrError;

    RtlRaiseException( &ExceptionRecord );
}

WINOLEAPI_(BOOL)
RoOriginateLanguageException (
    _In_ HRESULT error,
    _In_opt_ HSTRING message,
    _In_opt_ IUnknown *languageException
    )
{
    UNREFERENCED_PARAMETER( message );
    UNREFERENCED_PARAMETER( languageException );

    return LDK_HRESULT_FAILED( error );
}

WINOLEAPI
RoReportUnhandledError (
    _In_ IRestrictedErrorInfo *pRestrictedErrorInfo
    )
{
    if (pRestrictedErrorInfo == NULL) {
        return LDK_E_INVALIDARG;
    }

    return LDK_S_OK;
}

WINOLEAPI
CoGetApartmentType (
    _Out_ APTTYPE *pAptType,
    _Out_ APTTYPEQUALIFIER *pAptQualifier
    )
{
    PLDK_TEB Teb;

    if (pAptType == NULL ||
        pAptQualifier == NULL) {
        return LDK_E_POINTER;
    }

    Teb = LdkCurrentTeb();

    if (Teb->CombaseApartmentInitCount > 0) {
        *pAptType = (APTTYPE)Teb->CombaseApartmentType;
        *pAptQualifier = APTTYPEQUALIFIER_NONE;
        return LDK_S_OK;
    }

    if (Teb->CombaseMtaUsageCount > 0) {
        *pAptType = APTTYPE_MTA;
        *pAptQualifier = APTTYPEQUALIFIER_IMPLICIT_MTA;
        return LDK_S_OK;
    }

    *pAptType = APTTYPE_CURRENT;
    *pAptQualifier = APTTYPEQUALIFIER_NONE;
    return LDK_CO_E_NOTINITIALIZED;
}

WINOLEAPI
CoIncrementMTAUsage (
    _Out_ CO_MTA_USAGE_COOKIE *pCookie
    )
{
    PLDK_TEB Teb;

    if (pCookie == NULL) {
        return LDK_E_POINTER;
    }

    Teb = LdkCurrentTeb();
    Teb->CombaseMtaUsageCount++;
    *pCookie = (CO_MTA_USAGE_COOKIE)&LdkpCombaseMtaUsageCookie;

    return LDK_S_OK;
}

WINOLEAPI
CoDecrementMTAUsage (
    _In_ CO_MTA_USAGE_COOKIE Cookie
    )
{
    PLDK_TEB Teb;

    if (Cookie == NULL) {
        return LDK_E_INVALIDARG;
    }

    Teb = LdkCurrentTeb();
    if (Teb->CombaseMtaUsageCount > 0) {
        Teb->CombaseMtaUsageCount--;
    }

    return LDK_S_OK;
}

WINOLEAPI
RoInitialize (
    _In_ RO_INIT_TYPE initType
    )
{
    PLDK_TEB Teb;
    APTTYPE ApartmentType;

    if (initType == RO_INIT_SINGLETHREADED) {
        ApartmentType = APTTYPE_STA;
    } else if (initType == RO_INIT_MULTITHREADED) {
        ApartmentType = APTTYPE_MTA;
    } else {
        return LDK_E_INVALIDARG;
    }

    Teb = LdkCurrentTeb();

    if (Teb->CombaseApartmentInitCount > 0) {
        if (Teb->CombaseApartmentType != (LONG)ApartmentType) {
            return LDK_RPC_E_CHANGED_MODE;
        }

        Teb->CombaseApartmentInitCount++;
        return LDK_S_FALSE;
    }

    if (Teb->CombaseMtaUsageCount > 0 &&
        ApartmentType != APTTYPE_MTA) {
        return LDK_RPC_E_CHANGED_MODE;
    }

    Teb->CombaseApartmentType = (LONG)ApartmentType;
    Teb->CombaseApartmentInitCount = 1;

    return LDK_S_OK;
}

WINOLEAPI_(VOID)
RoUninitialize (
    VOID
    )
{
    PLDK_TEB Teb = LdkCurrentTeb();

    if (Teb->CombaseApartmentInitCount > 0) {
        Teb->CombaseApartmentInitCount--;
        if (Teb->CombaseApartmentInitCount == 0) {
            Teb->CombaseApartmentType = 0;
        }
    }
}

#pragma warning(disable:4152)
LDK_FUNCTION_REGISTRATION LdkpCombaseFunctionRegistration[] = {
    { "CoCreateFreeThreadedMarshaler", CoCreateFreeThreadedMarshaler },
    { "CoCreateInstance", CoCreateInstance },
    { "CoDecrementMTAUsage", CoDecrementMTAUsage },
    { "CoGetApartmentType", CoGetApartmentType },
    { "CoGetContextToken", CoGetContextToken },
    { "CoGetInterfaceAndReleaseStream", CoGetInterfaceAndReleaseStream },
    { "CoGetObjectContext", CoGetObjectContext },
    { "CoIncrementMTAUsage", CoIncrementMTAUsage },
    { "CoMarshalInterThreadInterfaceInStream", CoMarshalInterThreadInterfaceInStream },
    { "CoTaskMemAlloc", CoTaskMemAlloc },
    { "CoTaskMemFree", CoTaskMemFree },
    { "GetRestrictedErrorInfo", GetRestrictedErrorInfo },
    { "RoCaptureErrorContext", RoCaptureErrorContext },
    { "RoFailFastWithErrorContext", RoFailFastWithErrorContext },
    { "RoGetApartmentIdentifier", RoGetApartmentIdentifier },
    { "RoInitialize", RoInitialize },
    { "RoOriginateError", RoOriginateError },
    { "RoOriginateErrorW", RoOriginateErrorW },
    { "RoOriginateLanguageException", RoOriginateLanguageException },
    { "RoRegisterForApartmentShutdown", RoRegisterForApartmentShutdown },
    { "RoReportUnhandledError", RoReportUnhandledError },
    { "RoTransformError", RoTransformError },
    { "RoTransformErrorW", RoTransformErrorW },
    { "RoUninitialize", RoUninitialize },
    { "RoUnregisterForApartmentShutdown", RoUnregisterForApartmentShutdown },
    { "SetRestrictedErrorInfo", SetRestrictedErrorInfo },
    { "WindowsCreateString", WindowsCreateString },
    { "WindowsCreateStringReference", WindowsCreateStringReference },
    { "WindowsDeleteString", WindowsDeleteString },
    { "WindowsStringHasEmbeddedNull", WindowsStringHasEmbeddedNull },
    { NULL, NULL }
};
#pragma warning(default:4152)

LDK_MODULE LdkpCombaseModule = {
    RTL_CONSTANT_STRING("COMBASE.DLL"),
    RTL_CONSTANT_STRING("\\SystemRoot\\system32\\COMBASE.DLL"),
    LdkpCombaseFunctionRegistration,
    NULL
};
