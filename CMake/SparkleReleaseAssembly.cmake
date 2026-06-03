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

sparkle_require_variable(SPARKLE_REPOSITORY_ROOT)
sparkle_require_variable(SPARKLE_ARTIFACT_ROOT)
sparkle_require_variable(SPARKLE_DIST_ROOT)
sparkle_require_variable(SPARKLE_PACKAGE_VERSION)
sparkle_require_variable(SPARKLE_RELEASE_CHANNEL)
sparkle_require_variable(SPARKLE_PACKAGE_PLATFORM)

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
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"configuration\": \"${SPARKLE_BUILD_CONFIG}\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"toolchain\": \"recorded-by-phase6-validation\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"qtKit\": \"recorded-by-phase6-validation\",")
sparkle_append_line("${SPARKLE_BUILD_MANIFEST}" "  \"commit\": \"recorded-by-phase6-validation\",")
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

file(WRITE "${SPARKLE_RUNTIME_PACKAGE_ROOT}/RELEASE_NOTES.md" "# Sparkle ${SPARKLE_PACKAGE_VERSION}\n\nStatus: assembled for review only. Final validation and publishing are Phase 6 responsibilities.\n\n## Highlights\n\n- TODO\n\n## Known Issues\n\n- TODO\n\n## Regeneration\n\n- Launcher: Build > Build Launcher\n- Showcase editor/runtime: Build > Build Editor / Build Runtime\n- Cooked assets: Cook > Cook All\n")

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
message(STATUS "Final build/package validation was not run by Phase 5 assembly.")
