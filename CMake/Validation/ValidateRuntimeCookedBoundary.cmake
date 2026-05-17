if(NOT DEFINED RUNTIME_BOUNDARY_SOURCE_DIR)
    set(RUNTIME_BOUNDARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

cmake_path(NORMAL_PATH RUNTIME_BOUNDARY_SOURCE_DIR)

set(RUNTIME_MODULE_SOURCE_ROOTS
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/Application"
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/GameFramework"
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/Renderer"
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/RHI"
)

set(RUNTIME_MODULE_CMAKE_FILES
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/Application/CMakeLists.txt"
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/GameFramework/CMakeLists.txt"
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/Renderer/CMakeLists.txt"
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/RHI/CMakeLists.txt"
)

file(GLOB RUNTIME_PROJECT_CMAKE_FILES
    "${RUNTIME_BOUNDARY_SOURCE_DIR}/Projects/*/CMakeLists.txt"
)

set(FORBIDDEN_RUNTIME_SOURCE_TOKENS
    "SceneImporter"
    "SceneImportResult"
    "KtxTextureCooker"
    "AssetAuthoring"
    "cgltf"
    "assimp"
    "SparkleAssetAuthoring"
    "DdsTextureLoader"
    "WicTextureLoader"
)

set(FORBIDDEN_RUNTIME_CMAKE_TOKENS
    "SparkleAssetAuthoring"
    "Tools/AssetConverter"
    "AssetConverter/Public"
    "AssetConverter/Private"
    "assimp"
    "cgltf"
)

set(RUNTIME_BOUNDARY_VIOLATIONS "")

function(check_file_for_tokens file_path)
    set(options)
    set(one_value_args)
    set(multi_value_args TOKENS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    file(READ "${file_path}" file_text)
    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RUNTIME_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)

    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            set(RUNTIME_BOUNDARY_VIOLATIONS
                "${RUNTIME_BOUNDARY_VIOLATIONS}${relative_path}: found forbidden token '${token}'\n"
                PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

function(check_file_for_regex file_path regex diagnostic)
    file(READ "${file_path}" file_text)
    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RUNTIME_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)

    string(REGEX MATCH "${regex}" regex_match "${file_text}")
    if(regex_match)
        set(RUNTIME_BOUNDARY_VIOLATIONS
            "${RUNTIME_BOUNDARY_VIOLATIONS}${relative_path}: ${diagnostic}\n"
            PARENT_SCOPE)
    endif()
endfunction()

function(require_file_regex file_path regex diagnostic)
    file(READ "${file_path}" file_text)
    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RUNTIME_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)

    string(REGEX MATCH "${regex}" regex_match "${file_text}")
    if(NOT regex_match)
        set(RUNTIME_BOUNDARY_VIOLATIONS
            "${RUNTIME_BOUNDARY_VIOLATIONS}${relative_path}: ${diagnostic}\n"
            PARENT_SCOPE)
    endif()
endfunction()

foreach(module_root IN LISTS RUNTIME_MODULE_SOURCE_ROOTS)
    file(GLOB_RECURSE runtime_source_files
        "${module_root}/*.h"
        "${module_root}/*.hpp"
        "${module_root}/*.cpp"
        "${module_root}/*.cxx"
    )

    foreach(runtime_source_file IN LISTS runtime_source_files)
        check_file_for_tokens(
            "${runtime_source_file}"
            TOKENS ${FORBIDDEN_RUNTIME_SOURCE_TOKENS}
        )
    endforeach()
endforeach()

foreach(runtime_cmake_file IN LISTS RUNTIME_MODULE_CMAKE_FILES)
    check_file_for_tokens(
        "${runtime_cmake_file}"
        TOKENS ${FORBIDDEN_RUNTIME_CMAKE_TOKENS}
    )
endforeach()

set(application_cmake_file "${RUNTIME_BOUNDARY_SOURCE_DIR}/Engine/Application/CMakeLists.txt")
check_file_for_regex(
    "${application_cmake_file}"
    "target_link_libraries[ \t\r\n]*\\([ \t\r\n]*SparkleApplication[ \t\r\n]+[^\\)]*SparkleEditor"
    "runtime SparkleApplication target must not link SparkleEditor; use SparkleApplicationEditor for editor host code")
check_file_for_regex(
    "${application_cmake_file}"
    "target_link_libraries[ \t\r\n]*\\([ \t\r\n]*SparkleApplication[ \t\r\n]+[^\\)]*SparkleApplicationEditor"
    "runtime SparkleApplication target must not link the editor-host application target")

foreach(project_cmake_file IN LISTS RUNTIME_PROJECT_CMAKE_FILES)
    check_file_for_regex(
        "${project_cmake_file}"
        "target_link_libraries[ \t\r\n]*\\([ \t\r\n]*[A-Za-z0-9_]+Runtime[^\\)]*SparkleEditor"
        "runtime project target must not link SparkleEditor")
    check_file_for_regex(
        "${project_cmake_file}"
        "target_link_libraries[ \t\r\n]*\\([ \t\r\n]*[A-Za-z0-9_]+Runtime[^\\)]*SparkleApplicationEditor"
        "runtime project target must not link SparkleApplicationEditor")
endforeach()

set(showcase_cmake_file "${RUNTIME_BOUNDARY_SOURCE_DIR}/Projects/Showcase/CMakeLists.txt")
if(EXISTS "${showcase_cmake_file}")
    require_file_regex(
        "${showcase_cmake_file}"
        "target_link_libraries[ \t\r\n]*\\([ \t\r\n]*ShowcaseRuntime[ \t\r\n]+[^\\)]*SparkleApplication"
        "ShowcaseRuntime must link the runtime application host target")
    require_file_regex(
        "${showcase_cmake_file}"
        "target_link_libraries[ \t\r\n]*\\([ \t\r\n]*ShowcaseEditor[ \t\r\n]+[^\\)]*SparkleApplicationEditor"
        "ShowcaseEditor must link the editor-host application target")
endif()

if(RUNTIME_BOUNDARY_VIOLATIONS)
    string(PREPEND RUNTIME_BOUNDARY_VIOLATIONS
        "Cooked-runtime boundary validation failed. Runtime modules must stay free of source-import and authoring dependencies.\n")
    message(FATAL_ERROR "${RUNTIME_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "Cooked-runtime boundary check passed for Engine/Application, Engine/GameFramework, and Engine/Renderer.")