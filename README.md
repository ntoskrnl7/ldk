# ldk

**L**oad **D**ll into **K**ernel space

## Overview

- 이 프로젝트는 사용자 모드 Dll을 Kernel 영역에 로드하여 커널 드라이버에서 Dll의 코드를 직접 호출할 수 있도록 도와줍니다.
- Kernel32.dll과 Ntdll.dll는 커널에서 절대 사용할 수 없기 때문에 해당 모듈의 기능이 일부 구현되어있습니다.

## Requirements

- Windows 8 이상
- Visual Studio 2017

## Test Environments

- Windows 10
- Visual Studio 2017

## Contents

- [ldk](#ldk)
  - [Overview](#overview)
  - [Requirements](#requirements)
  - [Test Environments](#test-environments)
  - [Contents](#contents)
  - [Caution](#caution)
  - [Build](#build)
  - [Test](#test)
  - [Usage](#usage)
    - [CMake](#cmake)
      - [CMakeLists.txt](#cmakeliststxt)
    - [main.c](#mainc)
  - [TODO](#todo)

## Caution

이 프로젝트는 **매우 실험적인** 기능입니다.

1. ***TEB나 PEB에 접근하는 코드가 있는 모듈은 사용하지 마십시오***
2. ***Dll Load를 직접 사용하는것은 많은 시험을 거친 후 적용하시기 바랍니다.***

## Build

Visual Studio 프로젝트에 이 라이브러리를 적용할때 참고하시기 바랍니다.

***CMake 프로젝트에 이 라이브러리를 적용하시려면 [CMake](#cmake) 섹션을 참고하시기 바랍니다.***

1. 아래 명령을 수행하여 라이브러리를 빌드하시기 바랍니다.

    - Visual Studio 사용
      - {이 저장소}/msvc/ldk.sn 혹은 {이 저장소}/msvc/ldk.vcxproj를 열어서 빌드를 하시기 바랍니다.

    - CMake 사용

        ```Batch
        git clone https://github.com/ntoskrnl7/ldk
        cd ldk
        mkdir build && cd build
        cmake .. -DWDK_WINVER=0x0602
        cmake --build . --config Release
        ```

2. 빌드가 완료되었다면 아래 내용을 참고하여 드라이버 프로젝트에 적용하시기 바랍니다.

    1. **{이 저장소}/include**를 '**[추가 포함 디렉토리](https://docs.microsoft.com/cpp/build/reference/i-additional-include-directories#to-set-this-compiler-option-in-the-visual-studio-development-environment
    )** 속성'에 추가.
    2. **Ldk.lib**를 '**[추가 종속성](https://docs.microsoft.com/cpp/build/reference/dot-lib-files-as-linker-input?view=msvc-170#to-add-lib-files-as-linker-input-in-the-development-environment)** 속성'에 추가.
    3. **{이 저장소}/lib/\$(PlatformShortName)/\$(Configuration)**를 '**[추가 라이브러리 디렉토리](https://docs.microsoft.com/cpp/build/reference/libpath-additional-libpath?view=msvc-170#to-set-this-linker-option-in-the-visual-studio-development-environment)** 속성'에 추가

## Test

1. 아래 명령을 수행하여 라이브러리 및 테스트 코드를 빌드하시기 바랍니다.

    ```Batch
    git clone https://github.com/ntoskrnl7/ldk
    cd ldk/test
    mkdir build && cd build
    cmake .. -DWDK_WINVER=0x0602
    cmake --build .
    ```

2. build/Debug/LdkTest.sys를 설치 및 로드하시기 바랍니다.
3. 정상적으로 로드 및 언로드가 되는지 확인하시기 바랍니다.

## Usage

CMake를 사용하는것을 권장합니다.

### CMake

CMake를 사용하신다면 아래와 같이 CMakeLists.txt를 만드시기 바랍니다.

#### CMakeLists.txt

```CMake
cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

# create project
project(MyProject)

# add dependencies
include(cmake/CPM.cmake)
CPMAddPackage("gh:ntoskrnl7/ldk@0.7.1")

# add dependencies
CPMAddPackage("gh:ntoskrnl7/FindWDK#master")

# use FindWDK
CPMAddPackage("gh:ntoskrnl7/FindWDK#master")
list(APPEND CMAKE_MODULE_PATH "${FindWDK_SOURCE_DIR}/cmake")
find_package(WDK REQUIRED)

# add driver
wdk_add_driver(TestDrv CUSTOM_ENTRY_POINT "LdkDriverEntry" main.c)

# link dependencies
target_link_libraries(TestDrv Ldk)
```

### main.c

Condition Variable을 사용하는 간단한 샘플 코드입니다.

- 아래와 같이 진입점을 LdkDriverEntry로 설정한다면 LdkInitialize와 LdkTerminate를 호출할 필요가 없습니다. **(권장)**

    ```CMake
    wdk_add_driver(TestDrv CUSTOM_ENTRY_POINT "LdkDriverEntry" main.c)
    ```

- 만약 아래와 같이 진입점을 설정하지 않았다면 드라이버가 시작될 때는 LdkInitialize 함수를 반드시 호출해야하며, 드라이버 언로드 전에는 LdkTerminate 함수를 반드시 호출해야합니다.

    ```CMake
    wdk_add_driver(TestDrv main.c)
    ```

아래는 LdkDriverEntry를 진입점으로 설정한 프로젝트의 예제 코드입니다.

```C
#include <Ldk/Windows.h>

DRIVER_INITIALIZE DriverEntry;
DRIVER_UNLOAD DriverUnload;

typedef struct _COND_TEST_THREAD_PARAM {
    PCSTR name;
    BOOL ready;
    LONG processed;
    CONDITION_VARIABLE cv;
    CRITICAL_SECTION cs;
} COND_TEST_THREAD_PARAM, *PCOND_TEST_THREAD_PARAM;

DWORD
WINAPI
CondTestThreadProc (
    LPVOID lpThreadParameter
    )
{
    PCOND_TEST_THREAD_PARAM param = (PCOND_TEST_THREAD_PARAM)lpThreadParameter;

    PAGED_CODE();

    DbgPrint("%s - %d - start\n", param->name, GetCurrentThreadId());

    EnterCriticalSection(&param->cs);
    while (!param->ready) {
        SleepConditionVariableCS(&param->cv, &param->cs, INFINITE);
    }

    DbgPrint("%s - %d - processed (%d)\n", param->name, GetCurrentThreadId(), InterlockedIncrement(&param->processed));

    LeaveCriticalSection(&param->cs);

    DbgPrint("%s - %d - end\n", param->name, GetCurrentThreadId());

    WakeConditionVariable(&param->cv);
    return 0;
}

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(RegistryPath);

    COND_TEST_THREAD_PARAM param;
    param.name = "Test";
    param.ready = FALSE;
    param.processed = 0;
    InitializeCriticalSection(&param.cs);
    InitializeConditionVariable(&param.cv);
    HANDLE threads[5];
    for (int i = 0; i < 5; ++i) {
        threads[i] = CreateThread(NULL, 0, CondTestThreadProc, &param, 0, NULL);
    }

    EnterCriticalSection(&param.cs);
    param.ready = TRUE;
    LeaveCriticalSection(&param.cs);
    WakeAllConditionVariable(&param.cv);

    EnterCriticalSection(&param.cs);
    while (param.processed < 5) {
        SleepConditionVariableCS(&param.cv, &param.cs, INFINITE);
    }
    WakeAllConditionVariable(&param.cv);
    LeaveCriticalSection(&param.cs);

    WaitForMultipleObjects(5, threads, TRUE, INFINITE);
    DeleteCriticalSection(&param.cs);

    for (int i = 0; i < 5; ++i) {
        if (threads[i]) {
            CloseHandle(threads[i]);
        }
    }

    DriverObject->DriverUnload = DriverUnload;
    return STATUS_SUCCESS;
}

VOID
DriverUnload (
    _In_ PDRIVER_OBJECT driverObject
    )
{
    PAGED_CODE();
    UNREFERENCED_PARAMETER(driverObject);
}
```

## TODO

 아직 Dll을 로드하여 사용하기에는 API가 조금밖에 구현되지 않아서 간단한 Dll 밖에는 사용할 수 없습니다.

- [ ] 빠른 구현을 위해서 ReactOS 코드를 일부 사용하였으며, 추후 자체 구현해야합니다.
- [ ] 문서화
- [ ] 기타
