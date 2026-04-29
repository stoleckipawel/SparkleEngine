# SparkleEditor Module
# Editor UI system: panels, tools, viewport chrome, and editor-facing controls.
# Dependencies: SparkleCore, SparklePlatform, SparkleRHI, SparkleGameFramework
# Editor stays above runtime but should not depend on the broad tool-side
# authoring module by default. Source import and cooking remain under AssetConverter.

file(GLOB_RECURSE SPARKLE_EDITOR_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_EDITOR_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_EDITOR_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleEditor SHARED
        ${SPARKLE_EDITOR_PUBLIC_HEADERS}
        ${SPARKLE_EDITOR_PRIVATE_SOURCES}
        ${SPARKLE_EDITOR_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleEditor PRIVATE SPARKLE_EDITOR_EXPORTS)
else()
    add_library(SparkleEditor STATIC
        ${SPARKLE_EDITOR_PUBLIC_HEADERS}
        ${SPARKLE_EDITOR_PRIVATE_SOURCES}
        ${SPARKLE_EDITOR_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleEditor PUBLIC SPARKLE_STATIC)
endif()

target_include_directories(SparkleEditor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/ImGui
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Panels
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Sections
        # Engine root for module-prefixed includes
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        # Core module subdirectories for direct includes
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Time
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
)

target_compile_features(SparkleEditor PUBLIC cxx_std_20)

if(NOT DEFINED SPARKLE_FONT_AWESOME_SOLID_TTF OR NOT EXISTS "${SPARKLE_FONT_AWESOME_SOLID_TTF}")
    message(FATAL_ERROR "SparkleEditor requires Font Awesome Free Solid. FetchDependencies.cmake should download fa-solid-900.ttf before the editor target is configured.")
endif()
file(TO_CMAKE_PATH "${SPARKLE_FONT_AWESOME_SOLID_TTF}" SPARKLE_FONT_AWESOME_SOLID_TTF_NORMALIZED)
target_compile_definitions(SparkleEditor PRIVATE
    SPARKLE_FONT_AWESOME_SOLID_TTF="${SPARKLE_FONT_AWESOME_SOLID_TTF_NORMALIZED}"
)

# Precompiled header (per-module PCH)
target_precompile_headers(SparkleEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# Editor publicly exposes Core utilities plus the renderer and RHI contracts used
# by its UI surface. Runtime scene and window types remain forward-declared.
target_link_libraries(SparkleEditor 
    PUBLIC
        SparkleCore
        SparkleRHI
        SparkleRenderer
    PRIVATE
        SparkleGameFramework
        SparklePlatform
        imgui
)

set_target_properties(SparkleEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_EDITOR_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_EDITOR_PRIVATE_SOURCES} ${SPARKLE_EDITOR_PRIVATE_HEADERS})
