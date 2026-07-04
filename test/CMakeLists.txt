cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

if(POLICY CMP0149)
    cmake_policy(SET CMP0149 OLD)
endif()

project(LdkTest LANGUAGES C RC)

include(../cmake/CPM.cmake)

set(LDK_USE_RAISE_EXCEPTION ON)
set(LDK_USE_RTL_RAISE_EXCEPTION ON)
CPMAddPackage(NAME Ldk SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/..)
include("${CMAKE_CURRENT_LIST_DIR}/../cmake/Ldk.cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /WX")

file(GLOB SOURCE_FILES CONFIGURE_DEPENDS ./driver.c ./*/*.c)
list(FILTER SOURCE_FILES EXCLUDE REGEX ".*/win32/.*")

ldk_add_driver(LdkTest ${SOURCE_FILES})

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

add_library(Test SHARED dll.c dll.rc)
add_library(OrdinalImport SHARED ordinalimport.c)
add_library(AutoDependency SHARED autodependency.c)
add_library(DependencyOwner SHARED dependencyowner.c)
add_library(FailAttach SHARED failattach.c)
add_library(DelayDependency SHARED delaydependency.c)
add_library(DelayOwner SHARED delayowner.c)
add_library(TlsCallback SHARED tlscallback.c)
add_library(ThreadNotify SHARED threadnotify.c)
target_link_libraries(OrdinalImport PRIVATE Test)
target_link_libraries(DependencyOwner PRIVATE AutoDependency)
target_link_libraries(FailAttach PRIVATE AutoDependency)
target_link_libraries(DelayOwner PRIVATE DelayDependency delayimp)
set_property(TARGET Test APPEND_STRING PROPERTY LINK_FLAGS "/NOENTRY")
set_property(TARGET OrdinalImport APPEND_STRING PROPERTY LINK_FLAGS "/NOENTRY")
set_property(TARGET AutoDependency APPEND_STRING PROPERTY LINK_FLAGS "/NOENTRY")
set_property(TARGET DependencyOwner APPEND_STRING PROPERTY LINK_FLAGS "/NOENTRY")
set_property(TARGET DelayDependency APPEND_STRING PROPERTY LINK_FLAGS "/NOENTRY")
set_property(TARGET DelayOwner APPEND_STRING PROPERTY LINK_FLAGS "/NOENTRY /DELAYLOAD:DelayDependency.dll")
if(CMAKE_VS_PLATFORM_NAME STREQUAL "Win32" OR CMAKE_GENERATOR_PLATFORM STREQUAL "Win32")
    set_property(TARGET FailAttach APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:FailAttachDllMain@12 /NODEFAULTLIB")
    set_property(TARGET TlsCallback APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:TlsCallbackDllMain@12 /NODEFAULTLIB")
    set_property(TARGET ThreadNotify APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:ThreadNotifyDllMain@12 /NODEFAULTLIB")
else()
    set_property(TARGET FailAttach APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:FailAttachDllMain /NODEFAULTLIB")
    set_property(TARGET TlsCallback APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:TlsCallbackDllMain /NODEFAULTLIB")
    set_property(TARGET ThreadNotify APPEND_STRING PROPERTY LINK_FLAGS " /ENTRY:ThreadNotifyDllMain /NODEFAULTLIB")
endif()
set_property(TARGET TlsCallback APPEND_STRING PROPERTY LINK_FLAGS " /INCREMENTAL:NO")
set_property(TARGET ThreadNotify APPEND_STRING PROPERTY LINK_FLAGS " /INCREMENTAL:NO")
target_compile_options(FailAttach PRIVATE /GS-)
target_compile_options(TlsCallback PRIVATE /GS-)
target_compile_options(ThreadNotify PRIVATE /GS-)
add_dependencies(LdkTest Test OrdinalImport AutoDependency DependencyOwner FailAttach DelayDependency DelayOwner TlsCallback ThreadNotify)
