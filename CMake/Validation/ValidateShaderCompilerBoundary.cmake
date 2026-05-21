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
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Editor"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/GameFramework"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Renderer"
)

set(SHADER_COMPILER_RUNTIME_CMAKE_FILES
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/CMakeLists.txt"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/RHI/CMakeLists.txt"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Application/CMakeLists.txt"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Editor/CMakeLists.txt"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/GameFramework/CMakeLists.txt"
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Engine/Renderer/CMakeLists.txt"
)

set(SHADER_COMPILER_TOOL_SOURCE_ROOTS
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Tools/Shaders/ShaderCompiler"
)

set(SHADER_COMPILER_TOOL_CMAKE_FILES
    "${SHADER_COMPILER_BOUNDARY_SOURCE_DIR}/Tools/Shaders/ShaderCompiler/CMakeLists.txt"
)

# Architecture invariant: runtime modules consume validated cooked shader
# packages and registries only. They must not include or link tool-only compiler
# backend APIs, DXC, Slang, SPIRV-Reflect, ShaderCompileOptions, or
# ShaderCompileResult. The Application shader recook bridge may launch the
# external ShaderCompiler executable, but it must not turn runtime modules into
# shader compiler hosts.
set(FORBIDDEN_RUNTIME_SOURCE_TOKENS
    "IShaderBackend"
    "ShaderBackendPool"
    "ShaderBackendFactory"
    "BuiltinBackends"
    "Backend/IShaderBackend.h"
    "Backend/ShaderBackendPool.h"
    "Backend/ShaderBackendFactory.h"
    "Backend/BuiltinBackends.h"
    "Backend/ShaderTarget.h"
    "ShaderCompileProfile"
    "ShaderCacheKey"
    "ShaderCompileOptionsHasher"
    "IncludeClosureHasher"
    "LocalDiskShaderArtifactStore"
    "ShaderDebugArtifactSet"
    "DxcShaderCompiler"
    "DxcShaderBackend"
    "DxcContext"
    "IDxcCompiler"
    "dxcapi"
    "DxcCreateInstance"
    "ShaderCompileOptions"
    "ShaderCompileResult"
    "Compiler/DxcShaderCompiler.h"
    "Compiler/DxcContext.h"
    "Backends/Dxc/"
    "spirv_reflect.h"
    "SpvReflect"
    "spvReflect"
    "SlangShaderBackend"
    "SlangReflectionExtractor"
    "slang::"
    "slang.h"
    "slang-com-ptr.h"
    "Backends/Slang/"
    "Tools/Shaders/ShaderCompiler"
    "ShaderCompiler.exe"
    "SHADER_COMPILER_EXE"
    " --shader "
    "BuiltinShaderPackageLayouts"
    "BuildPackageBindingLayout"
    "ShaderPackageLayoutCatalog"
    "ShaderCookManifest"
    "ShaderPackages.ini"
    "inspect-manifest"
    "cook-shaders"
)

set(FORBIDDEN_RUNTIME_CMAKE_TOKENS
    "ShaderCompiler"
    "Tools/Shaders/ShaderCompiler"
    "dxcompiler"
    "spirv_reflect"
    "slang::slang"
    "SLANG_INCLUDE_DIR"
    "SLANG_SDK_ROOT"
)

set(FORBIDDEN_SHADER_COMPILER_SOURCE_TOKENS
    "RHI/Private/"
    "Engine/RHI/Private/"
    "RHI/Public/D3D12/"
    "BuiltinShaderPackageLayouts"
    "BuildPackageBindingLayout"
    "ShaderPackageLayoutCatalog"
    "ShaderCookManifest"
    "Manifest/"
    "ShaderPackages.ini"
    "inspect-manifest"
    "cook-shaders"
    "== \"--shader\""
    "BuildSingleShaderPackage"
    "singleShaderPath"
    "Application/"
    "Renderer/"
    "GameFramework/"
    "Editor/"
    "AssetConverter/"
)

# Backend containment invariant: DXC and Slang API details stay inside their
# backend folders. Tool orchestration talks through IShaderBackend; runtime
# modules never see backend implementation tokens at all.
set(FORBIDDEN_DXC_TOKENS_OUTSIDE_BACKEND
    "IDxcCompiler"
    "dxcapi"
    "DxcCreateInstance"
    "DxcShaderBackend"
    "DxcShaderCompiler"
    "DxcContext"
    # SPIRV-Reflect is consumed only by the SPIR-V reflection extractor that
    # lives next to the DXC backend; runtime and other tool code must not
    # take a dependency on it.
    "spirv_reflect.h"
    "SpvReflect"
    "spvReflect"
)

set(FORBIDDEN_SLANG_TOKENS_OUTSIDE_BACKEND
    "SlangShaderBackend"
    "SlangReflectionExtractor"
    "slang::"
    "slang.h"
    "slang-com-ptr.h"
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
        set(runtime_source_tokens ${FORBIDDEN_RUNTIME_SOURCE_TOKENS})
        set(runtime_source_file_normalized "${runtime_source_file}")
        cmake_path(NORMAL_PATH runtime_source_file_normalized)
        string(FIND "${runtime_source_file_normalized}" "/Engine/Application/Private/ShaderRecook/" shader_recook_source_index)
        if(NOT shader_recook_source_index EQUAL -1)
            list(REMOVE_ITEM runtime_source_tokens "ShaderCompiler.exe" "SHADER_COMPILER_EXE")
        endif()

        check_file_for_tokens(
            "${runtime_source_file}"
            TOKENS ${runtime_source_tokens}
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
        set(tool_source_file_normalized "${tool_source_file}")
        cmake_path(NORMAL_PATH tool_source_file_normalized)

        check_file_for_tokens(
            "${tool_source_file}"
            TOKENS ${FORBIDDEN_SHADER_COMPILER_SOURCE_TOKENS}
        )

        # Containment: DXC tokens may only appear under Backends/Dxc/.
        string(FIND "${tool_source_file_normalized}" "/Backends/Dxc/" dxc_backend_index)
        if(dxc_backend_index EQUAL -1)
            check_file_for_tokens(
                "${tool_source_file}"
                TOKENS ${FORBIDDEN_DXC_TOKENS_OUTSIDE_BACKEND}
            )
        endif()

        string(FIND "${tool_source_file_normalized}" "/Backends/Slang/" slang_backend_index)
        if(slang_backend_index EQUAL -1)
            check_file_for_tokens(
                "${tool_source_file}"
                TOKENS ${FORBIDDEN_SLANG_TOKENS_OUTSIDE_BACKEND}
            )
        endif()
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
        "Shader cooking boundary validation failed. Runtime/orchestration modules must consume cooked shader packages and registries without hosting compiler backends, and ShaderCompiler must stay free of runtime-private or high-level engine dependencies.\n")
    message(FATAL_ERROR "${SHADER_COMPILER_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "Shader cooking boundary check passed for runtime cooked-package consumption, compiler-backend containment, and Tools/Shaders/ShaderCompiler ownership.")
