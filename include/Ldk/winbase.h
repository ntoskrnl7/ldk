#pragma once

#ifndef _WINBASE_
#define _WINBASE_

#include "minwinbase.h"
#include "debugapi.h"
#include "errhandlingapi.h"
#include "handleapi.h"
#include "processthreadsapi.h"
#include "profileapi.h"
#include "synchapi.h"
#include "sysinfoapi.h"
#include "libloaderapi.h"
#include "utilapiset.h"
#include "threadpoolapiset.h"
#include "threadpoollegacyapiset.h"
#include "stringapiset.h"
#include "fibersapi.h"
#include "fileapi.h"
#include "heapapi.h"
#include "memoryapi.h"
#include "processenv.h"
#include "consoleapi.h"
#include "consoleapi2.h"
#include "timezoneapi.h"
#include "namepipeapi.h"

#include <winerror.h>

EXTERN_C_START

#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6


#define FILE_TYPE_UNKNOWN   0x0000
#define FILE_TYPE_DISK      0x0001
#define FILE_TYPE_CHAR      0x0002
#define FILE_TYPE_PIPE      0x0003
#define FILE_TYPE_REMOTE    0x8000


#define STD_INPUT_HANDLE    ((DWORD)-10)
#define STD_OUTPUT_HANDLE   ((DWORD)-11)
#define STD_ERROR_HANDLE    ((DWORD)-12)


#define INFINITE            0xFFFFFFFF  // Infinite timeout



#define CREATE_SUSPENDED				0x00000004

#define Yield()

#define FILE_BEGIN           0
#define FILE_CURRENT         1
#define FILE_END             2

#define WAIT_FAILED ((DWORD)0xFFFFFFFF)
#define WAIT_OBJECT_0       ((STATUS_WAIT_0 ) + 0 )

#define WAIT_ABANDONED         ((STATUS_ABANDONED_WAIT_0 ) + 0 )
#define WAIT_ABANDONED_0       ((STATUS_ABANDONED_WAIT_0 ) + 0 )

#define WAIT_IO_COMPLETION                  STATUS_USER_APC

#define SecureZeroMemory RtlSecureZeroMemory
#define CaptureStackBackTrace RtlCaptureStackBackTrace

//
// File creation flags must start at the high end since they
// are combined with the attributes
//

//
//  These are flags supported through CreateFile (W7) and CreateFile2 (W8 and beyond)
//

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_SESSION_AWARE         0x00800000
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#ifndef FILE_NAME_NORMALIZED
#define FILE_NAME_NORMALIZED 0x0
#endif

#ifndef FILE_NAME_OPENED
#define FILE_NAME_OPENED 0x8
#endif

#ifndef VOLUME_NAME_DOS
#define VOLUME_NAME_DOS 0x0
#endif

#ifndef VOLUME_NAME_GUID
#define VOLUME_NAME_GUID 0x1
#endif

#ifndef VOLUME_NAME_NT
#define VOLUME_NAME_NT 0x2
#endif

#ifndef VOLUME_NAME_NONE
#define VOLUME_NAME_NONE 0x4
#endif

#ifndef MOVEFILE_REPLACE_EXISTING
#define MOVEFILE_REPLACE_EXISTING 0x00000001
#endif

#ifndef MOVEFILE_COPY_ALLOWED
#define MOVEFILE_COPY_ALLOWED 0x00000002
#endif

#ifndef MOVEFILE_DELAY_UNTIL_REBOOT
#define MOVEFILE_DELAY_UNTIL_REBOOT 0x00000004
#endif

#ifndef MOVEFILE_WRITE_THROUGH
#define MOVEFILE_WRITE_THROUGH 0x00000008
#endif

#ifndef MOVEFILE_CREATE_HARDLINK
#define MOVEFILE_CREATE_HARDLINK 0x00000010
#endif

#ifndef MOVEFILE_FAIL_IF_NOT_TRACKABLE
#define MOVEFILE_FAIL_IF_NOT_TRACKABLE 0x00000020
#endif

#ifndef FILE_DISPOSITION_FLAG_DO_NOT_DELETE
#define FILE_DISPOSITION_FLAG_DO_NOT_DELETE 0x00000000
#define FILE_DISPOSITION_FLAG_DELETE 0x00000001
#define FILE_DISPOSITION_FLAG_POSIX_SEMANTICS 0x00000002
#define FILE_DISPOSITION_FLAG_FORCE_IMAGE_SECTION_CHECK 0x00000004
#define FILE_DISPOSITION_FLAG_ON_CLOSE 0x00000008
#define FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE 0x00000010
#endif

#ifndef FILE_RENAME_FLAG_REPLACE_IF_EXISTS
#define FILE_RENAME_FLAG_REPLACE_IF_EXISTS 0x00000001
#define FILE_RENAME_FLAG_POSIX_SEMANTICS 0x00000002
#define FILE_RENAME_FLAG_SUPPRESS_PIN_STATE_INHERITANCE 0x00000004
#endif

#ifndef SYMBOLIC_LINK_FLAG_DIRECTORY
#define SYMBOLIC_LINK_FLAG_DIRECTORY 0x1
#endif

#ifndef SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE
#define SYMBOLIC_LINK_FLAG_ALLOW_UNPRIVILEGED_CREATE 0x2
#endif

typedef struct _FILE_BASIC_INFO {
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    DWORD FileAttributes;
} FILE_BASIC_INFO, *PFILE_BASIC_INFO;

typedef struct _FILE_STANDARD_INFO {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    DWORD NumberOfLinks;
    BOOLEAN DeletePending;
    BOOLEAN Directory;
} FILE_STANDARD_INFO, *PFILE_STANDARD_INFO;

typedef struct _FILE_NAME_INFO {
    DWORD FileNameLength;
    WCHAR FileName[1];
} FILE_NAME_INFO, *PFILE_NAME_INFO;

typedef struct _FILE_DISPOSITION_INFO {
    BOOLEAN DeleteFile;
} FILE_DISPOSITION_INFO, *PFILE_DISPOSITION_INFO;

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable:4201)
#endif
typedef struct _FILE_RENAME_INFO {
    union {
        BOOLEAN ReplaceIfExists;
        DWORD Flags;
    } DUMMYUNIONNAME;
    HANDLE RootDirectory;
    DWORD FileNameLength;
    WCHAR FileName[1];
} FILE_RENAME_INFO, *PFILE_RENAME_INFO;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

typedef struct _FILE_ALLOCATION_INFO {
    LARGE_INTEGER AllocationSize;
} FILE_ALLOCATION_INFO, *PFILE_ALLOCATION_INFO;

typedef struct _FILE_DISPOSITION_INFO_EX {
    DWORD Flags;
} FILE_DISPOSITION_INFO_EX, *PFILE_DISPOSITION_INFO_EX;

typedef struct _FILE_END_OF_FILE_INFO {
    LARGE_INTEGER EndOfFile;
} FILE_END_OF_FILE_INFO, *PFILE_END_OF_FILE_INFO;

typedef enum _PRIORITY_HINT {
    IoPriorityHintVeryLow = 0,
    IoPriorityHintLow,
    IoPriorityHintNormal,
    MaximumIoPriorityHintType
} PRIORITY_HINT;

typedef struct _FILE_IO_PRIORITY_HINT_INFO {
    PRIORITY_HINT PriorityHint;
} FILE_IO_PRIORITY_HINT_INFO, *PFILE_IO_PRIORITY_HINT_INFO;

#ifndef FILE_CS_FLAG_CASE_SENSITIVE_DIR
#define FILE_CS_FLAG_CASE_SENSITIVE_DIR 0x00000001
#endif

typedef struct _FILE_CASE_SENSITIVE_INFO {
    ULONG Flags;
} FILE_CASE_SENSITIVE_INFO, *PFILE_CASE_SENSITIVE_INFO;

typedef struct _FILE_STREAM_INFO {
    DWORD NextEntryOffset;
    DWORD StreamNameLength;
    LARGE_INTEGER StreamSize;
    LARGE_INTEGER StreamAllocationSize;
    WCHAR StreamName[1];
} FILE_STREAM_INFO, *PFILE_STREAM_INFO;

#ifndef COMPRESSION_FORMAT_NONE
#define COMPRESSION_FORMAT_NONE 0x0000
#endif

typedef struct _FILE_COMPRESSION_INFO {
    LARGE_INTEGER CompressedFileSize;
    WORD CompressionFormat;
    UCHAR CompressionUnitShift;
    UCHAR ChunkShift;
    UCHAR ClusterShift;
    UCHAR Reserved[3];
} FILE_COMPRESSION_INFO, *PFILE_COMPRESSION_INFO;

typedef struct _FILE_ATTRIBUTE_TAG_INFO {
    DWORD FileAttributes;
    DWORD ReparseTag;
} FILE_ATTRIBUTE_TAG_INFO, *PFILE_ATTRIBUTE_TAG_INFO;

typedef struct _FILE_ID_INFO {
    ULONGLONG VolumeSerialNumber;
    FILE_ID_128 FileId;
} FILE_ID_INFO, *PFILE_ID_INFO;

typedef struct _FILE_ALIGNMENT_INFO {
    ULONG AlignmentRequirement;
} FILE_ALIGNMENT_INFO, *PFILE_ALIGNMENT_INFO;

#ifndef STORAGE_INFO_FLAGS_ALIGNED_DEVICE
#define STORAGE_INFO_FLAGS_ALIGNED_DEVICE                 0x00000001
#define STORAGE_INFO_FLAGS_PARTITION_ALIGNED_ON_DEVICE    0x00000002
#endif

#ifndef STORAGE_INFO_OFFSET_UNKNOWN
#define STORAGE_INFO_OFFSET_UNKNOWN 0xffffffff
#endif

typedef struct _FILE_STORAGE_INFO {
    ULONG LogicalBytesPerSector;
    ULONG PhysicalBytesPerSectorForAtomicity;
    ULONG PhysicalBytesPerSectorForPerformance;
    ULONG FileSystemEffectivePhysicalBytesPerSectorForAtomicity;
    ULONG Flags;
    ULONG ByteOffsetForSectorAlignment;
    ULONG ByteOffsetForPartitionAlignment;
} FILE_STORAGE_INFO, *PFILE_STORAGE_INFO;

#ifndef REMOTE_PROTOCOL_INFO_FLAG_LOOPBACK
#define REMOTE_PROTOCOL_INFO_FLAG_LOOPBACK              0x00000001
#define REMOTE_PROTOCOL_INFO_FLAG_OFFLINE               0x00000002
#define REMOTE_PROTOCOL_INFO_FLAG_PERSISTENT_HANDLE     0x00000004
#endif

#ifndef RPI_FLAG_SMB2_SHARECAP_TIMEWARP
#define RPI_FLAG_SMB2_SHARECAP_TIMEWARP                0x00000002
#define RPI_FLAG_SMB2_SHARECAP_DFS                     0x00000008
#define RPI_FLAG_SMB2_SHARECAP_CONTINUOUS_AVAILABILITY 0x00000010
#define RPI_FLAG_SMB2_SHARECAP_SCALEOUT                0x00000020
#define RPI_FLAG_SMB2_SHARECAP_CLUSTER                 0x00000040
#endif

#ifndef RPI_SMB2_SHAREFLAG_ENCRYPT_DATA
#define RPI_SMB2_SHAREFLAG_ENCRYPT_DATA                0x00000001
#define RPI_SMB2_SHAREFLAG_COMPRESS_DATA               0x00000002
#endif

#ifndef RPI_SMB2_FLAG_SERVERCAP_DFS
#define RPI_SMB2_FLAG_SERVERCAP_DFS                    0x00000001
#define RPI_SMB2_FLAG_SERVERCAP_LEASING                0x00000002
#define RPI_SMB2_FLAG_SERVERCAP_LARGEMTU               0x00000004
#define RPI_SMB2_FLAG_SERVERCAP_MULTICHANNEL           0x00000008
#define RPI_SMB2_FLAG_SERVERCAP_PERSISTENT_HANDLES     0x00000010
#define RPI_SMB2_FLAG_SERVERCAP_DIRECTORY_LEASING      0x00000020
#endif

typedef struct _FILE_REMOTE_PROTOCOL_INFO {
    USHORT StructureVersion;
    USHORT StructureSize;
    ULONG Protocol;
    USHORT ProtocolMajorVersion;
    USHORT ProtocolMinorVersion;
    USHORT ProtocolRevision;
    USHORT Reserved;
    ULONG Flags;
    struct {
        ULONG Reserved[8];
    } GenericReserved;
    union {
        struct {
            struct {
                ULONG Capabilities;
            } Server;
            struct {
                ULONG Capabilities;
#if defined(NTDDI_WIN10_NI) && (NTDDI_VERSION >= NTDDI_WIN10_NI)
                ULONG ShareFlags;
#else
                ULONG CachingFlags;
#endif
            } Share;
        } Smb2;
        ULONG Reserved[16];
    } ProtocolSpecific;
} FILE_REMOTE_PROTOCOL_INFO, *PFILE_REMOTE_PROTOCOL_INFO;

#ifndef FileBasicInfo
#define FileBasicInfo ((FILE_INFO_BY_HANDLE_CLASS)0)
#endif
#ifndef FileStandardInfo
#define FileStandardInfo ((FILE_INFO_BY_HANDLE_CLASS)1)
#endif
#ifndef FileNameInfo
#define FileNameInfo ((FILE_INFO_BY_HANDLE_CLASS)2)
#endif
#ifndef FileRenameInfo
#define FileRenameInfo ((FILE_INFO_BY_HANDLE_CLASS)3)
#endif
#ifndef FileDispositionInfo
#define FileDispositionInfo ((FILE_INFO_BY_HANDLE_CLASS)4)
#endif
#ifndef FileAllocationInfo
#define FileAllocationInfo ((FILE_INFO_BY_HANDLE_CLASS)5)
#endif
#ifndef FileEndOfFileInfo
#define FileEndOfFileInfo ((FILE_INFO_BY_HANDLE_CLASS)6)
#endif
#ifndef FileStreamInfo
#define FileStreamInfo ((FILE_INFO_BY_HANDLE_CLASS)7)
#endif
#ifndef FileCompressionInfo
#define FileCompressionInfo ((FILE_INFO_BY_HANDLE_CLASS)8)
#endif
#ifndef FileAttributeTagInfo
#define FileAttributeTagInfo ((FILE_INFO_BY_HANDLE_CLASS)9)
#endif
#ifndef FileRemoteProtocolInfo
#define FileRemoteProtocolInfo ((FILE_INFO_BY_HANDLE_CLASS)13)
#endif
#ifndef FileStorageInfo
#define FileStorageInfo ((FILE_INFO_BY_HANDLE_CLASS)16)
#endif
#ifndef FileAlignmentInfo
#define FileAlignmentInfo ((FILE_INFO_BY_HANDLE_CLASS)17)
#endif
#ifndef FileIdInfo
#define FileIdInfo ((FILE_INFO_BY_HANDLE_CLASS)18)
#endif
#ifndef FileDispositionInfoEx
#define FileDispositionInfoEx ((FILE_INFO_BY_HANDLE_CLASS)21)
#endif
#ifndef FileRenameInfoEx
#define FileRenameInfoEx ((FILE_INFO_BY_HANDLE_CLASS)22)
#endif
#ifndef FileCaseSensitiveInfo
#define FileCaseSensitiveInfo ((FILE_INFO_BY_HANDLE_CLASS)23)
#endif
#ifndef FileNormalizedNameInfo
#define FileNormalizedNameInfo ((FILE_INFO_BY_HANDLE_CLASS)24)
#endif

#define LDK_HAS_WINBASE_FILE_INFO_TYPES 1

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8)

//
//  These are flags supported only through CreateFile2 (W8 and beyond)
//
//  Due to the multiplexing of file creation flags, file attribute flags and
//  security QoS flags into a single DWORD (dwFlagsAndAttributes) parameter for
//  CreateFile, there is no way to add any more flags to CreateFile. Additional
//  flags for the create operation must be added to CreateFile2 only
//

#define FILE_FLAG_OPEN_REQUIRING_OPLOCK 0x00040000

#endif



//
// Define the Security Quality of Service bits to be passed
// into CreateFile
//

#define SECURITY_ANONYMOUS          ( SecurityAnonymous      << 16 )
#define SECURITY_IDENTIFICATION     ( SecurityIdentification << 16 )
#define SECURITY_IMPERSONATION      ( SecurityImpersonation  << 16 )
#define SECURITY_DELEGATION         ( SecurityDelegation     << 16 )

#define SECURITY_CONTEXT_TRACKING  0x00040000
#define SECURITY_EFFECTIVE_ONLY    0x00080000

#define SECURITY_SQOS_PRESENT      0x00100000
#define SECURITY_VALID_SQOS_FLAGS  0x001F0000



#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100

WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageA(
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) != 0, _At_((LPSTR*)lpBuffer, _Outptr_result_z_))
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) == 0, _Out_writes_z_(nSize))
             LPSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    );
WINBASEAPI
_Success_(return != 0)
DWORD
WINAPI
FormatMessageW(
    _In_     DWORD dwFlags,
    _In_opt_ LPCVOID lpSource,
    _In_     DWORD dwMessageId,
    _In_     DWORD dwLanguageId,
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) != 0, _At_((LPWSTR*)lpBuffer, _Outptr_result_z_))
    _When_((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) == 0, _Out_writes_z_(nSize))
             LPWSTR lpBuffer,
    _In_     DWORD nSize,
    _In_opt_ va_list *Arguments
    );

#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_FROM_STRING     0x00000400
#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0x00002000
#define FORMAT_MESSAGE_MAX_WIDTH_MASK  0x000000FF

WINBASEAPI
BOOL
WINAPI
CopyFileW(
    _In_ LPCWSTR lpExistingFileName,
    _In_ LPCWSTR lpNewFileName,
    _In_ BOOL bFailIfExists
    );

WINBASEAPI
BOOL
WINAPI
CreateDirectoryExW(
    _In_ LPCWSTR lpTemplateDirectory,
    _In_ LPCWSTR lpNewDirectory,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBASEAPI
BOOL
WINAPI
CreateHardLinkW(
    _In_ LPCWSTR lpFileName,
    _In_ LPCWSTR lpExistingFileName,
    _Reserved_ LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBASEAPI
BOOL
WINAPI
MoveFileExW(
    _In_ LPCWSTR lpExistingFileName,
    _In_opt_ LPCWSTR lpNewFileName,
    _In_ DWORD dwFlags
    );

WINBASEAPI
BOOLEAN
WINAPI
CreateSymbolicLinkW(
    _In_ LPCWSTR lpSymlinkFileName,
    _In_ LPCWSTR lpTargetFileName,
    _In_ DWORD dwFlags
    );



WINBASEAPI
_Success_(return != NULL)
_Post_writable_byte_size_(uBytes)
DECLSPEC_ALLOCATOR
HLOCAL
WINAPI
LocalAlloc(
    _In_ UINT uFlags,
    _In_ SIZE_T uBytes
    );

WINBASEAPI
_Ret_reallocated_bytes_(hMem, uBytes)
DECLSPEC_ALLOCATOR
HLOCAL
WINAPI
LocalReAlloc(
    _Frees_ptr_opt_ HLOCAL hMem,
    _In_ SIZE_T uBytes,
    _In_ UINT uFlags
    );

WINBASEAPI
_Ret_maybenull_
LPVOID
WINAPI
LocalLock(
    _In_ HLOCAL hMem
    );

WINBASEAPI
_Ret_maybenull_
HLOCAL
WINAPI
LocalHandle(
    _In_ LPCVOID pMem
    );

WINBASEAPI
BOOL
WINAPI
LocalUnlock(
    _In_ HLOCAL hMem
    );

WINBASEAPI
SIZE_T
WINAPI
LocalSize(
    _In_ HLOCAL hMem
    );

WINBASEAPI
_Success_(return==0)
_Ret_maybenull_
HLOCAL
WINAPI
LocalFree(
    _Frees_ptr_opt_ HLOCAL hMem
    );



#define SEM_FAILCRITICALERRORS      0x0001
#define SEM_NOGPFAULTERRORBOX       0x0002
#define SEM_NOALIGNMENTFAULTEXCEPT  0x0004
#define SEM_NOOPENFILEERRORBOX      0x8000



//
// Priority flags
//

#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (MAXLONG)

#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE

#define THREAD_MODE_BACKGROUND_BEGIN    0x00010000
#define THREAD_MODE_BACKGROUND_END      0x00020000



//
// Dual Mode API below this line. Dual Mode Structures also included.
//

#define STARTF_USESHOWWINDOW       0x00000001
#define STARTF_USESIZE             0x00000002
#define STARTF_USEPOSITION         0x00000004
#define STARTF_USECOUNTCHARS       0x00000008
#define STARTF_USEFILLATTRIBUTE    0x00000010
#define STARTF_RUNFULLSCREEN       0x00000020  // ignored for non-x86 platforms
#define STARTF_FORCEONFEEDBACK     0x00000040
#define STARTF_FORCEOFFFEEDBACK    0x00000080
#define STARTF_USESTDHANDLES       0x00000100

#if(WINVER >= 0x0400)

#define STARTF_USEHOTKEY           0x00000200
#define STARTF_TITLEISLINKNAME     0x00000800
#define STARTF_TITLEISAPPID        0x00001000
#define STARTF_PREVENTPINNING      0x00002000
#endif /* WINVER >= 0x0400 */

#if(WINVER >= 0x0600)
#define STARTF_UNTRUSTEDSOURCE     0x00008000
#endif /* WINVER >= 0x0600 */


#if (NTDDI_VERSION >= NTDDI_WIN10_FE)
     #define STARTF_HOLOGRAPHIC    0x00040000
#endif // (NTDDI_VERSION >= NTDDI_WIN10_FE)

EXTERN_C_END

#endif // _WINBASE_
