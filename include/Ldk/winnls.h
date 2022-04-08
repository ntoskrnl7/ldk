#ifndef _WINNLS_
#define _WINNLS_



//
//  Code Page Default Values.
//  Please Use Unicode, either UTF-16 (as in WCHAR) or UTF-8 (code page CP_ACP)
//
#define CP_ACP                    0           // default to ANSI code page
#define CP_OEMCP                  1           // default to OEM  code page
#define CP_MACCP                  2           // default to MAC  code page
#define CP_THREAD_ACP             3           // current thread's ANSI code page
#define CP_SYMBOL                 42          // SYMBOL translations

#define CP_UTF7                   65000       // UTF-7 translation
#define CP_UTF8                   65001       // UTF-8 translation



//
//  NLS version structure.
//

#if (WINVER >= _WIN32_WINNT_WIN8)
//
// New structures are the same
//

// The combination of dwNLSVersion, and guidCustomVersion
// identify specific sort behavior, persist those to ensure identical
// behavior in the future.
typedef struct _nlsversioninfo{
    DWORD dwNLSVersionInfoSize;     // sizeof(NLSVERSIONINFO) == 32 bytes
    DWORD dwNLSVersion;
    DWORD dwDefinedVersion;         // Deprecated, use dwNLSVersion instead
    DWORD dwEffectiveId;            // Deprecated, use guidCustomVerison instead
    GUID  guidCustomVersion;        // Explicit sort version
} NLSVERSIONINFO, *LPNLSVERSIONINFO;
#else
// 
// Windows 7 and below had different sizes
//

// This is to be deprecated, please use the NLSVERSIONINFOEX
// structure below in the future.  The difference is that
// guidCustomversion is required to uniquely identify a sort
typedef struct _nlsversioninfo{		// Use NLSVERSIONINFOEX instead
    DWORD dwNLSVersionInfoSize;     // 12 bytes
    DWORD dwNLSVersion;
    DWORD dwDefinedVersion;         // Deprecated, use dwNLSVersion instead
} NLSVERSIONINFO, *LPNLSVERSIONINFO;
#endif



//
//  Locale type constant.
//
typedef DWORD LCTYPE;

#endif // _WINNLS_