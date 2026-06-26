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
target_link_libraries(OrdinalImport PRIVATE Test)

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

target_compile_options(Test PRIVATE
    /W4
    /WX
)

target_link_libraries(LdkWin32ApiTest PRIVATE
    Synchronization
)

foreach(target LdkWin32ApiTest Test OrdinalImport)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/bin/Debug"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin/Release"
        RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin/RelWithDebInfo"
        RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/bin/MinSizeRel"
    )
endforeach()

add_dependencies(LdkWin32ApiTest Test OrdinalImport)
