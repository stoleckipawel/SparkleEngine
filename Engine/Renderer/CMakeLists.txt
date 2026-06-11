# SparkleRenderer Module
# High-level rendering: Camera, Materials, Render passes, Texture management
# Dependencies: SparkleCore, SparklePlatform, SparkleRHI, SparkleGameFramework runtime scene types
# Renderer consumes runtime scene/material data only. Source import, conversion,
# and authoring-side dependencies must stay out of this module.

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
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
)

target_compile_features(SparkleRenderer PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
sparkle_target_precompile_headers(SparkleRenderer
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# Renderer publicly exposes Core event handles and the RHI-facing render seam.
# Platform and runtime scene modules remain implementation-only.
target_link_libraries(SparkleRenderer 
    PUBLIC
        SparkleCore
        SparkleRHI
    PRIVATE
        SparklePlatform
        SparkleGameFramework
)

if(SPARKLE_ENABLE_NVIDIA_STREAMLINE AND TARGET NVIDIA::Streamline)
    target_link_libraries(SparkleRenderer PRIVATE NVIDIA::Streamline)
    target_compile_definitions(SparkleRenderer PRIVATE SPARKLE_WITH_NVIDIA_STREAMLINE=1)
else()
    target_compile_definitions(SparkleRenderer PRIVATE SPARKLE_WITH_NVIDIA_STREAMLINE=0)
endif()

sparkle_configure_runtime_support_artifacts(SparkleRenderer)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_RENDERER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_RENDERER_PRIVATE_SOURCES} ${SPARKLE_RENDERER_PRIVATE_HEADERS})
