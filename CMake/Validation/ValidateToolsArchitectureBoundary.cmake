if(NOT DEFINED TOOLS_ARCHITECTURE_SOURCE_DIR)
    set(TOOLS_ARCHITECTURE_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    TOOLS_ARCHITECTURE_SOURCE_DIR
    "${TOOLS_ARCHITECTURE_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH TOOLS_ARCHITECTURE_SOURCE_DIR)

set(TOOLS_ARCHITECTURE_ROOT "${TOOLS_ARCHITECTURE_SOURCE_DIR}/Tools")
set_property(GLOBAL PROPERTY TOOLS_ARCHITECTURE_VIOLATIONS "")

function(append_tools_architecture_violation violation_text)
    set_property(GLOBAL APPEND PROPERTY TOOLS_ARCHITECTURE_VIOLATIONS "${violation_text}\n")
endfunction()

function(relative_tools_architecture_path input_path output_variable)
    cmake_path(RELATIVE_PATH input_path BASE_DIRECTORY "${TOOLS_ARCHITECTURE_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
    set(${output_variable} "${relative_path}" PARENT_SCOPE)
endfunction()

function(check_tools_file_for_tokens file_path)
    set(options)
    set(one_value_args CONTEXT)
    set(multi_value_args TOKENS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    file(READ "${file_path}" file_text)
    relative_tools_architecture_path("${file_path}" relative_path)

    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            append_tools_architecture_violation("${relative_path}: ${CHECK_CONTEXT} found forbidden token '${token}'")
        endif()
    endforeach()
endfunction()

set(ALLOWED_TOOL_PUBLIC_HEADERS
    "${TOOLS_ARCHITECTURE_ROOT}/CookCommon/Public/CookArtifactCache.h"
    "${TOOLS_ARCHITECTURE_ROOT}/TextureCooker/Public/TextureCookRequestList.h"
)

foreach(allowed_header IN LISTS ALLOWED_TOOL_PUBLIC_HEADERS)
    cmake_path(NORMAL_PATH allowed_header)
endforeach()

file(GLOB_RECURSE tool_public_headers
    "${TOOLS_ARCHITECTURE_ROOT}/*/Public/*.h"
    "${TOOLS_ARCHITECTURE_ROOT}/*/Public/*.hpp"
)

foreach(public_header IN LISTS tool_public_headers)
    cmake_path(NORMAL_PATH public_header)
    list(FIND ALLOWED_TOOL_PUBLIC_HEADERS "${public_header}" allowed_header_index)
    if(allowed_header_index EQUAL -1)
        relative_tools_architecture_path("${public_header}" relative_path)
        append_tools_architecture_violation("${relative_path}: unapproved public tool header; move implementation details under Private or register a narrow shared seam here")
    endif()
endforeach()

set(FORBIDDEN_TOOL_PUBLIC_HEADER_TOKENS
    "Private/"
    "Tools/"
    "Application/"
    "Renderer/"
    "Editor/"
    "AssetConverter/"
    "ShaderCompiler/"
    "TextureCooker/Private"
    "RHI/Public/D3D12/"
    "D3D12/"
    "GameFramework/"
    "Core/Public/Diagnostics"
    "Logging::"
)

foreach(public_header IN LISTS tool_public_headers)
    check_tools_file_for_tokens(
        "${public_header}"
        CONTEXT "public tool header"
        TOKENS ${FORBIDDEN_TOOL_PUBLIC_HEADER_TOKENS}
    )
endforeach()

set(COOK_COMMON_SOURCE_ROOT "${TOOLS_ARCHITECTURE_ROOT}/CookCommon")
file(GLOB_RECURSE cook_common_source_files
    "${COOK_COMMON_SOURCE_ROOT}/*.h"
    "${COOK_COMMON_SOURCE_ROOT}/*.hpp"
    "${COOK_COMMON_SOURCE_ROOT}/*.cpp"
    "${COOK_COMMON_SOURCE_ROOT}/*.cxx"
)

set(FORBIDDEN_COOK_COMMON_SOURCE_TOKENS
    "RHI/"
    "GameFramework/"
    "Application/"
    "Renderer/"
    "Editor/"
    "AssetConverter"
    "TextureCooker"
    "ShaderCompiler"
    "D3D12/"
    "cgltf"
    "assimp"
    "dxcompiler"
    "slang"
)

foreach(cook_common_source_file IN LISTS cook_common_source_files)
    check_tools_file_for_tokens(
        "${cook_common_source_file}"
        CONTEXT "CookCommon must remain generic"
        TOKENS ${FORBIDDEN_COOK_COMMON_SOURCE_TOKENS}
    )
endforeach()

set(FORBIDDEN_COOK_COMMON_CMAKE_TOKENS
    "SparkleRHI"
    "SparkleGameFramework"
    "SparkleApplication"
    "SparkleRenderer"
    "SparkleEditor"
    "SparklePlatform"
    "TextureCookShared"
    "TextureCooker"
    "AssetConverter"
    "ShaderCompiler"
    "cgltf"
    "assimp"
    "dxcompiler"
    "slang"
)

check_tools_file_for_tokens(
    "${COOK_COMMON_SOURCE_ROOT}/CMakeLists.txt"
    CONTEXT "CookCommon CMake dependency surface"
    TOKENS ${FORBIDDEN_COOK_COMMON_CMAKE_TOKENS}
)

file(GLOB_RECURSE all_tool_source_files
    "${TOOLS_ARCHITECTURE_ROOT}/*.h"
    "${TOOLS_ARCHITECTURE_ROOT}/*.hpp"
    "${TOOLS_ARCHITECTURE_ROOT}/*.cpp"
    "${TOOLS_ARCHITECTURE_ROOT}/*.cxx"
)

set(FORBIDDEN_CROSS_TOOL_PRIVATE_INCLUDE_TOKENS
    "Tools/AssetConverter/Private"
    "Tools/TextureCooker/Private"
    "Tools/ShaderCompiler/Private"
    "Tools/CookCommon/Private"
    "Tools/ConsoleDiagnostics/Private"
    "AssetConverter/Private"
    "TextureCooker/Private"
    "ShaderCompiler/Private"
    "CookCommon/Private"
    "ConsoleDiagnostics/Private"
)

foreach(tool_source_file IN LISTS all_tool_source_files)
    check_tools_file_for_tokens(
        "${tool_source_file}"
        CONTEXT "tool source must not include another tool's Private surface"
        TOKENS ${FORBIDDEN_CROSS_TOOL_PRIVATE_INCLUDE_TOKENS}
    )
endforeach()

get_property(TOOLS_ARCHITECTURE_VIOLATIONS GLOBAL PROPERTY TOOLS_ARCHITECTURE_VIOLATIONS)

if(TOOLS_ARCHITECTURE_VIOLATIONS)
    string(PREPEND TOOLS_ARCHITECTURE_VIOLATIONS
        "Tools architecture validation failed. Tool implementation must stay private, shared seams must be narrow and explicit, and CookCommon must remain a generic tool-side service.\n")
    message(FATAL_ERROR "${TOOLS_ARCHITECTURE_VIOLATIONS}")
endif()

message(STATUS "Tools architecture boundary check passed for public tool seams, cross-tool private includes, and CookCommon generic ownership.")