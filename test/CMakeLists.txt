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

file(GLOB SOURCE_FILES ./*.c ./*/*.c)

wdk_add_driver(LdkTest CUSTOM_ENTRY_POINT "LdkDriverEntry" ${SOURCE_FILES})

target_link_libraries(LdkTest Ldk)