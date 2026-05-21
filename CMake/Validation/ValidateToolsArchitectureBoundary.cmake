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

set(REQUIRED_TOOL_DOMAIN_DIRECTORIES
    "Launcher"
    "Cooking"
    "Import"
    "Conversion"
    "Shaders"
)

set(FORBIDDEN_LEGACY_TOOL_DIRECTORIES
    "SparkleLauncher"
    "CookCommon"
    "AssetCooker"
    "TextureCooker"
    "SourceImportAdapters"
    "MeshCooker"
    "MaterialCooker"
    "SceneCooker"
    "AssetConverter"
    "ShaderCompiler"
    "Workflow"
    "Shader"
)

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

foreach(required_domain IN LISTS REQUIRED_TOOL_DOMAIN_DIRECTORIES)
    if(NOT IS_DIRECTORY "${TOOLS_ARCHITECTURE_ROOT}/${required_domain}")
        append_tools_architecture_violation("Tools/${required_domain}: missing required tool domain directory")
    endif()
endforeach()

foreach(forbidden_legacy_dir IN LISTS FORBIDDEN_LEGACY_TOOL_DIRECTORIES)
    if(IS_DIRECTORY "${TOOLS_ARCHITECTURE_ROOT}/${forbidden_legacy_dir}")
        append_tools_architecture_violation("Tools/${forbidden_legacy_dir}: legacy flat tool directory; move the tool under its owning domain")
    endif()
endforeach()

set(ALLOWED_TOOL_PUBLIC_HEADERS
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/BuildWorkspaceOperations.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/BuildProfileCatalog.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/CookOperations.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/LaunchOperations.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/LauncherPaths.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/MaintenanceOperations.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/OperationModel.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/ProcessRunner.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/ProjectDiscovery.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/RepositoryLocator.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Launcher/SparkleLauncher/Public/SparkleLauncher/ToolResolver.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/AssetCooker/Public/AssetCookRequest.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/AssetCooker/Public/AssetCookResult.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/AssetCooker/Public/AssetCookerTypes.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Import/SourceImportAdapters/Public/SourceImportDiagnostics.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Import/SourceImportAdapters/Public/SourceImportResult.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Import/SourceImportAdapters/Public/SourceSceneImporter.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/MeshCooker/Public/CookedMeshAssetBuild.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/MeshCooker/Public/MeshCooker.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/MaterialCooker/Public/CookedMaterialAssetBuild.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/MaterialCooker/Public/MaterialCooker.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/SceneCooker/Public/CookedSceneBuild.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/SceneCooker/Public/SceneCooker.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/CookCommon/Public/ToolConsole.h"
    "${TOOLS_ARCHITECTURE_ROOT}/Cooking/TextureCooker/Public/TextureCookRequestList.h"
)

foreach(allowed_header IN LISTS ALLOWED_TOOL_PUBLIC_HEADERS)
    cmake_path(NORMAL_PATH allowed_header)
endforeach()

file(GLOB_RECURSE tool_public_headers
    "${TOOLS_ARCHITECTURE_ROOT}/*/Public/*.h"
    "${TOOLS_ARCHITECTURE_ROOT}/*/Public/*.hpp"
    "${TOOLS_ARCHITECTURE_ROOT}/*/*/Public/*.h"
    "${TOOLS_ARCHITECTURE_ROOT}/*/*/Public/*.hpp"
    "${TOOLS_ARCHITECTURE_ROOT}/*/*/Public/*/*.h"
    "${TOOLS_ARCHITECTURE_ROOT}/*/*/Public/*/*.hpp"
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

file(GLOB_RECURSE all_tool_source_files
    "${TOOLS_ARCHITECTURE_ROOT}/*.h"
    "${TOOLS_ARCHITECTURE_ROOT}/*.hpp"
    "${TOOLS_ARCHITECTURE_ROOT}/*.cpp"
    "${TOOLS_ARCHITECTURE_ROOT}/*.cxx"
)

set(FORBIDDEN_CROSS_TOOL_PRIVATE_INCLUDE_TOKENS
    "Tools/Conversion/AssetConverter/Private"
    "Tools/Cooking/AssetCooker/Private"
    "Tools/Import/SourceImportAdapters/Private"
    "Tools/Cooking/MeshCooker/Private"
    "Tools/Cooking/MaterialCooker/Private"
    "Tools/Cooking/SceneCooker/Private"
    "Tools/Cooking/TextureCooker/Private"
    "Tools/Shaders/ShaderCompiler/Private"
    "AssetConverter/Private"
    "AssetCooker/Private"
    "SourceImportAdapters/Private"
    "MeshCooker/Private"
    "MaterialCooker/Private"
    "SceneCooker/Private"
    "TextureCooker/Private"
    "ShaderCompiler/Private"
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
        "Tools architecture validation failed. Tool implementation must stay private and shared seams must be narrow and explicit.\n")
    message(FATAL_ERROR "${TOOLS_ARCHITECTURE_VIOLATIONS}")
endif()

message(STATUS "Tools architecture boundary check passed for public tool seams and cross-tool private includes.")
