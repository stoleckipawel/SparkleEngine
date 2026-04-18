if(NOT DEFINED TEXTURE_COOKER_BOUNDARY_SOURCE_DIR)
    set(TEXTURE_COOKER_BOUNDARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    TEXTURE_COOKER_BOUNDARY_SOURCE_DIR
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH TEXTURE_COOKER_BOUNDARY_SOURCE_DIR)

set(TEXTURE_COOKER_RUNTIME_SOURCE_ROOTS
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Engine/RHI"
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Engine/Application"
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Engine/GameFramework"
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Engine/Renderer"
)

set(TEXTURE_COOKER_RUNTIME_CMAKE_FILES
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Engine/RHI/CMakeLists.txt"
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Engine/Application/CMakeLists.txt"
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Engine/GameFramework/CMakeLists.txt"
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Engine/Renderer/CMakeLists.txt"
)

set(TEXTURE_COOKER_ASSET_CONVERTER_SOURCE_ROOTS
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Tools/AssetConverter"
)

set(TEXTURE_COOKER_ASSET_CONVERTER_CMAKE_FILES
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Tools/AssetConverter/CMakeLists.txt"
)

set(TEXTURE_COOKER_TOOL_SOURCE_ROOTS
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Tools/TextureCooker"
)

set(TEXTURE_COOKER_TOOL_CMAKE_FILES
    "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}/Tools/TextureCooker/CMakeLists.txt"
)

set(FORBIDDEN_RUNTIME_SOURCE_TOKENS
    "TextureCooker.exe"
    "TEXTURE_COOKER_EXE"
    "TextureCookRequestList.h"
    "KtxTextureLoader"
)

set(FORBIDDEN_RUNTIME_CMAKE_TOKENS
    "Tools/TextureCooker"
    "TextureCookShared"
    "TextureCooker"
    "ktx"
)

set(FORBIDDEN_ASSET_CONVERTER_SOURCE_TOKENS
    "KtxTextureCooker"
    "D3D12/Textures/TextureLoader.h"
    "TextureLoader::Load"
    "ktxTexture"
    "D3D12/Textures/CookedTextureAsset.h"
)

set(FORBIDDEN_ASSET_CONVERTER_CMAKE_TOKENS
    "ktx"
    "SparkleRHI"
)

set(FORBIDDEN_TEXTURE_COOKER_SOURCE_TOKENS
    "SceneImporter"
    "SceneImportResult"
    "CookedSceneCooker"
    "TextureCookRequestBuilder"
    "CookedTextureAssetUtils.h"
    "CookedTextureReference.h"
    "cgltf"
    "assimp"
    "AssetConverter"
)

set(FORBIDDEN_TEXTURE_COOKER_CMAKE_TOKENS
    "SparkleApplication"
    "SparkleRenderer"
    "SparkleEditor"
    "SparkleGameFramework"
    "cgltf"
    "assimp"
    "ktx"
)

set(TEXTURE_COOKER_BOUNDARY_VIOLATIONS "")

function(check_file_for_tokens file_path)
    set(options)
    set(one_value_args)
    set(multi_value_args TOKENS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    file(READ "${file_path}" file_text)
    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${TEXTURE_COOKER_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)

    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            set(TEXTURE_COOKER_BOUNDARY_VIOLATIONS
                "${TEXTURE_COOKER_BOUNDARY_VIOLATIONS}${relative_path}: found forbidden token '${token}'\n"
                PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

foreach(runtime_root IN LISTS TEXTURE_COOKER_RUNTIME_SOURCE_ROOTS)
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

foreach(runtime_cmake_file IN LISTS TEXTURE_COOKER_RUNTIME_CMAKE_FILES)
    check_file_for_tokens(
        "${runtime_cmake_file}"
        TOKENS ${FORBIDDEN_RUNTIME_CMAKE_TOKENS}
    )
endforeach()

foreach(asset_converter_root IN LISTS TEXTURE_COOKER_ASSET_CONVERTER_SOURCE_ROOTS)
    file(GLOB_RECURSE asset_converter_source_files
        "${asset_converter_root}/*.h"
        "${asset_converter_root}/*.hpp"
        "${asset_converter_root}/*.cpp"
        "${asset_converter_root}/*.cxx"
    )

    foreach(asset_converter_source_file IN LISTS asset_converter_source_files)
        check_file_for_tokens(
            "${asset_converter_source_file}"
            TOKENS ${FORBIDDEN_ASSET_CONVERTER_SOURCE_TOKENS}
        )
    endforeach()
endforeach()

foreach(asset_converter_cmake_file IN LISTS TEXTURE_COOKER_ASSET_CONVERTER_CMAKE_FILES)
    check_file_for_tokens(
        "${asset_converter_cmake_file}"
        TOKENS ${FORBIDDEN_ASSET_CONVERTER_CMAKE_TOKENS}
    )
endforeach()

foreach(tool_root IN LISTS TEXTURE_COOKER_TOOL_SOURCE_ROOTS)
    file(GLOB_RECURSE tool_source_files
        "${tool_root}/*.h"
        "${tool_root}/*.hpp"
        "${tool_root}/*.cpp"
        "${tool_root}/*.cxx"
    )

    foreach(tool_source_file IN LISTS tool_source_files)
        check_file_for_tokens(
            "${tool_source_file}"
            TOKENS ${FORBIDDEN_TEXTURE_COOKER_SOURCE_TOKENS}
        )
    endforeach()
endforeach()

foreach(tool_cmake_file IN LISTS TEXTURE_COOKER_TOOL_CMAKE_FILES)
    check_file_for_tokens(
        "${tool_cmake_file}"
        TOKENS ${FORBIDDEN_TEXTURE_COOKER_CMAKE_TOKENS}
    )
endforeach()

if(TEXTURE_COOKER_BOUNDARY_VIOLATIONS)
    string(PREPEND TEXTURE_COOKER_BOUNDARY_VIOLATIONS
        "TextureCooker boundary validation failed. Runtime modules must stay free of TextureCooker tool references and legacy KTX runtime loading, AssetConverter must stay free of low-level texture loading and serialization implementation, and TextureCooker must stay free of scene-import and GameFramework-specific ownership.\n")
    message(FATAL_ERROR "${TEXTURE_COOKER_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "TextureCooker boundary check passed for runtime modules, Tools/AssetConverter, and Tools/TextureCooker.")