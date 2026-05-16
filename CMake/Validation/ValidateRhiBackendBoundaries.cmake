if(NOT DEFINED RHI_BACKEND_BOUNDARY_SOURCE_DIR)
    set(RHI_BACKEND_BOUNDARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    RHI_BACKEND_BOUNDARY_SOURCE_DIR
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH RHI_BACKEND_BOUNDARY_SOURCE_DIR)

set(RHI_BACKEND_BOUNDARY_VIOLATIONS "")

function(append_rhi_backend_violation message_text)
    set(RHI_BACKEND_BOUNDARY_VIOLATIONS
        "${RHI_BACKEND_BOUNDARY_VIOLATIONS}${message_text}\n"
        PARENT_SCOPE)
endfunction()

function(read_required_file file_path out_text)
    if(NOT EXISTS "${file_path}")
        append_rhi_backend_violation("missing required file: ${file_path}")
        set(${out_text} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${file_path}" file_text)
    set(${out_text} "${file_text}" PARENT_SCOPE)
endfunction()

function(require_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_rhi_backend_violation("${relative_path}: missing '${token}': ${description}")
    endif()
endfunction()

function(forbid_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(NOT match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_rhi_backend_violation("${relative_path}: found forbidden '${token}': ${description}")
    endif()
endfunction()

function(is_rhi_backend_private_path relative_path out_var)
    if(relative_path MATCHES "^Engine/RHI/Private/(D3D12|Vulkan)/")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(is_third_party_or_generated_path relative_path out_var)
    if(relative_path MATCHES "(^|/)Private/ThirdParty/" OR relative_path MATCHES "(^|/)third_party/" OR relative_path MATCHES "(^|/)build/")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(check_file_for_backend_tokens file_path)
    set(options)
    set(one_value_args DESCRIPTION)
    set(multi_value_args TOKENS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
    is_third_party_or_generated_path("${relative_path}" should_skip)
    if(should_skip)
        return()
    endif()

    file(READ "${file_path}" file_text)
    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            append_rhi_backend_violation("${relative_path}: found forbidden '${token}': ${CHECK_DESCRIPTION}")
        endif()
    endforeach()
endfunction()

set(rhi_cmake_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/CMakeLists.txt")
read_required_file("${rhi_cmake_path}" rhi_cmake_text)
if(rhi_cmake_text)
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "option(SPARKLE_RHI_WITH_D3D12" "backend enablement must be explicit")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "option(SPARKLE_RHI_WITH_VULKAN" "backend enablement must be explicit")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "set(SPARKLE_RHI_DEFAULT_BACKEND" "build default backend must be explicit")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "add_library(SparkleRHICommon OBJECT" "common RHI implementation must be separated from backend implementation")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "add_library(SparkleRHI_D3D12 STATIC" "D3D12 backend must have its own target")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "add_library(SparkleRHI_Vulkan INTERFACE" "Vulkan placeholder target must exist behind its option")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_COMMON_PRIVATE_SOURCES" "common source glob must be named and separated")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_D3D12_SOURCES" "D3D12 source glob must be named and separated")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "list(FILTER SPARKLE_RHI_COMMON_PRIVATE_SOURCES EXCLUDE REGEX \"/Private/D3D12/\")" "D3D12 sources must be excluded from common RHI sources")
    forbid_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_PRIVATE_SOURCES" "do not restore one undifferentiated private source glob")
    forbid_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_THIRD_PARTY_SOURCES" "backend third-party sources must be named per backend")
endif()

set(renderer_cmake_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/Renderer/CMakeLists.txt")
read_required_file("${renderer_cmake_path}" renderer_cmake_text)
if(renderer_cmake_text)
    foreach(token IN ITEMS d3d12 dxgi d3dcompiler dxguid vulkan SparkleD3D12MA SparkleVMA VMA)
        forbid_text("${renderer_cmake_path}" "${renderer_cmake_text}" "${token}" "Renderer must link through RHI targets instead of raw backend APIs or allocator libraries")
    endforeach()
endif()

set(common_rhi_private_root "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private")
if(EXISTS "${common_rhi_private_root}")
    file(GLOB_RECURSE rhi_private_files
        "${common_rhi_private_root}/*.h"
        "${common_rhi_private_root}/*.hpp"
        "${common_rhi_private_root}/*.cpp"
        "${common_rhi_private_root}/*.cxx"
    )

    foreach(rhi_private_file IN LISTS rhi_private_files)
        cmake_path(RELATIVE_PATH rhi_private_file BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        is_rhi_backend_private_path("${relative_path}" is_backend_path)
        if(is_backend_path)
            continue()
        endif()

        check_file_for_backend_tokens(
            "${rhi_private_file}"
            TOKENS
                "#include \"D3D12/"
                "#include <d3d12"
                "#include <dxgi"
                "#include \"Vulkan/"
                "#include <vulkan/"
                "D3D12MemAlloc"
                "vk_mem_alloc"
            DESCRIPTION "common RHI private code must not include backend-private native headers or allocator libraries"
        )
    endforeach()
endif()

set(high_level_roots
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/Application"
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/Editor"
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/GameFramework"
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/Renderer"
)

foreach(high_level_root IN LISTS high_level_roots)
    if(NOT EXISTS "${high_level_root}")
        continue()
    endif()

    file(GLOB_RECURSE high_level_files
        "${high_level_root}/*.h"
        "${high_level_root}/*.hpp"
        "${high_level_root}/*.cpp"
        "${high_level_root}/*.cxx"
        "${high_level_root}/CMakeLists.txt"
    )

    foreach(high_level_file IN LISTS high_level_files)
        check_file_for_backend_tokens(
            "${high_level_file}"
            TOKENS
                "#include \"D3D12/"
                "#include <d3d12"
                "#include <dxgi"
                "#include \"Vulkan/"
                "#include <vulkan/"
                "D3D12MemAlloc"
                "vk_mem_alloc"
            DESCRIPTION "Application, Editor, GameFramework, and Renderer must use backend-neutral RHI surfaces"
        )
    endforeach()
endforeach()

if(RHI_BACKEND_BOUNDARY_VIOLATIONS)
    string(PREPEND RHI_BACKEND_BOUNDARY_VIOLATIONS
        "RHI backend boundary validation failed. Keep common RHI and Renderer backend-neutral, and keep backend API libraries on backend targets.\n")
    message(FATAL_ERROR "${RHI_BACKEND_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "RHI backend boundary check passed for CMake target split, Renderer link hygiene, and common-source backend leakage.")