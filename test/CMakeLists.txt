cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(LdkTest LANGUAGES C)

include(../cmake/CPM.cmake)

set(LDK_USE_RAISE_EXCEPTION ON)
set(LDK_USE_RTL_RAISE_EXCEPTION ON)
CPMAddPackage(NAME Ldk SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)

CPMAddPackage("gh:ntoskrnl7/FindWDK#master")
list(APPEND CMAKE_MODULE_PATH "${FindWDK_SOURCE_DIR}/cmake")
find_package(WDK REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")

file(GLOB SOURCE_FILES ./driver.c ./*/*.c)
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*/win32/.*")

wdk_add_driver(LdkTest CUSTOM_ENTRY_POINT "LdkDriverEntry" ${SOURCE_FILES})

target_link_libraries(LdkTest Ldk)

foreach(flag
        CMAKE_C_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELWITHDEBINFO
        CMAKE_C_FLAGS_MINSIZEREL
        CMAKE_C_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_RELWITHDEBINFO
        CMAKE_CXX_FLAGS_MINSIZEREL
        CMAKE_CXX_FLAGS_DEBUG)
    string(REPLACE "/MDd"  "/MT" "${flag}" "${${flag}}")
    string(REPLACE "/MD"  "/MT" "${flag}" "${${flag}}")
    string(REPLACE "/MTd"  "/MT" "${flag}" "${${flag}}")
    string(REPLACE "/MT"  "/MT" "${flag}" "${${flag}}")
    string(REGEX REPLACE "/RTC(su|[1su])" "" "${flag}" "${${flag}}")
endforeach()

add_library(Test SHARED dll.c)
add_library(OrdinalImport SHARED ordinalimport.c)
target_link_libraries(OrdinalImport PRIVATE Test)
set_property(TARGET Test APPEND_STRING PROPERTY LINK_FLAGS "/NOENTRY")
set_property(TARGET OrdinalImport APPEND_STRING PROPERTY LINK_FLAGS "/NOENTRY")
add_dependencies(LdkTest Test OrdinalImport)
