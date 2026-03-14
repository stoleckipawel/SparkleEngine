# SparkleRHI Module
# Graphics API abstraction: D3D12 backend
# Dependencies: SparkleCore, SparklePlatform

file(GLOB_RECURSE SPARKLE_RHI_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_RHI_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_RHI_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleRHI SHARED
        ${SPARKLE_RHI_PUBLIC_HEADERS}
        ${SPARKLE_RHI_PRIVATE_SOURCES}
        ${SPARKLE_RHI_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleRHI PRIVATE SPARKLE_RHI_EXPORTS)
else()
    add_library(SparkleRHI STATIC
        ${SPARKLE_RHI_PUBLIC_HEADERS}
        ${SPARKLE_RHI_PRIVATE_SOURCES}
        ${SPARKLE_RHI_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleRHI PUBLIC SPARKLE_STATIC)
endif()

target_include_directories(SparkleRHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        # Engine root for module-prefixed includes
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        # Core module subdirectories for direct includes
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Time
        # Renderer module for DepthConvention.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../Renderer/Public
        # UI module for UI.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../UI/Public
        # GameFramework module for AssetTypes.h and PathRoot.h
        ${CMAKE_CURRENT_SOURCE_DIR}/../GameFramework/Public
        # D3D12 subdirectories for internal includes
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/Descriptors
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/Pipeline
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/Resources
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/Samplers
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/Shaders
        # Public D3D12 headers
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/D3D12
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/D3D12/Descriptors
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/D3D12/Pipeline
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/D3D12/Resources
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/D3D12/Samplers
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/D3D12/Shaders
        # Third-party (d3dx12.h)
        ${CMAKE_CURRENT_SOURCE_DIR}/../third_party
)

target_compile_features(SparkleRHI PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
target_precompile_headers(SparkleRHI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# RHI depends on Core and Platform
target_link_libraries(SparkleRHI 
    PUBLIC 
        SparkleCore
        SparklePlatform
    PRIVATE
        # D3D12 system libraries
        d3d12
        dxgi
        d3dcompiler
        dxguid
        dxcompiler
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
