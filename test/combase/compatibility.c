#if _KERNEL_MODE
#include <Ldk/Windows.h>
#include <Ldk/combaseapi.h>

BOOLEAN
CombaseCompatibilityTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, CombaseCompatibilityTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <stdio.h>
#define PAGED_CODE()
#endif

#define LDK_TEST_S_OK ((HRESULT)0L)
#define LDK_TEST_S_FALSE ((HRESULT)1L)
#define LDK_TEST_E_NOTIMPL ((HRESULT)0x80004001L)
#define LDK_TEST_E_NOINTERFACE ((HRESULT)0x80004002L)
#define LDK_TEST_E_POINTER ((HRESULT)0x80004003L)
#define LDK_TEST_E_FAIL ((HRESULT)0x80004005L)
#define LDK_TEST_E_INVALIDARG ((HRESULT)0x80070057L)
#define LDK_TEST_CO_E_NOTINITIALIZED ((HRESULT)0x800401F0L)
#define LDK_TEST_REGDB_E_CLASSNOTREG ((HRESULT)0x80040154L)
#define LDK_TEST_RPC_E_CHANGED_MODE ((HRESULT)0x80010106L)
#define LDK_TEST_RO_E_APARTMENT_IDENTIFIER_UNAVAILABLE ((HRESULT)0x80073DE1L)
#define LDK_TEST_RO_INIT_SINGLETHREADED 0
#define LDK_TEST_RO_INIT_MULTITHREADED 1
#define LDK_TEST_APTTYPE_CURRENT (-1)
#define LDK_TEST_APTTYPE_STA 0
#define LDK_TEST_APTTYPE_MTA 1
#define LDK_TEST_APTTYPEQUALIFIER_NONE 0
#define LDK_TEST_APTTYPEQUALIFIER_IMPLICIT_MTA 1

typedef LPVOID (WINAPI *PCO_TASK_MEM_ALLOC) (
    _In_ SIZE_T cb
    );

typedef VOID (WINAPI *PCO_TASK_MEM_FREE) (
    _Frees_ptr_opt_ LPVOID pv
    );

typedef HRESULT (WINAPI *PCO_CREATE_INSTANCE) (
    _In_ REFCLSID rclsid,
    _In_opt_ PVOID pUnkOuter,
    _In_ DWORD dwClsContext,
    _In_ REFIID riid,
    _Outptr_ PVOID *ppv
    );

typedef HRESULT (WINAPI *PCO_CREATE_FREE_THREADED_MARSHALER) (
    _In_opt_ PVOID punkOuter,
    _Outptr_ PVOID *ppunkMarshal
    );

typedef HRESULT (WINAPI *PCO_GET_APARTMENT_TYPE) (
    _Out_ int *pAptType,
    _Out_ int *pAptQualifier
    );

typedef HRESULT (WINAPI *PCO_GET_CONTEXT_TOKEN) (
    _Out_ ULONG_PTR *pToken
    );

typedef HRESULT (WINAPI *PCO_GET_OBJECT_CONTEXT) (
    _In_ REFIID riid,
    _Outptr_ PVOID *ppv
    );

typedef HRESULT (WINAPI *PCO_INCREMENT_MTA_USAGE) (
    _Out_ HANDLE *pCookie
    );

typedef HRESULT (WINAPI *PCO_DECREMENT_MTA_USAGE) (
    _In_ HANDLE Cookie
    );

typedef HRESULT (WINAPI *PCO_MARSHAL_INTER_THREAD_INTERFACE_IN_STREAM) (
    _In_ REFIID riid,
    _In_ PVOID pUnk,
    _Outptr_ PVOID *ppStm
    );

typedef HRESULT (WINAPI *PCO_GET_INTERFACE_AND_RELEASE_STREAM) (
    _In_ PVOID pStm,
    _In_ REFIID iid,
    _Outptr_ PVOID *ppv
    );

typedef HRESULT (WINAPI *PRO_INITIALIZE) (
    _In_ int initType
    );

typedef VOID (WINAPI *PRO_UNINITIALIZE) (
    VOID
    );

typedef HRESULT (WINAPI *PRO_GET_APARTMENT_IDENTIFIER) (
    _Out_ UINT64 *apartmentIdentifier
    );

typedef HRESULT (WINAPI *PRO_REGISTER_FOR_APARTMENT_SHUTDOWN) (
    _In_ PVOID callbackObject,
    _Out_ UINT64 *apartmentIdentifier,
    _Out_ HANDLE *regCookie
    );

typedef HRESULT (WINAPI *PRO_UNREGISTER_FOR_APARTMENT_SHUTDOWN) (
    _In_ HANDLE regCookie
    );

typedef HRESULT (WINAPI *PWINDOWS_CREATE_STRING) (
    _In_reads_opt_(length) PCWSTR sourceString,
    _In_ UINT32 length,
    _Outptr_result_maybenull_ PVOID *string
    );

typedef HRESULT (WINAPI *PWINDOWS_CREATE_STRING_REFERENCE) (
    _In_reads_opt_(length + 1) PCWSTR sourceString,
    _In_ UINT32 length,
    _Out_ PVOID hstringHeader,
    _Outptr_result_maybenull_ PVOID *string
    );

typedef HRESULT (WINAPI *PWINDOWS_DELETE_STRING) (
    _In_opt_ PVOID string
    );

typedef HRESULT (WINAPI *PWINDOWS_STRING_HAS_EMBEDDED_NULL) (
    _In_opt_ PVOID string,
    _Out_ BOOL *hasEmbedNull
    );

typedef HRESULT (WINAPI *PSET_RESTRICTED_ERROR_INFO) (
    _In_opt_ PVOID pRestrictedErrorInfo
    );

typedef HRESULT (WINAPI *PGET_RESTRICTED_ERROR_INFO) (
    _Outptr_result_maybenull_ PVOID *ppRestrictedErrorInfo
    );

typedef BOOL (WINAPI *PRO_ORIGINATE_ERROR) (
    _In_ HRESULT error,
    _In_opt_ PVOID message
    );

typedef BOOL (WINAPI *PRO_TRANSFORM_ERROR) (
    _In_ HRESULT oldError,
    _In_ HRESULT newError,
    _In_opt_ PVOID message
    );

typedef HRESULT (WINAPI *PRO_CAPTURE_ERROR_CONTEXT) (
    _In_ HRESULT hr
    );

typedef VOID (WINAPI *PRO_FAIL_FAST_WITH_ERROR_CONTEXT) (
    _In_ HRESULT hrError
    );

typedef BOOL (WINAPI *PRO_ORIGINATE_LANGUAGE_EXCEPTION) (
    _In_ HRESULT error,
    _In_opt_ PVOID message,
    _In_opt_ PVOID languageException
    );

typedef HRESULT (WINAPI *PRO_REPORT_UNHANDLED_ERROR) (
    _In_ PVOID pRestrictedErrorInfo
    );

#if defined(_WIN64)
#define LDK_TEST_HSTRING_HEADER_SIZE 24
#else
#define LDK_TEST_HSTRING_HEADER_SIZE 20
#endif

typedef struct _LDK_TEST_HSTRING_HEADER {
    union {
        PVOID Reserved1;
        UCHAR Reserved2[LDK_TEST_HSTRING_HEADER_SIZE];
    } Reserved;
} LDK_TEST_HSTRING_HEADER;

static
BOOLEAN
LdkpIsKnownApartment (
    _In_ int AptType,
    _In_ int AptQualifier
    )
{
    return (AptType == LDK_TEST_APTTYPE_CURRENT ||
            AptType == LDK_TEST_APTTYPE_STA ||
            AptType == LDK_TEST_APTTYPE_MTA) &&
           (AptQualifier == LDK_TEST_APTTYPEQUALIFIER_NONE ||
            AptQualifier == LDK_TEST_APTTYPEQUALIFIER_IMPLICIT_MTA);
}

static
BOOLEAN
LdkpTestCombaseTaskMemory (
    _In_ PCO_TASK_MEM_ALLOC TaskMemAlloc,
    _In_ PCO_TASK_MEM_FREE TaskMemFree
    )
{
    UCHAR *Buffer;

    Buffer = (UCHAR *)TaskMemAlloc( 64 );
    if (Buffer == NULL) {
        fprintf(stderr,
                "[Failed] CoTaskMemAlloc(64)\n" );
        return FALSE;
    }

    for (ULONG Index = 0; Index < 64; Index++) {
        Buffer[Index] = (UCHAR)(Index ^ 0x5a);
    }

    for (ULONG Index = 0; Index < 64; Index++) {
        if (Buffer[Index] != (UCHAR)(Index ^ 0x5a)) {
            fprintf(stderr,
                    "[Failed] COMBASE task memory write/read Index = %lu Value = 0x%02x\n",
                    Index,
                    Buffer[Index] );
            TaskMemFree( Buffer );
            return FALSE;
        }
    }

    TaskMemFree( Buffer );
    TaskMemFree( NULL );

    return TRUE;
}

static
BOOLEAN
LdkpTestCombaseApartmentExports (
    _In_ PCO_GET_APARTMENT_TYPE GetApartmentType,
    _In_ PCO_INCREMENT_MTA_USAGE IncrementMtaUsage,
    _In_ PCO_DECREMENT_MTA_USAGE DecrementMtaUsage,
    _In_ PRO_INITIALIZE RuntimeInitialize,
    _In_ PRO_UNINITIALIZE RuntimeUninitialize
    )
{
    HRESULT Result;
    int AptType;
    int AptQualifier;
    HANDLE Cookie;

    AptType = 0;
    AptQualifier = 0;
    Result = GetApartmentType( &AptType,
                               &AptQualifier );
    if (Result != LDK_TEST_S_OK &&
        Result != LDK_TEST_CO_E_NOTINITIALIZED) {
        fprintf(stderr,
                "[Failed] CoGetApartmentType initial Result = 0x%08lx Apt = %d Qualifier = %d\n",
                Result,
                AptType,
                AptQualifier );
        return FALSE;
    }

    if (Result == LDK_TEST_S_OK &&
        !LdkpIsKnownApartment( AptType,
                               AptQualifier )) {
        fprintf(stderr,
                "[Failed] CoGetApartmentType unknown initial Apt = %d Qualifier = %d\n",
                AptType,
                AptQualifier );
        return FALSE;
    }

    Cookie = NULL;
    Result = IncrementMtaUsage( &Cookie );
    if (Result != LDK_TEST_S_OK ||
        Cookie == NULL) {
        fprintf(stderr,
                "[Failed] CoIncrementMTAUsage Result = 0x%08lx Cookie = %p\n",
                Result,
                Cookie );
        return FALSE;
    }

    Result = GetApartmentType( &AptType,
                               &AptQualifier );
    if (Result == LDK_TEST_S_OK &&
        !LdkpIsKnownApartment( AptType,
                               AptQualifier )) {
        fprintf(stderr,
                "[Failed] CoGetApartmentType after MTA usage Apt = %d Qualifier = %d\n",
                AptType,
                AptQualifier );
        DecrementMtaUsage( Cookie );
        return FALSE;
    }

    Result = DecrementMtaUsage( Cookie );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] CoDecrementMTAUsage Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    Result = RuntimeInitialize( LDK_TEST_RO_INIT_MULTITHREADED );
    if (Result == LDK_TEST_S_OK ||
        Result == LDK_TEST_S_FALSE) {
        RuntimeUninitialize();
    } else if (Result != LDK_TEST_RPC_E_CHANGED_MODE) {
        fprintf(stderr,
                "[Failed] RoInitialize(MTA) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
LdkpTestCombaseApartmentIdentifierExports (
    _In_ PRO_GET_APARTMENT_IDENTIFIER GetApartmentIdentifier
    )
{
    HRESULT Result;
    UINT64 FirstIdentifier;
    UINT64 SecondIdentifier;

    FirstIdentifier = 0;
    Result = GetApartmentIdentifier( &FirstIdentifier );
    if (Result == LDK_TEST_CO_E_NOTINITIALIZED) {
        return TRUE;
    }
#if !_KERNEL_MODE
    if (Result == LDK_TEST_RO_E_APARTMENT_IDENTIFIER_UNAVAILABLE) {
        printf("[Skipped] RoGetApartmentIdentifier unavailable Result = 0x%08lx\n\n",
               Result );
        return TRUE;
    }
#endif

    if (Result != LDK_TEST_S_OK ||
        FirstIdentifier == 0) {
        fprintf(stderr,
                "[Failed] RoGetApartmentIdentifier first Result = 0x%08lx Identifier = 0x%I64x\n",
                Result,
                FirstIdentifier );
        return FALSE;
    }

    SecondIdentifier = 0;
    Result = GetApartmentIdentifier( &SecondIdentifier );
    if (Result != LDK_TEST_S_OK ||
        SecondIdentifier != FirstIdentifier) {
        fprintf(stderr,
                "[Failed] RoGetApartmentIdentifier second Result = 0x%08lx First = 0x%I64x Second = 0x%I64x\n",
                Result,
                FirstIdentifier,
                SecondIdentifier );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
LdkpIsExpectedComNoRuntimeResult (
    _In_ HRESULT Result
    )
{
    return Result == LDK_TEST_CO_E_NOTINITIALIZED ||
           Result == LDK_TEST_REGDB_E_CLASSNOTREG ||
           Result == LDK_TEST_E_NOINTERFACE ||
           Result == LDK_TEST_E_INVALIDARG ||
           Result == LDK_TEST_E_NOTIMPL;
}

static
BOOLEAN
LdkpTestCombaseLimitedComExports (
    _In_ PCO_CREATE_INSTANCE CreateInstance,
    _In_ PCO_CREATE_FREE_THREADED_MARSHALER CreateFreeThreadedMarshaler,
    _In_ PCO_GET_CONTEXT_TOKEN GetContextToken,
    _In_ PCO_GET_OBJECT_CONTEXT GetObjectContext,
    _In_ PCO_MARSHAL_INTER_THREAD_INTERFACE_IN_STREAM MarshalInterThreadInterfaceInStream,
    _In_ PCO_GET_INTERFACE_AND_RELEASE_STREAM GetInterfaceAndReleaseStream
    )
{
    HRESULT Result;
    ULONG_PTR Token;

    UNREFERENCED_PARAMETER( CreateInstance );
    UNREFERENCED_PARAMETER( CreateFreeThreadedMarshaler );
    UNREFERENCED_PARAMETER( GetObjectContext );
    UNREFERENCED_PARAMETER( MarshalInterThreadInterfaceInStream );
    UNREFERENCED_PARAMETER( GetInterfaceAndReleaseStream );

    Result = GetContextToken( NULL );
    if (Result != LDK_TEST_E_POINTER &&
        !LdkpIsExpectedComNoRuntimeResult( Result )) {
        fprintf(stderr,
                "[Failed] CoGetContextToken(NULL) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    Token = 0;
    Result = GetContextToken( &Token );
    if (Result == LDK_TEST_S_OK) {
        if (Token == 0) {
            fprintf(stderr,
                    "[Failed] CoGetContextToken returned zero token\n" );
            return FALSE;
        }
    } else if (!LdkpIsExpectedComNoRuntimeResult( Result )) {
        fprintf(stderr,
                "[Failed] CoGetContextToken Result = 0x%08lx Token = 0x%Ix\n",
                Result,
                Token );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
LdkpTestCombaseHstringExports (
    _In_ PWINDOWS_CREATE_STRING CreateString,
    _In_ PWINDOWS_CREATE_STRING_REFERENCE CreateStringReference,
    _In_ PWINDOWS_DELETE_STRING DeleteString,
    _In_ PWINDOWS_STRING_HAS_EMBEDDED_NULL StringHasEmbeddedNull
    )
{
    HRESULT Result;
    PVOID String;
    BOOL HasEmbeddedNull;
    WCHAR EmbeddedString[] = { L'A', L'\0', L'B', L'\0' };
    WCHAR ReferenceString[] = L"Reference";
    LDK_TEST_HSTRING_HEADER Header;

    String = NULL;
    Result = CreateString( L"Owned",
                           5,
                           &String );
    if (Result != LDK_TEST_S_OK ||
        String == NULL) {
        fprintf(stderr,
                "[Failed] WindowsCreateString(Owned) Result = 0x%08lx String = %p\n",
                Result,
                String );
        return FALSE;
    }

    HasEmbeddedNull = TRUE;
    Result = StringHasEmbeddedNull( String,
                                    &HasEmbeddedNull );
    if (Result != LDK_TEST_S_OK ||
        HasEmbeddedNull) {
        fprintf(stderr,
                "[Failed] WindowsStringHasEmbeddedNull(Owned) Result = 0x%08lx HasEmbeddedNull = %d\n",
                Result,
                HasEmbeddedNull );
        DeleteString( String );
        return FALSE;
    }

    Result = DeleteString( String );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] WindowsDeleteString(Owned) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    String = NULL;
    Result = CreateString( EmbeddedString,
                           3,
                           &String );
    if (Result != LDK_TEST_S_OK ||
        String == NULL) {
        fprintf(stderr,
                "[Failed] WindowsCreateString(embedded-null) Result = 0x%08lx String = %p\n",
                Result,
                String );
        return FALSE;
    }

    HasEmbeddedNull = FALSE;
    Result = StringHasEmbeddedNull( String,
                                    &HasEmbeddedNull );
    if (Result != LDK_TEST_S_OK ||
        !HasEmbeddedNull) {
        fprintf(stderr,
                "[Failed] WindowsStringHasEmbeddedNull(embedded-null) Result = 0x%08lx HasEmbeddedNull = %d\n",
                Result,
                HasEmbeddedNull );
        DeleteString( String );
        return FALSE;
    }

    Result = DeleteString( String );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] WindowsDeleteString(embedded-null) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    String = NULL;
    Result = CreateStringReference( ReferenceString,
                                    9,
                                    &Header,
                                    &String );
    if (Result != LDK_TEST_S_OK ||
        String == NULL) {
        fprintf(stderr,
                "[Failed] WindowsCreateStringReference Result = 0x%08lx String = %p\n",
                Result,
                String );
        return FALSE;
    }

    HasEmbeddedNull = TRUE;
    Result = StringHasEmbeddedNull( String,
                                    &HasEmbeddedNull );
    if (Result != LDK_TEST_S_OK ||
        HasEmbeddedNull) {
        fprintf(stderr,
                "[Failed] WindowsStringHasEmbeddedNull(reference) Result = 0x%08lx HasEmbeddedNull = %d\n",
                Result,
                HasEmbeddedNull );
        DeleteString( String );
        return FALSE;
    }

    Result = DeleteString( String );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] WindowsDeleteString(reference) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    String = (PVOID)(ULONG_PTR)1;
    Result = CreateString( NULL,
                           0,
                           &String );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] WindowsCreateString(empty) Result = 0x%08lx String = %p\n",
                Result,
                String );
        return FALSE;
    }

    HasEmbeddedNull = TRUE;
    Result = StringHasEmbeddedNull( String,
                                    &HasEmbeddedNull );
    if (Result != LDK_TEST_S_OK ||
        HasEmbeddedNull) {
        fprintf(stderr,
                "[Failed] WindowsStringHasEmbeddedNull(empty) Result = 0x%08lx HasEmbeddedNull = %d String = %p\n",
                Result,
                HasEmbeddedNull,
                String );
        DeleteString( String );
        return FALSE;
    }

    Result = DeleteString( String );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] WindowsDeleteString(empty) Result = 0x%08lx String = %p\n",
                Result,
                String );
        return FALSE;
    }

    return TRUE;
}

static
BOOLEAN
LdkpTestCombaseErrorContextExports (
    _In_ PSET_RESTRICTED_ERROR_INFO SetErrorInfo,
    _In_ PGET_RESTRICTED_ERROR_INFO GetErrorInfo,
    _In_ PRO_ORIGINATE_ERROR OriginateError,
    _In_ PRO_TRANSFORM_ERROR TransformError,
    _In_ PRO_CAPTURE_ERROR_CONTEXT CaptureErrorContext,
    _In_ PRO_ORIGINATE_LANGUAGE_EXCEPTION OriginateLanguageException
    )
{
    HRESULT Result;
    PVOID ErrorInfo;
    BOOL BooleanResult;

    ErrorInfo = (PVOID)(ULONG_PTR)1;
    Result = GetErrorInfo( &ErrorInfo );
    if (Result != LDK_TEST_S_FALSE ||
        ErrorInfo != NULL) {
        fprintf(stderr,
                "[Failed] GetRestrictedErrorInfo(empty) Result = 0x%08lx ErrorInfo = %p\n",
                Result,
                ErrorInfo );
        return FALSE;
    }

    Result = SetErrorInfo( NULL );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] SetRestrictedErrorInfo(NULL) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    Result = CaptureErrorContext( LDK_TEST_E_FAIL );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] RoCaptureErrorContext(E_FAIL) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    BooleanResult = OriginateError( LDK_TEST_S_OK,
                                    NULL );
    if (BooleanResult) {
        fprintf(stderr,
                "[Failed] RoOriginateError(S_OK) returned TRUE\n" );
        return FALSE;
    }

    BooleanResult = OriginateError( LDK_TEST_E_FAIL,
                                    NULL );
    if (!BooleanResult) {
        fprintf(stderr,
                "[Failed] RoOriginateError(E_FAIL) returned FALSE\n" );
        return FALSE;
    }

    BooleanResult = TransformError( LDK_TEST_E_FAIL,
                                    LDK_TEST_E_FAIL,
                                    NULL );
    if (BooleanResult) {
        fprintf(stderr,
                "[Failed] RoTransformError(same error) returned TRUE\n" );
        return FALSE;
    }

    BooleanResult = TransformError( LDK_TEST_E_FAIL,
                                    LDK_TEST_E_INVALIDARG,
                                    NULL );
    if (!BooleanResult) {
        fprintf(stderr,
                "[Failed] RoTransformError(E_FAIL -> E_INVALIDARG) returned FALSE\n" );
        return FALSE;
    }

    BooleanResult = OriginateLanguageException( LDK_TEST_E_FAIL,
                                                NULL,
                                                NULL );
    if (!BooleanResult) {
        fprintf(stderr,
                "[Failed] RoOriginateLanguageException(E_FAIL) returned FALSE\n" );
        return FALSE;
    }

    return TRUE;
}

#if _KERNEL_MODE
static
BOOLEAN
LdkpTestCombaseKernelLimitedComState (
    VOID
    )
{
    static const GUID ZeroGuid = { 0 };
    HRESULT Result;
    ULONG_PTR FirstToken;
    ULONG_PTR SecondToken;
    PVOID Object;
    LPUNKNOWN Marshal;
    LPSTREAM Stream;

    FirstToken = 0;
    Result = CoGetContextToken( &FirstToken );
    if (Result != LDK_TEST_S_OK ||
        FirstToken == 0) {
        fprintf(stderr,
                "[Failed] direct CoGetContextToken Result = 0x%08lx Token = 0x%Ix\n",
                Result,
                FirstToken );
        return FALSE;
    }

    SecondToken = 0;
    Result = CoGetContextToken( &SecondToken );
    if (Result != LDK_TEST_S_OK ||
        SecondToken != FirstToken) {
        fprintf(stderr,
                "[Failed] repeated CoGetContextToken Result = 0x%08lx First = 0x%Ix Second = 0x%Ix\n",
                Result,
                FirstToken,
                SecondToken );
        return FALSE;
    }

    Object = (PVOID)(ULONG_PTR)1;
    Result = CoCreateInstance( &ZeroGuid,
                               NULL,
                               0,
                               &ZeroGuid,
                               &Object );
    if (Result != LDK_TEST_CO_E_NOTINITIALIZED ||
        Object != NULL) {
        fprintf(stderr,
                "[Failed] uninitialized CoCreateInstance Result = 0x%08lx Object = %p\n",
                Result,
                Object );
        return FALSE;
    }

    Result = RoInitialize( RO_INIT_MULTITHREADED );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] limited COM RoInitialize(MTA) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    Object = (PVOID)(ULONG_PTR)1;
    Result = CoCreateInstance( &ZeroGuid,
                               NULL,
                               0,
                               &ZeroGuid,
                               &Object );
    if (Result != LDK_TEST_REGDB_E_CLASSNOTREG ||
        Object != NULL) {
        fprintf(stderr,
                "[Failed] initialized CoCreateInstance Result = 0x%08lx Object = %p\n",
                Result,
                Object );
        RoUninitialize();
        return FALSE;
    }

    Marshal = (LPUNKNOWN)(ULONG_PTR)1;
    Result = CoCreateFreeThreadedMarshaler( NULL,
                                            &Marshal );
    if (Result != LDK_TEST_E_NOTIMPL ||
        Marshal != NULL) {
        fprintf(stderr,
                "[Failed] CoCreateFreeThreadedMarshaler Result = 0x%08lx Marshal = %p\n",
                Result,
                Marshal );
        RoUninitialize();
        return FALSE;
    }

    Object = (PVOID)(ULONG_PTR)1;
    Result = CoGetObjectContext( &ZeroGuid,
                                 &Object );
    if (Result != LDK_TEST_E_NOINTERFACE ||
        Object != NULL) {
        fprintf(stderr,
                "[Failed] CoGetObjectContext Result = 0x%08lx Object = %p\n",
                Result,
                Object );
        RoUninitialize();
        return FALSE;
    }

    Stream = (LPSTREAM)(ULONG_PTR)1;
    Result = CoMarshalInterThreadInterfaceInStream( &ZeroGuid,
                                                    (LPUNKNOWN)&Object,
                                                    &Stream );
    if (Result != LDK_TEST_E_NOTIMPL ||
        Stream != NULL) {
        fprintf(stderr,
                "[Failed] CoMarshalInterThreadInterfaceInStream Result = 0x%08lx Stream = %p\n",
                Result,
                Stream );
        RoUninitialize();
        return FALSE;
    }

    Object = (PVOID)(ULONG_PTR)1;
    Result = CoGetInterfaceAndReleaseStream( (LPSTREAM)&Object,
                                             &ZeroGuid,
                                             &Object );
    if (Result != LDK_TEST_E_NOTIMPL ||
        Object != NULL) {
        fprintf(stderr,
                "[Failed] CoGetInterfaceAndReleaseStream Result = 0x%08lx Object = %p\n",
                Result,
                Object );
        RoUninitialize();
        return FALSE;
    }

    RoUninitialize();
    return TRUE;
}

static
BOOLEAN
LdkpTestCombaseKernelApartmentState (
    VOID
    )
{
    HRESULT Result;
    APTTYPE AptType;
    APTTYPEQUALIFIER AptQualifier;
    CO_MTA_USAGE_COOKIE Cookie;

    Result = CoGetApartmentType( &AptType,
                                 &AptQualifier );
    if (Result != LDK_TEST_CO_E_NOTINITIALIZED ||
        AptType != APTTYPE_CURRENT ||
        AptQualifier != APTTYPEQUALIFIER_NONE) {
        fprintf(stderr,
                "[Failed] initial CoGetApartmentType Result = 0x%08lx Apt = %d Qualifier = %d\n",
                Result,
                AptType,
                AptQualifier );
        return FALSE;
    }

    Result = RoInitialize( RO_INIT_MULTITHREADED );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] RoInitialize(MTA) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    Result = CoGetApartmentType( &AptType,
                                 &AptQualifier );
    if (Result != LDK_TEST_S_OK ||
        AptType != APTTYPE_MTA ||
        AptQualifier != APTTYPEQUALIFIER_NONE) {
        fprintf(stderr,
                "[Failed] MTA CoGetApartmentType Result = 0x%08lx Apt = %d Qualifier = %d\n",
                Result,
                AptType,
                AptQualifier );
        RoUninitialize();
        return FALSE;
    }

    Result = RoInitialize( RO_INIT_MULTITHREADED );
    if (Result != LDK_TEST_S_FALSE) {
        fprintf(stderr,
                "[Failed] repeated RoInitialize(MTA) Result = 0x%08lx\n",
                Result );
        RoUninitialize();
        return FALSE;
    }

    Result = RoInitialize( RO_INIT_SINGLETHREADED );
    if (Result != LDK_TEST_RPC_E_CHANGED_MODE) {
        fprintf(stderr,
                "[Failed] conflicting RoInitialize(STA) Result = 0x%08lx\n",
                Result );
        RoUninitialize();
        RoUninitialize();
        return FALSE;
    }

    RoUninitialize();
    RoUninitialize();

    Result = RoInitialize( RO_INIT_SINGLETHREADED );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] RoInitialize(STA) Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    Result = CoGetApartmentType( &AptType,
                                 &AptQualifier );
    if (Result != LDK_TEST_S_OK ||
        AptType != APTTYPE_STA ||
        AptQualifier != APTTYPEQUALIFIER_NONE) {
        fprintf(stderr,
                "[Failed] STA CoGetApartmentType Result = 0x%08lx Apt = %d Qualifier = %d\n",
                Result,
                AptType,
                AptQualifier );
        RoUninitialize();
        return FALSE;
    }

    RoUninitialize();

    Cookie = NULL;
    Result = CoIncrementMTAUsage( &Cookie );
    if (Result != LDK_TEST_S_OK ||
        Cookie == NULL) {
        fprintf(stderr,
                "[Failed] direct CoIncrementMTAUsage Result = 0x%08lx Cookie = %p\n",
                Result,
                Cookie );
        return FALSE;
    }

    Result = CoGetApartmentType( &AptType,
                                 &AptQualifier );
    if (Result != LDK_TEST_S_OK ||
        AptType != APTTYPE_MTA ||
        AptQualifier != APTTYPEQUALIFIER_IMPLICIT_MTA) {
        fprintf(stderr,
                "[Failed] implicit MTA CoGetApartmentType Result = 0x%08lx Apt = %d Qualifier = %d\n",
                Result,
                AptType,
                AptQualifier );
        CoDecrementMTAUsage( Cookie );
        return FALSE;
    }

    Result = CoDecrementMTAUsage( Cookie );
    if (Result != LDK_TEST_S_OK) {
        fprintf(stderr,
                "[Failed] direct CoDecrementMTAUsage Result = 0x%08lx\n",
                Result );
        return FALSE;
    }

    return TRUE;
}
#endif

BOOLEAN
CombaseCompatibilityTest (
    VOID
    )
{
    HMODULE Combase;
    PCO_TASK_MEM_ALLOC TaskMemAlloc;
    PCO_TASK_MEM_FREE TaskMemFree;
    PCO_CREATE_INSTANCE CreateInstance;
    PCO_CREATE_FREE_THREADED_MARSHALER CreateFreeThreadedMarshaler;
    PCO_GET_APARTMENT_TYPE GetApartmentType;
    PCO_GET_CONTEXT_TOKEN GetContextToken;
    PCO_GET_OBJECT_CONTEXT GetObjectContext;
    PCO_INCREMENT_MTA_USAGE IncrementMtaUsage;
    PCO_DECREMENT_MTA_USAGE DecrementMtaUsage;
    PCO_MARSHAL_INTER_THREAD_INTERFACE_IN_STREAM MarshalInterThreadInterfaceInStream;
    PCO_GET_INTERFACE_AND_RELEASE_STREAM GetInterfaceAndReleaseStream;
    PRO_INITIALIZE RuntimeInitialize;
    PRO_UNINITIALIZE RuntimeUninitialize;
    PRO_GET_APARTMENT_IDENTIFIER GetApartmentIdentifier;
    PRO_REGISTER_FOR_APARTMENT_SHUTDOWN RegisterForApartmentShutdown;
    PRO_UNREGISTER_FOR_APARTMENT_SHUTDOWN UnregisterForApartmentShutdown;
    PWINDOWS_CREATE_STRING CreateString;
    PWINDOWS_CREATE_STRING_REFERENCE CreateStringReference;
    PWINDOWS_DELETE_STRING DeleteString;
    PWINDOWS_STRING_HAS_EMBEDDED_NULL StringHasEmbeddedNull;
    PSET_RESTRICTED_ERROR_INFO SetErrorInfo;
    PGET_RESTRICTED_ERROR_INFO GetErrorInfo;
    PRO_ORIGINATE_ERROR OriginateError;
    PRO_TRANSFORM_ERROR TransformError;
    PRO_CAPTURE_ERROR_CONTEXT CaptureErrorContext;
    PRO_FAIL_FAST_WITH_ERROR_CONTEXT FailFastWithErrorContext;
    PRO_ORIGINATE_LANGUAGE_EXCEPTION OriginateLanguageException;
    PRO_REPORT_UNHANDLED_ERROR ReportUnhandledError;

    PAGED_CODE();

    printf("COMBASE compatibility test\n");

    Combase = LoadLibraryW( L"combase.dll" );
    if (Combase == NULL) {
        fprintf(stderr,
                "[Failed] LoadLibraryW(combase.dll) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    TaskMemAlloc = (PCO_TASK_MEM_ALLOC)GetProcAddress( Combase,
                                                       "CoTaskMemAlloc" );
    TaskMemFree = (PCO_TASK_MEM_FREE)GetProcAddress( Combase,
                                                     "CoTaskMemFree" );
    CreateInstance = (PCO_CREATE_INSTANCE)GetProcAddress(
        Combase,
        "CoCreateInstance" );
    CreateFreeThreadedMarshaler = (PCO_CREATE_FREE_THREADED_MARSHALER)GetProcAddress(
        Combase,
        "CoCreateFreeThreadedMarshaler" );
    GetApartmentType = (PCO_GET_APARTMENT_TYPE)GetProcAddress(
        Combase,
        "CoGetApartmentType" );
    GetContextToken = (PCO_GET_CONTEXT_TOKEN)GetProcAddress(
        Combase,
        "CoGetContextToken" );
    GetObjectContext = (PCO_GET_OBJECT_CONTEXT)GetProcAddress(
        Combase,
        "CoGetObjectContext" );
    IncrementMtaUsage = (PCO_INCREMENT_MTA_USAGE)GetProcAddress(
        Combase,
        "CoIncrementMTAUsage" );
    DecrementMtaUsage = (PCO_DECREMENT_MTA_USAGE)GetProcAddress(
        Combase,
        "CoDecrementMTAUsage" );
    MarshalInterThreadInterfaceInStream = (PCO_MARSHAL_INTER_THREAD_INTERFACE_IN_STREAM)GetProcAddress(
        Combase,
        "CoMarshalInterThreadInterfaceInStream" );
    GetInterfaceAndReleaseStream = (PCO_GET_INTERFACE_AND_RELEASE_STREAM)GetProcAddress(
        Combase,
        "CoGetInterfaceAndReleaseStream" );
    RuntimeInitialize = (PRO_INITIALIZE)GetProcAddress( Combase,
                                                        "RoInitialize" );
    RuntimeUninitialize = (PRO_UNINITIALIZE)GetProcAddress( Combase,
                                                            "RoUninitialize" );
    GetApartmentIdentifier = (PRO_GET_APARTMENT_IDENTIFIER)GetProcAddress(
        Combase,
        "RoGetApartmentIdentifier" );
    RegisterForApartmentShutdown = (PRO_REGISTER_FOR_APARTMENT_SHUTDOWN)GetProcAddress(
        Combase,
        "RoRegisterForApartmentShutdown" );
    UnregisterForApartmentShutdown = (PRO_UNREGISTER_FOR_APARTMENT_SHUTDOWN)GetProcAddress(
        Combase,
        "RoUnregisterForApartmentShutdown" );
    CreateString = (PWINDOWS_CREATE_STRING)GetProcAddress(
        Combase,
        "WindowsCreateString" );
    CreateStringReference = (PWINDOWS_CREATE_STRING_REFERENCE)GetProcAddress(
        Combase,
        "WindowsCreateStringReference" );
    DeleteString = (PWINDOWS_DELETE_STRING)GetProcAddress(
        Combase,
        "WindowsDeleteString" );
    StringHasEmbeddedNull = (PWINDOWS_STRING_HAS_EMBEDDED_NULL)GetProcAddress(
        Combase,
        "WindowsStringHasEmbeddedNull" );
    SetErrorInfo = (PSET_RESTRICTED_ERROR_INFO)GetProcAddress(
        Combase,
        "SetRestrictedErrorInfo" );
    GetErrorInfo = (PGET_RESTRICTED_ERROR_INFO)GetProcAddress(
        Combase,
        "GetRestrictedErrorInfo" );
    OriginateError = (PRO_ORIGINATE_ERROR)GetProcAddress(
        Combase,
        "RoOriginateError" );
    TransformError = (PRO_TRANSFORM_ERROR)GetProcAddress(
        Combase,
        "RoTransformError" );
    CaptureErrorContext = (PRO_CAPTURE_ERROR_CONTEXT)GetProcAddress(
        Combase,
        "RoCaptureErrorContext" );
    FailFastWithErrorContext = (PRO_FAIL_FAST_WITH_ERROR_CONTEXT)GetProcAddress(
        Combase,
        "RoFailFastWithErrorContext" );
    OriginateLanguageException = (PRO_ORIGINATE_LANGUAGE_EXCEPTION)GetProcAddress(
        Combase,
        "RoOriginateLanguageException" );
    ReportUnhandledError = (PRO_REPORT_UNHANDLED_ERROR)GetProcAddress(
        Combase,
        "RoReportUnhandledError" );
    if (TaskMemAlloc == NULL ||
        TaskMemFree == NULL ||
        CreateInstance == NULL ||
        CreateFreeThreadedMarshaler == NULL ||
        GetApartmentType == NULL ||
        GetContextToken == NULL ||
        GetObjectContext == NULL ||
        IncrementMtaUsage == NULL ||
        DecrementMtaUsage == NULL ||
        MarshalInterThreadInterfaceInStream == NULL ||
        GetInterfaceAndReleaseStream == NULL ||
        RuntimeInitialize == NULL ||
        RuntimeUninitialize == NULL ||
        GetApartmentIdentifier == NULL ||
        RegisterForApartmentShutdown == NULL ||
        UnregisterForApartmentShutdown == NULL ||
        CreateString == NULL ||
        CreateStringReference == NULL ||
        DeleteString == NULL ||
        StringHasEmbeddedNull == NULL ||
        SetErrorInfo == NULL ||
        GetErrorInfo == NULL ||
        OriginateError == NULL ||
        TransformError == NULL ||
        CaptureErrorContext == NULL ||
        FailFastWithErrorContext == NULL ||
        OriginateLanguageException == NULL ||
        ReportUnhandledError == NULL) {
        fprintf(stderr,
                "[Failed] COMBASE exports Alloc = %p Free = %p CoCreate = %p FTM = %p Apt = %p CtxToken = %p ObjCtx = %p MtaInc = %p MtaDec = %p Marshal = %p Unmarshal = %p RoInit = %p RoUninit = %p AptId = %p AptReg = %p AptUnreg = %p HstrCreate = %p HstrRef = %p HstrDelete = %p HstrNull = %p SetErr = %p GetErr = %p RoOrig = %p RoTrans = %p RoCap = %p RoFailFast = %p RoLang = %p RoReport = %p ErrorCode = %lu\n",
                TaskMemAlloc,
                TaskMemFree,
                CreateInstance,
                CreateFreeThreadedMarshaler,
                GetApartmentType,
                GetContextToken,
                GetObjectContext,
                IncrementMtaUsage,
                DecrementMtaUsage,
                MarshalInterThreadInterfaceInStream,
                GetInterfaceAndReleaseStream,
                RuntimeInitialize,
                RuntimeUninitialize,
                GetApartmentIdentifier,
                RegisterForApartmentShutdown,
                UnregisterForApartmentShutdown,
                CreateString,
                CreateStringReference,
                DeleteString,
                StringHasEmbeddedNull,
                SetErrorInfo,
                GetErrorInfo,
                OriginateError,
                TransformError,
                CaptureErrorContext,
                FailFastWithErrorContext,
                OriginateLanguageException,
                ReportUnhandledError,
                GetLastError() );
        FreeLibrary( Combase );
        return FALSE;
    }

    if (!LdkpTestCombaseTaskMemory( TaskMemAlloc,
                                    TaskMemFree ) ||
        !LdkpTestCombaseLimitedComExports( CreateInstance,
                                           CreateFreeThreadedMarshaler,
                                           GetContextToken,
                                           GetObjectContext,
                                           MarshalInterThreadInterfaceInStream,
                                           GetInterfaceAndReleaseStream ) ||
        !LdkpTestCombaseApartmentIdentifierExports( GetApartmentIdentifier ) ||
        !LdkpTestCombaseApartmentExports( GetApartmentType,
                                          IncrementMtaUsage,
                                          DecrementMtaUsage,
                                          RuntimeInitialize,
                                          RuntimeUninitialize ) ||
        !LdkpTestCombaseHstringExports( CreateString,
                                        CreateStringReference,
                                        DeleteString,
                                        StringHasEmbeddedNull ) ||
        !LdkpTestCombaseErrorContextExports( SetErrorInfo,
                                             GetErrorInfo,
                                             OriginateError,
                                             TransformError,
                                             CaptureErrorContext,
                                             OriginateLanguageException )) {
        FreeLibrary( Combase );
        return FALSE;
    }

#if _KERNEL_MODE
    {
        UCHAR *Buffer;

        Buffer = (UCHAR *)CoTaskMemAlloc( 32 );
        if (Buffer == NULL) {
            fprintf(stderr,
                    "[Failed] direct CoTaskMemAlloc(32)\n" );
            FreeLibrary( Combase );
            return FALSE;
        }

        Buffer[0] = 0xa5;
        Buffer[31] = 0x5a;
        CoTaskMemFree( Buffer );
        CoTaskMemFree( NULL );
    }

    if (!LdkpTestCombaseKernelApartmentState()) {
        FreeLibrary( Combase );
        return FALSE;
    }

    if (!LdkpTestCombaseKernelLimitedComState()) {
        FreeLibrary( Combase );
        return FALSE;
    }
#endif

    if (!FreeLibrary( Combase )) {
        fprintf(stderr,
                "[Failed] FreeLibrary(combase.dll) ErrorCode = %lu\n",
                GetLastError() );
        return FALSE;
    }

    printf("[Success] COMBASE compatibility test\n\n");
    return TRUE;
}
