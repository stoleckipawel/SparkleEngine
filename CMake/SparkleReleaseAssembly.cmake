cmake_minimum_required(VERSION 3.20)

function(sparkle_require_variable variable_name)
    if(NOT DEFINED ${variable_name} OR "${${variable_name}}" STREQUAL "")
        message(FATAL_ERROR "Required release assembly variable '${variable_name}' is not set.")
    endif()
endfunction()

function(sparkle_json_escape input output_variable)
    set(value "${input}")
    string(REPLACE "\\" "\\\\" value "${value}")
    string(REPLACE "\"" "\\\"" value "${value}")
    string(REPLACE "\n" "\\n" value "${value}")
    string(REPLACE "\r" "\\r" value "${value}")
    set(${output_variable} "${value}" PARENT_SCOPE)
endfunction()

function(sparkle_append_line file_path line)
    file(APPEND "${file_path}" "${line}\n")
endfunction()

function(sparkle_append_manifest_component component_id status source_path destination_path package_kind visibility binary_type producer regenerate_command)
    sparkle_json_escape("${component_id}" component_json)
    sparkle_json_escape("${source_path}" source_json)
    sparkle_json_escape("${destination_path}" destination_json)
    sparkle_json_escape("${package_kind}" package_kind_json)
    sparkle_json_escape("${visibility}" visibility_json)
    sparkle_json_escape("${binary_type}" binary_type_json)
    sparkle_json_escape("${producer}" producer_json)
    sparkle_json_escape("${regenerate_command}" regenerate_json)

    get_property(component_count GLOBAL PROPERTY SPARKLE_COMPONENT_MANIFEST_COUNT)
    if(NOT component_count)
        set(component_count 0)
    endif()
    if(component_count GREATER 0)
        file(APPEND "${SPARKLE_COMPONENT_MANIFEST_PATH}" ",\n")
    endif()

    file(APPEND "${SPARKLE_COMPONENT_MANIFEST_PATH}" "    { \"id\": \"${component_json}\", \"status\": \"${status}\", \"source\": \"${source_json}\", \"destination\": \"${destination_json}\", \"packageKind\": \"${package_kind_json}\", \"visibility\": \"${visibility_json}\", \"binaryType\": \"${binary_type_json}\", \"producer\": \"${producer_json}\", \"regenerate\": \"${regenerate_json}\" }")
    math(EXPR component_count "${component_count} + 1")
    set_property(GLOBAL PROPERTY SPARKLE_COMPONENT_MANIFEST_COUNT "${component_count}")
endfunction()

function(sparkle_copy_directory_if_present source_dir destination_dir component_id package_kind visibility binary_type producer regenerate_command)
    if(EXISTS "${source_dir}")
        file(MAKE_DIRECTORY "${destination_dir}")
        file(COPY "${source_dir}/" DESTINATION "${destination_dir}")
        set(status "present")
    else()
        set(status "missing")
    endif()

    sparkle_append_manifest_component(
        "${component_id}"
        "${status}"
        "${source_dir}"
        "${destination_dir}"
        "${package_kind}"
        "${visibility}"
        "${binary_type}"
        "${producer}"
        "${regenerate_command}")
endfunction()

function(sparkle_copy_file_if_present source_file destination_file component_id package_kind visibility binary_type producer regenerate_command)
    if(EXISTS "${source_file}")
        get_filename_component(destination_directory "${destination_file}" DIRECTORY)
        file(MAKE_DIRECTORY "${destination_directory}")
        configure_file("${source_file}" "${destination_file}" COPYONLY)
        set(status "present")
    else()
        set(status "missing")
    endif()

    sparkle_append_manifest_component(
        "${component_id}"
        "${status}"
        "${source_file}"
        "${destination_file}"
        "${package_kind}"
        "${visibility}"
        "${binary_type}"
        "${producer}"
        "${regenerate_command}")
endfunction()

function(sparkle_write_file_manifest root_dir manifest_path checksums_path package_label)
    file(WRITE "${manifest_path}" "{\n")
    sparkle_json_escape("${package_label}" package_label_json)
    sparkle_append_line("${manifest_path}" "  \"package\": \"${package_label_json}\",")
    sparkle_append_line("${manifest_path}" "  \"files\": [")
    file(WRITE "${checksums_path}" "")
    set(file_manifest_count 0)
    file(RELATIVE_PATH manifest_relative_path "${root_dir}" "${manifest_path}")
    file(RELATIVE_PATH checksums_relative_path "${root_dir}" "${checksums_path}")

    if(EXISTS "${root_dir}")
        file(GLOB_RECURSE package_files LIST_DIRECTORIES FALSE "${root_dir}/*")
        list(SORT package_files)
        foreach(package_file IN LISTS package_files)
            file(RELATIVE_PATH relative_file "${root_dir}" "${package_file}")
            if(relative_file STREQUAL manifest_relative_path OR relative_file STREQUAL checksums_relative_path)
                continue()
            endif()
            file(SHA256 "${package_file}" file_hash)
            file(SIZE "${package_file}" file_size)
            sparkle_json_escape("${relative_file}" relative_json)
            if(file_manifest_count GREATER 0)
                file(APPEND "${manifest_path}" ",\n")
            endif()
            file(APPEND "${manifest_path}" "    { \"path\": \"${relative_json}\", \"size\": ${file_size}, \"sha256\": \"${file_hash}\" }")
            sparkle_append_line("${checksums_path}" "${file_hash}  ${relative_file}")
            math(EXPR file_manifest_count "${file_manifest_count} + 1")
        endforeach()
    endif()

    if(file_manifest_count GREATER 0)
        file(APPEND "${manifest_path}" "\n")
    endif()
    sparkle_append_line("${manifest_path}" "  ]")
    sparkle_append_line("${manifest_path}" "}")
endfunction()

function(sparkle_capture_git_value output_variable)
    set(multi_value_args COMMAND_ARGS)
    cmake_parse_arguments(SPARKLE_GIT "" "" "${multi_value_args}" ${ARGN})

    set(captured_value "unavailable")
    if(DEFINED GIT_EXECUTABLE AND NOT "${GIT_EXECUTABLE}" STREQUAL "" AND EXISTS "${GIT_EXECUTABLE}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" ${SPARKLE_GIT_COMMAND_ARGS}
            WORKING_DIRECTORY "${SPARKLE_REPOSITORY_ROOT}"
            OUTPUT_VARIABLE git_output
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE git_result)
        if(git_result EQUAL 0 AND NOT "${git_output}" STREQUAL "")
            set(captured_value "${git_output}")
        endif()
    endif()

    set(${output_variable} "${captured_value}" PARENT_SCOPE)
endfunction()

function(sparkle_capture_release_metadata)
    if(NOT DEFINED GIT_EXECUTABLE OR "${GIT_EXECUTABLE}" STREQUAL "" OR NOT EXISTS "${GIT_EXECUTABLE}")
        if(DEFINED SPARKLE_GIT_EXE AND NOT "${SPARKLE_GIT_EXE}" STREQUAL "" AND EXISTS "${SPARKLE_GIT_EXE}")
            set(GIT_EXECUTABLE "${SPARKLE_GIT_EXE}")
        else()
            find_program(GIT_EXECUTABLE
                NAMES git git.exe
                PATHS
                    "C:/Program Files/Git/cmd"
                    "C:/Program Files/Git/bin"
                    "C:/Program Files (x86)/Git/cmd"
                    "C:/Program Files (x86)/Git/bin")
        endif()
    endif()

    sparkle_capture_git_value(SPARKLE_RELEASE_COMMIT COMMAND_ARGS rev-parse HEAD)
    sparkle_capture_git_value(SPARKLE_RELEASE_BRANCH COMMAND_ARGS rev-parse --abbrev-ref HEAD)

    set(SPARKLE_RELEASE_DIRTY "unknown")
    if(DEFINED GIT_EXECUTABLE AND NOT "${GIT_EXECUTABLE}" STREQUAL "" AND EXISTS "${GIT_EXECUTABLE}")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" status --porcelain
            WORKING_DIRECTORY "${SPARKLE_REPOSITORY_ROOT}"
            OUTPUT_VARIABLE git_status_output
            ERROR_QUIET
            OUTPUT_STRIP_TRAILING_WHITESPACE
            RESULT_VARIABLE git_status_result)
        if(git_status_result EQUAL 0)
            if("${git_status_output}" STREQUAL "")
                set(SPARKLE_RELEASE_DIRTY "false")
            else()
                set(SPARKLE_RELEASE_DIRTY "true")
            endif()
        endif()
    endif()

    if(DEFINED SPARKLE_RELEASE_CXX_COMPILER_ID AND NOT "${SPARKLE_RELEASE_CXX_COMPILER_ID}" STREQUAL "")
        set(SPARKLE_RELEASE_TOOLCHAIN "${SPARKLE_RELEASE_CXX_COMPILER_ID} ${SPARKLE_RELEASE_CXX_COMPILER_VERSION}")
    elseif(DEFINED CMAKE_CXX_COMPILER_ID AND NOT "${CMAKE_CXX_COMPILER_ID}" STREQUAL "")
        set(SPARKLE_RELEASE_TOOLCHAIN "${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
    else()
        set(SPARKLE_RELEASE_TOOLCHAIN "unavailable")
    endif()

    set(SPARKLE_RELEASE_GENERATOR "${SPARKLE_RELEASE_CMAKE_GENERATOR}")
    set(SPARKLE_RELEASE_PLATFORM "${SPARKLE_RELEASE_CMAKE_GENERATOR_PLATFORM}")
    set(SPARKLE_RELEASE_TOOLSET "${SPARKLE_RELEASE_CMAKE_GENERATOR_TOOLSET}")
    if("${SPARKLE_RELEASE_GENERATOR}" STREQUAL "")
        set(SPARKLE_RELEASE_GENERATOR "${CMAKE_GENERATOR}")
    endif()
    if("${SPARKLE_RELEASE_PLATFORM}" STREQUAL "")
        set(SPARKLE_RELEASE_PLATFORM "${CMAKE_GENERATOR_PLATFORM}")
    endif()
    if("${SPARKLE_RELEASE_TOOLSET}" STREQUAL "")
        set(SPARKLE_RELEASE_TOOLSET "${CMAKE_GENERATOR_TOOLSET}")
    endif()

    set(SPARKLE_RELEASE_QT_KIT "unavailable")
    if(DEFINED SPARKLE_RELEASE_QT_KIT_HINT AND NOT "${SPARKLE_RELEASE_QT_KIT_HINT}" STREQUAL "")
        list(GET SPARKLE_RELEASE_QT_KIT_HINT 0 SPARKLE_RELEASE_QT_KIT)
    elseif(DEFINED Qt6_DIR AND NOT "${Qt6_DIR}" STREQUAL "")
        get_filename_component(SPARKLE_RELEASE_QT_KIT "${Qt6_DIR}/../.." ABSOLUTE)
        cmake_path(NORMAL_PATH SPARKLE_RELEASE_QT_KIT OUTPUT_VARIABLE SPARKLE_RELEASE_QT_KIT)
    elseif(DEFINED SPARKLE_QT_ROOT AND NOT "${SPARKLE_QT_ROOT}" STREQUAL "")
        set(SPARKLE_RELEASE_QT_KIT "${SPARKLE_QT_ROOT}")
    endif()

    foreach(metadata_name
        SPARKLE_RELEASE_COMMIT
        SPARKLE_RELEASE_BRANCH
        SPARKLE_RELEASE_DIRTY
        SPARKLE_RELEASE_TOOLCHAIN
        SPARKLE_RELEASE_GENERATOR
        SPARKLE_RELEASE_PLATFORM
        SPARKLE_RELEASE_TOOLSET
        SPARKLE_RELEASE_QT_KIT)
        set(${metadata_name} "${${metadata_name}}" PARENT_SCOPE)
    endforeach()
endfunction()

sparkle_require_variable(SPARKLE_REPOSITORY_ROOT)
sparkle_require_variable(SPARKLE_ARTIFACT_ROOT)
sparkle_require_variable(SPARKLE_DIST_ROOT)
sparkle_require_variable(SPARKLE_PACKAGE_VERSION)
sparkle_require_variable(SPARKLE_RELEASE_CHANNEL)
sparkle_require_variable(SPARKLE_PACKAGE_PLATFORM)
sparkle_capture_release_metadata()

if(SPARKLE_BUILD_CONFIG STREQUAL "\${CONFIGURATION}" OR SPARKLE_BUILD_CONFIG STREQUAL "")
    set(SPARKLE_BUILD_CONFIG "DevelopmentEditor")
endif()

set(SPARKLE_RELEASE_ROOT "${SPARKLE_DIST_ROOT}/releases/${SPARKLE_PACKAGE_VERSION}")
set(SPARKLE_RUNTIME_PACKAGE_ID "sparkle-runtime")
set(SPARKLE_SYMBOLS_PACKAGE_ID "sparkle-symbols")
set(SPARKLE_DEPENDENCY_PACK_ID "sparkle-dependencies")
set(SPARKLE_RUNTIME_PACKAGE_NAME "${SPARKLE_RUNTIME_PACKAGE_ID}-${SPARKLE_PACKAGE_VERSION}-${SPARKLE_RELEASE_CHANNEL}-${SPARKLE_PACKAGE_PLATFORM}")
set(SPARKLE_SYMBOLS_PACKAGE_NAME "${SPARKLE_SYMBOLS_PACKAGE_ID}-${SPARKLE_PACKAGE_VERSION}-${SPARKLE_RELEASE_CHANNEL}-${SPARKLE_PACKAGE_PLATFORM}")
set(SPARKLE_RUNTIME_PACKAGE_ROOT "${SPARKLE_RELEASE_ROOT}/${SPARKLE_RUNTIME_PACKAGE_NAME}")
set(SPARKLE_SYMBOLS_PACKAGE_ROOT "${SPARKLE_RELEASE_ROOT}/${SPARKLE_SYMBOLS_PACKAGE_NAME}")
set(SPARKLE_COMPONENT_MANIFEST_PATH "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests/sparkle-bundled-runtime-components.json")

file(REMOVE_RECURSE "${SPARKLE_RUNTIME_PACKAGE_ROOT}" "${SPARKLE_SYMBOLS_PACKAGE_ROOT}")
file(MAKE_DIRECTORY
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Apps/ShowcaseEditor"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Apps/ShowcaseRuntime"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Projects/Showcase/Cooked"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Projects/Shared/Cooked"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/docs"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/licenses"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/redist"
    "${SPARKLE_SYMBOLS_PACKAGE_ROOT}/Symbols")

file(WRITE "${SPARKLE_COMPONENT_MANIFEST_PATH}" "{\n  \"components\": [\n")

set(SPARKLE_DEV_ROOT "${SPARKLE_ARTIFACT_ROOT}/dev")
set(SPARKLE_LAUNCHER_ARTIFACT_ROOT "${SPARKLE_DEV_ROOT}/launcher/${SPARKLE_BUILD_CONFIG}")
set(SPARKLE_SHOWCASE_EDITOR_ARTIFACT_ROOT "${SPARKLE_DEV_ROOT}/projects/Showcase/editor/${SPARKLE_BUILD_CONFIG}")
set(SPARKLE_SHOWCASE_RUNTIME_ARTIFACT_ROOT "${SPARKLE_DEV_ROOT}/projects/Showcase/runtime/${SPARKLE_BUILD_CONFIG}")
set(SPARKLE_SHOWCASE_COOKED_ROOT "${SPARKLE_DEV_ROOT}/projects/Showcase/cooked")
set(SPARKLE_SHARED_COOKED_ROOT "${SPARKLE_DEV_ROOT}/projects/Shared/cooked")

sparkle_copy_directory_if_present(
    "${SPARKLE_LAUNCHER_ARTIFACT_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}"
    "launcher"
    "runtime"
    "public"
    "app"
    "Build Launcher"
    "Build > Build Launcher")

sparkle_copy_directory_if_present(
    "${SPARKLE_SHOWCASE_EDITOR_ARTIFACT_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Apps/ShowcaseEditor"
    "showcase-editor"
    "runtime"
    "public"
    "app"
    "Build Editor"
    "Build > Build Editor")

sparkle_copy_directory_if_present(
    "${SPARKLE_SHOWCASE_RUNTIME_ARTIFACT_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Apps/ShowcaseRuntime"
    "showcase-runtime"
    "runtime"
    "public"
    "app"
    "Build Runtime"
    "Build > Build Runtime")

sparkle_copy_directory_if_present(
    "${SPARKLE_SHOWCASE_COOKED_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Projects/Showcase/Cooked"
    "showcase-cooked-assets"
    "runtime"
    "public"
    "generated-asset"
    "Cook All"
    "Cook > Cook All")

sparkle_copy_directory_if_present(
    "${SPARKLE_SHARED_COOKED_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/Projects/Shared/Cooked"
    "shared-cooked-assets"
    "runtime"
    "public"
    "generated-asset"
    "Cook All"
    "Cook > Cook All")

sparkle_copy_file_if_present(
    "${SPARKLE_REPOSITORY_ROOT}/LICENSE.txt"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/licenses/LICENSE.txt"
    "sparkle-license"
    "runtime"
    "public"
    "generated-asset"
    "Repository license"
    "N/A")

get_property(component_count GLOBAL PROPERTY SPARKLE_COMPONENT_MANIFEST_COUNT)
if(NOT component_count)
    set(component_count 0)
endif()
if(component_count GREATER 0)
    file(APPEND "${SPARKLE_COMPONENT_MANIFEST_PATH}" "\n")
endif()
file(APPEND "${SPARKLE_COMPONENT_MANIFEST_PATH}" "  ]\n}\n")

set(SPARKLE_RELEASE_MANIFEST "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests/sparkle-release-manifest.json")
file(WRITE "${SPARKLE_RELEASE_MANIFEST}" "{\n")
sparkle_append_line("${SPARKLE_RELEASE_MANIFEST}" "  \"version\": \"${SPARKLE_PACKAGE_VERSION}\",")
sparkle_append_line("${SPARKLE_RELEASE_MANIFEST}" "  \"channel\": \"${SPARKLE_RELEASE_CHANNEL}\",")
sparkle_append_line("${SPARKLE_RELEASE_MANIFEST}" "  \"platform\": \"${SPARKLE_PACKAGE_PLATFORM}\",")
sparkle_append_line("${SPARKLE_RELEASE_MANIFEST}" "  \"package\": \"${SPARKLE_RUNTIME_PACKAGE_NAME}\",")
sparkle_append_line("${SPARKLE_RELEASE_MANIFEST}" "  \"symbolsPackage\": \"${SPARKLE_SYMBOLS_PACKAGE_NAME}\",")
sparkle_append_line("${SPARKLE_RELEASE_MANIFEST}" "  \"source\": \"artifacts\",")
sparkle_append_line("${SPARKLE_RELEASE_MANIFEST}" "  \"publishReady\": false")
sparkle_append_line("${SPARKLE_RELEASE_MANIFEST}" "}")

set(SPARKLE_BUILD_MANIFEST "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests/sparkle-build-manifest.json")
file(WRITE "${SPARKLE_BUILD_MANIFEST}" "{\n")
sparkle_json_escape("${SPARKLE_BUILD_CONFIG}" SPARKLE_BUILD_CONFIG_JSON)
sparkle_json_escape("${SPARKLE_RELEASE_TOOLCHAIN}" SPARKLE_RELEASE_TOOLCHAIN_JSON)
sparkle_json_escape("${SPARKLE_RELEASE_GENERATOR}" SPARKLE_RELEASE_GENERATOR_JSON)
sparkle_json_escape("${SPARKLE_RELEASE_PLATFORM}" SPARKLE_RELEASE_PLATFORM_JSON)
sparkle_json_escape("${SPARKLE_RELEASE_TOOLSET}" SPARKLE_RELEASE_TOOLSET_JSON)
sparkle_json_escape("${SPARKLE_RELEASE_QT_KIT}" SPARKLE_RELEASE_QT_KIT_JSON)
sparkle_json_escape("${SPARKLE_RELEASE_COMMIT}" SPARKLE_RELEASE_COMMIT_JSON)
sparkle_json_escape("${SPARKLE_RELEASE_BRANCH}" SPARKLE_RELEASE_BRANCH_JSON)
sparkle_json_escape("${SPARKLE_RELEASE_DIRTY}" SPARKLE_RELEASE_DIRTY_JSON)
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"configuration\": \"${SPARKLE_BUILD_CONFIG_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"toolchain\": \"${SPARKLE_RELEASE_TOOLCHAIN_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"generator\": \"${SPARKLE_RELEASE_GENERATOR_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"generatorPlatform\": \"${SPARKLE_RELEASE_PLATFORM_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"generatorToolset\": \"${SPARKLE_RELEASE_TOOLSET_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"qtKit\": \"${SPARKLE_RELEASE_QT_KIT_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"commit\": \"${SPARKLE_RELEASE_COMMIT_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"branch\": \"${SPARKLE_RELEASE_BRANCH_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"dirty\": \"${SPARKLE_RELEASE_DIRTY_JSON}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"finalValidation\": false")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "}")

set(SPARKLE_DEPENDENCY_MANIFEST "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests/sparkle-dependency-manifest.json")
file(WRITE "${SPARKLE_DEPENDENCY_MANIFEST}" "{\n")
sparkle_append_line("${SPARKLE_DEPENDENCY_MANIFEST}" "  \"dependencyPackName\": \"${SPARKLE_DEPENDENCY_PACK_ID}-${SPARKLE_PACKAGE_VERSION}-${SPARKLE_RELEASE_CHANNEL}-${SPARKLE_PACKAGE_PLATFORM}\",")
sparkle_append_line("${SPARKLE_DEPENDENCY_MANIFEST}" "  \"groups\": [ \"core-workspace\", \"content-pipeline\", \"shader-compiler\", \"ktx-support\" ],")
sparkle_append_line("${SPARKLE_DEPENDENCY_MANIFEST}" "  \"runtimeRedistributables\": [ \"Qt runtime beside package-root SparkleLauncher.exe\", \"slang.dll beside ShaderCompiler in development package when present\", \"Sparkle runtime DLLs copied by declared product ownership when shared builds are enabled\" ],")
sparkle_append_line("${SPARKLE_DEPENDENCY_MANIFEST}" "  \"notes\": \"Phase 5 records package dependency naming and expected redistributable ownership. Final verification is Phase 6.\"")
sparkle_append_line("${SPARKLE_DEPENDENCY_MANIFEST}" "}")

set(SPARKLE_PACKAGE_MANIFEST "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests/sparkle-package-manifest.json")
file(WRITE "${SPARKLE_PACKAGE_MANIFEST}" "{\n")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "  \"packageKind\": \"runtime\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "  \"visibility\": \"public\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "  \"rootLauncherContract\": \"SparkleLauncher.exe is located at package root.\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "  \"navigation\": {")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"launcher\": \"SparkleLauncher.exe\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"apps\": \"Apps/\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"cookedAssets\": \"Projects/<Project>/Cooked/\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"manifests\": \"manifests/\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"docs\": \"docs/ and RELEASE_NOTES.md\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"licenses\": \"licenses/\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"runtimeSupport\": \"redist/\"")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "  },")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "  \"inclusionRules\": {")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"runtime\": \"Include public apps, runtime support files, cooked assets, manifests, release notes, and licenses owned by bundled runtime components.\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"development\": \"Reserve developer tools, import libraries, static libraries, headers, source-facing diagnostics, and dependency pack references for development packages.\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"symbols\": \"Keep symbol files and debug artifacts in a separate symbols package.\",")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "    \"internalPrivate\": \"Exclude internal and private surfaces from public runtime packages unless a future manifest rule explicitly promotes them.\"")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "  }")
sparkle_append_line("${SPARKLE_PACKAGE_MANIFEST}" "}")

file(WRITE "${SPARKLE_RUNTIME_PACKAGE_ROOT}/RELEASE_NOTES.md" "# Sparkle ${SPARKLE_PACKAGE_VERSION}\n\nStatus: assembled for review only. Final validation and publishing are Phase 6 responsibilities.\n\n## Highlights\n\n- Package-root launcher contract: `SparkleLauncher.exe` is staged at the runtime package root.\n- Runtime package layout separates apps, cooked assets, manifests, licenses, and redistributable support files.\n- Bundled runtime components are described by manifests so rebuild and recook workflows stay optional for first-run exploration.\n\n## Known Issues\n\n- This package is not publish-ready until the Phase 6 validation checklist passes on the release machine.\n- Missing bundled components are recorded in `manifests/sparkle-bundled-runtime-components.json` rather than silently substituted from local build output.\n\n## Regeneration\n\n- Launcher: Build > Build Launcher\n- Showcase editor/runtime: Build > Build Editor / Build Runtime\n- Cooked assets: Cook > Cook All\n")

if(EXISTS "${SPARKLE_ARTIFACT_ROOT}/symbols")
    file(COPY "${SPARKLE_ARTIFACT_ROOT}/symbols/" DESTINATION "${SPARKLE_SYMBOLS_PACKAGE_ROOT}/Symbols")
endif()

sparkle_write_file_manifest(
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests/sparkle-package-files.json"
    "${SPARKLE_RUNTIME_PACKAGE_ROOT}/manifests/SHA256SUMS.txt"
    "${SPARKLE_RUNTIME_PACKAGE_NAME}")

sparkle_write_file_manifest(
    "${SPARKLE_SYMBOLS_PACKAGE_ROOT}"
    "${SPARKLE_SYMBOLS_PACKAGE_ROOT}/sparkle-symbols-files.json"
    "${SPARKLE_SYMBOLS_PACKAGE_ROOT}/SHA256SUMS.txt"
    "${SPARKLE_SYMBOLS_PACKAGE_NAME}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${SPARKLE_RELEASE_ROOT}/${SPARKLE_SYMBOLS_PACKAGE_NAME}.zip" --format=zip "${SPARKLE_SYMBOLS_PACKAGE_NAME}"
    WORKING_DIRECTORY "${SPARKLE_RELEASE_ROOT}"
    RESULT_VARIABLE SPARKLE_SYMBOL_ARCHIVE_RESULT)
if(NOT SPARKLE_SYMBOL_ARCHIVE_RESULT EQUAL 0)
    message(WARNING "Symbols archive creation failed. Package folder remains available at ${SPARKLE_SYMBOLS_PACKAGE_ROOT}.")
endif()

message(STATUS "Sparkle runtime package assembled for review: ${SPARKLE_RUNTIME_PACKAGE_ROOT}")
message(STATUS "Sparkle symbols package assembled separately: ${SPARKLE_SYMBOLS_PACKAGE_ROOT}")
message(STATUS "Release assembly completed. Run the final validation checklist before publishing.")
