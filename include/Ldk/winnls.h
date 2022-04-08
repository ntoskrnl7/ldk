#ifndef _WINNLS_
#define _WINNLS_

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