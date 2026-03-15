# SparkleRenderer Module
# High-level rendering: Camera, Materials, Render passes, Texture management
# Dependencies: SparkleCore, SparklePlatform, SparkleRHI

file(GLOB_RECURSE SPARKLE_RENDERER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_RENDERER_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_RENDERER_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleRenderer SHARED
        ${SPARKLE_RENDERER_PUBLIC_HEADERS}
        ${SPARKLE_RENDERER_PRIVATE_SOURCES}
        ${SPARKLE_RENDERER_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleRenderer PRIVATE SPARKLE_RENDERER_EXPORTS)
else()
    add_library(SparkleRenderer STATIC
        ${SPARKLE_RENDERER_PUBLIC_HEADERS}
        ${SPARKLE_RENDERER_PRIVATE_SOURCES}
        ${SPARKLE_RENDERER_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleRenderer PUBLIC SPARKLE_STATIC)
endif()

target_include_directories(SparkleRenderer
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        # Engine root for module-prefixed includes
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        # Core module subdirectories for direct includes
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
        # RHI module subdirectories for direct includes
        ${CMAKE_CURRENT_SOURCE_DIR}/../RHI/Public/D3D12
        ${CMAKE_CURRENT_SOURCE_DIR}/../RHI/Public/D3D12/Descriptors
        ${CMAKE_CURRENT_SOURCE_DIR}/../RHI/Public/D3D12/Pipeline
        ${CMAKE_CURRENT_SOURCE_DIR}/../RHI/Public/D3D12/Resources
        ${CMAKE_CURRENT_SOURCE_DIR}/../RHI/Public/D3D12/Shaders
        # GameFramework module for Scene/Camera includes
        ${CMAKE_CURRENT_SOURCE_DIR}/../GameFramework/Public
        # UI module for UI.h include
        ${CMAKE_CURRENT_SOURCE_DIR}/../UI/Public
        # Third-party (d3dx12.h)
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

target_compile_features(SparkleRenderer PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
target_precompile_headers(SparkleRenderer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# Renderer depends on Core, Platform, RHI
# Note: Also needs GameFramework for Scene/Mesh/Assets and UI for rendering UI
# This creates bidirectional coupling that should be refactored later
target_link_libraries(SparkleRenderer 
    PUBLIC 
        SparkleCore
        SparklePlatform
        SparkleRHI
    SparkleUI
)

set_target_properties(SparkleRenderer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_RENDERER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_RENDERER_PRIVATE_SOURCES} ${SPARKLE_RENDERER_PRIVATE_HEADERS})
