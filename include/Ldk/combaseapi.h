#pragma once

#ifndef _COMBASEAPI_H_
#define _COMBASEAPI_H_

#include <ntifs.h>
#define _DEVIOCTL_

#include <wdm.h>
#include <guiddef.h>

#define WINBASEAPI
#include <minwindef.h>
#include <winerror.h>
#include "winnt.h"

EXTERN_C_START

#ifndef WINOLEAPI
#define WINOLEAPI WINBASEAPI HRESULT WINAPI
#endif

#ifndef WINOLEAPI_
#define WINOLEAPI_(type) WINBASEAPI type WINAPI
#endif

typedef enum _APTTYPEQUALIFIER {
    APTTYPEQUALIFIER_NONE = 0,
    APTTYPEQUALIFIER_IMPLICIT_MTA = 1,
    APTTYPEQUALIFIER_NA_ON_MTA = 2,
    APTTYPEQUALIFIER_NA_ON_STA = 3,
    APTTYPEQUALIFIER_NA_ON_IMPLICIT_MTA = 4,
    APTTYPEQUALIFIER_NA_ON_MAINSTA = 5,
    APTTYPEQUALIFIER_APPLICATION_STA = 6,
    APTTYPEQUALIFIER_RESERVED_1 = 7
} APTTYPEQUALIFIER;

typedef enum _APTTYPE {
    APTTYPE_CURRENT = -1,
    APTTYPE_STA = 0,
    APTTYPE_MTA = 1,
    APTTYPE_NA = 2,
    APTTYPE_MAINSTA = 3
} APTTYPE;

typedef enum _RO_INIT_TYPE {
    RO_INIT_SINGLETHREADED = 0,
    RO_INIT_MULTITHREADED = 1
} RO_INIT_TYPE;

DECLARE_HANDLE(CO_MTA_USAGE_COOKIE);

typedef struct HSTRING__ {
    int unused;
} HSTRING__;

typedef HSTRING__ *HSTRING;

typedef struct IUnknown IUnknown;
typedef struct IRestrictedErrorInfo IRestrictedErrorInfo;
typedef struct IApartmentShutdown IApartmentShutdown;
typedef struct IStream IStream;

typedef IUnknown *LPUNKNOWN;
typedef IStream *LPSTREAM;

DECLARE_HANDLE(APARTMENT_SHUTDOWN_REGISTRATION_COOKIE);

typedef struct HSTRING_HEADER {
    union {
        PVOID Reserved1;
#if defined(_WIN64)
        CHAR Reserved2[24];
#else
        CHAR Reserved2[20];
#endif
    } Reserved;
} HSTRING_HEADER;

WINOLEAPI_(LPVOID)
CoTaskMemAlloc(
    _In_ SIZE_T cb
    );

WINOLEAPI_(VOID)
CoTaskMemFree(
    _Frees_ptr_opt_ LPVOID pv
    );

WINOLEAPI
CoCreateInstance(
    _In_ REFCLSID rclsid,
    _In_opt_ LPUNKNOWN pUnkOuter,
    _In_ DWORD dwClsContext,
    _In_ REFIID riid,
    _COM_Outptr_ LPVOID *ppv
    );

WINOLEAPI
CoCreateFreeThreadedMarshaler(
    _In_opt_ LPUNKNOWN punkOuter,
    _Outptr_ LPUNKNOWN *ppunkMarshal
    );

WINOLEAPI
CoGetApartmentType(
    _Out_ APTTYPE *pAptType,
    _Out_ APTTYPEQUALIFIER *pAptQualifier
    );

WINOLEAPI
CoGetContextToken(
    _Out_ ULONG_PTR *pToken
    );

WINOLEAPI
CoGetObjectContext(
    _In_ REFIID riid,
    _COM_Outptr_ LPVOID *ppv
    );

WINOLEAPI
CoIncrementMTAUsage(
    _Out_ CO_MTA_USAGE_COOKIE *pCookie
    );

WINOLEAPI
CoDecrementMTAUsage(
    _In_ CO_MTA_USAGE_COOKIE Cookie
    );

WINOLEAPI
CoMarshalInterThreadInterfaceInStream(
    _In_ REFIID riid,
    _In_ LPUNKNOWN pUnk,
    _Outptr_ LPSTREAM *ppStm
    );

WINOLEAPI
CoGetInterfaceAndReleaseStream(
    _In_ LPSTREAM pStm,
    _In_ REFIID iid,
    _COM_Outptr_ LPVOID *ppv
    );

WINOLEAPI
RoInitialize(
    _In_ RO_INIT_TYPE initType
    );

WINOLEAPI_(VOID)
RoUninitialize(
    VOID
    );

WINOLEAPI
RoRegisterForApartmentShutdown(
    _In_ IApartmentShutdown *callbackObject,
    _Out_ UINT64 *apartmentIdentifier,
    _Out_ APARTMENT_SHUTDOWN_REGISTRATION_COOKIE *regCookie
    );

WINOLEAPI
RoUnregisterForApartmentShutdown(
    _In_ APARTMENT_SHUTDOWN_REGISTRATION_COOKIE regCookie
    );

WINOLEAPI
RoGetApartmentIdentifier(
    _Out_ UINT64 *apartmentIdentifier
    );

WINOLEAPI
WindowsCreateString(
    _In_reads_opt_(length) PCNZWCH sourceString,
    _In_ UINT32 length,
    _Outptr_result_maybenull_ HSTRING *string
    );

WINOLEAPI
WindowsCreateStringReference(
    _In_reads_opt_(length + 1) PCWSTR sourceString,
    _In_ UINT32 length,
    _Out_ HSTRING_HEADER *hstringHeader,
    _Outptr_result_maybenull_ HSTRING *string
    );

WINOLEAPI
WindowsDeleteString(
    _In_opt_ HSTRING string
    );

WINOLEAPI
WindowsStringHasEmbeddedNull(
    _In_opt_ HSTRING string,
    _Out_ BOOL *hasEmbedNull
    );

WINOLEAPI
SetRestrictedErrorInfo(
    _In_opt_ IRestrictedErrorInfo *pRestrictedErrorInfo
    );

WINOLEAPI
GetRestrictedErrorInfo(
    _Outptr_result_maybenull_ IRestrictedErrorInfo **ppRestrictedErrorInfo
    );

WINOLEAPI_(BOOL)
RoOriginateErrorW(
    _In_ HRESULT error,
    _In_ UINT cchMax,
    _In_reads_or_z_opt_(cchMax) PCWSTR message
    );

WINOLEAPI_(BOOL)
RoOriginateError(
    _In_ HRESULT error,
    _In_opt_ HSTRING message
    );

WINOLEAPI_(BOOL)
RoTransformErrorW(
    _In_ HRESULT oldError,
    _In_ HRESULT newError,
    _In_ UINT cchMax,
    _In_reads_or_z_opt_(cchMax) PCWSTR message
    );

WINOLEAPI_(BOOL)
RoTransformError(
    _In_ HRESULT oldError,
    _In_ HRESULT newError,
    _In_opt_ HSTRING message
    );

WINOLEAPI
RoCaptureErrorContext(
    _In_ HRESULT hr
    );

WINOLEAPI_(VOID)
RoFailFastWithErrorContext(
    _In_ HRESULT hrError
    );

WINOLEAPI_(BOOL)
RoOriginateLanguageException(
    _In_ HRESULT error,
    _In_opt_ HSTRING message,
    _In_opt_ IUnknown *languageException
    );

WINOLEAPI
RoReportUnhandledError(
    _In_ IRestrictedErrorInfo *pRestrictedErrorInfo
    );

EXTERN_C_END

#endif // _COMBASEAPI_H_
