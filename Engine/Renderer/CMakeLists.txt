# SparkleRenderer Module
# High-level rendering: Camera, Materials, Render passes, Texture management
# Dependencies: SparkleCore, SparklePlatform, SparkleRHI, SparkleGameFramework runtime scene types
# Renderer consumes runtime scene/material data only. Source import, conversion,
# and authoring-side dependencies must stay out of this module.
# Renderer-owned shader payload layouts live under Private/ShaderData; RHI owns only binding/resource contracts.

file(GLOB_RECURSE SPARKLE_RENDERER_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_RENDERER_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_RENDERER_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

file(GLOB_RECURSE SPARKLE_RENDERER_UPSCALING_NVIDIA_DLSS_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Upscaling/NvidiaDlss/*.cpp
)

file(GLOB_RECURSE SPARKLE_RENDERER_UPSCALING_NVIDIA_DLSS_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Upscaling/NvidiaDlss/*.h
)

file(GLOB_RECURSE SPARKLE_RENDERER_NVIDIA_RAY_RECONSTRUCTION_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/RayReconstruction/NvidiaDlssRayReconstruction/*.cpp
)

file(GLOB_RECURSE SPARKLE_RENDERER_NVIDIA_RAY_RECONSTRUCTION_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/RayReconstruction/NvidiaDlssRayReconstruction/*.h
)

file(GLOB_RECURSE SPARKLE_RENDERER_STREAMLINE_SHARED_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Streamline/*.cpp
)

file(GLOB_RECURSE SPARKLE_RENDERER_STREAMLINE_SHARED_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Streamline/*.h
)

set(SPARKLE_RENDERER_NVIDIA_STREAMLINE_PROVIDER_SOURCES
    ${SPARKLE_RENDERER_UPSCALING_NVIDIA_DLSS_SOURCES}
    ${SPARKLE_RENDERER_NVIDIA_RAY_RECONSTRUCTION_SOURCES}
    ${SPARKLE_RENDERER_STREAMLINE_SHARED_SOURCES}
)

set(SPARKLE_RENDERER_NVIDIA_STREAMLINE_PROVIDER_HEADERS
    ${SPARKLE_RENDERER_UPSCALING_NVIDIA_DLSS_HEADERS}
    ${SPARKLE_RENDERER_NVIDIA_RAY_RECONSTRUCTION_HEADERS}
    ${SPARKLE_RENDERER_STREAMLINE_SHARED_HEADERS}
)

list(FILTER SPARKLE_RENDERER_PRIVATE_SOURCES EXCLUDE REGEX "/Private/Upscaling/NvidiaDlss/")
list(FILTER SPARKLE_RENDERER_PRIVATE_SOURCES EXCLUDE REGEX "/Private/RayReconstruction/NvidiaDlssRayReconstruction/")
list(FILTER SPARKLE_RENDERER_PRIVATE_SOURCES EXCLUDE REGEX "/Private/Streamline/")
list(FILTER SPARKLE_RENDERER_PRIVATE_HEADERS EXCLUDE REGEX "/Private/Upscaling/NvidiaDlss/")
list(FILTER SPARKLE_RENDERER_PRIVATE_HEADERS EXCLUDE REGEX "/Private/RayReconstruction/NvidiaDlssRayReconstruction/")
list(FILTER SPARKLE_RENDERER_PRIVATE_HEADERS EXCLUDE REGEX "/Private/Streamline/")

file(GLOB_RECURSE SPARKLE_RENDERER_SHADER_REGISTRATION_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ShaderRegistrations/*.cpp
)

file(GLOB_RECURSE SPARKLE_RENDERER_SHADER_REGISTRATION_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/ShaderRegistrations/*.h
)

add_library(SparkleRendererShaderRegistrations OBJECT
    ${SPARKLE_RENDERER_SHADER_REGISTRATION_SOURCES}
    ${SPARKLE_RENDERER_SHADER_REGISTRATION_HEADERS}
)

target_include_directories(SparkleRendererShaderRegistrations
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ShaderRegistrations
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
)

target_compile_features(SparkleRendererShaderRegistrations PUBLIC cxx_std_20)

target_link_libraries(SparkleRendererShaderRegistrations
    PUBLIC
        SparkleRHI
    PRIVATE
        SparkleCore
)

sparkle_target_precompile_headers(SparkleRendererShaderRegistrations
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

add_library(SparkleRendererShaderRegistrationObjects INTERFACE)

target_sources(SparkleRendererShaderRegistrationObjects
    INTERFACE
        $<TARGET_OBJECTS:SparkleRendererShaderRegistrations>
)

target_link_libraries(SparkleRendererShaderRegistrationObjects
    INTERFACE
        SparkleRendererShaderRegistrations
)

add_library(SparkleRendererNvidiaStreamlineProviders STATIC
    ${SPARKLE_RENDERER_NVIDIA_STREAMLINE_PROVIDER_SOURCES}
    ${SPARKLE_RENDERER_NVIDIA_STREAMLINE_PROVIDER_HEADERS}
)

target_include_directories(SparkleRendererNvidiaStreamlineProviders
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
)

target_compile_features(SparkleRendererNvidiaStreamlineProviders PUBLIC cxx_std_20)

target_link_libraries(SparkleRendererNvidiaStreamlineProviders
    PUBLIC
        SparkleCore
        SparkleRHI
    PRIVATE
        SparklePlatform
)

if(SPARKLE_ENABLE_NVIDIA_STREAMLINE AND TARGET NVIDIA::Streamline)
    target_link_libraries(SparkleRendererNvidiaStreamlineProviders PRIVATE NVIDIA::Streamline)
    target_compile_definitions(SparkleRendererNvidiaStreamlineProviders PRIVATE SPARKLE_WITH_NVIDIA_STREAMLINE=1)
else()
    target_compile_definitions(SparkleRendererNvidiaStreamlineProviders PRIVATE SPARKLE_WITH_NVIDIA_STREAMLINE=0)
endif()

sparkle_target_precompile_headers(SparkleRendererNvidiaStreamlineProviders
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
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
        SparkleTasks
        SparkleRendererNvidiaStreamlineProviders
        imgui
)

if(SPARKLE_BUILD_SHARED)
    target_link_libraries(SparkleRenderer PRIVATE SparkleRendererShaderRegistrations)
else()
    target_link_libraries(SparkleRenderer PUBLIC SparkleRendererShaderRegistrationObjects)
endif()

sparkle_configure_runtime_support_artifacts(SparkleRenderer)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_RENDERER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_RENDERER_PRIVATE_SOURCES} ${SPARKLE_RENDERER_PRIVATE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private/Upscaling/NvidiaDlss PREFIX "Private\\Upscaling\\NvidiaDlss" FILES ${SPARKLE_RENDERER_UPSCALING_NVIDIA_DLSS_SOURCES} ${SPARKLE_RENDERER_UPSCALING_NVIDIA_DLSS_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private/RayReconstruction/NvidiaDlssRayReconstruction PREFIX "Private\\RayReconstruction\\NvidiaDlssRayReconstruction" FILES ${SPARKLE_RENDERER_NVIDIA_RAY_RECONSTRUCTION_SOURCES} ${SPARKLE_RENDERER_NVIDIA_RAY_RECONSTRUCTION_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private/Streamline PREFIX "Private\\Streamline" FILES ${SPARKLE_RENDERER_STREAMLINE_SHARED_SOURCES} ${SPARKLE_RENDERER_STREAMLINE_SHARED_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ShaderRegistrations PREFIX "ShaderRegistrations" FILES ${SPARKLE_RENDERER_SHADER_REGISTRATION_SOURCES} ${SPARKLE_RENDERER_SHADER_REGISTRATION_HEADERS})
