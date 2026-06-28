cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(LdkWin32ApiTest LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(LDK_WIN32_TEST_SOURCES
    ../kernel32/cond.c
    ../kernel32/curdir.c
    ../kernel32/environ.c
    ../kernel32/fibers.c
    ../kernel32/file.c
    ../kernel32/formatmessage.c
    ../kernel32/heap.c
    ../kernel32/library.c
    ../kernel32/nls.c
    ../kernel32/pipe.c
    ../kernel32/process.c
    ../kernel32/synchapi.c
    ../kernel32/threadpool.c
    ../ntdll/curdir.c
    ../ntdll/environ.c
    ../ntdll/keyedevent.c
    ../ntdll/ldr.c
)

add_executable(LdkWin32ApiTest
    main.c
    ${LDK_WIN32_TEST_SOURCES}
)

add_library(Test SHARED ../dll.c)
add_library(OrdinalImport SHARED ../ordinalimport.c)
add_library(AutoDependency SHARED ../autodependency.c)
add_library(DependencyOwner SHARED ../dependencyowner.c)
add_library(FailAttach SHARED ../failattach.c)
add_library(DelayDependency SHARED ../delaydependency.c)
add_library(DelayOwner SHARED ../delayowner.c)
target_link_libraries(OrdinalImport PRIVATE Test)
target_link_libraries(DependencyOwner PRIVATE AutoDependency)
target_link_libraries(FailAttach PRIVATE AutoDependency)
target_link_libraries(DelayOwner PRIVATE DelayDependency delayimp)

target_compile_definitions(LdkWin32ApiTest PRIVATE
    WINVER=0x0A00
    _WIN32_WINNT=0x0A00
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    _CRT_SECURE_NO_WARNINGS
)

target_compile_options(LdkWin32ApiTest PRIVATE
    /W4
    /WX
    /FI"${CMAKE_CURRENT_LIST_DIR}/compat.h"
)

foreach(target Test OrdinalImport AutoDependency DependencyOwner FailAttach DelayDependency DelayOwner)
    target_compile_options(${target} PRIVATE
        /W4
        /WX
    )
endforeach()

if(CMAKE_VS_PLATFORM_NAME STREQUAL "Win32" OR CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
    target_link_options(FailAttach PRIVATE /ENTRY:FailAttachDllMain@12 /NODEFAULTLIB)
else()
    target_link_options(FailAttach PRIVATE /ENTRY:FailAttachDllMain /NODEFAULTLIB)
endif()
target_compile_options(FailAttach PRIVATE /GS-)
target_link_options(DelayOwner PRIVATE /DELAYLOAD:DelayDependency.dll)

target_link_libraries(LdkWin32ApiTest PRIVATE
    Synchronization
)

foreach(target LdkWin32ApiTest Test OrdinalImport AutoDependency DependencyOwner FailAttach DelayDependency DelayOwner)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin/MinSizeRel"
    )
endforeach()

add_dependencies(LdkWin32ApiTest Test OrdinalImport AutoDependency DependencyOwner FailAttach DelayDependency DelayOwner)
