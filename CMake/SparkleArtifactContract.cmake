# Sparkle development artifact naming contract.
#
# Generated build trees are private CMake/MSBuild state. Runnable development
# products live under artifacts/.

if(NOT DEFINED SPARKLE_REPOSITORY_ROOT)
    set(SPARKLE_REPOSITORY_ROOT "${CMAKE_SOURCE_DIR}")
endif()
get_filename_component(SPARKLE_REPOSITORY_ROOT "${SPARKLE_REPOSITORY_ROOT}" ABSOLUTE)

set(_sparkle_canonical_build_root "${SPARKLE_REPOSITORY_ROOT}/build")
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

set(_sparkle_artifact_root "${SPARKLE_REPOSITORY_ROOT}/artifacts")
set(FETCHCONTENT_BASE_DIR "${_sparkle_canonical_build_root}/_deps" CACHE PATH
    "Shared Sparkle source-dependency cache; fixed beneath the canonical build root." FORCE)

set(_sparkle_legacy_generated_paths
    "${SPARKLE_REPOSITORY_ROOT}/Saved"
    "${SPARKLE_REPOSITORY_ROOT}/imgui.ini"
    "${SPARKLE_REPOSITORY_ROOT}/CMakeCache.txt"
    "${SPARKLE_REPOSITORY_ROOT}/CMakeFiles"
    "${SPARKLE_REPOSITORY_ROOT}/bin"
    "${SPARKLE_REPOSITORY_ROOT}/obj"
    "${SPARKLE_REPOSITORY_ROOT}/bld"
    "${SPARKLE_REPOSITORY_ROOT}/Debug"
    "${SPARKLE_REPOSITORY_ROOT}/DebugPublic"
    "${SPARKLE_REPOSITORY_ROOT}/Release"
    "${SPARKLE_REPOSITORY_ROOT}/Releases"
    "${SPARKLE_REPOSITORY_ROOT}/x64"
    "${SPARKLE_REPOSITORY_ROOT}/x86"
    "${SPARKLE_REPOSITORY_ROOT}/Intermediate"
    "${SPARKLE_REPOSITORY_ROOT}/DerivedDataCache"
    "${SPARKLE_REPOSITORY_ROOT}/Temp"
    "${SPARKLE_REPOSITORY_ROOT}/Tmp"
    "${SPARKLE_REPOSITORY_ROOT}/Scratch")
file(GLOB _sparkle_legacy_build_roots LIST_DIRECTORIES TRUE "${SPARKLE_REPOSITORY_ROOT}/build-*")
file(GLOB _sparkle_legacy_root_build_files LIST_DIRECTORIES FALSE
    "${SPARKLE_REPOSITORY_ROOT}/*.sln"
    "${SPARKLE_REPOSITORY_ROOT}/*.slnx"
    "${SPARKLE_REPOSITORY_ROOT}/*.vcxproj"
    "${SPARKLE_REPOSITORY_ROOT}/*.vcxproj.filters"
    "${SPARKLE_REPOSITORY_ROOT}/*.vcxproj.user"
    "${SPARKLE_REPOSITORY_ROOT}/cmake_install.cmake"
    "${SPARKLE_REPOSITORY_ROOT}/Makefile")
file(GLOB _sparkle_legacy_project_outputs LIST_DIRECTORIES TRUE
    "${SPARKLE_REPOSITORY_ROOT}/Projects/*/build"
    "${SPARKLE_REPOSITORY_ROOT}/Projects/*/Cooked"
    "${SPARKLE_REPOSITORY_ROOT}/Projects/*/cooked"
    "${SPARKLE_REPOSITORY_ROOT}/Projects/*/logs"
    "${SPARKLE_REPOSITORY_ROOT}/Projects/*/StreamlineLogs"
    "${SPARKLE_REPOSITORY_ROOT}/Projects/*/imgui.ini")
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
if(EXISTS "${SPARKLE_REPOSITORY_ROOT}/logs")
    message(WARNING
        "Legacy repository logs exist at '${SPARKLE_REPOSITORY_ROOT}/logs'. The current launcher can finish its self-update, then "
        "the Launcher Logs cleanup removes this transition output. Newly built processes write logs beneath the per-user state root.")
endif()

if(SPARKLE_ARTIFACT_VARIANT AND NOT SPARKLE_ARTIFACT_VARIANT MATCHES "^[A-Za-z0-9._-]+$")
    message(FATAL_ERROR "SPARKLE_ARTIFACT_VARIANT contains unsupported path characters: '${SPARKLE_ARTIFACT_VARIANT}'")
endif()

set(_sparkle_dependency_sync_build_tree FALSE)
set(_sparkle_launcher_product_build_tree FALSE)
get_filename_component(_sparkle_launcher_source_root "${SPARKLE_REPOSITORY_ROOT}/Tools/Launcher" ABSOLUTE)
get_filename_component(_sparkle_configured_source_root "${CMAKE_SOURCE_DIR}" ABSOLUTE)
if(_sparkle_configured_source_root STREQUAL _sparkle_launcher_source_root)
    set(_sparkle_expected_launcher_build_tree "private/tools/SparkleLauncher")
    if(NOT _sparkle_build_root_relative_to_canonical STREQUAL _sparkle_expected_launcher_build_tree)
        message(FATAL_ERROR
            "The Launcher product entry point must use exactly 'build/${_sparkle_expected_launcher_build_tree}'. "
            "Rejected build tree: '${_sparkle_configured_build_root}'.")
    endif()
    if(SPARKLE_ARTIFACT_VARIANT)
        message(FATAL_ERROR "The Launcher product entry point publishes the canonical Launcher artifact, not a variant.")
    endif()
    set(_sparkle_launcher_product_build_tree TRUE)
endif()

if(DEFINED SPARKLE_SYNC_SOURCE_DEPENDENCY AND NOT SPARKLE_SYNC_SOURCE_DEPENDENCY STREQUAL "")
    if(NOT SPARKLE_SYNC_SOURCE_DEPENDENCY MATCHES "^[A-Za-z0-9._-]+$")
        message(FATAL_ERROR
            "SPARKLE_SYNC_SOURCE_DEPENDENCY contains unsupported path characters: '${SPARKLE_SYNC_SOURCE_DEPENDENCY}'")
    endif()

    set(_sparkle_expected_dependency_sync_tree
        "_dependency-sync/${SPARKLE_SYNC_SOURCE_DEPENDENCY}")
    if(NOT _sparkle_build_root_relative_to_canonical STREQUAL _sparkle_expected_dependency_sync_tree)
        message(FATAL_ERROR
            "Source dependency sync must use exactly 'build/${_sparkle_expected_dependency_sync_tree}'. "
            "Rejected build tree: '${_sparkle_configured_build_root}'.")
    endif()
    if(SPARKLE_ARTIFACT_VARIANT)
        message(FATAL_ERROR "Source dependency sync does not publish an artifact variant.")
    endif()
    set(_sparkle_dependency_sync_build_tree TRUE)
endif()

if(_sparkle_dependency_sync_build_tree)
    # Selective dependency sync exits before product targets are generated. Its
    # CMake state remains private while fetched sources use build/_deps.
elseif(_sparkle_launcher_product_build_tree)
    # The Launcher has an owned product entry point and private build tree.
elseif(_sparkle_configured_build_root STREQUAL _sparkle_canonical_build_root)
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
    sparkle_stage_d3d12_pix_event_runtime(${target_name})
    sparkle_stage_external_capture_notices(${target_name})
endfunction()

function(sparkle_stage_d3d12_pix_event_runtime product_target)
    if(NOT TARGET Microsoft::WinPixEventRuntime OR NOT TARGET SparkleRHI_D3D12)
        add_custom_command(TARGET ${product_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E rm -f
                "$<TARGET_FILE_DIR:${product_target}>/WinPixEventRuntime.dll"
                "$<TARGET_FILE_DIR:${product_target}>/WinPixEventRuntime-LICENSE.txt"
            VERBATIM
        )
        return()
    endif()

    get_target_property(_sparkle_pix_enabled SparkleRHI_D3D12 SPARKLE_PIX_EVENTS_ENABLED)
    get_target_property(_sparkle_pix_license Microsoft::WinPixEventRuntime SPARKLE_LICENSE_FILE)
    add_custom_command(TARGET ${product_target} POST_BUILD
        COMMAND "$<${_sparkle_pix_enabled}:${CMAKE_COMMAND}>"
            "$<${_sparkle_pix_enabled}:-E>"
            "$<${_sparkle_pix_enabled}:copy_if_different>"
            "$<${_sparkle_pix_enabled}:$<TARGET_FILE:Microsoft::WinPixEventRuntime>>"
            "$<${_sparkle_pix_enabled}:$<TARGET_FILE_DIR:${product_target}>>"
        COMMAND "$<${_sparkle_pix_enabled}:${CMAKE_COMMAND}>"
            "$<${_sparkle_pix_enabled}:-E>"
            "$<${_sparkle_pix_enabled}:copy_if_different>"
            "$<${_sparkle_pix_enabled}:${_sparkle_pix_license}>"
            "$<${_sparkle_pix_enabled}:$<TARGET_FILE_DIR:${product_target}>/WinPixEventRuntime-LICENSE.txt>"
        COMMAND "$<$<NOT:${_sparkle_pix_enabled}>:${CMAKE_COMMAND}>"
            "$<$<NOT:${_sparkle_pix_enabled}>:-E>"
            "$<$<NOT:${_sparkle_pix_enabled}>:rm>"
            "$<$<NOT:${_sparkle_pix_enabled}>:-f>"
            "$<$<NOT:${_sparkle_pix_enabled}>:$<TARGET_FILE_DIR:${product_target}>/WinPixEventRuntime.dll>"
            "$<$<NOT:${_sparkle_pix_enabled}>:$<TARGET_FILE_DIR:${product_target}>/WinPixEventRuntime-LICENSE.txt>"
        COMMAND_EXPAND_LISTS
        VERBATIM
    )
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

function(sparkle_stage_external_capture_notices product_target)
    if(NOT TARGET SparkleRHI)
        return()
    endif()
    get_target_property(_capture_notices SparkleRHI SPARKLE_EXTERNAL_CAPTURE_NOTICES)
    if(NOT _capture_notices)
        return()
    endif()
    set(_capture_profile "$<AND:$<BOOL:${SPARKLE_RHI_WITH_EXTERNAL_CAPTURE}>,$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DevelopmentEditor>>>")
    foreach(_notice IN LISTS _capture_notices)
        string(REPLACE "|" ";" _notice_parts "${_notice}")
        list(GET _notice_parts 0 _notice_source)
        list(GET _notice_parts 1 _notice_name)
        add_custom_command(TARGET ${product_target} POST_BUILD
            COMMAND "$<${_capture_profile}:${CMAKE_COMMAND}>" "$<${_capture_profile}:-E>" "$<${_capture_profile}:copy_if_different>"
                "$<${_capture_profile}:${_notice_source}>" "$<${_capture_profile}:$<TARGET_FILE_DIR:${product_target}>/${_notice_name}>"
            COMMAND "$<$<NOT:${_capture_profile}>:${CMAKE_COMMAND}>" "$<$<NOT:${_capture_profile}>:-E>" "$<$<NOT:${_capture_profile}>:rm>" "$<$<NOT:${_capture_profile}>:-f>"
                "$<$<NOT:${_capture_profile}>:$<TARGET_FILE_DIR:${product_target}>/${_notice_name}>"
            COMMAND_EXPAND_LISTS VERBATIM)
    endforeach()
endfunction()
