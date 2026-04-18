if(NOT DEFINED SHADER_COMPILER_BOUNDARY_SOURCE_DIR)
    set(SHADER_COMPILER_BOUNDARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    SHADER_COMPILER_BOUNDARY_SOURCE_DIR
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH SHADER_COMPILER_BOUNDARY_SOURCE_DIR)

set(SHADER_COMPILER_RUNTIME_SOURCE_ROOTS
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/RHI"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Application"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/GameFramework"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Renderer"
)

set(SHADER_COMPILER_RUNTIME_CMAKE_FILES
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/RHI/CMakeLists.txt"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Application/CMakeLists.txt"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/GameFramework/CMakeLists.txt"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Renderer/CMakeLists.txt"
)

set(SHADER_COMPILER_TOOL_SOURCE_ROOTS
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Tools/ShaderCompiler"
)

set(SHADER_COMPILER_TOOL_CMAKE_FILES
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Tools/ShaderCompiler/CMakeLists.txt"
)

set(FORBIDDEN_RUNTIME_SOURCE_TOKENS
    "DxcShaderCompiler"
    "DxcContext"
    "Compiler/DxcShaderCompiler.h"
    "Compiler/DxcContext.h"
    "Tools/ShaderCompiler"
    "ShaderCompiler.exe"
    "SHADER_COMPILER_EXE"
)

set(FORBIDDEN_RUNTIME_CMAKE_TOKENS
    "ShaderCompiler"
    "Tools/ShaderCompiler"
    "dxcompiler"
)

set(FORBIDDEN_SHADER_COMPILER_SOURCE_TOKENS
    "RHI/Private/"
    "Engine/RHI/Private/"
    "RHI/Public/D3D12/"
    "Application/"
    "Renderer/"
    "GameFramework/"
    "Editor/"
    "AssetConverter/"
)

set(FORBIDDEN_SHADER_COMPILER_CMAKE_TOKENS
    "SparkleApplication"
    "SparkleRenderer"
    "SparkleGameFramework"
    "SparkleEditor"
    "SparklePlatform"
    "AssetConverter"
)

set(SHADER_COMPILER_BOUNDARY_VIOLATIONS "")

function(check_file_for_tokens file_path)
    set(options)
    set(one_value_args)
    set(multi_value_args TOKENS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    file(READ "${file_path}" file_text)
    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)

    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            set(SHADER_COMPILER_BOUNDARY_VIOLATIONS
                "${SHADER_COMPILER_BOUNDARY_VIOLATIONS}${relative_path}: found forbidden token '${token}'\n"
                PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

foreach(runtime_root IN LISTS SHADER_COMPILER_RUNTIME_SOURCE_ROOTS)
    file(GLOB_RECURSE runtime_source_files
        "${runtime_root}/*.h"
        "${runtime_root}/*.hpp"
        "${runtime_root}/*.cpp"
        "${runtime_root}/*.cxx"
    )

    foreach(runtime_source_file IN LISTS runtime_source_files)
        check_file_for_tokens(
            "${runtime_source_file}"
            TOKENS ${FORBIDDEN_RUNTIME_SOURCE_TOKENS}
        )
    endforeach()
endforeach()

foreach(runtime_cmake_file IN LISTS SHADER_COMPILER_RUNTIME_CMAKE_FILES)
    check_file_for_tokens(
        "${runtime_cmake_file}"
        TOKENS ${FORBIDDEN_RUNTIME_CMAKE_TOKENS}
    )
endforeach()

foreach(tool_root IN LISTS SHADER_COMPILER_TOOL_SOURCE_ROOTS)
    file(GLOB_RECURSE tool_source_files
        "${tool_root}/*.h"
        "${tool_root}/*.hpp"
        "${tool_root}/*.cpp"
        "${tool_root}/*.cxx"
    )

    foreach(tool_source_file IN LISTS tool_source_files)
        check_file_for_tokens(
            "${tool_source_file}"
            TOKENS ${FORBIDDEN_SHADER_COMPILER_SOURCE_TOKENS}
        )
    endforeach()
endforeach()

foreach(tool_cmake_file IN LISTS SHADER_COMPILER_TOOL_CMAKE_FILES)
    check_file_for_tokens(
        "${tool_cmake_file}"
        TOKENS ${FORBIDDEN_SHADER_COMPILER_CMAKE_TOKENS}
    )
endforeach()

if(SHADER_COMPILER_BOUNDARY_VIOLATIONS)
    string(PREPEND SHADER_COMPILER_BOUNDARY_VIOLATIONS
        "ShaderCompiler boundary validation failed. Runtime must stay free of tool-only DXC paths, and ShaderCompiler must stay free of runtime-private or high-level engine dependencies.\n")
    message(FATAL_ERROR "${SHADER_COMPILER_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "ShaderCompiler boundary check passed for Engine/RHI, runtime modules, and Tools/ShaderCompiler.")