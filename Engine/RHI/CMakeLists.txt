# SparkleRHI Module
# Graphics API abstraction: D3D12 backend
# Dependencies: SparkleCore, SparklePlatform
# Shared shader-parameter layout types live here so RHI does not reach upward into Renderer/GameFramework.

file(GLOB_RECURSE SPARKLE_RHI_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_RHI_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_RHI_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

set(SPARKLE_RHI_THIRD_PARTY_SOURCES
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleRHI SHARED
        ${SPARKLE_RHI_PUBLIC_HEADERS}
        ${SPARKLE_RHI_PRIVATE_SOURCES}
        ${SPARKLE_RHI_PRIVATE_HEADERS}
        ${SPARKLE_RHI_THIRD_PARTY_SOURCES}
    )
    target_compile_definitions(SparkleRHI PRIVATE SPARKLE_RHI_EXPORTS)
else()
    add_library(SparkleRHI STATIC
        ${SPARKLE_RHI_PUBLIC_HEADERS}
        ${SPARKLE_RHI_PRIVATE_SOURCES}
        ${SPARKLE_RHI_PRIVATE_HEADERS}
        ${SPARKLE_RHI_THIRD_PARTY_SOURCES}
    )
    target_compile_definitions(SparkleRHI PUBLIC SPARKLE_STATIC)
endif()

target_compile_definitions(SparkleRHI
	PRIVATE
    "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>,$<CONFIG:DevelopmentEditor>,$<CONFIG:DevelopmentGame>>:ENGINE_GPU_VALIDATION=1>"
    "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>,$<CONFIG:DevelopmentEditor>,$<CONFIG:DevelopmentGame>>:ENGINE_REPORT_LIVE_OBJECTS=1>"
    "$<$<OR:$<CONFIG:ShippingEditor>,$<CONFIG:ShippingGame>>:ENGINE_GPU_VALIDATION=0>"
    "$<$<OR:$<CONFIG:ShippingEditor>,$<CONFIG:ShippingGame>>:ENGINE_REPORT_LIVE_OBJECTS=0>"
)

target_include_directories(SparkleRHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        # Engine root for module-prefixed includes
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Time
)

target_compile_features(SparkleRHI PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
target_precompile_headers(SparkleRHI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# RHI publicly exposes Core-backed utility types only. Platform remains an
# implementation detail behind forward-declared window/timer types.
target_link_libraries(SparkleRHI 
    PUBLIC
        SparkleCore
    PRIVATE
        SparklePlatform
        # D3D12 system libraries
        imgui
        d3d12
        dxgi
        d3dcompiler
        dxguid
)

# Add /FS for parallel builds (PDB access)
if(MSVC)
    target_compile_options(SparkleRHI PRIVATE /FS)
endif()

set_target_properties(SparkleRHI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_RHI_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_RHI_PRIVATE_SOURCES} ${SPARKLE_RHI_PRIVATE_HEADERS})
source_group("ThirdParty\\ImGui" FILES ${SPARKLE_RHI_THIRD_PARTY_SOURCES})
