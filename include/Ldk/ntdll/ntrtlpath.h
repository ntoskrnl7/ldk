#pragma once

#ifndef _LDK_NTRTLPATH_
#define _LDK_NTRTLPATH_

EXTERN_C_START

//
// These are OUT Disposition values.
//
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_AMBIGUOUS   (0x00000001)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_UNC         (0x00000002)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_DRIVE       (0x00000003)
#define RTL_NT_PATH_NAME_TO_DOS_PATH_NAME_ALREADY_DOS (0x00000004)

//
// If either Path or Element end in a path seperator, then so does the result.
// The path seperator to put on the end is chosen from the existing ones.
//
#define RTL_APPEND_PATH_ELEMENT_ONLY_BACKSLASH_IS_SEPERATOR (0x00000001)
#define RTL_APPEND_PATH_ELEMENT_BUGFIX_CHECK_FIRST_THREE_CHARS_FOR_SLASH_TAKE_FOUND_SLASH_INSTEAD_OF_FIRST_CHAR (0x00000002)



__declspec(selectany) extern const UNICODE_STRING RtlNtPathSeperatorString = RTL_CONSTANT_STRING(L"\\");
__declspec(selectany) extern const UNICODE_STRING RtlDosPathSeperatorsString = RTL_CONSTANT_STRING(L"\\/");
__declspec(selectany) extern const UNICODE_STRING RtlAlternateDosPathSeperatorString = RTL_CONSTANT_STRING(L"/");

#define RtlCanonicalDosPathSeperatorString RtlNtPathSeperatorString
#define RtlPathSeperatorString             RtlNtPathSeperatorString

//++
//
// WCHAR
// RTL_IS_DOS_PATH_SEPARATOR(
//     IN WCHAR Ch
//     );
//
// Routine Description:
//
// Arguments:
//
//     Ch - 
//
// Return Value:
//
//     TRUE  if ch is \\ or /.
//     FALSE otherwise.
//--
#define RTL_IS_DOS_PATH_SEPARATOR(ch_) ((ch_) == '\\' || ((ch_) == '/'))
#define RTL_IS_DOS_PATH_SEPERATOR(ch_) RTL_IS_DOS_PATH_SEPARATOR(ch_)



//
// compatibility
//
#define RTL_IS_PATH_SEPERATOR RTL_IS_DOS_PATH_SEPERATOR
#define RTL_IS_PATH_SEPARATOR RTL_IS_DOS_PATH_SEPERATOR
#define RtlIsPathSeparator    RtlIsDosPathSeparator
#define RtlIsPathSeperator    RtlIsDosPathSeparator

//++
//
// WCHAR
// RTL_IS_NT_PATH_SEPARATOR(
//     IN WCHAR Ch
//     );
// 
// Routine Description:
//
// Arguments:
//
//     Ch - 
//
// Return Value:
//
//     TRUE  if ch is \\.
//     FALSE otherwise.
//--
#define RTL_IS_NT_PATH_SEPARATOR(ch_) ((ch_) == '\\')
#define RTL_IS_NT_PATH_SEPERATOR(ch_) RTL_IS_NT_PATH_SEPARATOR(ch_)

EXTERN_C_END

#endif // _LDK_NTRTLPATH_