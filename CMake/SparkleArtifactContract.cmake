# Sparkle artifact and package naming contract.
#
# Phase 1 introduces shared names only. These variables intentionally do not
# move build outputs yet; later phases can route targets through these roots.

set(SPARKLE_REPOSITORY_ROOT "${CMAKE_SOURCE_DIR}" CACHE PATH "Sparkle source repository root.")
set(SPARKLE_BUILD_ROOT "${CMAKE_BINARY_DIR}" CACHE PATH "Active generated CMake build tree root.")
set(SPARKLE_ARTIFACT_ROOT "${CMAKE_SOURCE_DIR}/artifacts" CACHE PATH "Generated development artifact root.")
set(SPARKLE_DIST_ROOT "${CMAKE_SOURCE_DIR}/dist" CACHE PATH "Assembled distributable package root.")
set(SPARKLE_DEV_ARTIFACT_ROOT "${SPARKLE_ARTIFACT_ROOT}/dev" CACHE PATH "Product-aware development artifact root.")
set(SPARKLE_DEV_LAUNCHER_ROOT "${SPARKLE_DEV_ARTIFACT_ROOT}/launcher" CACHE PATH "Sparkle Launcher development artifact root.")
set(SPARKLE_DEV_TOOLS_ROOT "${SPARKLE_DEV_ARTIFACT_ROOT}/tools" CACHE PATH "Development tool artifact root.")
set(SPARKLE_DEV_PROJECTS_ROOT "${SPARKLE_DEV_ARTIFACT_ROOT}/projects" CACHE PATH "Project editor/runtime artifact root.")
set(SPARKLE_DIAGNOSTICS_ROOT "${SPARKLE_ARTIFACT_ROOT}/diagnostics" CACHE PATH "Generated diagnostics artifact root.")
set(SPARKLE_SYMBOL_ROOT "${SPARKLE_ARTIFACT_ROOT}/symbols" CACHE PATH "Generated symbol artifact root.")

set(SPARKLE_RELEASE_CHANNEL "dev" CACHE STRING "Release channel: dev, preview, rc, or release.")
set_property(CACHE SPARKLE_RELEASE_CHANNEL PROPERTY STRINGS dev preview rc release)
set(SPARKLE_PACKAGE_VERSION "0.0.0-dev" CACHE STRING "Package version used by future artifact manifests.")
set(SPARKLE_PACKAGE_PLATFORM "windows-x64" CACHE STRING "Package platform identifier.")

set(SPARKLE_DEPENDENCY_CATEGORY_HOST_PREREQUISITE "host-prerequisite")
set(SPARKLE_DEPENDENCY_CATEGORY_SOURCE_DEPENDENCY_GROUP "source-dependency-group")
set(SPARKLE_DEPENDENCY_CATEGORY_RUNTIME_REDISTRIBUTABLE "runtime-redistributable")
set(SPARKLE_DEPENDENCY_CATEGORY_BUILD_OUTPUT "build-output")
set(SPARKLE_DEPENDENCY_CATEGORY_COOKED_OUTPUT "cooked-output")
set(SPARKLE_DEPENDENCY_CATEGORY_PROJECT_SELECTION "project-selection")

set(SPARKLE_VISIBILITY_PUBLIC "public")
set(SPARKLE_VISIBILITY_INTERNAL "internal")
set(SPARKLE_VISIBILITY_PRIVATE "private")

set(SPARKLE_BINARY_TYPE_APP "app")
set(SPARKLE_BINARY_TYPE_DEVELOPER_TOOL "developer-tool")
set(SPARKLE_BINARY_TYPE_RUNTIME_DLL "runtime-dll")
set(SPARKLE_BINARY_TYPE_PLUGIN_DLL "plugin-dll")
set(SPARKLE_BINARY_TYPE_IMPORT_LIBRARY "import-library")
set(SPARKLE_BINARY_TYPE_STATIC_LIBRARY "static-library")
set(SPARKLE_BINARY_TYPE_SYMBOL_FILE "symbol-file")
set(SPARKLE_BINARY_TYPE_GENERATED_ASSET "generated-asset")

set(SPARKLE_PACKAGE_ID_LAUNCHER "sparkle-launcher")
set(SPARKLE_PACKAGE_ID_RUNTIME "sparkle-runtime")
set(SPARKLE_PACKAGE_ID_EDITOR "sparkle-editor")
set(SPARKLE_PACKAGE_ID_DEV_TOOLS "sparkle-dev-tools")
set(SPARKLE_PACKAGE_ID_SYMBOLS "sparkle-symbols")
set(SPARKLE_PACKAGE_ID_DEPENDENCIES "sparkle-dependencies")

function(sparkle_set_product_artifact_directories target_name runtime_root symbol_owner)
    if(NOT TARGET ${target_name})
        message(FATAL_ERROR "Unknown Sparkle target '${target_name}'")
    endif()

    set_target_properties(${target_name} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${runtime_root}/$<CONFIG>"
        LIBRARY_OUTPUT_DIRECTORY "${runtime_root}/$<CONFIG>"
        ARCHIVE_OUTPUT_DIRECTORY "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/$<CONFIG>/lib"
        PDB_OUTPUT_DIRECTORY "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/$<CONFIG>"
        COMPILE_PDB_OUTPUT_DIRECTORY "${SPARKLE_SYMBOL_ROOT}/${symbol_owner}/$<CONFIG>/obj"
    )
endfunction()

function(sparkle_configure_launcher_artifacts target_name)
    sparkle_set_product_artifact_directories(${target_name} "${SPARKLE_DEV_LAUNCHER_ROOT}" "launcher")
endfunction()

function(sparkle_configure_development_tool_artifacts target_name)
    sparkle_set_product_artifact_directories(${target_name} "${SPARKLE_DEV_TOOLS_ROOT}/${target_name}" "tools/${target_name}")
endfunction()

function(sparkle_configure_project_artifacts target_name project_name product_role)
    sparkle_set_product_artifact_directories(${target_name} "${SPARKLE_DEV_PROJECTS_ROOT}/${project_name}/${product_role}" "projects/${project_name}/${product_role}")
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

message(STATUS "Sparkle roots: build=${SPARKLE_BUILD_ROOT}; artifacts=${SPARKLE_ARTIFACT_ROOT}; dev=${SPARKLE_DEV_ARTIFACT_ROOT}; dist=${SPARKLE_DIST_ROOT}")
message(STATUS "Sparkle package identity: version=${SPARKLE_PACKAGE_VERSION}; channel=${SPARKLE_RELEASE_CHANNEL}; platform=${SPARKLE_PACKAGE_PLATFORM}")
