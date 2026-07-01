@REM build.bat "PATH" "x86 | x64" | "ARM" | "ARM64" [Debug (default) | Release]
@REM build.bat "PATH" "2017 | 2019 | 2022" "x86 | x64" | "ARM" | "ARM64"
@REM build.bat "PATH" "2017 | 2019 | 2022" "x86 | x64" | "ARM" | "ARM64" [Debug (default) | Release]

@ECHO OFF

SETLOCAL ENABLEDELAYEDEXPANSION

IF EXIST "%1/CMakeLists.txt" (
        SET WORK_PATH=%1
    IF /I "%2" == "x86" (
        SET ARCH=Win32
        SET ARCH_NAME=x86
    )
    IF /I "%2" == "x64" (
        SET ARCH=x64
        SET ARCH_NAME=x64
    )
    IF /I "%2" == "Win32" (
        SET ARCH=Win32
        SET ARCH_NAME=x86
    )
    IF /I "%2" == "Win64" (
        SET ARCH=x64
        SET ARCH_NAME=x64
    )
    IF /I "%2" == "ARM" (
        SET ARCH=ARM
        SET ARCH_NAME=ARM
    )
    IF /I "%2" == "ARM64" (
        SET ARCH=ARM64
        SET ARCH_NAME=ARM64
    )
    IF /I "%3" == "Debug" (
        SET CONFIG=Debug
    )
    IF /I "%3" == "Release" (
        SET CONFIG=Release
    )
    SET BUILD_PATH=!WORK_PATH!/build_!ARCH_NAME!

    IF /I "!ARCH!" == "" (
        IF %2 == 2017 (
            SET GENERATOR="Visual Studio 15 2017"
            SET VS_VERSION=2017
        )
        IF %2 == 2019 (
            SET GENERATOR="Visual Studio 16 2019"
            SET VS_VERSION=2019
        )
        IF %2 == 2022 (
            SET GENERATOR="Visual Studio 17 2022"
            SET VS_VERSION=2022
        )
        IF /I "%3" == "x86" (
            SET ARCH=Win32
            SET ARCH_NAME=x86
        )
        IF /I "%3" == "x64" (
            SET ARCH=x64
            SET ARCH_NAME=x64
        )
        IF /I "%3" == "Win32" (
            SET ARCH=Win32
            SET ARCH_NAME=x86
        )
        IF /I "%3" == "Win64" (
            SET ARCH=x64
            SET ARCH_NAME=x64
        )
        IF /I "%3" == "ARM" (
            SET ARCH=ARM
            SET ARCH_NAME=ARM
        )
        IF /I "%3" == "ARM64" (
            SET ARCH=ARM64
            SET ARCH_NAME=ARM64
        )
        IF "!VS_VERSION!" == "" (
            SET BUILD_PATH=!WORK_PATH!/build_!ARCH_NAME!
        ) ELSE (
            SET BUILD_PATH=!WORK_PATH!/build_!VS_VERSION!_!ARCH_NAME!
        )
        IF /I "%4" == "Debug" (
            SET CONFIG=Debug
        )
        IF /I "%4" == "Release" (
            SET CONFIG=Release
        )
    )

    IF NOT "!ARCH!" == "" (
        SET CMAKE_ARCH=!ARCH!
        SET CMAKE_SDK_ARGS=-DLDK_WDK_VERSION=10.0.22621.0 -DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM=10.0.22621.0
        IF /I "!ARCH!" == "ARM" (
            SET CMAKE_ARCH=ARM,version=10.0.22621.0
            SET CMAKE_SDK_ARGS=-DLDK_WDK_VERSION=10.0.22621.0 -DCMAKE_SYSTEM_VERSION=10.0.22621.0 -DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION=10.0.22621.0 -DCMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION_MAXIMUM=10.0.22621.0
        )
        IF "!GENERATOR!" == "" (
            ECHO Unsupported Visual Studio.
            ECHO Use Visual Studio set as CMake default generator.
            ECHO cmake -S !WORK_PATH! -B !BUILD_PATH! -A !CMAKE_ARCH! -DCMAKE_CXX_FLAGS=/MP !CMAKE_SDK_ARGS!
            cmake -S !WORK_PATH! -B !BUILD_PATH! -A !CMAKE_ARCH! -DCMAKE_CXX_FLAGS=/MP !CMAKE_SDK_ARGS!
        ) ELSE (
            ECHO cmake -S !WORK_PATH! -B !BUILD_PATH! -A !CMAKE_ARCH! -DCMAKE_CXX_FLAGS=/MP !CMAKE_SDK_ARGS! -G !GENERATOR!
            cmake -S !WORK_PATH! -B !BUILD_PATH! -A !CMAKE_ARCH! -DCMAKE_CXX_FLAGS=/MP !CMAKE_SDK_ARGS! -G !GENERATOR!
        )
        IF ERRORLEVEL 1 (
            EXIT /B !ERRORLEVEL!
        )
        IF "!CONFIG!" == "" (
            ECHO cmake --build !BUILD_PATH!
            cmake --build !BUILD_PATH!
        ) ELSE (
            ECHO cmake --build !BUILD_PATH! --config !CONFIG!
            cmake --build !BUILD_PATH! --config !CONFIG!
        )
    ) ELSE (
        ECHO Unsupported architecture.
    )
) ELSE (
    ECHO "%1/CMakeLists.txt not exist"
)
ENDLOCAL
