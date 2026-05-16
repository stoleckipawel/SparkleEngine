if(NOT DEFINED FRAMEGRAPH_BOUNDARY_SOURCE_DIR)
    set(FRAMEGRAPH_BOUNDARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    FRAMEGRAPH_BOUNDARY_SOURCE_DIR
    "${FRAMEGRAPH_BOUNDARY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH FRAMEGRAPH_BOUNDARY_SOURCE_DIR)

set(FRAMEGRAPH_PRIVATE_ROOT "${FRAMEGRAPH_BOUNDARY_SOURCE_DIR}/Engine/Renderer/Private/FrameGraph")
set(FRAMEGRAPH_PUBLIC_ROOT "${FRAMEGRAPH_BOUNDARY_SOURCE_DIR}/Engine/Renderer/Public/FrameGraph")
set(FRAMEGRAPH_ENGINE_ROOT "${FRAMEGRAPH_BOUNDARY_SOURCE_DIR}/Engine")

set(FORBIDDEN_PUBLIC_FRAMEGRAPH_TOKENS
    "D3D12"
    "DXGI"
    "ID3D12"
    "ComPtr"
    "NativeResourceHandle"
)

set(FORBIDDEN_RENDER_GRAPH_TOKENS
    "RenderGraph"
)

set(FORBIDDEN_PRIVATE_EXPORT_TOKENS
    "SPARKLE_RENDERER_API"
)

set(FORBIDDEN_PUBLIC_PRIVATE_INCLUDE_TOKENS
    "#include \"Renderer/Private"
    "#include \"Engine/Renderer/Private"
    "#include \"../Private"
    "#include \"../../Private"
)

set(FORBIDDEN_EXECUTION_COMPILER_TOKENS
    "FrameGraphCompiler.h"
    "FrameGraphCompiler"
    "FrameGraph/Compiler/FrameGraphPlan.h"
    "Renderer/Private/FrameGraph/Compiler/FrameGraphPlan.h"
)

set(FORBIDDEN_FRAMEGRAPH_DESCRIPTOR_IDENTITY_TOKENS
    "RhiDescriptorAllocation"
    "shaderResourceViewCpu"
    "shaderResourceViewGpu"
    "unorderedAccessViewCpu"
    "unorderedAccessViewGpu"
)

set(FRAMEGRAPH_BOUNDARY_VIOLATIONS "")

function(check_file_for_tokens file_path)
    set(options)
    set(one_value_args DESCRIPTION)
    set(multi_value_args TOKENS ALLOWED_RELATIVE_PATHS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${FRAMEGRAPH_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)

    foreach(allowed_relative_path IN LISTS CHECK_ALLOWED_RELATIVE_PATHS)
        if(relative_path STREQUAL allowed_relative_path)
            return()
        endif()
    endforeach()

    file(READ "${file_path}" file_text)

    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            if(CHECK_DESCRIPTION)
                set(message_suffix ": ${CHECK_DESCRIPTION}")
            else()
                set(message_suffix "")
            endif()
            set(FRAMEGRAPH_BOUNDARY_VIOLATIONS
                "${FRAMEGRAPH_BOUNDARY_VIOLATIONS}${relative_path}: found forbidden token '${token}'${message_suffix}\n"
                PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

if(EXISTS "${FRAMEGRAPH_PRIVATE_ROOT}")
    file(GLOB_RECURSE private_framegraph_headers
        "${FRAMEGRAPH_PRIVATE_ROOT}/*.h"
        "${FRAMEGRAPH_PRIVATE_ROOT}/*.hpp"
    )

    foreach(private_framegraph_header IN LISTS private_framegraph_headers)
        check_file_for_tokens(
            "${private_framegraph_header}"
            TOKENS ${FORBIDDEN_PRIVATE_EXPORT_TOKENS}
            DESCRIPTION "private FrameGraph headers must not become Renderer DLL export contracts"
        )

        check_file_for_tokens(
            "${private_framegraph_header}"
            TOKENS ${FORBIDDEN_FRAMEGRAPH_DESCRIPTOR_IDENTITY_TOKENS}
            DESCRIPTION "FrameGraph view identity must use logical RHI resource view handles, not D3D12-shaped descriptor allocations"
        )
    endforeach()

    file(GLOB_RECURSE private_framegraph_sources
        "${FRAMEGRAPH_PRIVATE_ROOT}/*.cpp"
        "${FRAMEGRAPH_PRIVATE_ROOT}/*.cxx"
    )

    foreach(private_framegraph_source IN LISTS private_framegraph_sources)
        check_file_for_tokens(
            "${private_framegraph_source}"
            TOKENS ${FORBIDDEN_FRAMEGRAPH_DESCRIPTOR_IDENTITY_TOKENS}
            DESCRIPTION "FrameGraph view identity must use logical RHI resource view handles, not D3D12-shaped descriptor allocations"
        )
    endforeach()

    file(GLOB_RECURSE framegraph_execution_sources
        "${FRAMEGRAPH_PRIVATE_ROOT}/Execution/*.h"
        "${FRAMEGRAPH_PRIVATE_ROOT}/Execution/*.hpp"
        "${FRAMEGRAPH_PRIVATE_ROOT}/Execution/*.cpp"
        "${FRAMEGRAPH_PRIVATE_ROOT}/Execution/*.cxx"
    )

    foreach(framegraph_execution_source IN LISTS framegraph_execution_sources)
        check_file_for_tokens(
            "${framegraph_execution_source}"
            TOKENS ${FORBIDDEN_EXECUTION_COMPILER_TOKENS}
            DESCRIPTION "execution code must consume compiled plans through FrameGraph.h, not compiler construction headers"
        )
    endforeach()
endif()

if(EXISTS "${FRAMEGRAPH_PUBLIC_ROOT}")
    file(GLOB_RECURSE public_framegraph_headers
        "${FRAMEGRAPH_PUBLIC_ROOT}/*.h"
        "${FRAMEGRAPH_PUBLIC_ROOT}/*.hpp"
    )

    foreach(public_framegraph_header IN LISTS public_framegraph_headers)
        check_file_for_tokens(
            "${public_framegraph_header}"
            TOKENS ${FORBIDDEN_PUBLIC_PRIVATE_INCLUDE_TOKENS}
            DESCRIPTION "public FrameGraph contracts must not include private Renderer headers"
        )
        check_file_for_tokens(
            "${public_framegraph_header}"
            TOKENS ${FORBIDDEN_PUBLIC_FRAMEGRAPH_TOKENS}
            DESCRIPTION "public FrameGraph contracts must stay backend-neutral"
        )
    endforeach()
endif()

if(EXISTS "${FRAMEGRAPH_ENGINE_ROOT}")
    file(GLOB_RECURSE engine_source_files
        "${FRAMEGRAPH_ENGINE_ROOT}/*.h"
        "${FRAMEGRAPH_ENGINE_ROOT}/*.hpp"
        "${FRAMEGRAPH_ENGINE_ROOT}/*.cpp"
        "${FRAMEGRAPH_ENGINE_ROOT}/*.cxx"
    )

    foreach(engine_source_file IN LISTS engine_source_files)
        check_file_for_tokens(
            "${engine_source_file}"
            TOKENS ${FORBIDDEN_RENDER_GRAPH_TOKENS}
            DESCRIPTION "Sparkle source graph-owned types must use FrameGraph naming"
        )
    endforeach()
endif()

if(FRAMEGRAPH_BOUNDARY_VIOLATIONS)
    string(PREPEND FRAMEGRAPH_BOUNDARY_VIOLATIONS
        "FrameGraph boundary validation failed. Keep graph contracts backend-neutral, private implementation-only headers unexported, and execution separated from compiler construction details.\n")
    message(FATAL_ERROR "${FRAMEGRAPH_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "FrameGraph boundary check passed for Renderer public/private graph seams.")
