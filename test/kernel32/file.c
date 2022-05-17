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

DeleteTest:
    printf("Test DeleteFileA(Test.tmp)\n");
    rv = DeleteFileA( "Test.tmp" );
    if (rv) {
        printf("[Success] Test DeleteFileA(Test.tmp)\n\n");
    } else {
        fprintf(stderr, "[Failed] ErrorCode = %d\n", GetLastError());
        printf("[Failed] Test DeleteFileA(Test.tmp)\n\n");
    }
    return rv == TRUE;
}