if(NOT DEFINED SHADER_PACKAGE_PARITY_SOURCE_DIR)
    set(SHADER_PACKAGE_PARITY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    SHADER_PACKAGE_PARITY_SOURCE_DIR
    "${SHADER_PACKAGE_PARITY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH SHADER_PACKAGE_PARITY_SOURCE_DIR)

set(SHADER_PACKAGE_PARITY_VIOLATIONS "")
set_property(GLOBAL PROPERTY SPARKLE_SHADER_PACKAGE_PARITY_VIOLATIONS "")

function(append_shader_package_parity_violation message_text)
    set_property(GLOBAL APPEND PROPERTY SPARKLE_SHADER_PACKAGE_PARITY_VIOLATIONS "${message_text}")
endfunction()

function(read_required_shader_package_file file_path out_text)
    if(NOT EXISTS "${file_path}")
        append_shader_package_parity_violation("missing required file: ${file_path}")
        set(${out_text} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${file_path}" file_text)
    set(${out_text} "${file_text}" PARENT_SCOPE)
endfunction()

function(require_shader_package_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${SHADER_PACKAGE_PARITY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_shader_package_parity_violation("${relative_path}: missing '${token}': ${description}")
    endif()
endfunction()

function(forbid_shader_package_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(NOT match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${SHADER_PACKAGE_PARITY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_shader_package_parity_violation("${relative_path}: found forbidden '${token}': ${description}")
    endif()
endfunction()

set(render_pass_runtime_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Engine/Renderer/Private/Pipeline/RenderPassShaderRuntime.h")
read_required_shader_package_file("${render_pass_runtime_path}" render_pass_runtime_text)
if(render_pass_runtime_text)
    require_shader_package_text("${render_pass_runtime_path}" "${render_pass_runtime_text}" "capabilities.RequiredShaderBinaryFormat" "renderer shader package loading must ask the active RHI capability report which binary format it needs")
    require_shader_package_text("${render_pass_runtime_path}" "${render_pass_runtime_text}" "RhiBackendApiToString(capabilities.BackendApi)" "missing shader variant diagnostics must include the active backend")
    require_shader_package_text("${render_pass_runtime_path}" "${render_pass_runtime_text}" "CookedShaderBinaryFormatToString(requiredBinaryFormat)" "missing shader variant diagnostics must include the requested binary format")
    forbid_shader_package_text("${render_pass_runtime_path}" "${render_pass_runtime_text}" "CookedShaderBinaryFormat::Dxil" "Renderer must not hard-code D3D12 shader binaries")
    forbid_shader_package_text("${render_pass_runtime_path}" "${render_pass_runtime_text}" "CookedShaderBinaryFormat::SpirV" "Renderer must not hard-code Vulkan shader binaries")
endif()

set(renderer_root "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Engine/Renderer")
if(EXISTS "${renderer_root}")
    file(GLOB_RECURSE renderer_files
        "${renderer_root}/*.h"
        "${renderer_root}/*.hpp"
        "${renderer_root}/*.cpp"
        "${renderer_root}/*.cxx"
    )

    foreach(renderer_file IN LISTS renderer_files)
        file(READ "${renderer_file}" renderer_file_text)
        forbid_shader_package_text("${renderer_file}" "${renderer_file_text}" "CookedShaderBinaryFormat::Dxil" "Renderer must route binary selection through RenderHardwareInterface")
        forbid_shader_package_text("${renderer_file}" "${renderer_file_text}" "CookedShaderBinaryFormat::SpirV" "Renderer must route binary selection through RenderHardwareInterface")
    endforeach()
endif()

set(rhi_interface_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Engine/RHI/Public/Device/RenderHardwareInterface.h")
read_required_shader_package_file("${rhi_interface_path}" rhi_interface_text)
if(rhi_interface_text)
    require_shader_package_text("${rhi_interface_path}" "${rhi_interface_text}" "GetRequiredShaderBinaryFormat()" "each backend must declare its runtime shader binary format")
endif()

set(shader_package_utils_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Engine/RHI/Public/Shaders/CookedShaderPackageUtils.h")
read_required_shader_package_file("${shader_package_utils_path}" shader_package_utils_text)
if(shader_package_utils_text)
    require_shader_package_text("${shader_package_utils_path}" "${shader_package_utils_text}" "CookedShaderBinaryFormatToString" "diagnostics must share one cooked binary format formatter")
endif()

set(shader_package_header_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Engine/RHI/Public/Shaders/CookedShaderPackage.h")
read_required_shader_package_file("${shader_package_header_path}" shader_package_header_text)
if(shader_package_header_text)
    require_shader_package_text("${shader_package_header_path}" "${shader_package_header_text}" "enum class CookedShaderBinaryFormat" "cooked shader schema must carry backend binary format")
    require_shader_package_text("${shader_package_header_path}" "${shader_package_header_text}" "Dxil = 0" "DXIL must remain a first-class cooked shader binary format")
    require_shader_package_text("${shader_package_header_path}" "${shader_package_header_text}" "SpirV = 1" "SPIR-V must remain a first-class cooked shader binary format")
    require_shader_package_text("${shader_package_header_path}" "${shader_package_header_text}" "CookedShaderBinaryFormat Format" "each cooked binary record must declare its binary format")
endif()

set(shader_package_cache_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Engine/RHI/Private/Shaders/CookedShaderPackageCache.cpp")
read_required_shader_package_file("${shader_package_cache_path}" shader_package_cache_text)
if(shader_package_cache_text)
    require_shader_package_text("${shader_package_cache_path}" "${shader_package_cache_text}" "requiredBinaryFormat" "runtime package validation must be parameterized by backend binary format")
    require_shader_package_text("${shader_package_cache_path}" "${shader_package_cache_text}" "hasRequiredBinaryForStage" "runtime package validation must track required variants per stage")
    require_shader_package_text("${shader_package_cache_path}" "${shader_package_cache_text}" "is missing the required" "runtime package validation must fail clearly when a backend shader variant is absent")
    require_shader_package_text("${shader_package_cache_path}" "${shader_package_cache_text}" "CookedShaderBinaryFormatToString(requiredBinaryFormat)" "variant validation errors must name the requested binary format")
endif()

set(d3d12_rhi_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp")
read_required_shader_package_file("${d3d12_rhi_path}" d3d12_rhi_text)
if(d3d12_rhi_text)
    require_shader_package_text("${d3d12_rhi_path}" "${d3d12_rhi_text}" "GetRequiredShaderBinaryFormat()" "D3D12 backend must declare its required shader binary format")
    require_shader_package_text("${d3d12_rhi_path}" "${d3d12_rhi_text}" "return CookedShaderBinaryFormat::Dxil" "D3D12 must load DXIL shader binaries")
endif()

set(vulkan_rhi_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp")
read_required_shader_package_file("${vulkan_rhi_path}" vulkan_rhi_text)
if(vulkan_rhi_text)
    require_shader_package_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "GetRequiredShaderBinaryFormat()" "Vulkan backend must declare its required shader binary format")
    require_shader_package_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "return CookedShaderBinaryFormat::SpirV" "Vulkan must load SPIR-V shader binaries")
endif()

set(stage_compiler_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Tools/Shader/ShaderCompiler/Private/Cooking/StageCompiler.cpp")
read_required_shader_package_file("${stage_compiler_path}" stage_compiler_text)
if(stage_compiler_text)
    require_shader_package_text("${stage_compiler_path}" "${stage_compiler_text}" "IsSpirVTarget(options.Target)" "shader cooker must map target family to cooked binary format")
    require_shader_package_text("${stage_compiler_path}" "${stage_compiler_text}" "CookedShaderBinaryFormat::SpirV" "shader cooker must emit SPIR-V records")
    require_shader_package_text("${stage_compiler_path}" "${stage_compiler_text}" "CookedShaderBinaryFormat::Dxil" "shader cooker must emit DXIL records")
endif()

set(graph_builder_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Tools/Shader/ShaderCompiler/Private/Cooking/ShaderCookGraphBuilder.cpp")
read_required_shader_package_file("${graph_builder_path}" graph_builder_text)
if(graph_builder_text)
    require_shader_package_text("${graph_builder_path}" "${graph_builder_text}" "package.stages.size() * settings.targets.size()" "one logical shader package must be able to compile all requested target variants")
    require_shader_package_text("${graph_builder_path}" "${graph_builder_text}" "for (std::size_t targetIndex" "shader cook graph must create work per requested target")
endif()

set(cooked_package_writer_path "${SHADER_PACKAGE_PARITY_SOURCE_DIR}/Tools/Shader/ShaderCompiler/Private/Cooking/CookedPackageWriter.cpp")
read_required_shader_package_file("${cooked_package_writer_path}" cooked_package_writer_text)
if(cooked_package_writer_text)
    require_shader_package_text("${cooked_package_writer_path}" "${cooked_package_writer_text}" "binaryRecords.reserve(compiledStages.size())" "cooked packages must preserve every compiled target variant")
    require_shader_package_text("${cooked_package_writer_path}" "${cooked_package_writer_text}" ".Format = compiledStage.format" "cooked packages must serialize each variant's binary format")
endif()

get_property(SHADER_PACKAGE_PARITY_VIOLATIONS GLOBAL PROPERTY SPARKLE_SHADER_PACKAGE_PARITY_VIOLATIONS)
if(SHADER_PACKAGE_PARITY_VIOLATIONS)
    list(JOIN SHADER_PACKAGE_PARITY_VIOLATIONS "\n" SHADER_PACKAGE_PARITY_VIOLATIONS)
    string(PREPEND SHADER_PACKAGE_PARITY_VIOLATIONS
        "Shader package parity validation failed. Runtime shader selection must be backend-driven, packages must validate missing DXIL/SPIR-V variants, and the cooker must preserve multi-target variants.\n")
    message(FATAL_ERROR "${SHADER_PACKAGE_PARITY_VIOLATIONS}")
endif()

message(STATUS "Shader package parity check passed for renderer runtime selection, cooked package validation, and ShaderCompiler multi-target output.")
