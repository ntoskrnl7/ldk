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
#include <stdio.h>
#define PAGED_CODE()
#endif

BOOLEAN
FileTest (
    VOID
    )
{
    BOOL rv = TRUE;

    PAGED_CODE();

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
        return FALSE;
    }
    printf("[Success] Test CreateFileA(Test.tmp, CREATE_NEW) \n\n");


    printf("Test GetFullPathNameA(Test.tmp) \n");
    CHAR PathBuffer[128];
    PSTR FilePart;
    if (GetFullPathNameA( "Test.tmp", sizeof(PathBuffer), PathBuffer, &FilePart ) == 0) {
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
    CHAR Buffer[4];
    rv = ReadFile( hFile, Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    
    if (!rv) {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
    }
    if ((length != 4) || (memcmp(Buffer, "1234", 4))) {
        fprintf(stderr, "[Failed] Expect = 4, 1234, Actual = %d %s\n", length, Buffer);
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

    RtlZeroMemory( Buffer, sizeof(Buffer) );
    length = 0;
    rv = ReadFile( hFile, Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    if (!rv || length != 4 || memcmp(Buffer, "1234", 4)) {
        fprintf(stderr, "[Failed] ErrorCode = %d, length = %d\n", GetLastError(), length);
        printf("[Failed] Test CopyFileW(Test.tmp, TestCopy.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CopyFileW(Test.tmp, TestCopy.tmp)\n\n");

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
    UCHAR FileNameInfoBuffer[FIELD_OFFSET(FILE_NAME_INFO, FileName) + (MAX_PATH * sizeof(WCHAR))];
    PFILE_NAME_INFO NameInfo = (PFILE_NAME_INFO)FileNameInfoBuffer;
    RtlZeroMemory( FileNameInfoBuffer,
                   sizeof(FileNameInfoBuffer) );
    if (! GetFileInformationByHandleEx( hFile,
                                        FileNameInfo,
                                        NameInfo,
                                        sizeof(FileNameInfoBuffer) )) {
        fprintf(stderr,
                "[Failed] GetFileInformationByHandleEx(FileNameInfo) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("Test GetFileInformationByHandleEx(FileStreamInfo)\n");
    UCHAR FileStreamInfoBuffer[4096];
    ULONG FileStreamInfoBufferSize = (ULONG)sizeof(FileStreamInfoBuffer);
    ULONG FileStreamInfoHeaderSize = (ULONG)FIELD_OFFSET(FILE_STREAM_INFO, StreamName);
    PFILE_STREAM_INFO StreamInfo = (PFILE_STREAM_INFO)FileStreamInfoBuffer;
    RtlZeroMemory( FileStreamInfoBuffer,
                   sizeof(FileStreamInfoBuffer) );
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

        Entry = (PFILE_STREAM_INFO)(FileStreamInfoBuffer + StreamInfoOffset);
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

    printf("Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo)\n");
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
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    if (CompressionInfo.CompressedFileSize.QuadPart < 0) {
        fprintf(stderr,
                "[Failed] FileCompressionInfo returned negative compressed size\n");
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo)\n\n");
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
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    if ((AlignmentInfo.AlignmentRequirement &
         (AlignmentInfo.AlignmentRequirement + 1)) != 0) {
        fprintf(stderr,
                "[Failed] FileAlignmentInfo returned unexpected alignment mask = %lu\n",
                AlignmentInfo.AlignmentRequirement);
        CloseHandle( hFile );
        printf("[Failed] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetFileInformationByHandleEx(FileCompressionInfo / FileAlignmentInfo)\n\n");
    CloseHandle( hFile );
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
        printf("[Failed] Test CreateHardLinkW(Test.tmp, TestHardLink.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

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

    RtlZeroMemory( Buffer, sizeof(Buffer) );
    length = 0;
    rv = ReadFile( hFile, Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    if (!rv || length != 4 || memcmp(Buffer, "1234", 4)) {
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

    RtlZeroMemory( Buffer, sizeof(Buffer) );
    length = 0;
    rv = ReadFile( hFile, Buffer, 4, &length, NULL );
    CloseHandle( hFile );
    if (!rv || length != 4 || memcmp(Buffer, "1234", 4)) {
        fprintf(stderr, "[Failed] ErrorCode = %d, length = %d\n", GetLastError(), length);
        printf("[Failed] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test CreateSymbolicLinkW(TestSymlink.tmp, Test.tmp)\n\n");

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

    WCHAR FinalPath[MAX_PATH];
    DWORD FinalPathLength = GetFinalPathNameByHandleW( hFile,
                                                       FinalPath,
                                                       RTL_NUMBER_OF(FinalPath),
                                                       FILE_NAME_NORMALIZED | VOLUME_NAME_NT );
    if (FinalPathLength == 0 ||
        FinalPathLength >= RTL_NUMBER_OF(FinalPath) ||
        FinalPath[0] != L'\\') {
        CloseHandle( hFile );
        fprintf(stderr, "[Failed] FinalPathLength = %lu, ErrorCode = %d\n",
                FinalPathLength,
                GetLastError());
        printf("[Failed] Test GetFinalPathNameByHandleW(Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    FinalPathLength = GetFinalPathNameByHandleW( hFile,
                                                 FinalPath,
                                                 RTL_NUMBER_OF(FinalPath),
                                                 FILE_NAME_NORMALIZED | VOLUME_NAME_DOS );
    CloseHandle( hFile );
    if (FinalPathLength == 0 ||
        FinalPathLength >= RTL_NUMBER_OF(FinalPath) ||
        FinalPath[0] != L'\\' ||
        FinalPath[1] != L'\\' ||
        FinalPath[2] != L'?' ||
        FinalPath[3] != L'\\' ||
        FinalPath[5] != L':' ||
        FinalPath[6] != L'\\') {
        fprintf(stderr, "[Failed] FinalPathLength = %lu, ErrorCode = %d\n",
                FinalPathLength,
                GetLastError());
        printf("[Failed] Test GetFinalPathNameByHandleW(Test.tmp)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetFinalPathNameByHandleW(Test.tmp)\n\n");

    printf("Test GetTempPathA / GetTempPathW\n");
    CHAR TempPathA[MAX_PATH];
    WCHAR TempPathW[MAX_PATH];
    DWORD TempPathLengthA = GetTempPathA( RTL_NUMBER_OF(TempPathA),
                                          TempPathA );
    DWORD TempPathLengthW = GetTempPathW( RTL_NUMBER_OF(TempPathW),
                                          TempPathW );
    if (TempPathLengthA == 0 ||
        TempPathLengthA >= RTL_NUMBER_OF(TempPathA) ||
        TempPathA[TempPathLengthA - 1] != '\\') {
        fprintf(stderr, "[Failed] GetTempPathA ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    if (TempPathLengthW == 0 ||
        TempPathLengthW >= RTL_NUMBER_OF(TempPathW) ||
        TempPathW[TempPathLengthW - 1] != L'\\' ||
        GetFileAttributesW( TempPathW ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] GetTempPathW ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetTempPathA / GetTempPathW\n\n");

    printf("Test GetTempFileNameA / GetTempFileNameW\n");
    WCHAR TempFileNameW[MAX_PATH];
    CHAR TempFileNameA[MAX_PATH];
    UINT TempUniqueW = GetTempFileNameW( TempPathW,
                                         L"ldw",
                                         0,
                                         TempFileNameW );
    if (TempUniqueW == 0 ||
        GetFileAttributesW( TempFileNameW ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] GetTempFileNameW ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }
    if (! DeleteFileW( TempFileNameW )) {
        fprintf(stderr, "[Failed] DeleteFileW(GetTempFileNameW result) ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }

    UINT TempUniqueA = GetTempFileNameA( TempPathA,
                                         "lda",
                                         0,
                                         TempFileNameA );
    if (TempUniqueA == 0 ||
        GetFileAttributesA( TempFileNameA ) == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "[Failed] GetTempFileNameA ErrorCode = %d\n", GetLastError());
        DeleteFileW( TempFileNameW );
        rv = FALSE;
        goto DeleteTest;
    }
    if (! DeleteFileA( TempFileNameA )) {
        fprintf(stderr, "[Failed] DeleteFileA(GetTempFileNameA result) ErrorCode = %d\n", GetLastError());
        rv = FALSE;
        goto DeleteTest;
    }

    TempUniqueA = GetTempFileNameA( TempPathA,
                                    "ldn",
                                    0x4C44,
                                    TempFileNameA );
    if (TempUniqueA != 0x4C44 || TempFileNameA[0] == ANSI_NULL) {
        fprintf(stderr,
                "[Failed] GetTempFileNameA unique ErrorCode = %d Unique = 0x%04x\n",
                GetLastError(),
                TempUniqueA);
        rv = FALSE;
        goto DeleteTest;
    }
    printf("[Success] Test GetTempFileNameA / GetTempFileNameW\n\n");

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

    WCHAR RenameTargetPath[MAX_PATH];
    DWORD RenameTargetPathLength = GetFullPathNameW( L"TestRenameTarget.tmp",
                                                     RTL_NUMBER_OF(RenameTargetPath),
                                                     RenameTargetPath,
                                                     NULL );
    if (RenameTargetPathLength == 0 ||
        RenameTargetPathLength >= RTL_NUMBER_OF(RenameTargetPath)) {
        fprintf(stderr, "[Failed] GetFullPathNameW(TestRenameTarget.tmp) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfo)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    UCHAR RenameInfoBuffer[FIELD_OFFSET(FILE_RENAME_INFO, FileName) + MAX_PATH * sizeof(WCHAR)];
    PFILE_RENAME_INFO RenameInfo = (PFILE_RENAME_INFO)RenameInfoBuffer;
    RtlZeroMemory( RenameInfoBuffer,
                   sizeof(RenameInfoBuffer) );
    RenameInfo->ReplaceIfExists = FALSE;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = RenameTargetPathLength * sizeof(WCHAR);
    RtlCopyMemory( RenameInfo->FileName,
                   RenameTargetPath,
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
                                               RTL_NUMBER_OF(RenameTargetPath),
                                               RenameTargetPath,
                                               NULL );
    if (RenameTargetPathLength == 0 ||
        RenameTargetPathLength >= RTL_NUMBER_OF(RenameTargetPath)) {
        fprintf(stderr, "[Failed] GetFullPathNameW(TestRenameExTarget.tmp) ErrorCode = %d\n",
                GetLastError());
        CloseHandle( hFile );
        printf("[Failed] Test SetFileInformationByHandle(FileRenameInfoEx)\n\n");
        rv = FALSE;
        goto DeleteTest;
    }

    RtlZeroMemory( RenameInfoBuffer,
                   sizeof(RenameInfoBuffer) );
    RenameInfo->Flags = FILE_RENAME_FLAG_REPLACE_IF_EXISTS;
    RenameInfo->RootDirectory = NULL;
    RenameInfo->FileNameLength = RenameTargetPathLength * sizeof(WCHAR);
    RtlCopyMemory( RenameInfo->FileName,
                   RenameTargetPath,
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
    DeleteFileW( L"TestAllocation.tmp" );
    DeleteFileW( L"TestRenameSource.tmp" );
    DeleteFileW( L"TestRenameTarget.tmp" );
    DeleteFileW( L"TestRenameExSource.tmp" );
    DeleteFileW( L"TestRenameExTarget.tmp" );
    DeleteFileW( L"TestDisposition.tmp" );
    DeleteFileW( L"TestMoved.tmp" );
    DeleteFileW( L"TestCopy.tmp" );
    DeleteFileW( L"TestHardLink.tmp" );
    DeleteFileW( L"TestSymlink.tmp" );
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
    return rv == TRUE;
}
