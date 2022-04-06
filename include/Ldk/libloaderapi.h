#pragma once

EXTERN_C_START

WINBASEAPI
_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
GetModuleFileNameA(
    _In_opt_ HMODULE hModule,
    _Out_writes_to_(nSize, ((return < nSize) ? (return + 1) : nSize)) LPSTR lpFilename,
    _In_ DWORD nSize
    );

WINBASEAPI
_Success_(return != 0)
_Ret_range_(1, nSize)
DWORD
WINAPI
GetModuleFileNameW(
    _In_opt_ HMODULE hModule,
    _Out_writes_to_(nSize, ((return < nSize) ? (return + 1) : nSize)) LPWSTR lpFilename,
    _In_ DWORD nSize
    );


WINBASEAPI
_When_(lpModuleName == NULL, _Ret_notnull_)
_When_(lpModuleName != NULL, _Ret_maybenull_)
HMODULE
WINAPI
GetModuleHandleA(
    _In_opt_ LPCSTR lpModuleName
    );

WINBASEAPI
_When_(lpModuleName == NULL, _Ret_notnull_)
_When_(lpModuleName != NULL, _Ret_maybenull_)
HMODULE
WINAPI
GetModuleHandleW(
    _In_opt_ LPCWSTR lpModuleName
    );


#define GET_MODULE_HANDLE_EX_FLAG_PIN                 (0x00000001)
#define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT  (0x00000002)
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS        (0x00000004)

WINBASEAPI
BOOL
WINAPI
GetModuleHandleExA(
    _In_ DWORD dwFlags,
    _In_opt_ LPCSTR lpModuleName,
    _Out_ HMODULE * phModule
    );

WINBASEAPI
BOOL
WINAPI
GetModuleHandleExW(
    _In_ DWORD dwFlags,
    _In_opt_ LPCWSTR lpModuleName,
    _Out_ HMODULE * phModule
    );



WINBASEAPI
FARPROC
WINAPI
GetProcAddress(
    _In_ HMODULE hModule,
    _In_ LPCSTR lpProcName
    );

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryExA(
    _In_ LPCSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    );

WINBASEAPI
_Ret_maybenull_
HMODULE
WINAPI
LoadLibraryExW(
    _In_ LPCWSTR lpLibFileName,
    _Reserved_ HANDLE hFile,
    _In_ DWORD dwFlags
    );

#define DONT_RESOLVE_DLL_REFERENCES         0x00000001
#define LOAD_LIBRARY_AS_DATAFILE            0x00000002
// reserved for internal LOAD_PACKAGED_LIBRARY: 0x00000004
#define LOAD_WITH_ALTERED_SEARCH_PATH       0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL        0x00000010
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE      0x00000020
#define LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE  0x00000040
#define LOAD_LIBRARY_REQUIRE_SIGNED_TARGET  0x00000080
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR    0x00000100
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0x00000200
#define LOAD_LIBRARY_SEARCH_USER_DIRS       0x00000400
#define LOAD_LIBRARY_SEARCH_SYSTEM32        0x00000800
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS    0x00001000

#if (NTDDI_VERSION >= NTDDI_WIN10_RS1)

#define LOAD_LIBRARY_SAFE_CURRENT_DIRS      0x00002000

#define LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER   0x00004000

#else

//
// For anything building for downlevel, set the flag to be the same as LOAD_LIBRARY_SEARCH_SYSTEM32
// such that they're treated the same when running on older version of OS.
//

#define LOAD_LIBRARY_SEARCH_SYSTEM32_NO_FORWARDER   LOAD_LIBRARY_SEARCH_SYSTEM32

#endif // (_APISET_LIBLOADER_VER >= 0x0202)

#if (NTDDI_VERSION >= NTDDI_WIN10_RS2)

#define LOAD_LIBRARY_OS_INTEGRITY_CONTINUITY   0x00008000

#endif

WINBASEAPI
BOOL
WINAPI
FreeLibrary(
    _In_ HMODULE hLibModule
    );

EXTERN_C_END