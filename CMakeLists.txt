cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(POLICY CMP0149)
    cmake_policy(SET CMP0149 OLD)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/")
include(pkg-utils)

pkg_get_version(LDK ${CMAKE_CURRENT_SOURCE_DIR}/include/ldk/internal/version.h VERSION)

project(Ldk VERSION "${VERSION}" LANGUAGES C)

option(LDK_ENABLE_DIAGNOSTIC_BREAKPOINTS "Set to OFF to suppress LDK diagnostic KdBreakPoint calls" ON)

if ("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
    option(LDK_USE_RAISE_EXCEPTION  "Set to ON to use ldk!RaiseException" ON)
    option(LDK_USE_RTL_RAISE_EXCEPTION  "Set to ON to use ldk!RtlRaiseExceptionn" ON)
else()
    option(LDK_USE_RAISE_EXCEPTION  "Set to ON to use ldk!RaiseException" OFF)
    option(LDK_USE_RTL_RAISE_EXCEPTION  "Set to ON to use ldk!RtlRaiseExceptionn" OFF)
endif()

include(CPM)

CPMAddPackage("gh:ntoskrnl7/FindWDK#master")
if(CMAKE_VS_PLATFORM_NAME AND CMAKE_GENERATOR_PLATFORM MATCHES ",")
    # FindWDK derives library names from CMAKE_GENERATOR_PLATFORM and does not
    # understand Visual Studio's ",version=..." platform option.
    set(CMAKE_GENERATOR_PLATFORM "${CMAKE_VS_PLATFORM_NAME}")
endif()

list(APPEND CMAKE_MODULE_PATH "${FindWDK_SOURCE_DIR}/cmake")
find_package(WDK REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")

file(GLOB SOURCE_FILES ./src/*.c ./src/*/*.c ./src/ntdll/rtl/*.c)
list(APPEND SOURCE_FILES
    ./src/ntdll/alert.c
    ./src/ntdll/waitonaddress.c
)
list(REMOVE_DUPLICATES SOURCE_FILES)

wdk_add_library(Ldk STATIC ${SOURCE_FILES})

target_compile_definitions(Ldk PUBLIC -D_KERNEL32_)
if (NOT LDK_ENABLE_DIAGNOSTIC_BREAKPOINTS)
    target_compile_definitions(Ldk PRIVATE -DLDK_ENABLE_DIAGNOSTIC_BREAKPOINTS=0)
endif()
if (LDK_USE_RAISE_EXCEPTION)
    target_compile_definitions(Ldk PUBLIC -D_LDK_DEFINE_RAISE_EXCEPTION)
endif()
if (LDK_USE_RTL_RAISE_EXCEPTION)
    target_compile_definitions(Ldk PUBLIC -D_LDK_DEFINE_RTL_RAISE_EXCEPTION)
    # ntoskrnl.lib(ntoskrnl.exe) : error LNK2005: RtlRaiseException already defined in Ldk.lib(except.obj).
    target_link_options(Ldk PUBLIC "/FORCE:MULTIPLE")
endif()

set(LDK_PLATFORM_NAME "${CMAKE_VS_PLATFORM_NAME}")
if("${LDK_PLATFORM_NAME}" STREQUAL "")
    set(LDK_PLATFORM_NAME "${CMAKE_GENERATOR_PLATFORM}")
endif()

string( TOLOWER "${LDK_PLATFORM_NAME}" PLATFROM_SHORT_NAME )
if( PLATFROM_SHORT_NAME STREQUAL "win32" )
    set(PLATFROM_SHORT_NAME "x86")
elseif( PLATFROM_SHORT_NAME STREQUAL "arm" )
    set(PLATFROM_SHORT_NAME "ARM")
elseif( PLATFROM_SHORT_NAME STREQUAL "arm64" )
    set(PLATFROM_SHORT_NAME "ARM64")
endif()

set_target_properties(Ldk PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFROM_SHORT_NAME})

target_include_directories(Ldk PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

#
# Generate Debug Info
#
target_compile_options(Ldk PUBLIC "$<$<CONFIG:Release>:/Zi>")
target_link_options(Ldk PUBLIC "$<$<CONFIG:Release>:/DEBUG>")

#
# set library output path (release only)
#
string(TOLOWER "${LDK_PLATFORM_NAME}" PLATFROM_SHORT_NAME)
if(PLATFROM_SHORT_NAME STREQUAL "win32")
    set(PLATFROM_SHORT_NAME "x86")
elseif(PLATFROM_SHORT_NAME STREQUAL "arm")
    set(PLATFROM_SHORT_NAME "ARM")
elseif(PLATFROM_SHORT_NAME STREQUAL "arm64")
    set(PLATFROM_SHORT_NAME "ARM64")
endif()
set_target_properties(Ldk PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/lib/${PLATFROM_SHORT_NAME})
