#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
RegistryApiTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RegistryApiTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <winreg.h>
#include <stdio.h>
#define PAGED_CODE()
#endif

BOOLEAN
RegistryApiTest (
    VOID
    )
{
    HKEY Key = NULL;
    DWORD Type = 0;
    DWORD RequiredBytes = 0;
    WCHAR SystemRoot[128];
    LSTATUS Status;

    PAGED_CODE();

    printf("Registry API Test\n");

    Status = RegOpenKeyExW( HKEY_LOCAL_MACHINE,
                            L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                            0,
                            KEY_QUERY_VALUE,
                            &Key );
    if (Status != ERROR_SUCCESS ||
        Key == NULL) {
        fprintf(stderr,
                "[Failed] RegOpenKeyExW(CurrentVersion) Status = %ld Key = %p\n",
                Status,
                Key );
        return FALSE;
    }

    Status = RegQueryValueExW( Key,
                               L"SystemRoot",
                               NULL,
                               &Type,
                               NULL,
                               &RequiredBytes );
    if (Status != ERROR_SUCCESS ||
        RequiredBytes == 0 ||
        (Type != REG_SZ && Type != REG_EXPAND_SZ)) {
        fprintf(stderr,
                "[Failed] RegQueryValueExW(SystemRoot size) Status = %ld Type = %lu RequiredBytes = %lu\n",
                Status,
                Type,
                RequiredBytes );
        RegCloseKey( Key );
        return FALSE;
    }

    if (RequiredBytes > sizeof(SystemRoot)) {
        fprintf(stderr,
                "[Failed] RegQueryValueExW(SystemRoot) value too large RequiredBytes = %lu\n",
                RequiredBytes );
        RegCloseKey( Key );
        return FALSE;
    }

    Status = RegQueryValueExW( Key,
                               L"SystemRoot",
                               NULL,
                               &Type,
                               (LPBYTE)SystemRoot,
                               &RequiredBytes );
    if (Status != ERROR_SUCCESS ||
        RequiredBytes < sizeof(WCHAR) ||
        SystemRoot[0] == UNICODE_NULL) {
        fprintf(stderr,
                "[Failed] RegQueryValueExW(SystemRoot data) Status = %ld Type = %lu RequiredBytes = %lu Value = %ws\n",
                Status,
                Type,
                RequiredBytes,
                SystemRoot );
        RegCloseKey( Key );
        return FALSE;
    }

    Status = RegCloseKey( Key );
    if (Status != ERROR_SUCCESS) {
        fprintf(stderr,
                "[Failed] RegCloseKey Status = %ld\n",
                Status );
        return FALSE;
    }

    printf("[Success] Registry API Test\n\n");
    return TRUE;
}
