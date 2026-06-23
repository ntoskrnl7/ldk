#include "winbase.h"
#include <Ldk/winreg.h>
#include "../ntdll/ntdll.h"



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RegOpenKeyExW)
#pragma alloc_text(PAGE, RegQueryValueExW)
#pragma alloc_text(PAGE, RegCloseKey)
#endif



static
BOOLEAN
LdkpIsPredefinedRegistryKey (
    _In_ HKEY hKey
    )
{
    return hKey == HKEY_CLASSES_ROOT ||
           hKey == HKEY_CURRENT_USER ||
           hKey == HKEY_LOCAL_MACHINE ||
           hKey == HKEY_USERS ||
           hKey == HKEY_PERFORMANCE_DATA ||
           hKey == HKEY_CURRENT_CONFIG;
}



static
LPCWSTR
LdkpRegistryRootPath (
    _In_ HKEY hKey
    )
{
    if (hKey == HKEY_CLASSES_ROOT) {
        return L"\\Registry\\Machine\\Software\\Classes";
    }
    if (hKey == HKEY_LOCAL_MACHINE) {
        return L"\\Registry\\Machine";
    }
    if (hKey == HKEY_USERS) {
        return L"\\Registry\\User";
    }
    if (hKey == HKEY_CURRENT_CONFIG) {
        return L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current";
    }

    return NULL;
}



static
LSTATUS
LdkpBuildAbsoluteRegistryPath (
    _In_ HKEY hKey,
    _In_opt_ LPCWSTR lpSubKey,
    _Out_writes_(cchBuffer) LPWSTR Buffer,
    _In_ USHORT cchBuffer,
    _Out_ PUNICODE_STRING Path
    )
{
    LPCWSTR RootPath;
    SIZE_T RootLength;
    SIZE_T SubKeyLength;
    SIZE_T TotalLength;

    RootPath = LdkpRegistryRootPath( hKey );
    if (RootPath == NULL) {
        return ERROR_INVALID_HANDLE;
    }

    RootLength = wcslen( RootPath );
    SubKeyLength = (lpSubKey != NULL) ? wcslen( lpSubKey ) : 0;
    TotalLength = RootLength + ((SubKeyLength != 0) ? 1 + SubKeyLength : 0);
    if (TotalLength >= cchBuffer) {
        return ERROR_FILENAME_EXCED_RANGE;
    }

    RtlCopyMemory( Buffer,
                   RootPath,
                   RootLength * sizeof(WCHAR) );
    if (SubKeyLength != 0) {
        Buffer[RootLength] = L'\\';
        RtlCopyMemory( Buffer + RootLength + 1,
                       lpSubKey,
                       SubKeyLength * sizeof(WCHAR) );
    }
    Buffer[TotalLength] = UNICODE_NULL;

    Path->Buffer = Buffer;
    Path->Length = (USHORT)(TotalLength * sizeof(WCHAR));
    Path->MaximumLength = (USHORT)((TotalLength + 1) * sizeof(WCHAR));
    return ERROR_SUCCESS;
}



WINADVAPI
LSTATUS
APIENTRY
RegOpenKeyExW (
    _In_ HKEY hKey,
    _In_opt_ LPCWSTR lpSubKey,
    _Reserved_ DWORD ulOptions,
    _In_ REGSAM samDesired,
    _Out_ PHKEY phkResult
    )
{
    NTSTATUS Status;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    WCHAR KeyNameBuffer[512];
    LSTATUS Error;

    PAGED_CODE();

    if (ulOptions != 0) {
        return ERROR_INVALID_PARAMETER;
    }
    if (phkResult == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    *phkResult = NULL;

    if (LdkpIsPredefinedRegistryKey( hKey )) {
        Error = LdkpBuildAbsoluteRegistryPath( hKey,
                                               lpSubKey,
                                               KeyNameBuffer,
                                               RTL_NUMBER_OF(KeyNameBuffer),
                                               &KeyName );
        if (Error != ERROR_SUCCESS) {
            return Error;
        }

        InitializeObjectAttributes( &ObjectAttributes,
                                    &KeyName,
                                    OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                    NULL,
                                    NULL );
    } else {
        RtlInitUnicodeString( &KeyName,
                              (lpSubKey != NULL) ? lpSubKey : L"" );
        InitializeObjectAttributes( &ObjectAttributes,
                                    &KeyName,
                                    OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                    (HANDLE)hKey,
                                    NULL );
    }

    Status = ZwOpenKey( (PHANDLE)phkResult,
                        samDesired,
                        &ObjectAttributes );
    return RtlNtStatusToDosError( Status );
}



WINADVAPI
LSTATUS
APIENTRY
RegQueryValueExW (
    _In_ HKEY hKey,
    _In_opt_ LPCWSTR lpValueName,
    _Reserved_ LPDWORD lpReserved,
    _Out_opt_ LPDWORD lpType,
    _Out_writes_bytes_to_opt_(*lpcbData, *lpcbData) LPBYTE lpData,
    _Inout_opt_ LPDWORD lpcbData
    )
{
    NTSTATUS Status;
    UNICODE_STRING ValueName;
    ULONG ResultLength;
    ULONG BufferLength;
    PKEY_VALUE_PARTIAL_INFORMATION ValueInformation;
    LSTATUS Error;

    PAGED_CODE();

    if (lpReserved != NULL) {
        return ERROR_INVALID_PARAMETER;
    }
    if (LdkpIsPredefinedRegistryKey( hKey )) {
        return ERROR_INVALID_HANDLE;
    }
    if (lpData != NULL && lpcbData == NULL) {
        return ERROR_INVALID_PARAMETER;
    }

    RtlInitUnicodeString( &ValueName,
                          (lpValueName != NULL) ? lpValueName : L"" );

    ResultLength = 0;
    Status = ZwQueryValueKey( (HANDLE)hKey,
                              &ValueName,
                              KeyValuePartialInformation,
                              NULL,
                              0,
                              &ResultLength );
    if (Status != STATUS_BUFFER_TOO_SMALL &&
        Status != STATUS_BUFFER_OVERFLOW &&
        !NT_SUCCESS(Status)) {
        return RtlNtStatusToDosError( Status );
    }

    BufferLength = ResultLength;
    ValueInformation = RtlAllocateHeap( RtlProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        BufferLength );
    if (ValueInformation == NULL) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    Status = ZwQueryValueKey( (HANDLE)hKey,
                              &ValueName,
                              KeyValuePartialInformation,
                              ValueInformation,
                              BufferLength,
                              &ResultLength );
    if (!NT_SUCCESS(Status)) {
        Error = RtlNtStatusToDosError( Status );
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     ValueInformation );
        return Error;
    }

    if (lpType != NULL) {
        *lpType = ValueInformation->Type;
    }

    if (lpcbData != NULL) {
        if (lpData != NULL && *lpcbData < ValueInformation->DataLength) {
            *lpcbData = ValueInformation->DataLength;
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         ValueInformation );
            return ERROR_MORE_DATA;
        }

        *lpcbData = ValueInformation->DataLength;
    }

    if (lpData != NULL && ValueInformation->DataLength != 0) {
        RtlCopyMemory( lpData,
                       ValueInformation->Data,
                       ValueInformation->DataLength );
    }

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 ValueInformation );
    return ERROR_SUCCESS;
}



WINADVAPI
LSTATUS
APIENTRY
RegCloseKey (
    _In_ HKEY hKey
    )
{
    NTSTATUS Status;

    PAGED_CODE();

    if (LdkpIsPredefinedRegistryKey( hKey )) {
        return ERROR_SUCCESS;
    }

    Status = ZwClose( (HANDLE)hKey );
    return RtlNtStatusToDosError( Status );
}
