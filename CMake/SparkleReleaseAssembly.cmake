cmake_minimum_required(VERSION 3.20)

function(sparkle_require_variable variable_name)
    if(NOT DEFINED ${variable_name} OR "${${variable_name}}" STREQUAL "")
        message(FATAL_ERROR "Required release assembly variable '${variable_name}' is not set.")
    endif()
endfunction()

function(sparkle_require_existing_path path label)
    if(NOT EXISTS "${path}")
        message(FATAL_ERROR "Required package input '${label}' was not found: ${path}")
    endif()
endfunction()

function(sparkle_stage_directory source_dir destination_dir label)
    sparkle_require_existing_path("${source_dir}" "${label}")
    file(MAKE_DIRECTORY "${destination_dir}")
    file(COPY "${source_dir}/" DESTINATION "${destination_dir}")
endfunction()

function(sparkle_stage_file source_file destination_file label)
    sparkle_require_existing_path("${source_file}" "${label}")
    get_filename_component(destination_directory "${destination_file}" DIRECTORY)
    file(MAKE_DIRECTORY "${destination_directory}")
    configure_file("${source_file}" "${destination_file}" COPYONLY)
endfunction()

sparkle_require_variable(SPARKLE_REPOSITORY_ROOT)
sparkle_require_variable(SPARKLE_ARTIFACT_ROOT)
sparkle_require_variable(SPARKLE_DIST_ROOT)
sparkle_require_variable(SPARKLE_PACKAGE_VERSION)
sparkle_require_variable(SPARKLE_RELEASE_CHANNEL)
sparkle_require_variable(SPARKLE_PACKAGE_PLATFORM)
sparkle_require_variable(SPARKLE_PACKAGE_PROJECT_ID)

if(SPARKLE_BUILD_CONFIG STREQUAL "\${CONFIGURATION}" OR SPARKLE_BUILD_CONFIG STREQUAL "")
    set(SPARKLE_BUILD_CONFIG "DevelopmentEditor")
endif()

set(SPARKLE_RELEASE_ROOT "${SPARKLE_DIST_ROOT}/releases/${SPARKLE_PACKAGE_VERSION}")
set(SPARKLE_RUNTIME_PACKAGE_NAME "sparkle-runtime-${SPARKLE_PACKAGE_VERSION}-${SPARKLE_RELEASE_CHANNEL}-${SPARKLE_PACKAGE_PLATFORM}")
set(SPARKLE_SYMBOLS_PACKAGE_NAME "sparkle-symbols-${SPARKLE_PACKAGE_VERSION}-${SPARKLE_RELEASE_CHANNEL}-${SPARKLE_PACKAGE_PLATFORM}")
set(SPARKLE_RUNTIME_PACKAGE_ROOT "${SPARKLE_RELEASE_ROOT}/${SPARKLE_RUNTIME_PACKAGE_NAME}")
set(SPARKLE_SYMBOLS_PACKAGE_ROOT "${SPARKLE_RELEASE_ROOT}/${SPARKLE_SYMBOLS_PACKAGE_NAME}")

set(SPARKLE_DEV_ROOT "${SPARKLE_ARTIFACT_ROOT}/dev")
set(SPARKLE_PROJECT_ARTIFACT_ROOT "${SPARKLE_DEV_ROOT}/projects/${SPARKLE_PACKAGE_PROJECT_ID}")
set(SPARKLE_LAUNCHER_ARTIFACT_ROOT "${SPARKLE_DEV_ROOT}/launcher/${SPARKLE_BUILD_CONFIG}")
set(SPARKLE_EDITOR_ARTIFACT_ROOT "${SPARKLE_PROJECT_ARTIFACT_ROOT}/editor/${SPARKLE_BUILD_CONFIG}")
set(SPARKLE_RUNTIME_ARTIFACT_ROOT "${SPARKLE_PROJECT_ARTIFACT_ROOT}/runtime/${SPARKLE_BUILD_CONFIG}")
set(SPARKLE_PROJECT_COOKED_ROOT "${SPARKLE_PROJECT_ARTIFACT_ROOT}/cooked")
set(SPARKLE_PROJECT_COOKED_SHADER_ROOT "${SPARKLE_PROJECT_COOKED_ROOT}/Shaders")
set(SPARKLE_SYMBOL_SOURCE_ROOT "${SPARKLE_ARTIFACT_ROOT}/symbols")

set(SPARKLE_PACKAGE_PROJECT_ROOT "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Projects/${SPARKLE_PACKAGE_PROJECT_ID}")

file(REMOVE_RECURSE "${SPARKLE_RUNTIME_PACKAGE_ROOT}" "${SPARKLE_SYMBOLS_PACKAGE_ROOT}")
file(MAKE_DIRECTORY
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Apps/${SPARKLE_PACKAGE_PROJECT_ID}Editor"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Apps/${SPARKLE_PACKAGE_PROJECT_ID}Runtime"
    "${SPARKLE_PACKAGE_PROJECT_ROOT}/Cooked"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/licenses"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests"
    "${SPARKLE_SYMBOLS_PACKAGE_ROOT}/Symbols")

file(WRITE "${SPARKLE_RUNTIME_PACKAGE_ROOT}/.sparkle" "Sparkle runtime package root\n")
file(WRITE "${SPARKLE_PACKAGE_PROJECT_ROOT}/.sparkle-project" "Sparkle packaged project\n")
file(WRITE "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests/sparkle-package-manifest.json" "{\n}\n")

sparkle_stage_directory(
    "${SPARKLE_LAUNCHER_ARTIFACT_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}"
    "launcher runtime artifacts")

sparkle_stage_directory(
    "${SPARKLE_EDITOR_ARTIFACT_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Apps/${SPARKLE_PACKAGE_PROJECT_ID}Editor"
    "editor runtime artifacts")

sparkle_stage_directory(
    "${SPARKLE_RUNTIME_ARTIFACT_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Apps/${SPARKLE_PACKAGE_PROJECT_ID}Runtime"
    "runtime app artifacts")

sparkle_stage_directory(
    "${SPARKLE_PROJECT_COOKED_SHADER_ROOT}"
    "${SPARKLE_PACKAGE_PROJECT_ROOT}/Cooked/Shaders"
    "cooked shader packages")

sparkle_stage_file(
    "${SPARKLE_REPOSITORY_ROOT}/LICENSE.txt"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/licenses/LICENSE.txt"
    "repository license")

sparkle_stage_directory(
    "${SPARKLE_SYMBOL_SOURCE_ROOT}"
    "${SPARKLE_SYMBOLS_PACKAGE_ROOT}/Symbols"
    "symbol artifacts")

message(STATUS "Sparkle runtime package assembled: ${SPARKLE_RUNTIME_PACKAGE_ROOT}")
message(STATUS "Sparkle symbols package assembled separately: ${SPARKLE_SYMBOLS_PACKAGE_ROOT}")
