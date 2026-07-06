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
if(CMAKE_VS_PLATFORM_NAME AND CMAKE_GENERATOR_PLATFORM MATCHES ",")
    # FindWDK derives library names from CMAKE_GENERATOR_PLATFORM and does not
    # understand Visual Studio's ",version=..." platform option.
    set(CMAKE_GENERATOR_PLATFORM "${CMAKE_VS_PLATFORM_NAME}")
endif()

list(APPEND CMAKE_MODULE_PATH "${FindWDK_SOURCE_DIR}/cmake")
find_package(WDK REQUIRED)

function(ldk_import_wdk_libraries _version _out_imported)
    set(_ldk_wdk_platform_candidates "${WDK_PLATFORM}")
    string(TOLOWER "${WDK_PLATFORM}" _ldk_wdk_platform_lower)
    string(TOUPPER "${WDK_PLATFORM}" _ldk_wdk_platform_upper)
    list(APPEND _ldk_wdk_platform_candidates
        "${_ldk_wdk_platform_lower}"
        "${_ldk_wdk_platform_upper}")
    list(REMOVE_DUPLICATES _ldk_wdk_platform_candidates)

    set(_ldk_wdk_libraries)
    foreach(_ldk_wdk_platform IN LISTS _ldk_wdk_platform_candidates)
        file(GLOB _ldk_wdk_platform_libraries "${WDK_ROOT}/Lib/${_version}/km/${_ldk_wdk_platform}/*.lib")
        list(APPEND _ldk_wdk_libraries ${_ldk_wdk_platform_libraries})
    endforeach()
    list(REMOVE_DUPLICATES _ldk_wdk_libraries)
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
    set(_ldk_wdk_platform_candidates "${WDK_PLATFORM}")
    string(TOLOWER "${WDK_PLATFORM}" _ldk_wdk_platform_lower)
    string(TOUPPER "${WDK_PLATFORM}" _ldk_wdk_platform_upper)
    list(APPEND _ldk_wdk_platform_candidates
        "${_ldk_wdk_platform_lower}"
        "${_ldk_wdk_platform_upper}")
    list(REMOVE_DUPLICATES _ldk_wdk_platform_candidates)

    set(_ldk_wdk_library_dirs)
    foreach(_ldk_wdk_platform IN LISTS _ldk_wdk_platform_candidates)
        file(GLOB _ldk_wdk_platform_library_dirs LIST_DIRECTORIES true "${WDK_ROOT}/Lib/*/km/${_ldk_wdk_platform}")
        list(APPEND _ldk_wdk_library_dirs ${_ldk_wdk_platform_library_dirs})
    endforeach()
    list(REMOVE_DUPLICATES _ldk_wdk_library_dirs)

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
if(NOT TARGET WDK::CNG)
    message(FATAL_ERROR "WDK::CNG was not found for platform ${WDK_PLATFORM}. Set LDK_WDK_VERSION or CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION to an installed WDK version with cng.lib.")
endif()

function(ldk_get_prebuilt_arch _out_arch)
    set(_ldk_platform_name "${CMAKE_VS_PLATFORM_NAME}")
    if("${_ldk_platform_name}" STREQUAL "")
        set(_ldk_platform_name "${CMAKE_GENERATOR_PLATFORM}")
    endif()

    string(TOLOWER "${_ldk_platform_name}" _ldk_platform_name_lower)

    if("${_ldk_platform_name_lower}" STREQUAL "x64")
        set(_arch x64)
    elseif("${_ldk_platform_name_lower}" STREQUAL "win32")
        set(_arch x86)
    elseif("${_ldk_platform_name_lower}" STREQUAL "arm")
        set(_arch ARM)
    elseif("${_ldk_platform_name_lower}" STREQUAL "arm64")
        set(_arch ARM64)
    else()
        message(FATAL_ERROR "Unsupported LDK prebuilt platform: ${_ldk_platform_name}")
    endif()

    set(${_out_arch} "${_arch}" PARENT_SCOPE)
endfunction()

function(ldk_get_prebuilt_toolset _out_toolset)
    if(DEFINED LDK_PREBUILT_TOOLSET AND NOT "${LDK_PREBUILT_TOOLSET}" STREQUAL "")
        set(_toolset "${LDK_PREBUILT_TOOLSET}")
    elseif(DEFINED MSVC_TOOLSET_VERSION AND NOT "${MSVC_TOOLSET_VERSION}" STREQUAL "")
        set(_toolset "v${MSVC_TOOLSET_VERSION}")
    elseif(DEFINED CMAKE_VS_PLATFORM_TOOLSET AND CMAKE_VS_PLATFORM_TOOLSET MATCHES "^v[0-9]+$")
        set(_toolset "${CMAKE_VS_PLATFORM_TOOLSET}")
    else()
        message(FATAL_ERROR "Unable to determine the LDK prebuilt MSVC toolset. Set LDK_PREBUILT_TOOLSET to v142, v143, or v145.")
    endif()

    if(NOT "${_toolset}" STREQUAL "v142" AND NOT "${_toolset}" STREQUAL "v143" AND NOT "${_toolset}" STREQUAL "v145")
        message(FATAL_ERROR "Unsupported LDK prebuilt MSVC toolset: ${_toolset}. Supported toolsets are v142, v143, and v145.")
    endif()

    set(${_out_toolset} "${_toolset}" PARENT_SCOPE)
endfunction()

function(ldk_get_prebuilt_library _out_path _configuration)
    ldk_get_prebuilt_arch(_arch)
    ldk_get_prebuilt_toolset(_toolset)

    if("${_configuration}" STREQUAL "Debug")
        set(_config_dir Debug)
    else()
        set(_config_dir Release)
    endif()

    set(_has_toolset_layout OFF)
    foreach(_known_toolset IN ITEMS v142 v143 v145)
        if(EXISTS "${_LDK_ROOT}/lib/native/${_known_toolset}")
            set(_has_toolset_layout ON)
        endif()
    endforeach()

    set(_candidate_paths
        "${_LDK_ROOT}/lib/native/${_toolset}/${_arch}/${_config_dir}/Ldk.lib"
    )

    if(DEFINED LDK_ALLOW_PREBUILT_TOOLSET_FALLBACK AND LDK_ALLOW_PREBUILT_TOOLSET_FALLBACK)
        if(NOT "${_toolset}" STREQUAL "v143")
            list(APPEND _candidate_paths
                "${_LDK_ROOT}/lib/native/v143/${_arch}/${_config_dir}/Ldk.lib"
            )
        endif()
    endif()

    if(NOT _has_toolset_layout)
        list(APPEND _candidate_paths
            "${_LDK_ROOT}/lib/native/${_arch}/${_config_dir}/Ldk.lib"
        )
    endif()

    foreach(_candidate_path IN LISTS _candidate_paths)
        file(TO_CMAKE_PATH "${_candidate_path}" _candidate_path)
        if(EXISTS "${_candidate_path}")
            set(${_out_path} "${_candidate_path}" PARENT_SCOPE)
            return()
        endif()
    endforeach()

    set(${_out_path} "" PARENT_SCOPE)
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
        ldk_get_prebuilt_arch(_arch)
        ldk_get_prebuilt_toolset(_toolset)
        message(FATAL_ERROR "No LDK prebuilt libraries were found under ${_LDK_ROOT}/lib/native for ${_toolset}/${_arch}.")
    endif()

    target_link_libraries(${_target} WDK::CNG)
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
