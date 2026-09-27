# Sparkle development artifact naming contract.
#
# Generated build trees are private CMake/MSBuild state. Runnable development
# products live under artifacts/.

set(_sparkle_canonical_build_root "${CMAKE_SOURCE_DIR}/build")
get_filename_component(_sparkle_canonical_build_root "${_sparkle_canonical_build_root}" ABSOLUTE)
get_filename_component(_sparkle_configured_build_root "${CMAKE_BINARY_DIR}" ABSOLUTE)
set(SPARKLE_ARTIFACT_VARIANT "" CACHE STRING "Artifact namespace for an isolated build/variants/<name> workspace.")

file(RELATIVE_PATH
    _sparkle_build_root_relative_to_canonical
    "${_sparkle_canonical_build_root}"
    "${_sparkle_configured_build_root}")

if(IS_ABSOLUTE "${_sparkle_build_root_relative_to_canonical}"
   OR _sparkle_build_root_relative_to_canonical MATCHES "^\\.\\.([/\\\\]|$)")
    message(FATAL_ERROR
        "Sparkle build trees must stay under '${_sparkle_canonical_build_root}'. "
        "Use SparkleLauncher for the canonical workspace or configure an isolated validation tree as "
        "'build/variants/<name>' with '-DSPARKLE_ARTIFACT_VARIANT=<name>'. "
        "Rejected build tree: '${_sparkle_configured_build_root}'.")
endif()

set(_sparkle_artifact_root "${CMAKE_SOURCE_DIR}/artifacts")
set(FETCHCONTENT_BASE_DIR "${_sparkle_canonical_build_root}/_deps" CACHE PATH
    "Shared Sparkle source-dependency cache; fixed beneath the canonical build root." FORCE)

set(_sparkle_legacy_generated_paths
    "${CMAKE_SOURCE_DIR}/Saved"
    "${CMAKE_SOURCE_DIR}/imgui.ini"
    "${CMAKE_SOURCE_DIR}/CMakeCache.txt"
    "${CMAKE_SOURCE_DIR}/CMakeFiles"
    "${CMAKE_SOURCE_DIR}/bin"
    "${CMAKE_SOURCE_DIR}/obj"
    "${CMAKE_SOURCE_DIR}/bld"
    "${CMAKE_SOURCE_DIR}/Debug"
    "${CMAKE_SOURCE_DIR}/DebugPublic"
    "${CMAKE_SOURCE_DIR}/Release"
    "${CMAKE_SOURCE_DIR}/Releases"
    "${CMAKE_SOURCE_DIR}/x64"
    "${CMAKE_SOURCE_DIR}/x86"
    "${CMAKE_SOURCE_DIR}/Intermediate"
    "${CMAKE_SOURCE_DIR}/DerivedDataCache"
    "${CMAKE_SOURCE_DIR}/Temp"
    "${CMAKE_SOURCE_DIR}/Tmp"
    "${CMAKE_SOURCE_DIR}/Scratch")
file(GLOB _sparkle_legacy_build_roots LIST_DIRECTORIES TRUE "${CMAKE_SOURCE_DIR}/build-*")
file(GLOB _sparkle_legacy_root_build_files LIST_DIRECTORIES FALSE
    "${CMAKE_SOURCE_DIR}/*.sln"
    "${CMAKE_SOURCE_DIR}/*.slnx"
    "${CMAKE_SOURCE_DIR}/*.vcxproj"
    "${CMAKE_SOURCE_DIR}/*.vcxproj.filters"
    "${CMAKE_SOURCE_DIR}/*.vcxproj.user"
    "${CMAKE_SOURCE_DIR}/cmake_install.cmake"
    "${CMAKE_SOURCE_DIR}/Makefile")
file(GLOB _sparkle_legacy_project_outputs LIST_DIRECTORIES TRUE
    "${CMAKE_SOURCE_DIR}/Projects/*/build"
    "${CMAKE_SOURCE_DIR}/Projects/*/Cooked"
    "${CMAKE_SOURCE_DIR}/Projects/*/cooked"
    "${CMAKE_SOURCE_DIR}/Projects/*/logs"
    "${CMAKE_SOURCE_DIR}/Projects/*/StreamlineLogs"
    "${CMAKE_SOURCE_DIR}/Projects/*/imgui.ini")
list(APPEND _sparkle_legacy_generated_paths
    ${_sparkle_legacy_build_roots}
    ${_sparkle_legacy_root_build_files}
    ${_sparkle_legacy_project_outputs})

set(_sparkle_existing_legacy_generated_paths)
foreach(_sparkle_legacy_path IN LISTS _sparkle_legacy_generated_paths)
    if(EXISTS "${_sparkle_legacy_path}")
        list(APPEND _sparkle_existing_legacy_generated_paths "${_sparkle_legacy_path}")
    endif()
endforeach()
if(_sparkle_existing_legacy_generated_paths)
    list(JOIN _sparkle_existing_legacy_generated_paths "\n  - " _sparkle_legacy_path_list)
    message(FATAL_ERROR
        "Legacy generated paths exist outside Sparkle's owned output roots:\n  - ${_sparkle_legacy_path_list}\n"
        "Use SparkleLauncher cleanup after preserving any wanted captures. New builds must write only to build/, artifacts/, "
        "or the documented per-user state root.")
endif()

# A launcher built before the per-user logging cutover creates repository-root
# logs while bootstrapping its replacement. Keep this path visible and warn, but
# do not strand that one-way self-update route with a configure failure.
if(EXISTS "${CMAKE_SOURCE_DIR}/logs")
    message(WARNING
        "Legacy repository logs exist at '${CMAKE_SOURCE_DIR}/logs'. The current launcher can finish its self-update, then "
        "the Launcher Logs cleanup removes this transition output. Newly built processes write logs beneath the per-user state root.")
endif()

if(SPARKLE_ARTIFACT_VARIANT AND NOT SPARKLE_ARTIFACT_VARIANT MATCHES "^[A-Za-z0-9._-]+$")
    message(FATAL_ERROR "SPARKLE_ARTIFACT_VARIANT contains unsupported path characters: '${SPARKLE_ARTIFACT_VARIANT}'")
endif()

if(_sparkle_configured_build_root STREQUAL _sparkle_canonical_build_root)
    if(SPARKLE_ARTIFACT_VARIANT)
        message(FATAL_ERROR
            "The canonical 'build/' tree must publish to canonical 'artifacts/'. "
            "SPARKLE_ARTIFACT_VARIANT is valid only with its matching 'build/variants/<name>' tree.")
    endif()
else()
    if(NOT _sparkle_build_root_relative_to_canonical MATCHES "^variants[/\\\\][A-Za-z0-9._-]+$")
        message(FATAL_ERROR
            "Alternate Sparkle build trees must use exactly 'build/variants/<name>'. "
            "Rejected build tree: '${_sparkle_configured_build_root}'.")
    endif()

    get_filename_component(_sparkle_build_variant_name "${_sparkle_configured_build_root}" NAME)
    if(NOT SPARKLE_ARTIFACT_VARIANT STREQUAL _sparkle_build_variant_name)
        message(FATAL_ERROR
            "Alternate Sparkle build tree '${_sparkle_configured_build_root}' must publish to its matching isolated artifact namespace. "
            "Pass '-DSPARKLE_ARTIFACT_VARIANT=${_sparkle_build_variant_name}'.")
    endif()
endif()

if(SPARKLE_ARTIFACT_VARIANT)
    set(_sparkle_active_artifact_root "${_sparkle_artifact_root}/${SPARKLE_ARTIFACT_VARIANT}")
else()
    set(_sparkle_active_artifact_root "${_sparkle_artifact_root}")
endif()

set(_sparkle_development_artifact_root "${_sparkle_active_artifact_root}/dev")
set(_sparkle_launcher_artifact_root "${_sparkle_development_artifact_root}/launcher")
set(_sparkle_tool_artifact_root "${_sparkle_development_artifact_root}/tools")
set(_sparkle_project_artifact_root "${_sparkle_development_artifact_root}/projects")
set(_sparkle_runtime_support_artifact_root "${_sparkle_development_artifact_root}/runtime-support")
set(_sparkle_library_artifact_root "${_sparkle_development_artifact_root}/libraries")
set(_sparkle_symbol_artifact_root "${_sparkle_active_artifact_root}/symbols")

function(_sparkle_set_target_artifact_directories target_name runtime_root symbol_owner)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Unknown Sparkle target '${target_name}'")
    endif()

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${runtime_root}/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${runtime_root}/$<CONFIG>"
        ARCHIVE_OUTPUT_DIRECTORY "${_sparkle_library_artifact_root}/${symbol_owner}/$<CONFIG>"
        PDB_OUTPUT_DIRECTORY "${_sparkle_symbol_artifact_root}/${symbol_owner}/$<CONFIG>"
        COMPILE_PDB_OUTPUT_DIRECTORY "${_sparkle_symbol_artifact_root}/${symbol_owner}/$<CONFIG>/obj"
    )

    foreach(config_type IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${config_type}" config_upper)
        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${runtime_root}/${config_type}"
            LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${runtime_root}/${config_type}"
            ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${_sparkle_library_artifact_root}/${symbol_owner}/${config_type}"
            PDB_OUTPUT_DIRECTORY_${config_upper} "${_sparkle_symbol_artifact_root}/${symbol_owner}/${config_type}"
            COMPILE_PDB_OUTPUT_DIRECTORY_${config_upper} "${_sparkle_symbol_artifact_root}/${symbol_owner}/${config_type}/obj"
        )
    endforeach()

    if(NOT CMAKE_CONFIGURATION_TYPES AND CMAKE_BUILD_TYPE)
        string(TOUPPER "${CMAKE_BUILD_TYPE}" config_upper)
        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${runtime_root}/${CMAKE_BUILD_TYPE}"
            LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${runtime_root}/${CMAKE_BUILD_TYPE}"
            ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${_sparkle_library_artifact_root}/${symbol_owner}/${CMAKE_BUILD_TYPE}"
            PDB_OUTPUT_DIRECTORY_${config_upper} "${_sparkle_symbol_artifact_root}/${symbol_owner}/${CMAKE_BUILD_TYPE}"
            COMPILE_PDB_OUTPUT_DIRECTORY_${config_upper} "${_sparkle_symbol_artifact_root}/${symbol_owner}/${CMAKE_BUILD_TYPE}/obj"
        )
    endif()
endfunction()

function(sparkle_configure_launcher_artifacts target_name)
    _sparkle_set_target_artifact_directories(${target_name} "${_sparkle_launcher_artifact_root}" "launcher")
endfunction()

function(sparkle_configure_development_tool_artifacts target_name)
    _sparkle_set_target_artifact_directories(${target_name} "${_sparkle_tool_artifact_root}/${target_name}" "tools/${target_name}")
endfunction()

function(sparkle_configure_runtime_support_artifacts target_name)
    _sparkle_set_target_artifact_directories(
        ${target_name}
        "${_sparkle_runtime_support_artifact_root}/${target_name}"
        "runtime-support/${target_name}")
endfunction()

function(sparkle_configure_project_artifacts target_name project_name product_role)
    _sparkle_set_target_artifact_directories(
        ${target_name}
        "${_sparkle_project_artifact_root}/${project_name}/${product_role}"
        "projects/${project_name}/${product_role}")
endfunction()

function(sparkle_get_project_cooked_directory output_variable project_name)
    set(${output_variable} "${_sparkle_project_artifact_root}/${project_name}/cooked" PARENT_SCOPE)
endfunction()

function(sparkle_declare_runtime_dll_owner product_target)
    if(NOT TARGET ${product_target})
        message(FATAL_ERROR "Unknown Sparkle product target '${product_target}'")
    endif()

    if(NOT SPARKLE_BUILD_SHARED)
        return()
    endif()

    foreach(runtime_target IN LISTS ARGN)
        if(TARGET ${runtime_target})
            add_custom_command(TARGET ${product_target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "$<TARGET_FILE:${runtime_target}>"
                    "$<TARGET_FILE_DIR:${product_target}>"
                COMMENT "Copying ${runtime_target} runtime DLL for ${product_target}"
                VERBATIM
            )
        endif()
    endforeach()
endfunction()

function(sparkle_stage_nvidia_streamline_runtime product_target)
    if(NOT TARGET ${product_target})
        message(FATAL_ERROR "Unknown Sparkle product target '${product_target}'")
    endif()

    if(NOT SPARKLE_ENABLE_NVIDIA_STREAMLINE)
        return()
    endif()

    if(NOT DEFINED SPARKLE_NVIDIA_STREAMLINE_RUNTIME_DLLS)
        message(FATAL_ERROR
            "NVIDIA Streamline runtime DLL list is not configured. "
            "FetchDependencies.cmake must run before staging Streamline.")
    endif()

    foreach(runtime_dll IN LISTS SPARKLE_NVIDIA_STREAMLINE_RUNTIME_DLLS)
        if(NOT EXISTS "${runtime_dll}")
            message(FATAL_ERROR "NVIDIA Streamline runtime DLL is missing: '${runtime_dll}'")
        endif()
        add_custom_command(TARGET ${product_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${runtime_dll}"
                "$<TARGET_FILE_DIR:${product_target}>"
            COMMENT "Copying NVIDIA Streamline runtime ${runtime_dll} for ${product_target}"
            VERBATIM
        )
    endforeach()
endfunction()

message(STATUS
    "Sparkle roots: build=${_sparkle_configured_build_root}; artifacts=${_sparkle_active_artifact_root}; "
    "dev=${_sparkle_development_artifact_root}")
message(STATUS "Sparkle artifact variant: ${SPARKLE_ARTIFACT_VARIANT}")
