#include "ntdll.h"
#include "../ldk.h"
#include <limits.h>
#include <Ldk/ntdll/nturtl.h>
#include <Ldk/ntdll/nrtlstringandbuffer.h>
#include <Ldk/ntdll/ntrtlpath.h>
#include <Ldk/ntdll/nturtl.h>



NTSTATUS
RtlpCheckDeviceName (
    _In_ PCUNICODE_STRING DevName,
    _In_ ULONG DeviceNameOffset,
    _Out_ PBOOLEAN NameInvalid
    );

VOID
RtlpResetDriveEnvironment (
    _In_ WCHAR DriveLetter
    );

VOID
RtlpValidateCurrentDirectory (
    _In_ PCURDIR CurDir
    );

VOID
RtlpCheckRelativeDrive (
    _In_ WCHAR NewDrive
    );

ULONG
RtlGetFullPathName_Ustr ( 
    _In_ PCUNICODE_STRING FileName,
    _In_ ULONG nBufferLength,
    _Out_ PWSTR lpBuffer,
    _Out_opt_ PWSTR *lpFilePart,
    _Out_ PBOOLEAN NameInvalid,
    _Out_ RTL_PATH_TYPE *InputPathType
    );

BOOLEAN
RtlDoesFileExists_UstrEx (
    _In_ PCUNICODE_STRING FileNameString,
    _In_ BOOLEAN TreatDeniedOrSharingAsHit
    );



#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, RtlpCheckDeviceName)
#pragma alloc_text(PAGE, RtlpResetDriveEnvironment)
#pragma alloc_text(PAGE, RtlpValidateCurrentDirectory)
#pragma alloc_text(PAGE, RtlpCheckRelativeDrive)
#pragma alloc_text(PAGE, RtlGetFullPathName_Ustr)
#pragma alloc_text(PAGE, RtlGetFullPathName_U)
#pragma alloc_text(PAGE, RtlGetCurrentDirectory_U)
#pragma alloc_text(PAGE, RtlSetCurrentDirectory_U)
#pragma alloc_text(PAGE, RtlDosPathNameToNtPathName_U)
#pragma alloc_text(PAGE, RtlDosPathNameToRelativeNtPathName_U)
#pragma alloc_text(PAGE, RtlDoesFileExists_UstrEx)
#pragma alloc_text(PAGE, RtlDoesFileExists_U)
#endif



#define IS_PATH_SEPARATOR_U(ch) (((ch) == L'\\') || ((ch) == L'/'))
#define IS_END_OF_COMPONENT_U(ch) (IS_PATH_SEPARATOR_U(ch) || (ch) == UNICODE_NULL)
#define IS_DOT_U(s) ( (s)[0] == L'.' && IS_END_OF_COMPONENT_U( (s)[1] ))
#define IS_DOT_DOT_U(s) ( (s)[0] == L'.' && IS_DOT_U( (s) + 1))
#define IS_DRIVE_LETTER(ch) (((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'))
#define IS_END_OF_COMPONENT_USTR(s, len) ((len) == 0 || IS_PATH_SEPARATOR_U((s)[0]))
#define IS_DOT_USTR(s, len) ((len) >= sizeof(WCHAR) && (s)[0] == L'.' && IS_END_OF_COMPONENT_USTR( (s) + 1, (len) - sizeof(WCHAR) ))
#define IS_DOT_DOT_USTR(s, len) ((len) >= sizeof(WCHAR) && (s)[0] == L'.' && IS_DOT_USTR( (s) + 1, (len) - sizeof(WCHAR) ))

const UNICODE_STRING RtlpDosLPTDevice = RTL_CONSTANT_STRING(L"LPT");
const UNICODE_STRING RtlpDosCOMDevice = RTL_CONSTANT_STRING(L"COM");
const UNICODE_STRING RtlpDosPRNDevice = RTL_CONSTANT_STRING(L"PRN");
const UNICODE_STRING RtlpDosAUXDevice = RTL_CONSTANT_STRING(L"AUX");
const UNICODE_STRING RtlpDosNULDevice = RTL_CONSTANT_STRING(L"NUL");
const UNICODE_STRING RtlpDosCONDevice = RTL_CONSTANT_STRING(L"CON");

const UNICODE_STRING RtlpDosSlashCONDevice   = RTL_CONSTANT_STRING(L"\\\\.\\CON");
const UNICODE_STRING RtlpSlashSlashDot       = RTL_CONSTANT_STRING(L"\\\\.\\");
const UNICODE_STRING RtlpDosDevicesPrefix    = RTL_CONSTANT_STRING(L"\\??\\");
const UNICODE_STRING RtlpDosDevicesUncPrefix = RTL_CONSTANT_STRING(L"\\??\\UNC\\");

#define RtlpLongestPrefix   RtlpDosDevicesUncPrefix.Length



typedef struct _RTLP_CURDIR_REF {
    LONG RefCount;
    HANDLE DirectoryHandle;
} RTLP_CURDIR_REF;

PRTLP_CURDIR_REF RtlpCurDirRef = NULL;

FORCEINLINE
VOID
RtlpInitializeCurDirRef (
    _Out_ PRTLP_CURDIR_REF CurDirRef,
    _In_ HANDLE DirectoryHandle
    )
{
    ASSERT(CurDirRef);
    CurDirRef->RefCount = 1;
    CurDirRef->DirectoryHandle = DirectoryHandle;
}

FORCEINLINE
VOID
RtlpReferenceCurDirRef (
    _Inout_ PRTLP_CURDIR_REF CurDirRef
    )
{
    ASSERT(CurDirRef);
    if (CurDirRef) {
        InterlockedIncrement( &CurDirRef->RefCount );
    }
}

FORCEINLINE
VOID
RtlpDereferenceCurDirRef (
    _Inout_ PRTLP_CURDIR_REF CurDirRef
    )
{
    ASSERT(CurDirRef);
    if (CurDirRef && !InterlockedDecrement( &CurDirRef->RefCount )) {
        RTL_VERIFY(NT_SUCCESS(ZwClose( CurDirRef->DirectoryHandle )));
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     CurDirRef );
    }
}

NTSTATUS
RtlpCheckDeviceName (
    _In_ PCUNICODE_STRING DevName,
    _In_ ULONG DeviceNameOffset,
    _Out_ PBOOLEAN NameInvalid
    )
{
    PAGED_CODE();

    PWSTR DevPath = RtlAllocateHeap( RtlProcessHeap(),
                                     0,
                                     DevName->Length );
    if (! DevPath) {
        *NameInvalid = FALSE;
        return STATUS_NO_MEMORY;
    }
    *NameInvalid = TRUE;
    try {
        RtlCopyMemory( DevPath,
                       DevName->Buffer,
                       DevName->Length );
        DevPath[DeviceNameOffset >> 1] = L'.';
        DevPath[(DeviceNameOffset >> 1) + 1] = UNICODE_NULL;
        *NameInvalid = !RtlDoesFileExists_U( DevPath );
    } finally {
        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     DevPath );
    }
    return STATUS_SUCCESS;
}

ULONG
RtlpComputeBackupIndex (
    _In_ PCURDIR CurDir
    )
{
    ULONG BackupIndex;
    PWSTR UncPathPointer;
    ULONG NumberOfPathSeparators;
    RTL_PATH_TYPE CurDirPathType;

    CurDirPathType = RtlDetermineDosPathNameType_U( CurDir->DosPath.Buffer );
    BackupIndex = 3;
    if (CurDirPathType == RtlPathTypeUncAbsolute) {

        UncPathPointer = CurDir->DosPath.Buffer + 2;
        NumberOfPathSeparators = 0;

        while (*UncPathPointer) {
            if (IS_PATH_SEPARATOR_U(*UncPathPointer)) {
                NumberOfPathSeparators++;
                if (NumberOfPathSeparators == 2) {
                    break;
                }
            }
            UncPathPointer++;
        }
        BackupIndex = (ULONG)(UncPathPointer - CurDir->DosPath.Buffer);
    }
    return BackupIndex;
}

VOID
RtlpResetDriveEnvironment (
    _In_ WCHAR DriveLetter
    )
{
    WCHAR NameBuffer[4];
    UNICODE_STRING Name;
    UNICODE_STRING Value;
    WCHAR ValueBuffer[4];

    PAGED_CODE();

    NameBuffer[0] = L'=';
    NameBuffer[1] = DriveLetter;
    NameBuffer[2] = L':';
    NameBuffer[3] = L'\0';
    RtlInitUnicodeString( &Name,
                          NameBuffer );


    ValueBuffer[0] = DriveLetter;
    ValueBuffer[1] = L':';
    ValueBuffer[2] = L'\\';
    ValueBuffer[3] = L'\0';
    RtlInitUnicodeString( &Value,
                          ValueBuffer );

    RtlSetEnvironmentVariable( NULL,
                               &Name,
                               &Value );
}


ULONG RtlpSavedDismountCount = (ULONG)-1;

VOID
RtlpValidateCurrentDirectory (
    _In_ PCURDIR CurDir
    )
{    
    PAGED_CODE();

    if (((ULONG_PTR)CurDir->Handle & 1) == 0 && USER_SHARED_DATA->DismountCount == RtlpSavedDismountCount) {
        return;
    }
    if (CurDir->Handle == NULL) {
        return;
    }

    RtlpSavedDismountCount = USER_SHARED_DATA->DismountCount;
    
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = ZwFsControlFile( CurDir->Handle,
                                       NULL,
                                       NULL,
                                       NULL,
                                       &IoStatus,
                                       FSCTL_IS_VOLUME_MOUNTED,
                                       NULL,
                                       0,
                                       NULL,
                                       0 );
    if (Status == STATUS_WRONG_VOLUME || Status == STATUS_VOLUME_DISMOUNTED) {
        RtlpDereferenceCurDirRef(RtlpCurDirRef);
        RtlpCurDirRef = NULL;
        CurDir->Handle = NULL;
        Status = RtlSetCurrentDirectory_U( &CurDir->DosPath );
        if (! NT_SUCCESS(Status)) {
            WCHAR PathNameBuffer[4];
            PathNameBuffer[0] = CurDir->DosPath.Buffer[0];
            PathNameBuffer[1] = CurDir->DosPath.Buffer[1];
            PathNameBuffer[2] = CurDir->DosPath.Buffer[2];
            PathNameBuffer[3] = UNICODE_NULL;
            RtlpResetDriveEnvironment( PathNameBuffer[0] );

            UNICODE_STRING PathName;
            RtlInitUnicodeString( &PathName,
                                  PathNameBuffer );
            
            RtlSetCurrentDirectory_U( &PathName );
        }

    }
}

VOID
RtlpCheckRelativeDrive (
    _In_ WCHAR NewDrive
    )
{
    PAGED_CODE();

    WCHAR NameBuffer[4];
    UNICODE_STRING Name;
    WCHAR ValueBuffer[DOS_MAX_PATH_LENGTH + (sizeof(L"\\DosDevices\\") / sizeof(WCHAR) - 1)];
    UNICODE_STRING Value;

    NameBuffer[0] = L'=';
    NameBuffer[1] = (WCHAR)NewDrive;
    NameBuffer[2] = L':';
    NameBuffer[3] = UNICODE_NULL;
    RtlInitUnicodeString( &Name,
                          NameBuffer);
    Value.Length = 0;
    Value.MaximumLength = DOS_MAX_PATH_LENGTH << 1;
    Value.Buffer = &ValueBuffer[ RtlpDosDevicesPrefix.Length >> 1];

    if (! NT_SUCCESS(RtlQueryEnvironmentVariable_U( NULL,
                                                    &Name,
                                                    &Value ))) {
        Value.Buffer[0] = (WCHAR)NewDrive;
        Value.Buffer[1] = L':';
        Value.Buffer[2] = L'\\';
        Value.Buffer[3] = UNICODE_NULL;
        Value.Length = 6;
    }
    Value.MaximumLength = sizeof(ValueBuffer);
    Value.Length = Value.Length + RtlpDosDevicesPrefix.Length;
    Value.Buffer = ValueBuffer;
    RtlCopyMemory( ValueBuffer,
                   RtlpDosDevicesPrefix.Buffer,
                   RtlpDosDevicesPrefix.Length );

    OBJECT_ATTRIBUTES ObjectAttributes;
    InitializeObjectAttributes( &ObjectAttributes,
                                &Value,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL );

    ULONG HardErrorValue;
    RtlSetThreadErrorMode( RTL_ERRORMODE_FAILCRITICALERRORS,
                           &HardErrorValue );

    HANDLE DirHandle;
    IO_STATUS_BLOCK IoStatus;
    NTSTATUS Status = ZwOpenFile( &DirHandle,
                                  SYNCHRONIZE,
                                  &ObjectAttributes,
                                  &IoStatus,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );

    RtlSetThreadErrorMode( HardErrorValue,
                           NULL );

    if (NT_SUCCESS(Status)) {
        ZwClose( DirHandle );
        return;
    }
    RtlpResetDriveEnvironment( NewDrive );
}



VOID
LdkpTerminateCurDir (
    VOID
    )
{
    if (RtlpCurDirRef) {
        RtlpDereferenceCurDirRef( RtlpCurDirRef );
    }
}



VOID
NTAPI
RtlReleaseRelativeName (
    _In_ PRTL_RELATIVE_NAME_U RelativeName
    )
{
    ASSERT(RelativeName);

    if (RelativeName->CurDirRef) {
        RtlpDereferenceCurDirRef( RelativeName->CurDirRef );
        RelativeName->CurDirRef = NULL;
    }
}

RTL_PATH_TYPE
RtlDetermineDosPathNameType_Ustr (
    _In_ PCUNICODE_STRING String
    )
{
    RTL_PATH_TYPE ReturnValue;
    PCWSTR DosFileName = String->Buffer;
#define ENOUGH_CHARS(_cch) (String->Length >= ((_cch) * sizeof(WCHAR)))
    if (ENOUGH_CHARS(1) && IS_PATH_SEPARATOR_U(*DosFileName)) {
        if (ENOUGH_CHARS(2) && IS_PATH_SEPARATOR_U(*(DosFileName + 1)) ) {
            if (ENOUGH_CHARS(3) && (DosFileName[2] == '.' || DosFileName[2] == '?') ) {
                if (ENOUGH_CHARS(4) && IS_PATH_SEPARATOR_U(*(DosFileName + 3))) {
                    ReturnValue = RtlPathTypeLocalDevice;
                } else if ( String->Length == (3 * sizeof(WCHAR)) ){
                    ReturnValue = RtlPathTypeRootLocalDevice;
                } else {
                    ReturnValue = RtlPathTypeUncAbsolute;
                }
            } else {
                ReturnValue = RtlPathTypeUncAbsolute;
            }
        } else {
            ReturnValue = RtlPathTypeRooted;
        }
    } else if (ENOUGH_CHARS(2) && *DosFileName && *(DosFileName + 1)==L':') {
        if (ENOUGH_CHARS(3) && IS_PATH_SEPARATOR_U(*(DosFileName + 2))) {
            ReturnValue = RtlPathTypeDriveAbsolute;
        } else  {
            ReturnValue = RtlPathTypeDriveRelative;
        }
    } else {
        ReturnValue = RtlPathTypeRelative;
    }
#undef ENOUGH_CHARS
return ReturnValue;
}

RTL_PATH_TYPE
NTAPI
RtlDetermineDosPathNameType_U (
    _In_ PCWSTR DosFileName
    )
{
    RTL_PATH_TYPE ReturnValue;
    ASSERT(DosFileName);

    if (IS_PATH_SEPARATOR_U(*DosFileName)) {
        if (IS_PATH_SEPARATOR_U(*(DosFileName + 1))) {
            if (DosFileName[2] == '.' || DosFileName[2] == '?') {
                if (IS_PATH_SEPARATOR_U(*(DosFileName + 3))){
                    ReturnValue = RtlPathTypeLocalDevice;
                } else if ((*(DosFileName + 3)) == UNICODE_NULL){
                    ReturnValue = RtlPathTypeRootLocalDevice;
                } else {
                    ReturnValue = RtlPathTypeUncAbsolute;
                }
            } else {
                ReturnValue = RtlPathTypeUncAbsolute;
            }
        } else {
            ReturnValue = RtlPathTypeRooted;
        }
    } else if ((*DosFileName) && (*(DosFileName + 1) == L':')) {
        if (IS_PATH_SEPARATOR_U(*(DosFileName + 2))) {
            ReturnValue = RtlPathTypeDriveAbsolute;
        } else  {
            ReturnValue = RtlPathTypeDriveRelative;
        }
    } else {
        ReturnValue = RtlPathTypeRelative;
    }
    return ReturnValue;
}

ULONG
RtlIsDosDeviceName_Ustr (
    _In_ PCUNICODE_STRING DosFileName
    )
{
    switch (RtlDetermineDosPathNameType_Ustr( DosFileName )) {
    case RtlPathTypeLocalDevice:
        if (RtlEqualUnicodeString( DosFileName,
                                   &RtlpDosSlashCONDevice,
                                   TRUE )) {
            return 0x00080006;
        }
    case RtlPathTypeUncAbsolute:
    case RtlPathTypeUnknown:
        return 0;
    }

    USHORT ColonBias = 0;
    UNICODE_STRING UnicodeString = *DosFileName;
    USHORT OriginalLength = UnicodeString.Length;
    USHORT NumberOfCharacters = OriginalLength >> 1;
    if (NumberOfCharacters && UnicodeString.Buffer[NumberOfCharacters - 1] == L':') {
        UnicodeString.Length -= sizeof(WCHAR);
        NumberOfCharacters--;
        ColonBias = 1;
    }
    if (NumberOfCharacters == 0) {
        return 0;
    }

    WCHAR wch = UnicodeString.Buffer[NumberOfCharacters - 1];
    while (NumberOfCharacters && (wch == L'.' || wch == L' ')) {
        UnicodeString.Length -= sizeof(WCHAR);
        NumberOfCharacters--;
        ColonBias++;
        if (NumberOfCharacters > 0) {
            wch = UnicodeString.Buffer[NumberOfCharacters - 1];
        }
    }
    
    LPWSTR p;
    ULONG ReturnLength = NumberOfCharacters << 1;
    ULONG ReturnOffset = 0;
    if (NumberOfCharacters) {
        p = UnicodeString.Buffer + NumberOfCharacters - 1;
        while (p >= UnicodeString.Buffer) {
            if (*p == L'\\' || *p == L'/' || (*p == L':' && p == UnicodeString.Buffer + 1)) {
                p++;
                if (p >= (UnicodeString.Buffer + (OriginalLength / sizeof(WCHAR)))) {
                    return 0;
                }
                wch = (*p) | 0x20;
                if (! (wch == L'l' || wch == L'c' || wch == L'p' || wch == L'a' || wch == L'n')) {
                    return 0;
                }
                ReturnOffset = (ULONG)((PSZ)p - (PSZ)UnicodeString.Buffer);
                UnicodeString.Length = OriginalLength - (USHORT)((PCHAR)p - (PCHAR)UnicodeString.Buffer);
                UnicodeString.Buffer = p;
                NumberOfCharacters = UnicodeString.Length >> 1;
                NumberOfCharacters = NumberOfCharacters - ColonBias;
                ReturnLength = NumberOfCharacters << 1;
                UnicodeString.Length -= ColonBias * sizeof(WCHAR);
                break;
            }
            p--;
        }

        wch = UnicodeString.Buffer[0] | 0x20;
        if (! ( wch == L'l' || wch == L'c' || wch == L'p' || wch == L'a' || wch == L'n' ) ) {
            return 0;
        }
    }

    p = UnicodeString.Buffer;
    while (p < UnicodeString.Buffer + NumberOfCharacters && *p != L'.' && *p != L':') {
        p++;
    }

    while (p > UnicodeString.Buffer && p[-1] == L' ') {
        p--;
    }

    NumberOfCharacters = (USHORT)(p - UnicodeString.Buffer);
    UnicodeString.Length = NumberOfCharacters * sizeof( WCHAR );
    if (NumberOfCharacters == 4 && iswdigit( UnicodeString.Buffer[3] )) {
        if ((WCHAR)UnicodeString.Buffer[3] == L'0') {
            return 0;
        } else {
            UnicodeString.Length -= sizeof(WCHAR);
            if (RtlEqualUnicodeString( &UnicodeString,
                                       &RtlpDosLPTDevice,
                                       TRUE ) ||
                RtlEqualUnicodeString( &UnicodeString,
                                       &RtlpDosCOMDevice,
                                       TRUE ) ) {
                ReturnLength = NumberOfCharacters << 1;
            } else {
                return 0;
            }
        }
    } else if (NumberOfCharacters != 3) {
        return 0;
    } else if (RtlEqualUnicodeString( &UnicodeString,
                                      &RtlpDosPRNDevice,
                                      TRUE ) ) {
        ReturnLength = NumberOfCharacters << 1;
    } else if (RtlEqualUnicodeString( &UnicodeString,
                                      &RtlpDosAUXDevice,
                                      TRUE )) {
        ReturnLength = NumberOfCharacters << 1;
    } else if (RtlEqualUnicodeString( &UnicodeString,
                                      &RtlpDosNULDevice,
                                      TRUE ) ) {
        ReturnLength = NumberOfCharacters << 1;
    } else if (RtlEqualUnicodeString( &UnicodeString,
                                      &RtlpDosCONDevice,
                                      TRUE ) ) {
        ReturnLength = NumberOfCharacters << 1;
    } else {
        return 0;
    }
    return ReturnLength | (ReturnOffset << 16);
}

ULONG
NTAPI
RtlIsDosDeviceName_U (
    _In_ PCWSTR DosFileName
    )
{
    UNICODE_STRING UnicodeString;
    if (NT_SUCCESS(RtlInitUnicodeStringEx( &UnicodeString,
                                           DosFileName ))) {
        return RtlIsDosDeviceName_Ustr( &UnicodeString );
    }
    return 0;
}

ULONG
RtlGetFullPathName_Ustr ( 
    _In_ PCUNICODE_STRING FileName,
    _In_ ULONG nBufferLength,
    _Out_ PWSTR lpBuffer,
    _Out_opt_ PWSTR *lpFilePart,
    _Out_ PBOOLEAN NameInvalid,
    _Out_ RTL_PATH_TYPE *InputPathType
    )
{
    PAGED_CODE();

    if (ARGUMENT_PRESENT(NameInvalid)) {
        *NameInvalid = FALSE;
    }

    if (nBufferLength > MAXUSHORT) {
        nBufferLength = MAXUSHORT - 1;
    }

    *InputPathType = RtlPathTypeUnknown;

    UNICODE_STRING UnicodeString = *FileName;
    PWSTR lpFileName = UnicodeString.Buffer;
    ULONG NumberOfCharacters = UnicodeString.Length >> 1;
    LONG PathNameLength = UnicodeString.Length;
    if (PathNameLength == 0 || UnicodeString.Buffer[0] == UNICODE_NULL) {
        return 0;
    }

    ULONG DeviceNameLength = PathNameLength;
    WCHAR wch = UnicodeString.Buffer[(DeviceNameLength >> 1) - 1];
    while (DeviceNameLength && wch == L' ') {
        DeviceNameLength -= sizeof(WCHAR);
        if (DeviceNameLength) {
            wch = UnicodeString.Buffer[(DeviceNameLength >> 1) - 1];
        }
    }
    if (! DeviceNameLength) {
        return 0;
    }

    BOOLEAN StripTrailingSlash = !(lpFileName[NumberOfCharacters - 1] == L'\\' || lpFileName[NumberOfCharacters - 1] == L'/');
    DeviceNameLength = RtlIsDosDeviceName_Ustr( &UnicodeString );
    if (DeviceNameLength) {
        if (ARGUMENT_PRESENT(lpFilePart)) {
            *lpFilePart = NULL;
        }
        ULONG DeviceNameOffset = DeviceNameLength >> 16;
        DeviceNameLength &= 0x0000ffff;
        if (ARGUMENT_PRESENT(NameInvalid) && DeviceNameOffset ) {
            if ((! NT_SUCCESS(RtlpCheckDeviceName( &UnicodeString,
                                                   DeviceNameOffset,
                                                   NameInvalid ))) || (*NameInvalid)) {
                return 0;
            }
        }
        PathNameLength = DeviceNameLength + RtlpSlashSlashDot.Length;
        if (PathNameLength < (LONG)nBufferLength) {
            RtlCopyMemory( lpBuffer,
                           RtlpSlashSlashDot.Buffer,
                           RtlpSlashSlashDot.Length );
            RtlMoveMemory( (PVOID)((PUCHAR)lpBuffer+RtlpSlashSlashDot.Length),
                           (PSZ)lpFileName+DeviceNameOffset,
                           DeviceNameLength );
            RtlZeroMemory( (PVOID)((PUCHAR)lpBuffer+RtlpSlashSlashDot.Length+DeviceNameLength),
                           sizeof(UNICODE_NULL) );
        } else {
            PathNameLength += sizeof(UNICODE_NULL);
            if (PathNameLength > MAXUSHORT) {
                PathNameLength = 0;
            }
        }
        return PathNameLength;
    }

    UNICODE_STRING FullPath;
    FullPath.MaximumLength = (USHORT)nBufferLength;
    FullPath.Length = 0;
    FullPath.Buffer = lpBuffer;
    RtlZeroMemory( lpBuffer,
                   nBufferLength );
    PCURDIR CurDir = &NtCurrentPeb()->ProcessParameters->CurrentDirectory;

    RTL_PATH_TYPE PathType = *InputPathType = RtlDetermineDosPathNameType_Ustr( &UnicodeString );
    PWSTR Source = lpFileName;
    ULONG PrefixSourceLength = 0;
    UNICODE_STRING Prefix;
    Prefix.Length = 0;
    Prefix.MaximumLength = 0;
    Prefix.Buffer = NULL;

    RtlAcquirePebLock();

    ULONG BackupIndex;
    UCHAR CurDrive, NewDrive;
    ULONG PathLength = 0;
    try {
        switch (PathType) {
        case RtlPathTypeUncAbsolute: {
            ULONG NumberOfPathSeparators = 0;
            PWSTR UncPathPointer = lpFileName + 2;
            USHORT i = 2 * sizeof(WCHAR);
            while (i < UnicodeString.Length) {
                if (IS_PATH_SEPARATOR_U(*UncPathPointer)) {
                    NumberOfPathSeparators++;
                    if (NumberOfPathSeparators == 2) {
                        break;
                    }
                }
                i += sizeof(WCHAR);
                UncPathPointer++;
            }
            BackupIndex = (ULONG)(UncPathPointer - lpFileName);
            PrefixSourceLength = BackupIndex << 1;
            Source += BackupIndex;
            break;
        }
        case RtlPathTypeLocalDevice:
            PrefixSourceLength = RtlpSlashSlashDot.Length;
            BackupIndex = 4;
            Source += BackupIndex;
            break;
        case RtlPathTypeRootLocalDevice:
            Prefix = RtlpSlashSlashDot;
            Prefix.Length = (USHORT)(Prefix.Length - (USHORT)(2*sizeof(UNICODE_NULL)));
            PrefixSourceLength = Prefix.Length + sizeof(UNICODE_NULL);
            BackupIndex = 3;
            Source += BackupIndex;
            PathNameLength -= BackupIndex * sizeof( WCHAR );
            break;
        case RtlPathTypeDriveAbsolute:
            CurDrive = (UCHAR)RtlUpcaseUnicodeChar( CurDir->DosPath.Buffer[0] );
            NewDrive = (UCHAR)RtlUpcaseUnicodeChar( lpFileName[0] );
            if (CurDrive == NewDrive) {
                RtlpValidateCurrentDirectory( CurDir );
            }
            BackupIndex = 3;
            break;

        case RtlPathTypeDriveRelative:
            CurDrive = (UCHAR)RtlUpcaseUnicodeChar( CurDir->DosPath.Buffer[0] );
            NewDrive = (UCHAR)RtlUpcaseUnicodeChar( lpFileName[0] );
            if (CurDrive == NewDrive) {
                RtlpValidateCurrentDirectory( CurDir );
                Prefix = *(PUNICODE_STRING)&CurDir->DosPath;
            } else {
                RtlpCheckRelativeDrive( (WCHAR)NewDrive );

                UNICODE_STRING Name;
                WCHAR NameBuffer[4];
                NameBuffer[0] = L'=';
                NameBuffer[1] = (WCHAR)NewDrive;
                NameBuffer[2] = L':';
                NameBuffer[3] = UNICODE_NULL;
                RtlInitUnicodeString( &Name,
                                      NameBuffer );
                Prefix = FullPath;
                NTSTATUS Status = RtlQueryEnvironmentVariable_U( NULL,
                                                                 &Name,
                                                                 &Prefix );
                if (NT_SUCCESS(Status)) {
                    ULONG LastChar = Prefix.Length >> 1;
                    if (LastChar > 3) {
                        Prefix.Buffer[LastChar] = L'\\';
                        Prefix.Length += sizeof(UNICODE_NULL);
                    }
                } else {
                    if (Status == STATUS_BUFFER_TOO_SMALL) {
                        PathNameLength = (ULONG)(Prefix.Length) + PathNameLength + 2;
                        if (PathNameLength > MAXUSHORT) {
                            PathNameLength = 0;
                        }
                        PathLength =PathNameLength;
                        leave;
                    } else {
                        Status = STATUS_SUCCESS;
                        NameBuffer[0] = (WCHAR)NewDrive;
                        NameBuffer[1] = L':';
                        NameBuffer[2] = L'\\';
                        NameBuffer[3] = UNICODE_NULL;
                        RtlInitUnicodeString( &Prefix,
                                              NameBuffer );
                    }
                }
            }
            BackupIndex = 3;
            Source += 2;
            PathNameLength -= 2 * sizeof( WCHAR );
            break;
        case RtlPathTypeRooted:
            BackupIndex = RtlpComputeBackupIndex( CurDir );
            if (BackupIndex == 3) {
                Prefix = CurDir->DosPath;
                Prefix.Length = 2 * sizeof(UNICODE_NULL);
            } else {
                Prefix = CurDir->DosPath;
                Prefix.Length = (USHORT)(BackupIndex << 1);
            }
            break;
        case RtlPathTypeRelative:
            RtlpValidateCurrentDirectory( CurDir );
            Prefix = CurDir->DosPath;
            BackupIndex = RtlpComputeBackupIndex(CurDir);
            break;
        default:
            PathLength = 0;
            leave;
        }

        ULONG MaximumLength = PathNameLength + Prefix.Length;
        if ((MaximumLength + sizeof(WCHAR)) > nBufferLength) {
            if ((NumberOfCharacters > 1) || (*lpFileName != L'.')) {
                MaximumLength += sizeof(UNICODE_NULL);
                if (MaximumLength > MAXUSHORT) {
                    MaximumLength = 0;
                }
                PathLength =  MaximumLength;
                leave;
            }

            if (NumberOfCharacters == 1 && *lpFileName == L'.') {
                if (Prefix.Length == 6) {
                    if (nBufferLength <= Prefix.Length) {
                        PathLength = (ULONG)(Prefix.Length + (USHORT)sizeof(UNICODE_NULL));
                        leave;
                    }
                } else {
                    if (nBufferLength < Prefix.Length) {
                        PathLength = (ULONG)Prefix.Length;
                        leave;
                    }
                    for(USHORT i = 0, j = 0; i < Prefix.Length; i += sizeof(WCHAR), j++){
                        if (Prefix.Buffer[j] == L'\\' || Prefix.Buffer[j] == L'/' ) {
                            FullPath.Buffer[j] = L'\\';
                        } else {
                            FullPath.Buffer[j] = Prefix.Buffer[j];
                        }
                    }
                    FullPath.Length = Prefix.Length - (USHORT)sizeof(L'\\');
                    goto skipit;
                }
            } else {
                if (MaximumLength > MAXUSHORT) {
                    MaximumLength = 0;
                }
                PathLength = MaximumLength;
                leave;
            }
        }

        if (PrefixSourceLength || Prefix.Buffer != FullPath.Buffer) {
            for(USHORT i = 0, j = 0; i < PrefixSourceLength; i += sizeof(WCHAR), j++){
                if (lpFileName[j] == L'\\' || lpFileName[j] == L'/' ) {
                    FullPath.Buffer[j] = L'\\';
                } else {
                    FullPath.Buffer[j] = lpFileName[j];
                }
            }
            FullPath.Length = (USHORT)PrefixSourceLength;
            for(USHORT i = 0, j = 0; i < Prefix.Length; i += sizeof(WCHAR), j++) {
                if (Prefix.Buffer[j] == L'\\' || Prefix.Buffer[j] == L'/') {
                    FullPath.Buffer[j+(FullPath.Length >> 1)] = L'\\';
                } else {
                    FullPath.Buffer[j+(FullPath.Length >> 1)] = Prefix.Buffer[j];
                }
            }
            FullPath.Length = FullPath.Length + Prefix.Length;
        } else {
            FullPath.Length = Prefix.Length;
        }
skipit:
        PWSTR Dest = (PWSTR)((PUCHAR)FullPath.Buffer + FullPath.Length);
        *Dest = UNICODE_NULL;

        ULONG i = (ULONG)((PCHAR)Source - (PCHAR)lpFileName);
        ULONG j;
        while (i < UnicodeString.Length) {
            i += sizeof(WCHAR);
            switch (*Source) {
            case L'\\':
            case L'/':
                if (*(Dest - 1) != L'\\') {
                    *Dest++ = L'\\';
                }
                Source++;
                break;
            case '.':
                j = UnicodeString.Length - i + sizeof(WCHAR);
                if (IS_DOT_USTR(Source, j)) {
                    Source++;
                    if ((i < UnicodeString.Length) && IS_PATH_SEPARATOR_U(*Source)) {
                        Source++;
                        i += sizeof(WCHAR);
                    }
                } else if (IS_DOT_DOT_USTR(Source, j)) {
                    while (*Dest != L'\\') {
                        *Dest = UNICODE_NULL;
                        Dest--;
                    }
                    do {
                        if (Dest ==  FullPath.Buffer + (BackupIndex - 1)) {
                            break;
                        }
                        *Dest = UNICODE_NULL;
                        Dest--;
                    } while (*Dest != L'\\');
                    if (Dest ==  FullPath.Buffer + (BackupIndex - 1)) {
                        Dest++;
                    }
                    Source += 2;
                    i += sizeof(WCHAR);
                }
                break;
            default:
                i -= sizeof(WCHAR);
                j = UnicodeString.Length - i;
                while (! IS_END_OF_COMPONENT_USTR(Source, j) ) {
                    j -= sizeof(WCHAR);
                    i += sizeof(WCHAR);
                    *Dest++ = *Source++;
                }
                if (IS_PATH_SEPARATOR_U(*Source) && Dest[-1] == L'.') {
                    Dest--;
                }
                break;
            }
        }
        *Dest = UNICODE_NULL;
        if (StripTrailingSlash) {
            if (Dest > (FullPath.Buffer + BackupIndex) && *(Dest-1) == L'\\') {
                Dest--;
                *Dest = UNICODE_NULL;
            }
        }
        FullPath.Length = (USHORT)(PtrToUlong(Dest) - PtrToUlong(FullPath.Buffer));
        while (Dest > FullPath.Buffer && (Dest[-1] == L' ' || Dest[-1] == L'.')) {
            *--Dest = UNICODE_NULL;
            FullPath.Length -= sizeof( WCHAR );
        }

        if (ARGUMENT_PRESENT(lpFilePart)) {
            Source = Dest-1;
            Dest = NULL;
            while (Source > FullPath.Buffer ) {
                if (*Source == L'\\') {
                    Dest = Source + 1;
                    break;
                }
                Source--;
            }
            if (Dest && *Dest) {
                if (PathType == RtlPathTypeUncAbsolute) {
                    if (Dest < (FullPath.Buffer + BackupIndex)) {
                        *lpFilePart = NULL;
                        PathLength = (ULONG)FullPath.Length;
                        leave;
                    }
                }
                *lpFilePart = Dest;
            } else {
                *lpFilePart = NULL;
            }
        }
        PathLength = (ULONG)FullPath.Length;
    } finally {
        RtlReleasePebLock();
    }
    return PathLength;
}

ULONG
NTAPI
RtlGetFullPathName_U (
    _In_ PCWSTR lpFileName,
    _In_ ULONG nBufferLength,
    _Out_bytecapcount_(nBufferLength) PWSTR lpBuffer,
    _Out_opt_ PWSTR *lpFilePart
    )
{
    UNICODE_STRING FileName;

    PAGED_CODE();

    if (! NT_SUCCESS(RtlInitUnicodeStringEx( &FileName,
                                             lpFileName ))) {
        return 0;
    }

    RTL_PATH_TYPE PathType;
    return RtlGetFullPathName_Ustr( &FileName,
                                    nBufferLength,
                                    lpBuffer,
                                    lpFilePart,
                                    NULL,
                                    &PathType ); 
}

ULONG
NTAPI
RtlGetCurrentDirectory_U (
    _In_ ULONG nBufferLength,
    _Out_ PWSTR lpBuffer
    )
{
    PAGED_CODE();

    PCURDIR CurDir = &NtCurrentPeb()->ProcessParameters->CurrentDirectory;

    RtlAcquirePebLock();    
    PWSTR CurDirName = CurDir->DosPath.Buffer;
    ULONG Length = CurDir->DosPath.Length >> 1;
    ASSERT(CurDirName && (Length > 0));

    if ((Length > 1) && (CurDirName[Length - 2] != L':')) {
        if (nBufferLength < (Length) << 1) {
            RtlReleasePebLock();
            return (Length) << 1;
        }
    } else {
        if (nBufferLength <= (Length << 1)) {
            RtlReleasePebLock();
            return ((Length + 1) << 1);
        }
    }

    try {
        RtlCopyMemory( lpBuffer,
                       CurDirName,
                       Length << 1 );
        ASSERT(lpBuffer[Length - 1] == L'\\');
        if ((Length > 1) && (lpBuffer[Length - 2] == L':')) {
            lpBuffer[Length] = UNICODE_NULL;
        } else {
            lpBuffer[Length - 1] = UNICODE_NULL;
            Length--;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        RtlReleasePebLock();
        return 0L;
    }
    RtlReleasePebLock();
    return Length << 1;
}

NTSTATUS
NTAPI
RtlSetCurrentDirectory_U (
    _In_ PCUNICODE_STRING PathName
    )
{
    PAGED_CODE();

    PPEB Peb = NtCurrentPeb();
    PCURDIR CurDir = &Peb->ProcessParameters->CurrentDirectory;

    ULONG IsDevice = RtlIsDosDeviceName_Ustr( PathName );
    UNICODE_STRING DosDir;
    DosDir.Buffer = NULL;
    HANDLE HandleToClose = NULL;

    RtlAcquirePebLock();
    Peb->EnvironmentUpdateCount += 1;
    if (((ULONG_PTR)CurDir->Handle & OBJ_HANDLE_TAGBITS) == RTL_USER_PROC_CURDIR_CLOSE) {
        HandleToClose = CurDir->Handle;
        CurDir->Handle = NULL;
    }

    NTSTATUS Status = STATUS_SUCCESS;
    PVOID FreeBuffer = NULL;
    HANDLE NewDirectoryHandle = NULL;
    HANDLE HandleToClose1 = NULL;
    PRTLP_CURDIR_REF CurDirRefToDereference = NULL;
    try {
        try {
            if (IsDevice) {
                Status = STATUS_NOT_A_DIRECTORY;
                leave;
            }

            ULONG DosDirLength = CurDir->DosPath.MaximumLength;
            DosDir.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                             0,
                                             DosDirLength );
            if (! DosDir.Buffer) {
                Status = STATUS_NO_MEMORY;
                leave;
            }

            DosDir.Length = 0;
            DosDir.MaximumLength = (USHORT)DosDirLength;    
            RTL_PATH_TYPE InputPathType;
            DosDirLength = RtlGetFullPathName_Ustr( PathName,
                                                    DosDirLength,
                                                    DosDir.Buffer,
                                                    NULL,
                                                    NULL,
                                                    &InputPathType );
            if (! DosDirLength) {
                Status = STATUS_OBJECT_NAME_INVALID;
                leave;
            }
            if (DosDirLength > DosDir.MaximumLength) {
                Status = STATUS_NAME_TOO_LONG;
                leave;
            }

            UNICODE_STRING NtFileName;
            if (! RtlDosPathNameToNtPathName_U( DosDir.Buffer,
                                                &NtFileName,
                                                NULL,
                                                NULL )) {
                Status = STATUS_OBJECT_NAME_INVALID;
                leave;
            }
            FreeBuffer = NtFileName.Buffer;

            OBJECT_ATTRIBUTES ObjectAttributes;
            IO_STATUS_BLOCK IoStatus;
            FILE_FS_DEVICE_INFORMATION DeviceInfo;
            InitializeObjectAttributes( &ObjectAttributes,
                                        &NtFileName,
                                        OBJ_CASE_INSENSITIVE | OBJ_INHERIT | OBJ_KERNEL_HANDLE,
                                        NULL,
                                        NULL );

            if (((ULONG_PTR)CurDir->Handle & OBJ_HANDLE_TAGBITS) ==  RTL_USER_PROC_CURDIR_INHERIT) {
                NewDirectoryHandle = (HANDLE)((ULONG_PTR)CurDir->Handle & ~OBJ_HANDLE_TAGBITS);
                CurDir->Handle = NULL;
                Status = ZwQueryVolumeInformationFile( NewDirectoryHandle,
                                                       &IoStatus,
                                                       &DeviceInfo,
                                                       sizeof(DeviceInfo),
                                                       FileFsDeviceInformation );
                if (! NT_SUCCESS(Status)) {
                    Status = RtlSetCurrentDirectory_U( PathName );
                    leave;
                }
                if (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA) {
                    NewDirectoryHandle = (HANDLE)((ULONG_PTR)NewDirectoryHandle | 1);
                }
            } else {
                Status = ZwOpenFile( &NewDirectoryHandle,
                                     FILE_TRAVERSE | SYNCHRONIZE,
                                     &ObjectAttributes,
                                     &IoStatus,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT );
                if (! NT_SUCCESS(Status)) {
                    leave;
                }
                Status = ZwQueryVolumeInformationFile( NewDirectoryHandle,
                                                       &IoStatus,
                                                       &DeviceInfo,
                                                       sizeof(DeviceInfo),
                                                       FileFsDeviceInformation );
                if (! NT_SUCCESS(Status)) {
                    leave;
                }            
                if (DeviceInfo.Characteristics & FILE_REMOVABLE_MEDIA) {
                    NewDirectoryHandle = (HANDLE)((ULONG_PTR)NewDirectoryHandle | 1);
                }
            }
            ULONG DosDirCharCount = DosDirLength >> 1;
            DosDir.Length = (USHORT)DosDirLength;
            if (DosDir.Buffer[DosDirCharCount - 1] != L'\\') {
                if ((DosDirCharCount + 2) >
                    (DosDir.MaximumLength / sizeof(WCHAR))) {
                    Status = STATUS_NAME_TOO_LONG;
                    leave;
                }
                DosDir.Buffer[DosDirCharCount] = L'\\';
                DosDir.Buffer[DosDirCharCount + 1] = UNICODE_NULL;
                DosDir.Length += sizeof( UNICODE_NULL );
            }

            if (RtlpCurDirRef && RtlpCurDirRef->RefCount == 1) {
                HandleToClose1 = RtlpCurDirRef->DirectoryHandle;
                RtlpCurDirRef->DirectoryHandle = NewDirectoryHandle;
            } else {
                CurDirRefToDereference = RtlpCurDirRef;
                RtlpCurDirRef = RtlAllocateHeap( RtlProcessHeap(),
                                                 0,
                                                 sizeof(RTLP_CURDIR_REF));
                if (! RtlpCurDirRef) {
                    RtlpCurDirRef = CurDirRefToDereference;
                    CurDirRefToDereference = NULL;
                    Status = STATUS_NO_MEMORY;
                    leave;
                }
                RtlpInitializeCurDirRef( RtlpCurDirRef,
                                         NewDirectoryHandle );
            }
            CurDir->Handle = NewDirectoryHandle;
            NewDirectoryHandle = NULL;
            RtlCopyMemory( CurDir->DosPath.Buffer,
                           DosDir.Buffer,
                           DosDir.Length + sizeof (UNICODE_NULL) );
            CurDir->DosPath.Length = DosDir.Length;

        } finally {
            RtlReleasePebLock();
            if (DosDir.Buffer) {
                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             DosDir.Buffer );
            }
            if (FreeBuffer) {
                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             FreeBuffer );
            }
            if (NewDirectoryHandle) {
                ZwClose( NewDirectoryHandle );
            }
            if (HandleToClose) {
                ZwClose( HandleToClose );
            }
            if (HandleToClose1) {
                ZwClose( HandleToClose1 );
            }
            if (CurDirRefToDereference) {
                RtlpDereferenceCurDirRef( CurDirRefToDereference );
            }
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        Status = STATUS_ACCESS_VIOLATION;
    }
    return Status;
}

BOOLEAN
RtlpDosPathNameToRelativeNtPathName_Ustr (
    _In_ BOOLEAN CaptureRelativeName,
    _In_ PCUNICODE_STRING DosFileNameString,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_opt_ PRTL_RELATIVE_NAME_U RelativeName
    )
{
    PWSTR FullNtPathName = NULL;
    BOOLEAN UseWin32Name;
    WCHAR StaticDosBuffer[DOS_MAX_PATH_LENGTH + 1];
    ULONG BufferLength = sizeof(StaticDosBuffer);
    PWSTR FullDosPathName = NULL;
    UNICODE_STRING UnicodeString = *DosFileNameString;

    PAGED_CODE();

    if (UnicodeString.Length > 8 && UnicodeString.Buffer[0] == '\\' &&
        UnicodeString.Buffer[1] == '\\' && UnicodeString.Buffer[2] == '?' &&
        UnicodeString.Buffer[3] == '\\' ) {
        UseWin32Name = TRUE;
    } else {
        FullNtPathName = RtlAllocateHeap( RtlProcessHeap(),
                                          0,
                                          BufferLength );
        if (! FullNtPathName) {
            return FALSE;
        }
        BufferLength += RtlpLongestPrefix;
        FullDosPathName = &StaticDosBuffer[0];
        UseWin32Name = FALSE;
    }

    BOOLEAN ReturnValue = TRUE;
    RtlAcquirePebLock();
    try {
        try {
            if (UseWin32Name) {
                ULONG NtFileLength = UnicodeString.Length - 8 + RtlpDosDevicesPrefix.Length;
                if (NtFileLength > (MAXUSHORT - sizeof(UNICODE_NULL))) {
                    ReturnValue = FALSE;
                    leave;
                }
                FullNtPathName = RtlAllocateHeap( RtlProcessHeap(),
                                                  0,
                                                  NtFileLength + sizeof(UNICODE_NULL) );
                if (! FullNtPathName) {
                    ReturnValue = FALSE;
                    leave;
                }

                RtlCopyMemory( FullNtPathName,
                               RtlpDosDevicesPrefix.Buffer,
                               RtlpDosDevicesPrefix.Length );

                RtlCopyMemory( (PUCHAR)FullNtPathName + RtlpDosDevicesPrefix.Length,
                               UnicodeString.Buffer + 4,
                               UnicodeString.Length - 8 );
                FullNtPathName[NtFileLength >> 1] = UNICODE_NULL;

                if (ARGUMENT_PRESENT(RelativeName)) {
                    RelativeName->RelativeName.Length = 0;
                    RelativeName->RelativeName.MaximumLength = 0;
                    RelativeName->RelativeName.Buffer = 0;
                    RelativeName->ContainingDirectory = NULL;
                    RelativeName->CurDirRef = NULL;
                }

                if (ARGUMENT_PRESENT(FilePart)) {
                    PWSTR Source = &FullNtPathName[(NtFileLength - 1) >> 1];
                    PWSTR Dest = NULL;
                    while (Source > FullNtPathName) {
                        if (*Source == L'\\') {
                            Dest = Source + 1;
                            break;
                        }
                        Source--;
                    }
                    *FilePart = (Dest && *Dest) ? Dest : NULL;
                }

                NtFileName->Buffer = FullNtPathName;
                NtFileName->Length = (USHORT)(NtFileLength);
                NtFileName->MaximumLength = (USHORT)(NtFileLength + sizeof(UNICODE_NULL));
                ReturnValue = TRUE;
                leave;
            }
            ULONG DosPathLength = (DOS_MAX_PATH_LENGTH << 1);
            BOOLEAN NameInvalid;
            RTL_PATH_TYPE InputDosPathType;
            ULONG FullDosPathNameLength = RtlGetFullPathName_Ustr( &UnicodeString,
                                                                   DosPathLength,
                                                                   FullDosPathName,
                                                                   FilePart,
                                                                   &NameInvalid,
                                                                   &InputDosPathType );
            if (NameInvalid || !FullDosPathNameLength || FullDosPathNameLength > DosPathLength) {
                ReturnValue = FALSE;
                leave;
            }

            UNICODE_STRING Prefix = RtlpDosDevicesPrefix;
            ULONG DosPathNameOffset = 0;
            switch (RtlDetermineDosPathNameType_U( FullDosPathName )) {
            case RtlPathTypeUncAbsolute:
                Prefix = RtlpDosDevicesUncPrefix;
                DosPathNameOffset = 2;
                break;
            case RtlPathTypeLocalDevice:
                DosPathNameOffset = 4;
                break;
            case RtlPathTypeDriveAbsolute:
            case RtlPathTypeDriveRelative:
            case RtlPathTypeRooted:
            case RtlPathTypeRelative:
                break;
            case RtlPathTypeRootLocalDevice:
            default:
                ASSERT(FALSE);
            }

            RtlCopyMemory( FullNtPathName,
                           Prefix.Buffer,
                           Prefix.Length );
            RtlCopyMemory( (PUCHAR)FullNtPathName+Prefix.Length,
                           FullDosPathName + DosPathNameOffset,
                           FullDosPathNameLength - (DosPathNameOffset << 1) );

            NtFileName->Buffer = FullNtPathName;
            NtFileName->Length = (USHORT)(FullDosPathNameLength-(DosPathNameOffset << 1))+Prefix.Length;
            NtFileName->MaximumLength = (USHORT)BufferLength;        
            ULONG LastCharacter = NtFileName->Length >> 1;
            FullNtPathName[LastCharacter] = UNICODE_NULL;

            if (ARGUMENT_PRESENT(FilePart)) {
                if (*FilePart) {
                    UNICODE_STRING UnicodeFilePart;
                    if (! NT_SUCCESS(RtlInitUnicodeStringEx( &UnicodeFilePart,
                                                             *FilePart ))) {
                        ReturnValue = FALSE;
                        leave;
                    }
                    *FilePart = &FullNtPathName[LastCharacter] - (UnicodeFilePart.Length >> 1);
                }
            }
            if (ARGUMENT_PRESENT(RelativeName)) {
                RelativeName->RelativeName.Length = 0;
                RelativeName->RelativeName.MaximumLength = 0;
                RelativeName->RelativeName.Buffer = 0;
                RelativeName->ContainingDirectory = NULL;
                RelativeName->CurDirRef = NULL;
                if (InputDosPathType == RtlPathTypeRelative) {
                    PCURDIR CurDir = &NtCurrentPeb()->ProcessParameters->CurrentDirectory;
                    if (CurDir->Handle) {
                        UNICODE_STRING FullDosPathString;
                        if (! NT_SUCCESS(RtlInitUnicodeStringEx( &FullDosPathString,
                                                                 FullDosPathName ))) {
                            ReturnValue = FALSE;
                            leave;
                        }
                        if (CurDir->DosPath.Length <= FullDosPathString.Length) {
                            FullDosPathString.Length = CurDir->DosPath.Length;
                            if (RtlEqualUnicodeString( (PUNICODE_STRING)&CurDir->DosPath,
                                                       &FullDosPathString,
                                                       TRUE )) {
                                RelativeName->RelativeName.Buffer = (PWSTR)((PCHAR)FullNtPathName + Prefix.Length - (DosPathNameOffset << 1) + CurDir->DosPath.Length);
                                RelativeName->RelativeName.Length = (USHORT)FullDosPathNameLength - CurDir->DosPath.Length;
                                if (*RelativeName->RelativeName.Buffer == L'\\') {
                                    RelativeName->RelativeName.Buffer += 1;
                                    RelativeName->RelativeName.Length -= sizeof(WCHAR);
                                }
                                RelativeName->RelativeName.MaximumLength = RelativeName->RelativeName.Length;
                                if (CaptureRelativeName) {
                                    ASSERT(RtlpCurDirRef);
                                    ASSERT(RtlpCurDirRef->DirectoryHandle == CurDir->Handle);
                                    RelativeName->CurDirRef = RtlpCurDirRef;
                                    RtlpReferenceCurDirRef( RtlpCurDirRef );
                                }
                                RelativeName->ContainingDirectory = CurDir->Handle;
                            }
                        }
                    }
                }
            }
        } except (EXCEPTION_EXECUTE_HANDLER) {
            ReturnValue = FALSE;
        }
    } finally {
        RtlReleasePebLock();
        if (ReturnValue == FALSE && FullNtPathName) {
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         FullNtPathName );
        }
    }
    return ReturnValue;
}

BOOLEAN
NTAPI
RtlDosPathNameToNtPathName_U (
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Reserved_ PVOID Reserved
    )
{
    UNICODE_STRING DosFileNameString;
    SIZE_T Length = 0;

    PAGED_CODE();

    if (DosFileName) {
        Length = wcslen( DosFileName ) * sizeof( WCHAR );
        if (Length + sizeof(UNICODE_NULL) >= UNICODE_STRING_MAX_BYTES) {
            return FALSE;
        }
        DosFileNameString.MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
    } else {
        DosFileNameString.MaximumLength = 0;
    }

    DosFileNameString.Buffer = (PWSTR) DosFileName;
    DosFileNameString.Length = (USHORT)Length;

    return RtlpDosPathNameToRelativeNtPathName_Ustr( FALSE,
                                                     &DosFileNameString,
                                                     NtFileName,
                                                     FilePart,
                                                     (PRTL_RELATIVE_NAME_U)Reserved );
}

BOOLEAN
NTAPI
RtlDosPathNameToRelativeNtPathName_U (
    _In_ PCWSTR DosFileName,
    _Out_ PUNICODE_STRING NtFileName,
    _Out_opt_ PWSTR *FilePart,
    _Out_ PRTL_RELATIVE_NAME_U RelativeName
    )
{    
    UNICODE_STRING DosFileNameString;
    SIZE_T Length = 0;

    PAGED_CODE();

    if (DosFileName) {
        Length = wcslen( DosFileName ) * sizeof( WCHAR );
        if (Length + sizeof(UNICODE_NULL) >= UNICODE_STRING_MAX_BYTES) {
            return FALSE;
        }
        DosFileNameString.MaximumLength = (USHORT)(Length + sizeof(UNICODE_NULL));
    } else {
        DosFileNameString.MaximumLength = 0;
    }

    DosFileNameString.Buffer = (PWSTR) DosFileName;
    DosFileNameString.Length = (USHORT)Length;

    return RtlpDosPathNameToRelativeNtPathName_Ustr( TRUE,
                                                     &DosFileNameString,
                                                     NtFileName,
                                                     FilePart,
                                                     RelativeName );
}

BOOLEAN
RtlDoesFileExists_UstrEx (
    _In_ PCUNICODE_STRING FileName,
    _In_ BOOLEAN TreatDeniedOrSharingAsHit
    )
{
    UNICODE_STRING NtFileName;
    RTL_RELATIVE_NAME_U RelativeName;

    PAGED_CODE();

    if (! RtlpDosPathNameToRelativeNtPathName_Ustr( TRUE,
                                                    FileName,
                                                    &NtFileName,
                                                    NULL,
                                                    &RelativeName )) {
        return FALSE;
    }

    PVOID FreeBuffer = NtFileName.Buffer;
    if (RelativeName.RelativeName.Length) {
        NtFileName = RelativeName.RelativeName;
    } else {
        RelativeName.ContainingDirectory = NULL;
    }

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    FILE_NETWORK_OPEN_INFORMATION FileInfo;
    InitializeObjectAttributes( &ObjectAttributes,
                                &NtFileName,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                RelativeName.ContainingDirectory,
                                NULL );
    Status = ZwQueryFullAttributesFile( &ObjectAttributes,
                                        &FileInfo );
    RtlReleaseRelativeName( &RelativeName );
    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 FreeBuffer );

    if (NT_SUCCESS(Status)) {
        return TRUE;
    }
    if (Status == STATUS_SHARING_VIOLATION || Status == STATUS_ACCESS_DENIED) {
        return TreatDeniedOrSharingAsHit;
    }
    return FALSE;
}

BOOLEAN
NTAPI
RtlDoesFileExists_U (
    _In_ PCWSTR FileName
    )
{
    UNICODE_STRING Unicode;

    PAGED_CODE();

    if (NT_SUCCESS(RtlInitUnicodeStringEx( &Unicode,
                                           FileName ))) {
        return RtlDoesFileExists_UstrEx( &Unicode,
                                         TRUE );
    }
    return FALSE;
}

ULONG
NTAPI
RtlDosSearchPath_U (
    _In_ PCWSTR lpPath,
    _In_ PCWSTR lpFileName,
    _In_opt_ PCWSTR lpExtension,
    _In_ ULONG nBufferLength,
    _Out_ PWSTR lpBuffer,
    _Out_ PWSTR *lpFilePart
    )
{
    PAGED_CODE();

    if (RtlDetermineDosPathNameType_U( lpFileName ) != RtlPathTypeRelative) {
        UNICODE_STRING FileName;
        RtlInitUnicodeString( &FileName,
                              lpFileName );
        if (! RtlDoesFileExists_UstrEx( &FileName,
                                        TRUE ) ) {
            return 0;
        }
        return RtlGetFullPathName_U( lpFileName,
                                     nBufferLength,
                                     lpBuffer,
                                     lpFilePart );
    }

    UNICODE_STRING Unicode;
    NTSTATUS Status;
    ULONG ExtensionLength = 1;
    PCWSTR p = lpFileName;
    while (*p) {
        if (*p == L'.') {
            ExtensionLength = 0;
            break;
        }
        p++;
    }
    if (ExtensionLength) {
        if (ARGUMENT_PRESENT(lpExtension)) {
            Status = RtlInitUnicodeStringEx( &Unicode,
                                             lpExtension );
            if (! NT_SUCCESS(Status)) {
                return 0;
            }
            ExtensionLength = Unicode.Length;
        } else {
            ExtensionLength = 0;
        }
    }

    Status = RtlInitUnicodeStringEx( &Unicode,
                                     lpPath );
    if (! NT_SUCCESS(Status)) {
        return 0;
    }
    ULONG PathLength = Unicode.Length;

    Status = RtlInitUnicodeStringEx( &Unicode,
                                     lpFileName );
    if (! NT_SUCCESS(Status)) {
        return 0;
    }
    ULONG FileLength = Unicode.Length;
    UNICODE_STRING FileName;
    FileName.MaximumLength = (USHORT)(PathLength + FileLength + ExtensionLength + 3 * sizeof(UNICODE_NULL));
    FileName.Buffer = RtlAllocateHeap( RtlProcessHeap(),
                                       0,
                                       FileName.MaximumLength );
    if (! FileName.Buffer) {
        return 0;
    }

    PWSTR Cursor;
    do {
        Cursor = FileName.Buffer;
        while (*lpPath) {
            if (*lpPath == L';') {
                lpPath++;
                break;
            }
            *Cursor++ = *lpPath++;
        }
        if (Cursor != FileName.Buffer && Cursor [ -1 ] != L'\\' ) {
            *Cursor++ = L'\\';
        }
        if (*lpPath == UNICODE_NULL) {
            lpPath = NULL;
        }
        RtlCopyMemory( Cursor,
                       lpFileName,
                       FileLength );
        if (ExtensionLength) {            
            RtlCopyMemory( Add2Ptr(Cursor, FileLength),
                           lpExtension,
                           ExtensionLength + sizeof(UNICODE_NULL) );
            FileName.Length = (USHORT)(((Cursor - FileName.Buffer) * sizeof(WCHAR)) + FileLength + ExtensionLength);
        } else {
            *(PWSTR)(Add2Ptr(Cursor, FileLength)) = UNICODE_NULL;
            FileName.Length = (USHORT)(((Cursor - FileName.Buffer) * sizeof(WCHAR)) + FileLength);
        }

        if (RtlDoesFileExists_UstrEx( &FileName,
                                      FALSE )) {
            RTL_PATH_TYPE PathType;
            PathLength = RtlGetFullPathName_Ustr( &FileName,
                                                  nBufferLength,
                                                  lpBuffer,
                                                  lpFilePart,
                                                  NULL,
                                                  &PathType );
            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         FileName.Buffer );
            return PathLength;
        }
    } while (lpPath);

    RtlFreeHeap( RtlProcessHeap(),
                 0,
                 FileName.Buffer );
    return 0;
}