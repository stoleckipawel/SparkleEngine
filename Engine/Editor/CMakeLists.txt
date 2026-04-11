# SparkleEditor Module
# Editor UI system: overlays, panels, tools, and editor-facing controls.
# Dependencies: SparkleCore, SparklePlatform, SparkleRHI

file(GLOB_RECURSE SPARKLE_EDITOR_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_EDITOR_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_EDITOR_PRIVATE_HEADERS
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
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Panels
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Sections
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

# Precompiled header (per-module PCH)
target_precompile_headers(SparkleEditor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# Editor depends on GameFramework plus editor-only import/UI dependencies.
target_link_libraries(SparkleEditor 
    PUBLIC 
        SparkleAssetAuthoring
        SparkleGameFramework
        SparkleCore
        SparklePlatform
        SparkleRHI
    PRIVATE
        imgui
)

set_target_properties(SparkleEditor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_EDITOR_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_EDITOR_PRIVATE_SOURCES} ${SPARKLE_EDITOR_PRIVATE_HEADERS})
