# SparkleRHI Module
# Graphics API abstraction: D3D12 backend
# Dependencies: SparkleCore, SparklePlatform
# Shared shader-parameter layout types live here so RHI does not reach upward into Renderer/GameFramework.

include(FetchContent)

if(NOT DEFINED SPARKLE_GIT_PROGRESS)
    set(SPARKLE_GIT_PROGRESS FALSE)
endif()

FetchContent_Declare(sparkle_d3d12ma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/D3D12MemoryAllocator.git
    GIT_TAG        0fa62ed3a0a69b73230a8ec1faa752d4061c8dc8 # v3.1.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   ${SPARKLE_GIT_PROGRESS}
)

FetchContent_GetProperties(sparkle_d3d12ma)
if(NOT sparkle_d3d12ma_POPULATED)
    FetchContent_Populate(sparkle_d3d12ma)
endif()

set(SPARKLE_D3D12MA_SOURCES
    ${sparkle_d3d12ma_SOURCE_DIR}/src/D3D12MemAlloc.cpp
    ${sparkle_d3d12ma_SOURCE_DIR}/include/D3D12MemAlloc.h
)

add_library(SparkleD3D12MA STATIC ${SPARKLE_D3D12MA_SOURCES})

target_include_directories(SparkleD3D12MA
    PUBLIC
        ${sparkle_d3d12ma_SOURCE_DIR}/include
)

target_compile_features(SparkleD3D12MA PUBLIC cxx_std_14)

target_link_libraries(SparkleD3D12MA
    PUBLIC
        d3d12
        dxgi
        dxguid
)

if(MSVC)
    target_compile_options(SparkleD3D12MA PRIVATE /W0 /FS)
endif()

set_target_properties(SparkleD3D12MA PROPERTIES FOLDER "ThirdParty")

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
sparkle_target_precompile_headers(SparkleRHI
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
        SparkleD3D12MA
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
source_group("ThirdParty\\D3D12MA" FILES ${SPARKLE_D3D12MA_SOURCES})
