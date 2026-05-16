if(NOT DEFINED RHI_MEMORY_BOUNDARY_SOURCE_DIR)
    set(RHI_MEMORY_BOUNDARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    RHI_MEMORY_BOUNDARY_SOURCE_DIR
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH RHI_MEMORY_BOUNDARY_SOURCE_DIR)

set(RHI_MEMORY_ENGINE_ROOT "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine")

set(RHI_MEMORY_PUBLIC_HEADER_ROOTS
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/RHI/Public"
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/Application/Public"
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/Editor/Public"
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/GameFramework/Public"
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/Renderer/Public"
)

set(RHI_MEMORY_HIGH_LEVEL_SOURCE_ROOTS
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/Application"
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/Editor"
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/GameFramework"
    "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}/Engine/Renderer"
)

set(FORBIDDEN_ALLOCATOR_PUBLIC_TOKENS
    "D3D12MA::"
    "D3D12MemAlloc"
    "VmaAllocator"
    "VmaAllocation"
    "vk_mem_alloc"
)

set(FORBIDDEN_BACKEND_MEMORY_PUBLIC_TOKENS
    "D3D12Memory"
    "VulkanMemory"
)

set(FORBIDDEN_DIRECT_D3D12_ALLOCATION_TOKENS
    "CreateCommittedResource"
    "CreateHeap"
    "CreatePlacedResource"
)

set(FORBIDDEN_DEFRAGMENTATION_TOKENS
    "BeginDefragmentation"
    "DefragmentationPass"
    "DEFRAGMENTATION"
)

set(FORBIDDEN_DIRECT_ALLOCATOR_JSON_TOKENS
    "BuildStatsString"
    "vmaBuildStatsString"
    "vmaFreeStatsString"
)

set(RHI_MEMORY_BOUNDARY_VIOLATIONS "")

# Temporary legacy baseline for direct D3D12 resource allocation before the
# D3D12MA migration. Counts are maximums, so deleting old calls is allowed but
# adding new calls outside the memory service fails this gate.
set(LEGACY_DIRECT_D3D12_ALLOCATION_LIMITS
    "Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp|CreateCommittedResource|5"
    "Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp|CreateHeap|1"
    "Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp|CreatePlacedResource|2"
    "Engine/RHI/Private/D3D12/Diagnostics/D3D12RenderDiagnostics.cpp|CreateCommittedResource|1"
    "Engine/RHI/Private/D3D12/Resources/D3D12ConstantBuffer.h|CreateCommittedResource|1"
    "Engine/RHI/Private/D3D12/Resources/D3D12LinearAllocator.cpp|CreateCommittedResource|1"
    "Engine/RHI/Private/D3D12/Resources/D3D12Texture.cpp|CreateCommittedResource|2"
    "Engine/RHI/Private/D3D12/Resources/D3D12UploadBuffer.cpp|CreateCommittedResource|1"
)

function(append_rhi_memory_violation message_text)
    set(RHI_MEMORY_BOUNDARY_VIOLATIONS
        "${RHI_MEMORY_BOUNDARY_VIOLATIONS}${message_text}\n"
        PARENT_SCOPE)
endfunction()

function(is_generated_or_third_party_path relative_path out_var)
    if(relative_path MATCHES "(^|/)Private/ThirdParty/" OR relative_path MATCHES "(^|/)third_party/")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(is_d3d12_memory_service_path relative_path out_var)
    if(relative_path MATCHES "^Engine/RHI/Private/D3D12/Memory/.*\\.(h|hpp|cpp|cxx)$")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(count_token_occurrences text token out_var)
    string(LENGTH "${text}" original_length)
    string(LENGTH "${token}" token_length)
    string(REPLACE "${token}" "" text_without_token "${text}")
    string(LENGTH "${text_without_token}" replaced_length)
    math(EXPR removed_length "${original_length} - ${replaced_length}")
    math(EXPR token_count "${removed_length} / ${token_length}")

    set(${out_var} ${token_count} PARENT_SCOPE)
endfunction()

function(get_legacy_direct_allocation_limit relative_path token out_var)
    set(limit 0)
    foreach(limit_entry IN LISTS LEGACY_DIRECT_D3D12_ALLOCATION_LIMITS)
        string(REPLACE "|" ";" limit_fields "${limit_entry}")
        list(GET limit_fields 0 limit_path)
        list(GET limit_fields 1 limit_token)
        list(GET limit_fields 2 limit_count)
        if(relative_path STREQUAL limit_path AND token STREQUAL limit_token)
            set(limit ${limit_count})
        endif()
    endforeach()

    set(${out_var} ${limit} PARENT_SCOPE)
endfunction()

function(check_file_for_tokens file_path)
    set(options)
    set(one_value_args DESCRIPTION)
    set(multi_value_args TOKENS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
    is_generated_or_third_party_path("${relative_path}" should_skip)
    if(should_skip)
        return()
    endif()

    file(READ "${file_path}" file_text)
    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            append_rhi_memory_violation("${relative_path}: found forbidden token '${token}': ${CHECK_DESCRIPTION}")
        endif()
    endforeach()
endfunction()

function(check_file_for_direct_d3d12_allocations file_path)
    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_MEMORY_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
    is_generated_or_third_party_path("${relative_path}" should_skip)
    if(should_skip)
        return()
    endif()

    is_d3d12_memory_service_path("${relative_path}" is_memory_service)
    if(is_memory_service)
        return()
    endif()

    file(READ "${file_path}" file_text)
    foreach(token IN LISTS FORBIDDEN_DIRECT_D3D12_ALLOCATION_TOKENS)
        count_token_occurrences("${file_text}" "${token}" token_count)
        if(token_count EQUAL 0)
            continue()
        endif()

        get_legacy_direct_allocation_limit("${relative_path}" "${token}" legacy_limit)
        if(token_count GREATER legacy_limit)
            append_rhi_memory_violation(
                "${relative_path}: found ${token_count} '${token}' call(s), allowed ${legacy_limit}. Route GPU resource allocation through Engine/RHI/Private/D3D12/Memory/D3D12GpuMemoryAllocator instead."
            )
        endif()
    endforeach()
endfunction()

foreach(public_header_root IN LISTS RHI_MEMORY_PUBLIC_HEADER_ROOTS)
    if(NOT EXISTS "${public_header_root}")
        continue()
    endif()

    file(GLOB_RECURSE public_header_files
        "${public_header_root}/*.h"
        "${public_header_root}/*.hpp"
    )

    foreach(public_header_file IN LISTS public_header_files)
        check_file_for_tokens(
            "${public_header_file}"
            TOKENS ${FORBIDDEN_ALLOCATOR_PUBLIC_TOKENS}
            DESCRIPTION "allocator implementation details must stay behind backend-private RHI memory services"
        )
        check_file_for_tokens(
            "${public_header_file}"
            TOKENS ${FORBIDDEN_BACKEND_MEMORY_PUBLIC_TOKENS}
            DESCRIPTION "public memory diagnostics/types must use backend-neutral names"
        )
    endforeach()
endforeach()

foreach(high_level_source_root IN LISTS RHI_MEMORY_HIGH_LEVEL_SOURCE_ROOTS)
    if(NOT EXISTS "${high_level_source_root}")
        continue()
    endif()

    file(GLOB_RECURSE high_level_source_files
        "${high_level_source_root}/*.h"
        "${high_level_source_root}/*.hpp"
        "${high_level_source_root}/*.cpp"
        "${high_level_source_root}/*.cxx"
    )

    foreach(high_level_source_file IN LISTS high_level_source_files)
        check_file_for_tokens(
            "${high_level_source_file}"
            TOKENS ${FORBIDDEN_ALLOCATOR_PUBLIC_TOKENS}
            DESCRIPTION "Application, Editor, GameFramework, and Renderer must not call D3D12MA/VMA directly"
        )
        check_file_for_tokens(
            "${high_level_source_file}"
            TOKENS ${FORBIDDEN_DIRECT_ALLOCATOR_JSON_TOKENS}
            DESCRIPTION "allocator JSON/stat dump access must go through RenderMemoryDiagnostics"
        )
    endforeach()
endforeach()

if(EXISTS "${RHI_MEMORY_ENGINE_ROOT}")
    file(GLOB_RECURSE engine_source_files
        "${RHI_MEMORY_ENGINE_ROOT}/*.h"
        "${RHI_MEMORY_ENGINE_ROOT}/*.hpp"
        "${RHI_MEMORY_ENGINE_ROOT}/*.cpp"
        "${RHI_MEMORY_ENGINE_ROOT}/*.cxx"
    )

    foreach(engine_source_file IN LISTS engine_source_files)
        check_file_for_tokens(
            "${engine_source_file}"
            TOKENS ${FORBIDDEN_DEFRAGMENTATION_TOKENS}
            DESCRIPTION "D3D12MA/VMA defragmentation is out of scope until Sparkle has a relocation policy"
        )
        check_file_for_direct_d3d12_allocations("${engine_source_file}")
    endforeach()
endif()

if(RHI_MEMORY_BOUNDARY_VIOLATIONS)
    string(PREPEND RHI_MEMORY_BOUNDARY_VIOLATIONS
        "RHI memory boundary validation failed. Keep allocator libraries backend-private, route GPU resource allocation through the D3D12 memory service, keep public memory contracts backend-neutral, and leave defragmentation out of this scope.\n")
    message(FATAL_ERROR "${RHI_MEMORY_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "RHI memory boundary check passed for allocator leakage, direct D3D12 allocation baseline, defragmentation scope, and diagnostics routing.")