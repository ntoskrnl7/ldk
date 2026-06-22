if(NOT DEFINED LDK_USE_DRIVER_ENTRY)
    set(LDK_USE_DRIVER_ENTRY ON)
endif()

if(DEFINED ldk_SOURCE_DIR)
    set(_LDK_ROOT "${ldk_SOURCE_DIR}")
elseif(DEFINED Ldk_SOURCE_DIR)
    set(_LDK_ROOT "${Ldk_SOURCE_DIR}")
elseif(DEFINED ldk_ROOT)
    set(_LDK_ROOT "${ldk_ROOT}")
elseif(DEFINED LDK_ROOT)
    set(_LDK_ROOT "${LDK_ROOT}")
else()
    get_filename_component(_LDK_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

if(NOT DEFINED LDK_USE_PREBUILT)
    if(NOT TARGET Ldk AND EXISTS "${_LDK_ROOT}/lib/native")
        set(LDK_USE_PREBUILT ON)
    else()
        set(LDK_USE_PREBUILT OFF)
    endif()
endif()

if(EXISTS "${_LDK_ROOT}/cmake/CPM.cmake")
    include("${_LDK_ROOT}/cmake/CPM.cmake")
else()
    set(CPM_DOWNLOAD_VERSION 0.32.0)

    if(CPM_SOURCE_CACHE)
        get_filename_component(CPM_SOURCE_CACHE ${CPM_SOURCE_CACHE} ABSOLUTE)
        set(CPM_DOWNLOAD_LOCATION "${CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    elseif(DEFINED ENV{CPM_SOURCE_CACHE})
        set(CPM_DOWNLOAD_LOCATION "$ENV{CPM_SOURCE_CACHE}/cpm/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    else()
        set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
    endif()

    if(NOT EXISTS ${CPM_DOWNLOAD_LOCATION})
        message(STATUS "Downloading CPM.cmake to ${CPM_DOWNLOAD_LOCATION}")
        file(DOWNLOAD
            https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake
            ${CPM_DOWNLOAD_LOCATION})
    endif()

    include(${CPM_DOWNLOAD_LOCATION})
endif()

CPMAddPackage("gh:ntoskrnl7/FindWDK#master")
list(APPEND CMAKE_MODULE_PATH "${FindWDK_SOURCE_DIR}/cmake")
find_package(WDK REQUIRED)

function(ldk_import_wdk_libraries _version _out_imported)
    file(GLOB _ldk_wdk_libraries "${WDK_ROOT}/Lib/${_version}/km/${WDK_PLATFORM}/*.lib")
    if(NOT _ldk_wdk_libraries)
        set(${_out_imported} OFF PARENT_SCOPE)
        return()
    endif()

    foreach(_library IN LISTS _ldk_wdk_libraries)
        get_filename_component(_library_name "${_library}" NAME_WE)
        string(TOUPPER "${_library_name}" _library_name)

        if(NOT TARGET WDK::${_library_name})
            add_library(WDK::${_library_name} INTERFACE IMPORTED)
        endif()

        set_property(TARGET WDK::${_library_name} PROPERTY INTERFACE_LINK_LIBRARIES "${_library}")
    endforeach()

    set(${_out_imported} ON PARENT_SCOPE)
endfunction()

function(ldk_get_installed_wdk_library_versions _out_versions)
    file(GLOB _ldk_wdk_library_dirs LIST_DIRECTORIES true "${WDK_ROOT}/Lib/*/km/${WDK_PLATFORM}")
    set(_versions)

    foreach(_library_dir IN LISTS _ldk_wdk_library_dirs)
        get_filename_component(_km_dir "${_library_dir}" DIRECTORY)
        get_filename_component(_version_dir "${_km_dir}" DIRECTORY)
        get_filename_component(_version "${_version_dir}" NAME)
        list(APPEND _versions "${_version}")
    endforeach()

    list(REMOVE_DUPLICATES _versions)
    set(${_out_versions} "${_versions}" PARENT_SCOPE)
endfunction()

set(_LDK_WDK_VERSION_CANDIDATES)
foreach(_version IN ITEMS
        "${LDK_WDK_VERSION}"
        "${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION}"
        "${CMAKE_SYSTEM_VERSION}"
        "${WDK_VERSION}")
    if(NOT "${_version}" STREQUAL "")
        list(APPEND _LDK_WDK_VERSION_CANDIDATES "${_version}")
    endif()
endforeach()
ldk_get_installed_wdk_library_versions(_LDK_INSTALLED_WDK_LIBRARY_VERSIONS)
list(APPEND _LDK_WDK_VERSION_CANDIDATES ${_LDK_INSTALLED_WDK_LIBRARY_VERSIONS})
list(REMOVE_DUPLICATES _LDK_WDK_VERSION_CANDIDATES)

foreach(_version IN LISTS _LDK_WDK_VERSION_CANDIDATES)
    ldk_import_wdk_libraries("${_version}" _ldk_imported_wdk_libraries)
    if(_ldk_imported_wdk_libraries)
        if(NOT "${WDK_VERSION}" STREQUAL "${_version}")
            if(EXISTS "${WDK_ROOT}/Include/${_version}/km/ntddk.h")
                message(STATUS "Using WDK ${_version} headers and libraries for LDK CMake helpers")
                set(WDK_VERSION "${_version}")
            else()
                message(STATUS "Using WDK ${_version} libraries with WDK ${WDK_VERSION} headers for LDK CMake helpers")
            endif()
        endif()

        break()
    endif()
endforeach()

if(NOT TARGET WDK::NTOSKRNL)
    message(FATAL_ERROR "WDK::NTOSKRNL was not found for platform ${WDK_PLATFORM}. Set LDK_WDK_VERSION or CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION to an installed WDK version with kernel libraries.")
endif()

function(ldk_get_prebuilt_arch _out_arch)
    if("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "x64")
        set(_arch x64)
    elseif("${CMAKE_VS_PLATFORM_NAME}" STREQUAL "Win32")
        set(_arch x86)
    else()
        message(FATAL_ERROR "Unsupported LDK prebuilt platform: ${CMAKE_VS_PLATFORM_NAME}")
    endif()

    set(${_out_arch} "${_arch}" PARENT_SCOPE)
endfunction()

function(ldk_get_prebuilt_library _out_path _configuration)
    ldk_get_prebuilt_arch(_arch)

    if("${_configuration}" STREQUAL "Debug")
        set(_config_dir Debug)
    else()
        set(_config_dir Release)
    endif()

    set(_path "${_LDK_ROOT}/lib/native/${_arch}/${_config_dir}/Ldk.lib")
    file(TO_CMAKE_PATH "${_path}" _path)
    if(NOT EXISTS "${_path}")
        set(${_out_path} "" PARENT_SCOPE)
        return()
    endif()

    set(${_out_path} "${_path}" PARENT_SCOPE)
endfunction()

function(ldk_link_prebuilt_libraries _target)
    ldk_get_prebuilt_library(_ldk_debug Debug)
    ldk_get_prebuilt_library(_ldk_release Release)

    if(_ldk_debug AND _ldk_release)
        target_link_libraries(
            ${_target}
            debug "${_ldk_debug}"
            optimized "${_ldk_release}"
        )
    elseif(_ldk_debug)
        target_link_libraries(${_target} "${_ldk_debug}")
    elseif(_ldk_release)
        target_link_libraries(${_target} "${_ldk_release}")
    else()
        message(FATAL_ERROR "No LDK prebuilt libraries were found under ${_LDK_ROOT}/lib/native for ${CMAKE_VS_PLATFORM_NAME}.")
    endif()

    target_include_directories(${_target} PUBLIC "${_LDK_ROOT}/include")
    target_compile_definitions(${_target} PUBLIC "_KERNEL32_")
    target_link_options(${_target} PUBLIC "/FORCE:MULTIPLE")
endfunction()

function(ldk_add_driver _target)
    if(LDK_USE_DRIVER_ENTRY)
        wdk_add_driver(${_target} CUSTOM_ENTRY_POINT LdkDriverEntry ${ARGN})
    else()
        wdk_add_driver(${_target} ${ARGN})
    endif()

    if(LDK_USE_PREBUILT)
        ldk_link_prebuilt_libraries(${_target})
    else()
        if(NOT TARGET Ldk)
            message(FATAL_ERROR "Ldk target was not found. Add LDK with CPMAddPackage or use an unpacked LDK native release bundle.")
        endif()

        target_link_libraries(${_target} Ldk)
    endif()
endfunction()
