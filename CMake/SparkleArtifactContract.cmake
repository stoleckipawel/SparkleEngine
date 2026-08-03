# Sparkle development artifact naming contract.
#
# Generated build trees are private CMake/MSBuild state. Runnable development
# products live under artifacts/.

set(SPARKLE_BUILD_ROOT "${CMAKE_BINARY_DIR}")
set(SPARKLE_ARTIFACT_ROOT "${CMAKE_SOURCE_DIR}/artifacts" CACHE PATH "Generated development artifact root.")
set(SPARKLE_ARTIFACT_VARIANT "" CACHE STRING "Optional artifact namespace for an alternate build tree.")

if(SPARKLE_ARTIFACT_VARIANT AND NOT SPARKLE_ARTIFACT_VARIANT MATCHES "^[A-Za-z0-9._-]+$")
    message(FATAL_ERROR "SPARKLE_ARTIFACT_VARIANT contains unsupported path characters: '${SPARKLE_ARTIFACT_VARIANT}'")
endif()

if(SPARKLE_ARTIFACT_VARIANT)
    set(SPARKLE_ACTIVE_ARTIFACT_ROOT "${SPARKLE_ARTIFACT_ROOT}/${SPARKLE_ARTIFACT_VARIANT}")
else()
    set(SPARKLE_ACTIVE_ARTIFACT_ROOT "${SPARKLE_ARTIFACT_ROOT}")
endif()

set(SPARKLE_DEV_ARTIFACT_ROOT "${SPARKLE_ACTIVE_ARTIFACT_ROOT}/dev")
set(SPARKLE_DEV_LAUNCHER_ROOT "${SPARKLE_DEV_ARTIFACT_ROOT}/launcher")
set(SPARKLE_DEV_TOOLS_ROOT "${SPARKLE_DEV_ARTIFACT_ROOT}/tools")
set(SPARKLE_DEV_PROJECTS_ROOT "${SPARKLE_DEV_ARTIFACT_ROOT}/projects")
set(SPARKLE_DEV_RUNTIME_SUPPORT_ROOT "${SPARKLE_DEV_ARTIFACT_ROOT}/runtime-support")
set(SPARKLE_DEV_LIBRARY_ROOT "${SPARKLE_DEV_ARTIFACT_ROOT}/libraries")
set(SPARKLE_DIAGNOSTICS_ROOT "${SPARKLE_ACTIVE_ARTIFACT_ROOT}/diagnostics")
set(SPARKLE_SYMBOL_ROOT "${SPARKLE_ACTIVE_ARTIFACT_ROOT}/symbols")

function(sparkle_set_product_artifact_directories target_name runtime_root symbol_owner)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Unknown Sparkle target '${target_name}'")
    endif()

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${runtime_root}/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${runtime_root}/$<CONFIG>"
        ARCHIVE_OUTPUT_DIRECTORY "${SPARKLE_DEV_LIBRARY_ROOT}/${symbol_owner}/$<CONFIG>"
        PDB_OUTPUT_DIRECTORY "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/$<CONFIG>"
        COMPILE_PDB_OUTPUT_DIRECTORY "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/$<CONFIG>/obj"
    )

    foreach(config_type IN LISTS CMAKE_CONFIGURATION_TYPES)
        string(TOUPPER "${config_type}" config_upper)
        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${runtime_root}/${config_type}"
            LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${runtime_root}/${config_type}"
            ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${SPARKLE_DEV_LIBRARY_ROOT}/${symbol_owner}/${config_type}"
            PDB_OUTPUT_DIRECTORY_${config_upper} "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/${config_type}"
            COMPILE_PDB_OUTPUT_DIRECTORY_${config_upper} "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/${config_type}/obj"
        )
    endforeach()

    if(NOT CMAKE_CONFIGURATION_TYPES AND CMAKE_BUILD_TYPE)
        string(TOUPPER "${CMAKE_BUILD_TYPE}" config_upper)
        set_target_properties(${target_name} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${runtime_root}/${CMAKE_BUILD_TYPE}"
            LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${runtime_root}/${CMAKE_BUILD_TYPE}"
            ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${SPARKLE_DEV_LIBRARY_ROOT}/${symbol_owner}/${CMAKE_BUILD_TYPE}"
            PDB_OUTPUT_DIRECTORY_${config_upper} "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/${CMAKE_BUILD_TYPE}"
            COMPILE_PDB_OUTPUT_DIRECTORY_${config_upper} "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/${CMAKE_BUILD_TYPE}/obj"
        )
    endif()
endfunction()

function(sparkle_configure_launcher_artifacts target_name)
    sparkle_set_product_artifact_directories(${target_name} "${SPARKLE_DEV_LAUNCHER_ROOT}" "launcher")
endfunction()

function(sparkle_configure_development_tool_artifacts target_name)
    sparkle_set_product_artifact_directories(${target_name} "${SPARKLE_DEV_TOOLS_ROOT}/${target_name}" "tools/${target_name}")
endfunction()

function(sparkle_configure_project_artifacts target_name project_name product_role)
    sparkle_set_product_artifact_directories(
        ${target_name}
        "${SPARKLE_DEV_PROJECTS_ROOT}/${project_name}/${product_role}"
        "projects/${project_name}/${product_role}")
endfunction()

function(sparkle_configure_runtime_support_artifacts target_name)
    sparkle_set_product_artifact_directories(
        ${target_name}
        "${SPARKLE_DEV_RUNTIME_SUPPORT_ROOT}/${target_name}"
        "runtime-support/${target_name}")
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
    "Sparkle roots: build=${SPARKLE_BUILD_ROOT}; artifacts=${SPARKLE_ACTIVE_ARTIFACT_ROOT}; "
    "dev=${SPARKLE_DEV_ARTIFACT_ROOT}")
message(STATUS "Sparkle artifact variant: ${SPARKLE_ARTIFACT_VARIANT}")
