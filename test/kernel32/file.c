#if _KERNEL_MODE
#include <Ldk/Windows.h>

BOOLEAN
FileTest (
    VOID
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FileTest)
#endif

#define stdout DPFLTR_INFO_LEVEL
#define stderr DPFLTR_ERROR_LEVEL
#define fprintf(_f_, ...)   (DbgPrintEx(DPFLTR_IHVDRIVER_ID, _f_, __VA_ARGS__))
#define printf(...)         (fprintf(stdout, __VA_ARGS__))
#else
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#define PAGED_CODE()
#endif

typedef struct _FILE_TEST_COPYFILE2_CONTEXT {
    LONG StreamStarted;
    LONG StreamFinished;
    LONG ChunkFinished;
} FILE_TEST_COPYFILE2_CONTEXT, *PFILE_TEST_COPYFILE2_CONTEXT;

typedef struct _FILE_TEST_STAT_BASIC_INFORMATION {
    LARGE_INTEGER FileId;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    DWORD FileAttributes;
    DWORD ReparseTag;
    DWORD NumberOfLinks;
    DWORD DeviceType;
    DWORD DeviceCharacteristics;
    DWORD Reserved;
    LARGE_INTEGER VolumeSerialNumber;
    BYTE FileId128[16];
} FILE_TEST_STAT_BASIC_INFORMATION, *PFILE_TEST_STAT_BASIC_INFORMATION;

typedef struct _FILE_TEST_STAT_LX_INFORMATION {
    LARGE_INTEGER FileId;
    LARGE_INTEGER CreationTime;
    LARGE_INTEGER LastAccessTime;
    LARGE_INTEGER LastWriteTime;
    LARGE_INTEGER ChangeTime;
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    DWORD FileAttributes;
    DWORD ReparseTag;
    DWORD NumberOfLinks;
    ACCESS_MASK EffectiveAccess;
    DWORD LxFlags;
    DWORD LxUid;
    DWORD LxGid;
    DWORD LxMode;
    DWORD LxDeviceIdMajor;
    DWORD LxDeviceIdMinor;
} FILE_TEST_STAT_LX_INFORMATION, *PFILE_TEST_STAT_LX_INFORMATION;

#pragma warning(push)
#pragma warning(disable:4324)
typedef struct _FILE_TEST_WORKSPACE {
    CHAR PathBuffer[128];
    CHAR Buffer[4];
    DECLSPEC_ALIGN(8) UCHAR FileNameInfoBuffer[FIELD_OFFSET(FILE_NAME_INFO, FileName) + (MAX_PATH * sizeof(WCHAR))];
    DECLSPEC_ALIGN(8) UCHAR FileNormalizedNameInfoBuffer[FIELD_OFFSET(FILE_NAME_INFO, FileName) + (MAX_PATH * sizeof(WCHAR))];
    DECLSPEC_ALIGN(8) UCHAR FileStreamInfoBuffer[4096];
    DECLSPEC_ALIGN(8) UCHAR DirectoryInfoBuffer[4096];
    WCHAR FinalPath[MAX_PATH];
    CHAR TempPathA[MAX_PATH];
    WCHAR TempPathW[MAX_PATH];
    WCHAR TempFileNameW[MAX_PATH];
    CHAR TempFileNameA[MAX_PATH];
    WCHAR RenameTargetPath[MAX_PATH];
    DECLSPEC_ALIGN(8) UCHAR RenameInfoBuffer[FIELD_OFFSET(FILE_RENAME_INFO, FileName) + MAX_PATH * sizeof(WCHAR)];
} FILE_TEST_WORKSPACE, *PFILE_TEST_WORKSPACE;
#pragma warning(pop)

typedef BOOL (WINAPI *GET_FILE_INFORMATION_BY_NAME_FN)(
    _In_ PCWSTR FileName,
    _In_ int FileInformationClass,
    _Out_writes_bytes_(FileInfoBufferSize) PVOID FileInfoBuffer,
    _In_ ULONG FileInfoBufferSize
    );

#ifndef FileIdExtdDirectoryRestartInfo
#define FileIdExtdDirectoryRestartInfo ((FILE_INFO_BY_HANDLE_CLASS)20)
#endif
#ifndef FileCaseSensitiveInfo
#define FileCaseSensitiveInfo ((FILE_INFO_BY_HANDLE_CLASS)23)
#endif

static
COPYFILE2_MESSAGE_ACTION
CALLBACK
FileTestCopyFile2Progress (
    _In_ const COPYFILE2_MESSAGE *Message,
    _In_opt_ PVOID CallbackContext
    )
{
    PFILE_TEST_COPYFILE2_CONTEXT Context = (PFILE_TEST_COPYFILE2_CONTEXT)CallbackContext;

    if (Context == NULL || Message == NULL) {
        return COPYFILE2_PROGRESS_CANCEL;
    }

    switch (Message->Type) {
    case COPYFILE2_CALLBACK_STREAM_STARTED:
        InterlockedIncrement( &Context->StreamStarted );
        break;
    case COPYFILE2_CALLBACK_STREAM_FINISHED:
        InterlockedIncrement( &Context->StreamFinished );
        break;
    case COPYFILE2_CALLBACK_CHUNK_FINISHED:
        InterlockedIncrement( &Context->ChunkFinished );
        break;
    default:
        break;
    }

    return COPYFILE2_PROGRESS_CONTINUE;
}

static
ULONG
FileTestWideByteLength (
    _In_z_ LPCWSTR Text
    )
{
    ULONG Length = 0;

    while (Text[Length] != UNICODE_NULL) {
        Length++;
    }

    return Length * sizeof(WCHAR);
}

static
BOOL
FileTestDirectoryInfoContains (
    _In_reads_bytes_(BufferSize) const UCHAR *Buffer,
    _In_ ULONG BufferSize,
    _In_ ULONG FileNameLengthOffset,
    _In_ ULONG FileNameOffset,
    _In_z_ LPCWSTR ExpectedName
    )
{
    ULONG Offset = 0;
    ULONG ExpectedNameLength = FileTestWideByteLength( ExpectedName );

    for (;;) {
        const UCHAR *Entry;
        ULONG NextEntryOffset;
        ULONG FileNameLength;

        if (Offset > BufferSize - sizeof(ULONG)) {
            return FALSE;
        }

        Entry = Buffer + Offset;
        if (FileNameLengthOffset > BufferSize - Offset - sizeof(ULONG) ||
            FileNameOffset > BufferSize - Offset) {
            return FALSE;
        }

        NextEntryOffset = *(const ULONG *)Entry;
        FileNameLength = *(const ULONG *)(Entry + FileNameLengthOffset);
        if (FileNameLength <= BufferSize - Offset - FileNameOffset &&
            FileNameLength == ExpectedNameLength &&
            RtlCompareMemory( Entry + FileNameOffset,
                              ExpectedName,
                              ExpectedNameLength ) == ExpectedNameLength) {
            return TRUE;
        }

        if (NextEntryOffset == 0) {
            break;
        }

        if (NextEntryOffset > BufferSize - Offset) {
            return FALSE;
        }

        Offset += NextEntryOffset;
    }

    return FALSE;
}

BOOLEAN
FileTest (
    VOID
    )
{
    BOOL rv = TRUE;
    PFILE_TEST_WORKSPACE Workspace;

    PAGED_CODE();

    Workspace = HeapAlloc( GetProcessHeap(),
                           HEAP_ZERO_MEMORY,
                           sizeof(*Workspace) );
    if (Workspace == NULL) {
        fprintf(stderr, "[Failed] FileTest workspace allocation failed ErrorCode = %d\n",
                GetLastError());
        return FALSE;
    }

    printf("File Test\n");

    printf("Test GetDriveTypeA / GetDriveTypeW\n");
    if (GetDriveTypeA( "C:\\" ) != GetDriveTypeW( L"C:\\" )) {
        fprintf(stderr, "[Failed] GetDriveTypeA(C:\\) == GetDriveTypeW(C:\\)\n");
        rv = FALSE;
    }
    if (GetDriveTypeA( NULL ) != GetDriveTypeW( L"C:\\" )) {
        fprintf(stderr, "[Failed] GetDriveTypeA(NULL) == GetDriveTypeW(C:\\)\n");
        rv = FALSE;
    }
    if (rv) {
        printf("[Success] Test GetDriveTypeA / GetDriveTypeW\n\n");
    } else {
        printf("[Failed] Test GetDriveTypeA / GetDriveTypeW\n\n");
    }

    printf("Test CreateFileA(Test.tmp, CREATE_NEW)\n");
    DeleteFileW( L"Test.tmp" );
    HANDLE hFile = CreateFileA( "Test.tmp", GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateFileA(Test.tmp, CREATE_NEW) \n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CreateFileA(Test.tmp, CREATE_NEW) \n\n");


    printf("Test GetFullPathNameA(Test.tmp) \n");
    PSTR FilePart;
    if (GetFullPathNameA( "Test.tmp", sizeof(Workspace->PathBuffer), Workspace->PathBuffer, &FilePart ) == 0) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        rv = FALSE;
    }
    if (strcmp("Test.tmp", FilePart)) {
        fprintf(stderr, "[Failed] FilePart (Expect = Test.tmp, Actual = %s)\n", FilePart);
        rv = FALSE;
    }
    if (rv) {
        printf("[Success] Test GetFullPathNameA(Test.tmp) \n\n");
    } else {
        printf("[Failed] Test GetFullPathNameA(Test.tmp) \n\n");
    }

    
    printf("Test WriteFile(Test.tmp, 1234) \n");
    DWORD length;
    rv = WriteFile( hFile, "1234", 4, &length, NULL );
    CloseHandle( hFile );
    if (rv) {
        printf("[Success] Test WriteFile(Test.tmp, 1234) \n\n");
    } else {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test WriteFile(Test.tmp, 1234) \n\n");
        goto DeleteTest;
    }


    printf("Test CreateFileA(Test.tmp, OPEN_EXISTING) \n");
    hFile = CreateFileW( L"Test.tmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateFileA(Test.tmp, OPEN_EXISTING) \n\n");
        goto DeleteTest;
    }
    printf("[Success] Test CreateFileA(Test.tmp, OPEN_EXISTING) \n\n");


    printf("Test ReadFile(Test.tmp) \n");
    rv = ReadFile( hFile, Workspace->Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    
    if (!rv) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
    }
    if ((length != 4) || (memcmp(Workspace->Buffer, "1234", 4))) {
        fprintf(stderr, "[Failed] Expect = 4, 1234, Actual = %d %s\n", length, Workspace->Buffer);
        rv = FALSE;
    }
    if (rv) {
        printf("[Success] Test ReadFile(Test.tmp)\n\n");
    } else {
        printf("[Failed] Test ReadFile(Test.tmp)\n\n");
    }

    printf("Test CopyFileW(Test.tmp, TestCopy.tmp)\n");
    DeleteFileW( L"TestCopy.tmp" );
    rv = CopyFileW( L"Test.tmp", L"TestCopy.tmp", TRUE );
    if (!rv) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CopyFileW(Test.tmp, TestCopy.tmp)\n\n");
        goto DeleteTest;
    }

    hFile = CreateFileW( L"TestCopy.tmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CopyFileW(Test.tmp, TestCopy.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( Workspace->Buffer, sizeof(Workspace->Buffer) );
    length = 0;
    rv = ReadFile( hFile, Workspace->Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    if (!rv || length != 4 || memcmp(Workspace->Buffer, "1234", 4)) {
        fprintf(stderr, "[Failed] ErrorCode = %d, length = %d\n", GetLastError(), length);
        printf("[Failed] Test CopyFileW(Test.tmp, TestCopy.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CopyFileW(Test.tmp, TestCopy.tmp)\n\n");

    printf("Test CopyFileA / GetFileAttributesExA / SetFileAttributesA\n");
    DeleteFileW( L"TestCopyA.tmp" );
    rv = CopyFileA( "Test.tmp",
                    "TestCopyA.tmp",
                    TRUE );
    if (!rv) {
        fprintf(stderr, "[Failed] CopyFileA ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CopyFileA / GetFileAttributesExA / SetFileAttributesA\n\n");
        goto DeleteTest;
    }

    WIN32_FILE_ATTRIBUTE_DATA AttributeDataA;
    RtlZeroMemory( &AttributeDataA,
                   sizeof(AttributeDataA) );
    rv = GetFileAttributesExA( "TestCopyA.tmp",
                               GetFileExInfoStandard,
                               &AttributeDataA );
    if (!rv ||
        AttributeDataA.nFileSizeHigh != 0 ||
        AttributeDataA.nFileSizeLow != 4 ||
        FlagOn(AttributeDataA.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        fprintf(stderr,
                "[Failed] GetFileAttributesExA ErrorCode = %d Size = %lu:%lu Attributes = 0x%08lx\n",
                GetLastError(),
                AttributeDataA.nFileSizeHigh,
                AttributeDataA.nFileSizeLow,
                AttributeDataA.dwFileAttributes);
        printf("[Failed] Test CopyFileA / GetFileAttributesExA / SetFileAttributesA\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    rv = SetFileAttributesA( "TestCopyA.tmp",
                             FILE_ATTRIBUTE_NORMAL );
    if (!rv) {
        fprintf(stderr, "[Failed] SetFileAttributesA ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CopyFileA / GetFileAttributesExA / SetFileAttributesA\n\n");
        goto DeleteTest;
    }
    printf("[Success] Test CopyFileA / GetFileAttributesExA / SetFileAttributesA\n\n");

    printf("Test CreateFile2(TestCreateFile2.tmp)\n");
    DeleteFileW( L"TestCreateFile2.tmp" );
    CREATEFILE2_EXTENDED_PARAMETERS CreateFile2Parameters;
    RtlZeroMemory( &CreateFile2Parameters,
                   sizeof(CreateFile2Parameters) );
    CreateFile2Parameters.dwSize = sizeof(CreateFile2Parameters);
    CreateFile2Parameters.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;

    hFile = CreateFile2( L"TestCreateFile2.tmp",
                         GENERIC_WRITE,
                         0,
                         CREATE_NEW,
                         &CreateFile2Parameters );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] CreateFile2 ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateFile2(TestCreateFile2.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    length = 0;
    rv = WriteFile( hFile,
                    "5678",
                    4,
                    &length,
                    NULL );
    CloseHandle( hFile );
    if (!rv || length != 4) {
        fprintf(stderr, "[Failed] CreateFile2 WriteFile ErrorCode = %d Length = %lu\n",
                GetLastError(),
                length);
        printf("[Failed] Test CreateFile2(TestCreateFile2.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CreateFile2(TestCreateFile2.tmp)\n\n");

    printf("Test CopyFile2(Test.tmp, TestCopy2.tmp)\n");
    DeleteFileW( L"TestCopy2.tmp" );
    FILE_TEST_COPYFILE2_CONTEXT CopyFile2Context;
    COPYFILE2_EXTENDED_PARAMETERS CopyFile2Parameters;
    RtlZeroMemory( &CopyFile2Context,
                   sizeof(CopyFile2Context) );
    RtlZeroMemory( &CopyFile2Parameters,
                   sizeof(CopyFile2Parameters) );
    CopyFile2Parameters.dwSize = sizeof(CopyFile2Parameters);
    CopyFile2Parameters.dwCopyFlags = COPY_FILE_FAIL_IF_EXISTS;
    CopyFile2Parameters.pProgressRoutine = FileTestCopyFile2Progress;
    CopyFile2Parameters.pvCallbackContext = &CopyFile2Context;

    HRESULT CopyFile2Result = CopyFile2( L"Test.tmp",
                                         L"TestCopy2.tmp",
                                         &CopyFile2Parameters );
    if (CopyFile2Result != 0) {
        fprintf(stderr, "[Failed] CopyFile2 Result = 0x%08lx\n", CopyFile2Result);
        printf("[Failed] Test CopyFile2(Test.tmp, TestCopy2.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    if (CopyFile2Context.StreamStarted == 0 ||
        CopyFile2Context.StreamFinished == 0 ||
        CopyFile2Context.ChunkFinished == 0) {
        fprintf(stderr,
                "[Failed] CopyFile2 callback counts StreamStarted = %ld StreamFinished = %ld ChunkFinished = %ld\n",
                CopyFile2Context.StreamStarted,
                CopyFile2Context.StreamFinished,
                CopyFile2Context.ChunkFinished);
        printf("[Failed] Test CopyFile2(Test.tmp, TestCopy2.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    hFile = CreateFileW( L"TestCopy2.tmp", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] CopyFile2 output open ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CopyFile2(Test.tmp, TestCopy2.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( Workspace->Buffer, sizeof(Workspace->Buffer) );
    length = 0;
    rv = ReadFile( hFile, Workspace->Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    if (!rv || length != 4 || memcmp(Workspace->Buffer, "1234", 4)) {
        fprintf(stderr, "[Failed] CopyFile2 readback ErrorCode = %d Length = %lu\n",
                GetLastError(),
                length);
        printf("[Failed] Test CopyFile2(Test.tmp, TestCopy2.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    BOOL CopyFile2Cancel = TRUE;
    RtlZeroMemory( &CopyFile2Parameters,
                   sizeof(CopyFile2Parameters) );
    CopyFile2Parameters.dwSize = sizeof(CopyFile2Parameters);
    CopyFile2Parameters.pfCancel = &CopyFile2Cancel;
    CopyFile2Result = CopyFile2( L"Test.tmp",
                                 L"TestCopy2Cancel.tmp",
                                 &CopyFile2Parameters );
    if (CopyFile2Result == 0) {
        fprintf(stderr, "[Failed] CopyFile2 cancellation unexpectedly succeeded\n");
        printf("[Failed] Test CopyFile2(Test.tmp, TestCopy2.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CopyFile2(Test.tmp, TestCopy2.tmp)\n\n");

    printf("Test GetFileInformationByName(Test.tmp)\n");
    HMODULE Kernel32 = GetModuleHandleW( L"kernel32.dll" );
    GET_FILE_INFORMATION_BY_NAME_FN GetFileInformationByNameFn =
        Kernel32 != NULL ?
            (GET_FILE_INFORMATION_BY_NAME_FN)GetProcAddress( Kernel32,
                                                             "GetFileInformationByName" ) :
            NULL;
    if (GetFileInformationByNameFn == NULL) {
#if _KERNEL_MODE
        fprintf(stderr, "[Failed] GetProcAddress(GetFileInformationByName) ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test GetFileInformationByName(Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
#else
        printf("[Skipped] Test GetFileInformationByName(Test.tmp) export is unavailable\n\n");
#endif
    } else {
        FILE_TEST_STAT_LX_INFORMATION StatLxInfo;
        FILE_TEST_STAT_BASIC_INFORMATION StatBasicInfo;

        RtlZeroMemory( &StatLxInfo,
                       sizeof(StatLxInfo) );
        rv = GetFileInformationByNameFn( L"Test.tmp",
                                         1,
                                         &StatLxInfo,
                                         sizeof(StatLxInfo) );
        if (!rv) {
#if _KERNEL_MODE
            fprintf(stderr, "[Failed] GetFileInformationByName(FileStatLxByNameInfo) ErrorCode = %d\n", GetLastError());
            printf("[Failed] Test GetFileInformationByName(Test.tmp)\n\n");
            goto DeleteTest;
#else
            printf("[Skipped] GetFileInformationByName(FileStatLxByNameInfo) ErrorCode = %d\n", GetLastError());
#endif
        } else if (StatLxInfo.EndOfFile.QuadPart != 4 ||
                   FlagOn(StatLxInfo.FileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
            fprintf(stderr,
                    "[Failed] GetFileInformationByName FileStatLxByNameInfo Size = %I64d Attributes = 0x%08lx\n",
                    StatLxInfo.EndOfFile.QuadPart,
                    StatLxInfo.FileAttributes);
            printf("[Failed] Test GetFileInformationByName(Test.tmp)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }

        RtlZeroMemory( &StatBasicInfo,
                       sizeof(StatBasicInfo) );
        rv = GetFileInformationByNameFn( L"Test.tmp",
                                         3,
                                         &StatBasicInfo,
                                         sizeof(StatBasicInfo) );
        if (!rv) {
            fprintf(stderr, "[Failed] GetFileInformationByName ErrorCode = %d\n", GetLastError());
            printf("[Failed] Test GetFileInformationByName(Test.tmp)\n\n");
            goto DeleteTest;
        }

        if (StatBasicInfo.EndOfFile.QuadPart != 4 ||
            FlagOn(StatBasicInfo.FileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
            fprintf(stderr,
                    "[Failed] GetFileInformationByName size/attributes Size = %I64d Attributes = 0x%08lx\n",
                    StatBasicInfo.EndOfFile.QuadPart,
                    StatBasicInfo.FileAttributes);
            printf("[Failed] Test GetFileInformationByName(Test.tmp)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }
        printf("[Success] Test GetFileInformationByName(Test.tmp)\n\n");
    }

    printf("Test MoveFileExW(TestCopy.tmp, TestMoved.tmp)\n");
    DeleteFileW( L"TestMoved.tmp" );
    rv = MoveFileExW( L"TestCopy.tmp",
                      L"TestMoved.tmp",
                      MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING );
    if (!rv) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test MoveFileExW(TestCopy.tmp, TestMoved.tmp)\n\n");
        goto DeleteTest;
    }
    if (GetFileAttributesW( L"TestCopy.tmp" ) != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW( L"TestMoved.tmp" ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] TestCopy.tmp/TestMoved.tmp rename state is invalid\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test MoveFileExW(TestCopy.tmp, TestMoved.tmp)\n\n");

    printf("Test MoveFileA / MoveFileW / MoveFileExA\n");
    DeleteFileW( L"TestMoveA.tmp" );
    DeleteFileW( L"TestMoveATarget.tmp" );
    DeleteFileW( L"TestMoveW.tmp" );
    DeleteFileW( L"TestMoveWTarget.tmp" );
    DeleteFileW( L"TestMoveExA.tmp" );
    DeleteFileW( L"TestMoveExATarget.tmp" );

    hFile = CreateFileW( L"TestMoveA.tmp",
                         GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] CreateFileW(TestMoveA.tmp) ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    CloseHandle( hFile );
    hFile = INVALID_HANDLE_VALUE;
    rv = MoveFileA( "TestMoveA.tmp",
                    "TestMoveATarget.tmp" );
    if (!rv ||
        GetFileAttributesW( L"TestMoveA.tmp" ) != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW( L"TestMoveATarget.tmp" ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] MoveFileA ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test MoveFileA / MoveFileW / MoveFileExA\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    hFile = CreateFileW( L"TestMoveW.tmp",
                         GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] CreateFileW(TestMoveW.tmp) ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    CloseHandle( hFile );
    hFile = INVALID_HANDLE_VALUE;
    rv = MoveFileW( L"TestMoveW.tmp",
                    L"TestMoveWTarget.tmp" );
    if (!rv ||
        GetFileAttributesW( L"TestMoveW.tmp" ) != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW( L"TestMoveWTarget.tmp" ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] MoveFileW ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test MoveFileA / MoveFileW / MoveFileExA\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    hFile = CreateFileW( L"TestMoveExA.tmp",
                         GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         CREATE_NEW,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] CreateFileW(TestMoveExA.tmp) ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    CloseHandle( hFile );
    hFile = INVALID_HANDLE_VALUE;
    rv = MoveFileExA( "TestMoveExA.tmp",
                      "TestMoveExATarget.tmp",
                      MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED );
    if (!rv ||
        GetFileAttributesW( L"TestMoveExA.tmp" ) != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW( L"TestMoveExATarget.tmp" ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] MoveFileExA ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test MoveFileA / MoveFileW / MoveFileExA\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test MoveFileA / MoveFileW / MoveFileExA\n\n");

    printf("Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n");
    DeleteFileW( L"TestHardLink.tmp" );
    rv = CreateHardLinkW( L"TestHardLink.tmp",
                          L"Test.tmp",
                          NULL );
    if (!rv) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        goto DeleteTest;
    }

    printf("Test CreateHardLinkA(TestHardLinkA.tmp, Test.tmp)\n");
    DeleteFileW( L"TestHardLinkA.tmp" );
    rv = CreateHardLinkA( "TestHardLinkA.tmp",
                          "Test.tmp",
                          NULL );
    if (!rv ||
        GetFileAttributesW( L"TestHardLinkA.tmp" ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] CreateHardLinkA ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateHardLinkA(TestHardLinkA.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CreateHardLinkA(TestHardLinkA.tmp, Test.tmp)\n\n");

    printf("Test CreateFileW(Test.tmp:LdkStream)\n");
    BOOL StreamCreated = FALSE;
    HANDLE hStream = CreateFileW( L"Test.tmp:LdkStream",
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                  NULL,
                                  CREATE_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL );
    if (hStream == INVALID_HANDLE_VALUE) {
        printf("[Skipped] Test CreateFileW(Test.tmp:LdkStream) ErrorCode = %d\n\n",
               GetLastError());
    } else {
        DWORD StreamBytes;
        rv = WriteFile( hStream,
                        "ads",
                        3,
                        &StreamBytes,
                        NULL );
        CloseHandle( hStream );
        if (!rv || StreamBytes != 3) {
            fprintf(stderr,
                    "[Failed] Test CreateFileW(Test.tmp:LdkStream) ErrorCode = %d StreamBytes = %lu\n",
                    GetLastError(),
                    rv ? StreamBytes : 0);
            printf("[Failed] Test CreateFileW(Test.tmp:LdkStream)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }
        StreamCreated = TRUE;
        printf("[Success] Test CreateFileW(Test.tmp:LdkStream)\n\n");
    }

    hFile = CreateFileW( L"Test.tmp",
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    BY_HANDLE_FILE_INFORMATION HandleInfo;
    rv = GetFileInformationByHandle( hFile,
                                     &HandleInfo );
    printf("Test GetFileInformationByHandleEx(FileNameInfo / FileNormalizedNameInfo)\n");
    PFILE_NAME_INFO NameInfo = (PFILE_NAME_INFO)Workspace->FileNameInfoBuffer;
    PFILE_NAME_INFO NormalizedNameInfo = (PFILE_NAME_INFO)Workspace->FileNormalizedNameInfoBuffer;
    RtlZeroMemory( Workspace->FileNameInfoBuffer,
                   sizeof(Workspace->FileNameInfoBuffer) );
    if (! GetFileInformationByHandleEx( hFile,
                                        FileNameInfo,
                                        NameInfo,
                                        sizeof(Workspace->FileNameInfoBuffer) )) {
        fprintf(stderr,
                "[Failed] GetFileInformationByHandleEx(FileNameInfo) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileNameInfo / FileNormalizedNameInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    RtlZeroMemory( Workspace->FileNormalizedNameInfoBuffer,
                   sizeof(Workspace->FileNormalizedNameInfoBuffer) );
    if (! GetFileInformationByHandleEx( hFile,
                                        FileNormalizedNameInfo,
                                        NormalizedNameInfo,
                                        sizeof(Workspace->FileNormalizedNameInfoBuffer) )) {
        fprintf(stderr,
                "[Failed] GetFileInformationByHandleEx(FileNormalizedNameInfo) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileNameInfo / FileNormalizedNameInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("Test GetFileInformationByHandleEx(FileStreamInfo)\n");
    ULONG FileStreamInfoBufferSize = (ULONG)sizeof(Workspace->FileStreamInfoBuffer);
    ULONG FileStreamInfoHeaderSize = (ULONG)FIELD_OFFSET(FILE_STREAM_INFO, StreamName);
    PFILE_STREAM_INFO StreamInfo = (PFILE_STREAM_INFO)Workspace->FileStreamInfoBuffer;
    RtlZeroMemory( Workspace->FileStreamInfoBuffer,
                   sizeof(Workspace->FileStreamInfoBuffer) );
    if (! GetFileInformationByHandleEx( hFile,
                                        FileStreamInfo,
                                        StreamInfo,
                                        FileStreamInfoBufferSize )) {
        fprintf(stderr,
                "[Failed] GetFileInformationByHandleEx(FileStreamInfo) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileStreamInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    static const WCHAR DefaultStreamName[] = L"::$DATA";
    static const WCHAR TestStreamName[] = L":LdkStream:$DATA";
    ULONG DefaultStreamNameLength = (RTL_NUMBER_OF(DefaultStreamName) - 1) * sizeof(WCHAR);
    ULONG TestStreamNameLength = (RTL_NUMBER_OF(TestStreamName) - 1) * sizeof(WCHAR);
    ULONG StreamInfoOffset = 0;
    ULONG StreamCount = 0;
    BOOL FoundDefaultStream = FALSE;
    BOOL FoundTestStream = FALSE;
    for (;;) {
        PFILE_STREAM_INFO Entry;
        ULONG EntrySize;

        if (StreamInfoOffset > FileStreamInfoBufferSize -
            FileStreamInfoHeaderSize) {
            fprintf(stderr,
                    "[Failed] FileStreamInfo entry offset is outside the buffer\n");
            CloseHandle( hFile );
            printf("[Failed] Test GetFileInformationByHandleEx(FileStreamInfo)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }

        Entry = (PFILE_STREAM_INFO)(Workspace->FileStreamInfoBuffer + StreamInfoOffset);
        if (Entry->StreamNameLength == 0 ||
            (Entry->StreamNameLength % sizeof(WCHAR)) != 0) {
            fprintf(stderr,
                    "[Failed] FileStreamInfo returned invalid stream name length = %lu\n",
                    Entry->StreamNameLength);
            CloseHandle( hFile );
            printf("[Failed] Test GetFileInformationByHandleEx(FileStreamInfo)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }

        EntrySize = FileStreamInfoHeaderSize + Entry->StreamNameLength;
        if (EntrySize > FileStreamInfoBufferSize - StreamInfoOffset) {
            fprintf(stderr,
                    "[Failed] FileStreamInfo entry extends past the buffer\n");
            CloseHandle( hFile );
            printf("[Failed] Test GetFileInformationByHandleEx(FileStreamInfo)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }
        if (Entry->StreamSize.QuadPart < 0 ||
            Entry->StreamAllocationSize.QuadPart < 0) {
            fprintf(stderr,
                    "[Failed] FileStreamInfo returned negative stream sizes\n");
            CloseHandle( hFile );
            printf("[Failed] Test GetFileInformationByHandleEx(FileStreamInfo)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }

        StreamCount++;
        if (Entry->StreamNameLength == DefaultStreamNameLength &&
            RtlCompareMemory( Entry->StreamName,
                              DefaultStreamName,
                              DefaultStreamNameLength ) ==
                              DefaultStreamNameLength) {
            FoundDefaultStream = TRUE;
        }
        if (Entry->StreamNameLength == TestStreamNameLength &&
            RtlCompareMemory( Entry->StreamName,
                              TestStreamName,
                              TestStreamNameLength ) ==
                              TestStreamNameLength) {
            FoundTestStream = TRUE;
        }

        if (Entry->NextEntryOffset == 0) {
            break;
        }
        if (Entry->NextEntryOffset < EntrySize ||
            Entry->NextEntryOffset > FileStreamInfoBufferSize - StreamInfoOffset) {
            fprintf(stderr,
                    "[Failed] FileStreamInfo returned invalid next offset = %lu\n",
                    Entry->NextEntryOffset);
            CloseHandle( hFile );
            printf("[Failed] Test GetFileInformationByHandleEx(FileStreamInfo)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }
        StreamInfoOffset += Entry->NextEntryOffset;
    }

    if (StreamCount == 0 ||
        !FoundDefaultStream ||
        (StreamCreated && !FoundTestStream)) {
        fprintf(stderr,
                "[Failed] FileStreamInfo streams Count = %lu Default = %d Test = %d ExpectedTest = %d\n",
                StreamCount,
                FoundDefaultStream,
                FoundTestStream,
                StreamCreated);
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileStreamInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetFileInformationByHandleEx(FileStreamInfo)\n\n");

    printf("Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n");
    FILE_COMPRESSION_INFO CompressionInfo;
    RtlZeroMemory( &CompressionInfo,
                   sizeof(CompressionInfo) );
    if (! GetFileInformationByHandleEx( hFile,
                                        FileCompressionInfo,
                                        &CompressionInfo,
                                        sizeof(CompressionInfo) )) {
        fprintf(stderr,
                "[Failed] GetFileInformationByHandleEx(FileCompressionInfo) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    if (CompressionInfo.CompressedFileSize.QuadPart < 0) {
        fprintf(stderr,
                "[Failed] FileCompressionInfo returned negative compressed size\n");
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FILE_ALIGNMENT_INFO AlignmentInfo;
    RtlZeroMemory( &AlignmentInfo,
                   sizeof(AlignmentInfo) );
    if (! GetFileInformationByHandleEx( hFile,
                                        FileAlignmentInfo,
                                        &AlignmentInfo,
                                        sizeof(AlignmentInfo) )) {
        fprintf(stderr,
                "[Failed] GetFileInformationByHandleEx(FileAlignmentInfo) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    if ((AlignmentInfo.AlignmentRequirement &
         (AlignmentInfo.AlignmentRequirement + 1)) != 0) {
        fprintf(stderr,
                "[Failed] FileAlignmentInfo returned unexpected alignment mask = %lu\n",
                AlignmentInfo.AlignmentRequirement);
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FILE_STORAGE_INFO StorageInfo;
    RtlZeroMemory( &StorageInfo,
                   sizeof(StorageInfo) );
    if (! GetFileInformationByHandleEx( hFile,
                                        FileStorageInfo,
                                        &StorageInfo,
                                        sizeof(StorageInfo) )) {
        fprintf(stderr,
                "[Failed] GetFileInformationByHandleEx(FileStorageInfo) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    if (StorageInfo.LogicalBytesPerSector == 0 ||
        StorageInfo.PhysicalBytesPerSectorForAtomicity == 0 ||
        StorageInfo.PhysicalBytesPerSectorForPerformance == 0 ||
        StorageInfo.FileSystemEffectivePhysicalBytesPerSectorForAtomicity == 0) {
        fprintf(stderr,
                "[Failed] FileStorageInfo returned invalid sector sizes Logical = %lu Atomic = %lu Performance = %lu Effective = %lu\n",
                StorageInfo.LogicalBytesPerSector,
                StorageInfo.PhysicalBytesPerSectorForAtomicity,
                StorageInfo.PhysicalBytesPerSectorForPerformance,
                StorageInfo.FileSystemEffectivePhysicalBytesPerSectorForAtomicity);
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FILE_REMOTE_PROTOCOL_INFO RemoteProtocolInfo;
    RtlZeroMemory( &RemoteProtocolInfo,
                   sizeof(RemoteProtocolInfo) );
    SetLastError( ERROR_SUCCESS );
    BOOL RemoteProtocolResult =
        GetFileInformationByHandleEx( hFile,
                                      FileRemoteProtocolInfo,
                                      &RemoteProtocolInfo,
                                      sizeof(RemoteProtocolInfo) );
    if (RemoteProtocolResult) {
        if (RemoteProtocolInfo.StructureVersion == 0 ||
            RemoteProtocolInfo.StructureSize > sizeof(RemoteProtocolInfo)) {
            fprintf(stderr,
                    "[Failed] FileRemoteProtocolInfo returned invalid header Version = %hu Size = %hu\n",
                    RemoteProtocolInfo.StructureVersion,
                    RemoteProtocolInfo.StructureSize);
            CloseHandle( hFile );
            printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }
    } else if (GetLastError() != ERROR_INVALID_PARAMETER) {
        fprintf(stderr,
                "[Failed] FileRemoteProtocolInfo ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    SetLastError( ERROR_SUCCESS );
    if (GetFileInformationByHandleEx( hFile,
                                      FileRemoteProtocolInfo,
                                      &RemoteProtocolInfo,
                                      FIELD_OFFSET(FILE_REMOTE_PROTOCOL_INFO,
                                                   GenericReserved) ) ||
        GetLastError() != ERROR_BAD_LENGTH) {
        fprintf(stderr,
                "[Failed] FileRemoteProtocolInfo short buffer ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    printf("[Success] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo / FileStorageInfo / FileRemoteProtocolInfo)\n\n");
    CloseHandle( hFile );

    printf("Test DeviceIoControl(FSCTL_GET_COMPRESSION) overlapped\n");
    HANDLE hOverlappedEvent = CreateEventW( NULL,
                                            TRUE,
                                            FALSE,
                                            NULL );
    if (hOverlappedEvent == NULL) {
        fprintf(stderr, "[Failed] CreateEventW ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test DeviceIoControl(FSCTL_GET_COMPRESSION) overlapped\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    hFile = CreateFileW( L"Test.tmp",
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] CreateFileW(FILE_FLAG_OVERLAPPED) ErrorCode = %d\n", GetLastError());
        CloseHandle( hOverlappedEvent );
        printf("[Failed] Test DeviceIoControl(FSCTL_GET_COMPRESSION) overlapped\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    OVERLAPPED Overlapped;
    USHORT CompressionFormat = 0xffff;
    DWORD IoBytesReturned = 0;
    RtlZeroMemory( &Overlapped,
                   sizeof(Overlapped) );
    Overlapped.hEvent = hOverlappedEvent;

    SetLastError( ERROR_SUCCESS );
    rv = DeviceIoControl( hFile,
                          FSCTL_GET_COMPRESSION,
                          NULL,
                          0,
                          &CompressionFormat,
                          sizeof(CompressionFormat),
                          &IoBytesReturned,
                          &Overlapped );
    if (!rv && GetLastError() == ERROR_IO_PENDING) {
        DWORD WaitResult = WaitForSingleObject( hOverlappedEvent,
                                                5000 );
        if (WaitResult != WAIT_OBJECT_0) {
            fprintf(stderr,
                    "[Failed] DeviceIoControl overlapped wait Wait = 0x%08lx ErrorCode = %d\n",
                    WaitResult,
                    GetLastError());
            CloseHandle( hFile );
            CloseHandle( hOverlappedEvent );
            printf("[Failed] Test DeviceIoControl(FSCTL_GET_COMPRESSION) overlapped\n\n");
            rv = FALSE;
            goto DeleteTest;
        }

        if (Overlapped.Internal != 0) {
            fprintf(stderr,
                    "[Failed] DeviceIoControl overlapped status = 0x%Ix\n",
                    Overlapped.Internal);
            CloseHandle( hFile );
            CloseHandle( hOverlappedEvent );
            printf("[Failed] Test DeviceIoControl(FSCTL_GET_COMPRESSION) overlapped\n\n");
            rv = FALSE;
            goto DeleteTest;
        }

        IoBytesReturned = (DWORD)Overlapped.InternalHigh;
        rv = TRUE;
    }

    CloseHandle( hFile );
    CloseHandle( hOverlappedEvent );
    if (!rv ||
        IoBytesReturned != sizeof(CompressionFormat) ||
        CompressionFormat == 0xffff) {
        fprintf(stderr,
                "[Failed] DeviceIoControl overlapped ErrorCode = %d Bytes = %lu Compression = 0x%04x\n",
                GetLastError(),
                IoBytesReturned,
                CompressionFormat);
        printf("[Failed] Test DeviceIoControl(FSCTL_GET_COMPRESSION) overlapped\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test DeviceIoControl(FSCTL_GET_COMPRESSION) overlapped\n\n");

    printf("Test GetFileInformationByHandleEx directory enumeration classes\n");
    HANDLE hDirectory = CreateFileW( L".",
                                     GENERIC_READ,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                     NULL,
                                     OPEN_EXISTING,
                                     FILE_FLAG_BACKUP_SEMANTICS,
                                     NULL );
    if (hDirectory == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] CreateFileW(.) ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test GetFileInformationByHandleEx directory enumeration classes\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( Workspace->DirectoryInfoBuffer,
                   sizeof(Workspace->DirectoryInfoBuffer) );
    rv = GetFileInformationByHandleEx( hDirectory,
                                       FileFullDirectoryRestartInfo,
                                       Workspace->DirectoryInfoBuffer,
                                       sizeof(Workspace->DirectoryInfoBuffer) );
    if (!rv ||
        !FileTestDirectoryInfoContains( Workspace->DirectoryInfoBuffer,
                                        sizeof(Workspace->DirectoryInfoBuffer),
                                        FIELD_OFFSET(FILE_FULL_DIR_INFO, FileNameLength),
                                        FIELD_OFFSET(FILE_FULL_DIR_INFO, FileName),
                                        L"Test.tmp" )) {
        fprintf(stderr,
                "[Failed] FileFullDirectoryRestartInfo ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hDirectory );
        printf("[Failed] Test GetFileInformationByHandleEx directory enumeration classes\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( Workspace->DirectoryInfoBuffer,
                   sizeof(Workspace->DirectoryInfoBuffer) );
    rv = GetFileInformationByHandleEx( hDirectory,
                                       FileIdBothDirectoryRestartInfo,
                                       Workspace->DirectoryInfoBuffer,
                                       sizeof(Workspace->DirectoryInfoBuffer) );
    if (!rv ||
        !FileTestDirectoryInfoContains( Workspace->DirectoryInfoBuffer,
                                        sizeof(Workspace->DirectoryInfoBuffer),
                                        FIELD_OFFSET(FILE_ID_BOTH_DIR_INFO, FileNameLength),
                                        FIELD_OFFSET(FILE_ID_BOTH_DIR_INFO, FileName),
                                        L"Test.tmp" )) {
        fprintf(stderr,
                "[Failed] FileIdBothDirectoryRestartInfo ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hDirectory );
        printf("[Failed] Test GetFileInformationByHandleEx directory enumeration classes\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( Workspace->DirectoryInfoBuffer,
                   sizeof(Workspace->DirectoryInfoBuffer) );
    rv = GetFileInformationByHandleEx( hDirectory,
                                       FileIdExtdDirectoryRestartInfo,
                                       Workspace->DirectoryInfoBuffer,
                                       sizeof(Workspace->DirectoryInfoBuffer) );
    if (!rv ||
        !FileTestDirectoryInfoContains( Workspace->DirectoryInfoBuffer,
                                        sizeof(Workspace->DirectoryInfoBuffer),
                                        FIELD_OFFSET(FILE_ID_EXTD_DIR_INFO, FileNameLength),
                                        FIELD_OFFSET(FILE_ID_EXTD_DIR_INFO, FileName),
                                        L"Test.tmp" )) {
        fprintf(stderr,
                "[Failed] FileIdExtdDirectoryRestartInfo ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hDirectory );
        printf("[Failed] Test GetFileInformationByHandleEx directory enumeration classes\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    CloseHandle( hDirectory );
    printf("[Success] Test GetFileInformationByHandleEx directory enumeration classes\n\n");

    if (!rv || HandleInfo.nNumberOfLinks < 2) {
        fprintf(stderr,
                "[Failed] nNumberOfLinks = %lu, ErrorCode = %d\n",
                rv ? HandleInfo.nNumberOfLinks : 0,
                GetLastError());
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    static const WCHAR ExpectedFileName[] = L"Test.tmp";
    ULONG FileNameChars = NameInfo->FileNameLength / sizeof(WCHAR);
    ULONG ExpectedFileNameChars = RTL_NUMBER_OF(ExpectedFileName) - 1;
    if (FileNameChars < ExpectedFileNameChars ||
        RtlCompareMemory( NameInfo->FileName + FileNameChars - ExpectedFileNameChars,
                          ExpectedFileName,
                          ExpectedFileNameChars * sizeof(WCHAR) ) !=
                          ExpectedFileNameChars * sizeof(WCHAR)) {
        fprintf(stderr, "[Failed] GetFileInformationByHandleEx(FileNameInfo) returned unexpected name\n");
        printf("[Failed] Test GetFileInformationByHandleEx(FileNameInfo / FileNormalizedNameInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    ULONG NormalizedFileNameChars = NormalizedNameInfo->FileNameLength / sizeof(WCHAR);
    if (NormalizedFileNameChars < ExpectedFileNameChars ||
        RtlCompareMemory( NormalizedNameInfo->FileName + NormalizedFileNameChars - ExpectedFileNameChars,
                          ExpectedFileName,
                          ExpectedFileNameChars * sizeof(WCHAR) ) !=
                          ExpectedFileNameChars * sizeof(WCHAR)) {
        fprintf(stderr,
                "[Failed] GetFileInformationByHandleEx(FileNormalizedNameInfo) returned unexpected name\n");
        printf("[Failed] Test GetFileInformationByHandleEx(FileNameInfo / FileNormalizedNameInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetFileInformationByHandleEx(FileNameInfo / FileNormalizedNameInfo)\n\n");

    if (!DeleteFileW( L"Test.tmp" )) {
        fprintf(stderr, "[Failed] DeleteFileW(Test.tmp) ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    hFile = CreateFileW( L"TestHardLink.tmp",
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( Workspace->Buffer, sizeof(Workspace->Buffer) );
    length = 0;
    rv = ReadFile( hFile, Workspace->Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    if (!rv || length != 4 || memcmp(Workspace->Buffer, "1234", 4)) {
        fprintf(stderr, "[Failed] ErrorCode = %d, length = %d\n", GetLastError(), length);
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    if (!CopyFileW( L"TestHardLink.tmp",
                    L"Test.tmp",
                    TRUE )) {
        fprintf(stderr, "[Failed] Restore Test.tmp ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");

    printf("Test FindFirstFileExW flags\n");
    WIN32_FIND_DATAW FindFlagsData;
    DWORD FindFirstFlags = FIND_FIRST_EX_CASE_SENSITIVE | FIND_FIRST_EX_LARGE_FETCH;
#ifdef FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY
    FindFirstFlags |= FIND_FIRST_EX_ON_DISK_ENTRIES_ONLY;
#endif
    HANDLE hFindFlags = FindFirstFileExW( L"Test.tmp",
                                          FindExInfoBasic,
                                          &FindFlagsData,
                                          FindExSearchNameMatch,
                                          NULL,
                                          FindFirstFlags );
    if (hFindFlags == INVALID_HANDLE_VALUE ||
        wcscmp( FindFlagsData.cFileName,
                L"Test.tmp" ) != 0) {
        fprintf(stderr,
                "[Failed] FindFirstFileExW supported flags ErrorCode = %d\n",
                GetLastError());
        if (hFindFlags != INVALID_HANDLE_VALUE) {
            FindClose( hFindFlags );
        }
        printf("[Failed] Test FindFirstFileExW flags\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    FindClose( hFindFlags );

    SetLastError( ERROR_SUCCESS );
    hFindFlags = FindFirstFileExW( L"Test.tmp",
                                   FindExInfoBasic,
                                   &FindFlagsData,
                                   FindExSearchNameMatch,
                                   NULL,
                                   0x80000000u );
    if (hFindFlags != INVALID_HANDLE_VALUE ||
        GetLastError() != ERROR_INVALID_PARAMETER) {
        fprintf(stderr,
                "[Failed] FindFirstFileExW invalid flags ErrorCode = %d\n",
                GetLastError());
        if (hFindFlags != INVALID_HANDLE_VALUE) {
            FindClose( hFindFlags );
        }
        printf("[Failed] Test FindFirstFileExW flags\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test FindFirstFileExW flags\n\n");

    printf("Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n");
    DeleteFileW( L"TestSymlink.tmp" );
    rv = CreateSymbolicLinkW( L"TestSymlink.tmp",
                              L"Test.tmp",
                              0 );
    if (!rv) {
        DWORD Error = GetLastError();
#if !_KERNEL_MODE
        if (Error == ERROR_PRIVILEGE_NOT_HELD ||
            Error == ERROR_ACCESS_DENIED) {
            printf("[Skipped] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp) ErrorCode = %lu\n\n",
                   Error);
            goto AfterSymlinkTest;
        }
#endif
        fprintf(stderr, "[Failed] ErrorCode = %d\n", Error);
        printf("[Failed] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    DWORD SymlinkAttributes = GetFileAttributesW( L"TestSymlink.tmp" );
    if (SymlinkAttributes == INVALID_FILE_ATTRIBUTES ||
        !(SymlinkAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        fprintf(stderr, "[Failed] SymlinkAttributes = 0x%08lx, ErrorCode = %d\n",
                SymlinkAttributes,
                GetLastError());
        printf("[Failed] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    WIN32_FIND_DATAW SymlinkFindData;
    HANDLE hFind = FindFirstFileW( L"TestSymlink.tmp",
                                   &SymlinkFindData );
    if (hFind == INVALID_HANDLE_VALUE ||
        !(SymlinkFindData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        SymlinkFindData.dwReserved0 != IO_REPARSE_TAG_SYMLINK) {
        fprintf(stderr, "[Failed] SymlinkFindData Attr = 0x%08lx, Tag = 0x%08lx, ErrorCode = %d\n",
                hFind == INVALID_HANDLE_VALUE ? 0 : SymlinkFindData.dwFileAttributes,
                hFind == INVALID_HANDLE_VALUE ? 0 : SymlinkFindData.dwReserved0,
                GetLastError());
        if (hFind != INVALID_HANDLE_VALUE) {
            FindClose( hFind );
        }
        printf("[Failed] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    FindClose( hFind );

    hFile = CreateFileW( L"TestSymlink.tmp",
                         FILE_READ_ATTRIBUTES,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_OPEN_REPARSE_POINT,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FILE_ATTRIBUTE_TAG_INFO SymlinkTagInfo;
    if (! GetFileInformationByHandleEx( hFile,
                                        FileAttributeTagInfo,
                                        &SymlinkTagInfo,
                                        sizeof(SymlinkTagInfo) ) ||
        !(SymlinkTagInfo.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) ||
        SymlinkTagInfo.ReparseTag != IO_REPARSE_TAG_SYMLINK) {
        fprintf(stderr, "[Failed] SymlinkTagInfo Attr = 0x%08lx, Tag = 0x%08lx, ErrorCode = %d\n",
                SymlinkTagInfo.FileAttributes,
                SymlinkTagInfo.ReparseTag,
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    CloseHandle( hFile );

    hFile = CreateFileW( L"TestSymlink.tmp",
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( Workspace->Buffer, sizeof(Workspace->Buffer) );
    length = 0;
    rv = ReadFile( hFile, Workspace->Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    if (!rv || length != 4 || memcmp(Workspace->Buffer, "1234", 4)) {
        fprintf(stderr, "[Failed] ErrorCode = %d, length = %d\n", GetLastError(), length);
        printf("[Failed] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");

    printf("Test CreateSymbolicLinkA(TestSymlinkA.tmp, Test.tmp)\n");
    DeleteFileW( L"TestSymlinkA.tmp" );
    rv = CreateSymbolicLinkA( "TestSymlinkA.tmp",
                              "Test.tmp",
                              0 );
    if (!rv) {
        fprintf(stderr, "[Failed] CreateSymbolicLinkA ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test CreateSymbolicLinkA(TestSymlinkA.tmp, Test.tmp)\n\n");
        goto DeleteTest;
    }
    SymlinkAttributes = GetFileAttributesW( L"TestSymlinkA.tmp" );
    if (SymlinkAttributes == INVALID_FILE_ATTRIBUTES ||
        !(SymlinkAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
        fprintf(stderr, "[Failed] SymlinkA Attributes = 0x%08lx, ErrorCode = %d\n",
                SymlinkAttributes,
                GetLastError());
        printf("[Failed] Test CreateSymbolicLinkA(TestSymlinkA.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CreateSymbolicLinkA(TestSymlinkA.tmp, Test.tmp)\n\n");

#if !_KERNEL_MODE
AfterSymlinkTest:
#endif
    printf("Test GetFinalPathNameByHandleW(Test.tmp)\n");
    hFile = CreateFileW( L"Test.tmp",
                         GENERIC_READ,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test GetFinalPathNameByHandleW(Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    DWORD FinalPathLength = GetFinalPathNameByHandleW( hFile,
                                                       Workspace->FinalPath,
                                                       RTL_NUMBER_OF(Workspace->FinalPath),
                                                       FILE_NAME_NORMALIZED | VOLUME_NAME_NT );
    if (FinalPathLength == 0 ||
        FinalPathLength >= RTL_NUMBER_OF(Workspace->FinalPath) ||
        Workspace->FinalPath[0] != L'\\') {
        CloseHandle( hFile );
        fprintf(stderr, "[Failed] FinalPathLength = %lu, ErrorCode = %d\n",
                FinalPathLength,
                GetLastError());
        printf("[Failed] Test GetFinalPathNameByHandleW(Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FinalPathLength = GetFinalPathNameByHandleW( hFile,
                                                 Workspace->FinalPath,
                                                 RTL_NUMBER_OF(Workspace->FinalPath),
                                                 FILE_NAME_NORMALIZED | VOLUME_NAME_DOS );
    CloseHandle( hFile );
    if (FinalPathLength == 0 ||
        FinalPathLength >= RTL_NUMBER_OF(Workspace->FinalPath) ||
        Workspace->FinalPath[0] != L'\\' ||
        Workspace->FinalPath[1] != L'\\' ||
        Workspace->FinalPath[2] != L'?' ||
        Workspace->FinalPath[3] != L'\\' ||
        Workspace->FinalPath[5] != L':' ||
        Workspace->FinalPath[6] != L'\\') {
        fprintf(stderr, "[Failed] FinalPathLength = %lu, ErrorCode = %d\n",
                FinalPathLength,
                GetLastError());
        printf("[Failed] Test GetFinalPathNameByHandleW(Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetFinalPathNameByHandleW(Test.tmp)\n\n");

    printf("Test GetTempPathA / GetTempPathW\n");
    DWORD TempPathLengthA = GetTempPathA( RTL_NUMBER_OF(Workspace->TempPathA),
                                          Workspace->TempPathA );
    DWORD TempPathLengthW = GetTempPathW( RTL_NUMBER_OF(Workspace->TempPathW),
                                          Workspace->TempPathW );
    if (TempPathLengthA == 0 ||
        TempPathLengthA >= RTL_NUMBER_OF(Workspace->TempPathA) ||
        Workspace->TempPathA[TempPathLengthA - 1] != '\\') {
        fprintf(stderr, "[Failed] GetTempPathA ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    if (TempPathLengthW == 0 ||
        TempPathLengthW >= RTL_NUMBER_OF(Workspace->TempPathW) ||
        Workspace->TempPathW[TempPathLengthW - 1] != L'\\' ||
        GetFileAttributesW( Workspace->TempPathW ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] GetTempPathW ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetTempPathA / GetTempPathW\n\n");

    printf("Test GetTempFileNameA / GetTempFileNameW\n");
    UINT TempUniqueW = GetTempFileNameW( Workspace->TempPathW,
                                         L"ldw",
                                         0,
                                         Workspace->TempFileNameW );
    if (TempUniqueW == 0 ||
        GetFileAttributesW( Workspace->TempFileNameW ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] GetTempFileNameW ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    if (! DeleteFileW( Workspace->TempFileNameW )) {
        fprintf(stderr, "[Failed] DeleteFileW(GetTempFileNameW result) ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }

    UINT TempUniqueA = GetTempFileNameA( Workspace->TempPathA,
                                         "lda",
                                         0,
                                         Workspace->TempFileNameA );
    if (TempUniqueA == 0 ||
        GetFileAttributesA( Workspace->TempFileNameA ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] GetTempFileNameA ErrorCode = %d\n", GetLastError());
        DeleteFileW( Workspace->TempFileNameW );
        rv = FALSE;
        goto DeleteTest;
    }
    if (! DeleteFileA( Workspace->TempFileNameA )) {
        fprintf(stderr, "[Failed] DeleteFileA(GetTempFileNameA result) ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }

    TempUniqueA = GetTempFileNameA( Workspace->TempPathA,
                                    "ldn",
                                    0x4C44,
                                    Workspace->TempFileNameA );
    if (TempUniqueA != 0x4C44 || Workspace->TempFileNameA[0] == ANSI_NULL) {
        fprintf(stderr,
                "[Failed] GetTempFileNameA unique ErrorCode = %d Unique = 0x%04x\n",
                GetLastError(),
                TempUniqueA);
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetTempFileNameA / GetTempFileNameW\n\n");

    printf("Test SetFileInformationByHandle(FileCaseSensitiveInfo)\n");
    RemoveDirectoryW( L"TestCaseSensitiveDir.tmp" );
    rv = CreateDirectoryW( L"TestCaseSensitiveDir.tmp",
                           NULL );
    if (!rv) {
        fprintf(stderr, "[Failed] CreateDirectoryW(TestCaseSensitiveDir.tmp) ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileCaseSensitiveInfo)\n\n");
        goto DeleteTest;
    }

    hFile = CreateFileW( L"TestCaseSensitiveDir.tmp",
                         FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         OPEN_EXISTING,
                         FILE_FLAG_BACKUP_SEMANTICS,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] Open TestCaseSensitiveDir.tmp ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileCaseSensitiveInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FILE_CASE_SENSITIVE_INFO CaseSensitiveInfo;
    RtlZeroMemory( &CaseSensitiveInfo,
                   sizeof(CaseSensitiveInfo) );
    rv = SetFileInformationByHandle( hFile,
                                     FileCaseSensitiveInfo,
                                     &CaseSensitiveInfo,
                                     sizeof(CaseSensitiveInfo) );
    DWORD CaseSensitiveError = GetLastError();
    CloseHandle( hFile );
    if (!rv) {
#if _KERNEL_MODE
        fprintf(stderr, "[Failed] SetFileInformationByHandle(FileCaseSensitiveInfo) ErrorCode = %d\n",
                CaseSensitiveError);
        printf("[Failed] Test SetFileInformationByHandle(FileCaseSensitiveInfo)\n\n");
        goto DeleteTest;
#else
        if (CaseSensitiveError != ERROR_ACCESS_DENIED &&
            CaseSensitiveError != ERROR_INVALID_PARAMETER &&
            CaseSensitiveError != ERROR_NOT_SUPPORTED &&
            CaseSensitiveError != ERROR_PRIVILEGE_NOT_HELD) {
            fprintf(stderr, "[Failed] SetFileInformationByHandle(FileCaseSensitiveInfo) ErrorCode = %d\n",
                    CaseSensitiveError);
            printf("[Failed] Test SetFileInformationByHandle(FileCaseSensitiveInfo)\n\n");
            goto DeleteTest;
        }
        printf("[Skipped] Test SetFileInformationByHandle(FileCaseSensitiveInfo) ErrorCode = %d\n\n",
               CaseSensitiveError);
#endif
    } else {
        printf("[Success] Test SetFileInformationByHandle(FileCaseSensitiveInfo)\n\n");
    }

    printf("Test SetFileInformationByHandle(FileAllocationInfo)\n");
    DeleteFileW( L"TestAllocation.tmp" );
    hFile = CreateFileW( L"TestAllocation.tmp",
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileAllocationInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FILE_ALLOCATION_INFO AllocationInfo;
    AllocationInfo.AllocationSize.QuadPart = 4096;
    rv = SetFileInformationByHandle( hFile,
                                     FileAllocationInfo,
                                     &AllocationInfo,
                                     sizeof(AllocationInfo) );
    CloseHandle( hFile );
    if (!rv) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileAllocationInfo)\n\n");
        goto DeleteTest;
    }
    printf("[Success] Test SetFileInformationByHandle(FileAllocationInfo)\n\n");

    printf("Test SetFileInformationByHandle(FileIoPriorityHintInfo)\n");
    DeleteFileW( L"TestIoPriority.tmp" );
    hFile = CreateFileW( L"TestIoPriority.tmp",
                         GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileIoPriorityHintInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    /* Windows validates this buffer with pointer-size alignment on x64. */
    ULONGLONG PriorityHintInfoStorage[(sizeof(FILE_IO_PRIORITY_HINT_INFO) +
                                       sizeof(ULONGLONG) - 1) /
                                      sizeof(ULONGLONG)];
    PFILE_IO_PRIORITY_HINT_INFO PriorityHintInfo =
        (PFILE_IO_PRIORITY_HINT_INFO)PriorityHintInfoStorage;
    RtlZeroMemory( PriorityHintInfoStorage,
                   sizeof(PriorityHintInfoStorage) );
    PriorityHintInfo->PriorityHint = IoPriorityHintNormal;
    rv = SetFileInformationByHandle( hFile,
                                     FileIoPriorityHintInfo,
                                     PriorityHintInfo,
                                     sizeof(*PriorityHintInfo) );
    CloseHandle( hFile );
    if (!rv) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileIoPriorityHintInfo)\n\n");
        goto DeleteTest;
    }
    printf("[Success] Test SetFileInformationByHandle(FileIoPriorityHintInfo)\n\n");

    {
        printf("Test SetFileInformationByHandle(FileCaseSensitiveInfo short buffer)\n");
        RemoveDirectoryW( L"TestCaseSensitiveDir" );
        if (! CreateDirectoryW( L"TestCaseSensitiveDir", NULL )) {
            fprintf(stderr, "[Failed] CreateDirectoryW(TestCaseSensitiveDir) ErrorCode = %d\n",
                    GetLastError());
            printf("[Failed] Test SetFileInformationByHandle(FileCaseSensitiveInfo short buffer)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }

        HANDLE hCaseSensitiveDirectory = CreateFileW( L"TestCaseSensitiveDir",
                                                      FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                                                      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                                      NULL,
                                                      OPEN_EXISTING,
                                                      FILE_FLAG_BACKUP_SEMANTICS,
                                                      NULL );
        if (hCaseSensitiveDirectory == INVALID_HANDLE_VALUE) {
            fprintf(stderr, "[Failed] Open TestCaseSensitiveDir ErrorCode = %d\n",
                    GetLastError());
            printf("[Failed] Test SetFileInformationByHandle(FileCaseSensitiveInfo short buffer)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }

        FILE_CASE_SENSITIVE_INFO ShortCaseSensitiveInfo;
        ShortCaseSensitiveInfo.Flags = 0;
        rv = SetFileInformationByHandle( hCaseSensitiveDirectory,
                                         FileCaseSensitiveInfo,
                                         &ShortCaseSensitiveInfo,
                                         sizeof(ShortCaseSensitiveInfo) - 1 );
        DWORD ShortCaseSensitiveError = GetLastError();
        CloseHandle( hCaseSensitiveDirectory );
        if (rv || ShortCaseSensitiveError != ERROR_BAD_LENGTH) {
            fprintf(stderr,
                    "[Failed] FileCaseSensitiveInfo short buffer rv = %d ErrorCode = %d\n",
                    rv,
                    ShortCaseSensitiveError);
            printf("[Failed] Test SetFileInformationByHandle(FileCaseSensitiveInfo short buffer)\n\n");
            rv = FALSE;
            goto DeleteTest;
        }
        rv = TRUE;
        printf("[Success] Test SetFileInformationByHandle(FileCaseSensitiveInfo short buffer)\n\n");
    }

    printf("Test SetFileInformationByHandle(FileRenameInfo)\n");
    DeleteFileW( L"TestRenameSource.tmp" );
    DeleteFileW( L"TestRenameTarget.tmp" );
    hFile = CreateFileW( L"TestRenameSource.tmp",
                         GENERIC_READ | GENERIC_WRITE | DELETE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    DWORD RenameTargetPathLength = GetFullPathNameW( L"TestRenameTarget.tmp",
                                                     RTL_NUMBER_OF(Workspace->RenameTargetPath),
                                                     Workspace->RenameTargetPath,
                                                     NULL );
    if (RenameTargetPathLength == 0 ||
        RenameTargetPathLength >= RTL_NUMBER_OF(Workspace->RenameTargetPath)) {
        fprintf(stderr, "[Failed] GetFullPathNameW(TestRenameTarget.tmp) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    PFILE_RENAME_INFO RenameInfo = (PFILE_RENAME_INFO)Workspace->RenameInfoBuffer;
    RtlZeroMemory( Workspace->RenameInfoBuffer,
                   sizeof(Workspace->RenameInfoBuffer) );
    RenameInfo->ReplaceIfExists = FALSE;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = RenameTargetPathLength * sizeof(WCHAR);
    RtlCopyMemory( RenameInfo->FileName,
                   Workspace->RenameTargetPath,
                   RenameInfo->FileNameLength );
    rv = SetFileInformationByHandle( hFile,
                                     FileRenameInfo,
                                     RenameInfo,
                                     FIELD_OFFSET(FILE_RENAME_INFO, FileName) +
                                     RenameInfo->FileNameLength );
    CloseHandle( hFile );
    if (!rv ||
        GetFileAttributesW( L"TestRenameSource.tmp" ) != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW( L"TestRenameTarget.tmp" ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test SetFileInformationByHandle(FileRenameInfo)\n\n");

    printf("Test SetFileInformationByHandle(FileRenameInfoEx)\n");
    DeleteFileW( L"TestRenameExSource.tmp" );
    DeleteFileW( L"TestRenameExTarget.tmp" );
    hFile = CreateFileW( L"TestRenameExSource.tmp",
                         GENERIC_READ | GENERIC_WRITE | DELETE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfoEx)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    HANDLE hRenameTarget = CreateFileW( L"TestRenameExTarget.tmp",
                                        GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                        NULL,
                                        CREATE_ALWAYS,
                                        FILE_ATTRIBUTE_NORMAL,
                                        NULL );
    if (hRenameTarget == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] Create target ErrorCode = %d\n", GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfoEx)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    CloseHandle( hRenameTarget );

    RenameTargetPathLength = GetFullPathNameW( L"TestRenameExTarget.tmp",
                                               RTL_NUMBER_OF(Workspace->RenameTargetPath),
                                               Workspace->RenameTargetPath,
                                               NULL );
    if (RenameTargetPathLength == 0 ||
        RenameTargetPathLength >= RTL_NUMBER_OF(Workspace->RenameTargetPath)) {
        fprintf(stderr, "[Failed] GetFullPathNameW(TestRenameExTarget.tmp) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfoEx)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( Workspace->RenameInfoBuffer,
                   sizeof(Workspace->RenameInfoBuffer) );
    RenameInfo->Flags = FILE_RENAME_FLAG_REPLACE_IF_EXISTS;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = RenameTargetPathLength * sizeof(WCHAR);
    RtlCopyMemory( RenameInfo->FileName,
                   Workspace->RenameTargetPath,
                   RenameInfo->FileNameLength );
    rv = SetFileInformationByHandle( hFile,
                                     FileRenameInfoEx,
                                     RenameInfo,
                                     FIELD_OFFSET(FILE_RENAME_INFO, FileName) +
                                     RenameInfo->FileNameLength );
    CloseHandle( hFile );
    if (!rv ||
        GetFileAttributesW( L"TestRenameExSource.tmp" ) != INVALID_FILE_ATTRIBUTES ||
        GetFileAttributesW( L"TestRenameExTarget.tmp" ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfoEx)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test SetFileInformationByHandle(FileRenameInfoEx)\n\n");

    printf("Test SetFileInformationByHandle(FileDispositionInfoEx)\n");
    DeleteFileW( L"TestDisposition.tmp" );
    hFile = CreateFileW( L"TestDisposition.tmp",
                         DELETE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES,
                         FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
                         NULL,
                         CREATE_ALWAYS,
                         FILE_ATTRIBUTE_NORMAL,
                         NULL );
    if (hFile == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileDispositionInfoEx)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FILE_DISPOSITION_INFO_EX DispositionInfoEx;
    DispositionInfoEx.Flags = FILE_DISPOSITION_FLAG_DELETE |
                              FILE_DISPOSITION_FLAG_POSIX_SEMANTICS |
                              FILE_DISPOSITION_FLAG_IGNORE_READONLY_ATTRIBUTE;
    rv = SetFileInformationByHandle( hFile,
                                     FileDispositionInfoEx,
                                     &DispositionInfoEx,
                                     sizeof(DispositionInfoEx) );
    CloseHandle( hFile );
    if (!rv) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test SetFileInformationByHandle(FileDispositionInfoEx)\n\n");
        goto DeleteTest;
    }
    if (GetFileAttributesW( L"TestDisposition.tmp" ) != INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] TestDisposition.tmp still exists\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test SetFileInformationByHandle(FileDispositionInfoEx)\n\n");

    printf("Test GetErrorMode / SetErrorMode\n");
    UINT ErrorMode = GetErrorMode();
    UINT PreviousMode = SetErrorMode( ErrorMode );
    if (PreviousMode != ErrorMode) {
        fprintf(stderr, "[Failed] PreviousMode = %u, ErrorMode = %u\n", PreviousMode, ErrorMode);
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetErrorMode / SetErrorMode\n\n");

DeleteTest:
    RemoveDirectoryW( L"TestCaseSensitiveDir.tmp" );
    DeleteFileW( L"TestAllocation.tmp" );
    DeleteFileW( L"TestIoPriority.tmp" );
    DeleteFileW( L"TestRenameSource.tmp" );
    DeleteFileW( L"TestRenameTarget.tmp" );
    DeleteFileW( L"TestRenameExSource.tmp" );
    DeleteFileW( L"TestRenameExTarget.tmp" );
    DeleteFileW( L"TestDisposition.tmp" );
    RemoveDirectoryW( L"TestCaseSensitiveDir" );
    DeleteFileW( L"TestCreateFile2.tmp" );
    DeleteFileW( L"TestCopyA.tmp" );
    DeleteFileW( L"TestCopy2.tmp" );
    DeleteFileW( L"TestCopy2Cancel.tmp" );
    DeleteFileW( L"TestMoved.tmp" );
    DeleteFileW( L"TestMoveA.tmp" );
    DeleteFileW( L"TestMoveATarget.tmp" );
    DeleteFileW( L"TestMoveW.tmp" );
    DeleteFileW( L"TestMoveWTarget.tmp" );
    DeleteFileW( L"TestMoveExA.tmp" );
    DeleteFileW( L"TestMoveExATarget.tmp" );
    DeleteFileW( L"TestCopy.tmp" );
    DeleteFileW( L"TestHardLink.tmp" );
    DeleteFileW( L"TestHardLinkA.tmp" );
    DeleteFileW( L"TestSymlink.tmp" );
    DeleteFileW( L"TestSymlinkA.tmp" );
    printf("Test DeleteFileA(Test.tmp)\n");
    BOOL DeleteResult = DeleteFileA( "Test.tmp" );
    if (DeleteResult) {
        printf("[Success] Test DeleteFileA(Test.tmp)\n\n");
    } else {
        DWORD Error = GetLastError();
        if (Error != ERROR_FILE_NOT_FOUND &&
            Error != ERROR_PATH_NOT_FOUND) {
            fprintf(stderr, "[Failed] ErrorCode = %d\n", Error);
            printf("[Failed] Test DeleteFileA(Test.tmp)\n\n");
            rv = FALSE;
        }
    }
    HeapFree( GetProcessHeap(),
              0,
              Workspace );
    return rv == TRUE;
}
