# SparkleRHI Module
# Graphics API abstraction: common RHI contracts plus backend-private implementations.
# Dependencies: SparkleCore, SparklePlatform
# Shared shader-parameter layout types live here so RHI does not reach upward into Renderer/GameFramework.

include(FetchContent)

option(SPARKLE_RHI_WITH_D3D12 "Enable the D3D12 RHI backend" ON)
option(SPARKLE_RHI_WITH_VULKAN "Enable the placeholder Vulkan RHI backend target" ON)
set(SPARKLE_RHI_DEFAULT_BACKEND "D3D12" CACHE STRING "Default RHI backend: D3D12 or Vulkan")
set_property(CACHE SPARKLE_RHI_DEFAULT_BACKEND PROPERTY STRINGS D3D12 Vulkan)
string(TOUPPER "${SPARKLE_RHI_DEFAULT_BACKEND}" SPARKLE_RHI_DEFAULT_BACKEND_UPPER)

if(NOT SPARKLE_RHI_DEFAULT_BACKEND_UPPER STREQUAL "D3D12" AND NOT SPARKLE_RHI_DEFAULT_BACKEND_UPPER STREQUAL "VULKAN")
    message(FATAL_ERROR "SPARKLE_RHI_DEFAULT_BACKEND must be D3D12 or Vulkan.")
endif()

if(SPARKLE_RHI_DEFAULT_BACKEND_UPPER STREQUAL "D3D12" AND NOT SPARKLE_RHI_WITH_D3D12)
    message(FATAL_ERROR "SPARKLE_RHI_DEFAULT_BACKEND is D3D12, but SPARKLE_RHI_WITH_D3D12 is OFF.")
endif()

if(SPARKLE_RHI_DEFAULT_BACKEND_UPPER STREQUAL "VULKAN" AND NOT SPARKLE_RHI_WITH_VULKAN)
    message(FATAL_ERROR "SPARKLE_RHI_DEFAULT_BACKEND is Vulkan, but SPARKLE_RHI_WITH_VULKAN is OFF.")
endif()

if(NOT DEFINED SPARKLE_GIT_PROGRESS)
    set(SPARKLE_GIT_PROGRESS FALSE)
endif()

file(GLOB_RECURSE SPARKLE_RHI_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_RHI_COMMON_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_RHI_COMMON_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

list(FILTER SPARKLE_RHI_COMMON_PRIVATE_SOURCES EXCLUDE REGEX "/Private/D3D12/")
list(FILTER SPARKLE_RHI_COMMON_PRIVATE_HEADERS EXCLUDE REGEX "/Private/D3D12/")

add_library(SparkleRHICommon OBJECT
    ${SPARKLE_RHI_PUBLIC_HEADERS}
    ${SPARKLE_RHI_COMMON_PRIVATE_SOURCES}
    ${SPARKLE_RHI_COMMON_PRIVATE_HEADERS}
)

function(sparkle_configure_rhi_implementation_target target_name)
    target_include_directories(${target_name}
        PUBLIC
            ${CMAKE_CURRENT_SOURCE_DIR}/Public
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/Private
            # Engine root for module-prefixed includes
            ${CMAKE_CURRENT_SOURCE_DIR}/..
            ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
            ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Time
    )

    target_compile_features(${target_name} PUBLIC cxx_std_20)

    target_compile_definitions(${target_name}
        PRIVATE
            SPARKLE_RHI_WITH_D3D12=$<BOOL:${SPARKLE_RHI_WITH_D3D12}>
            SPARKLE_RHI_WITH_VULKAN=$<BOOL:${SPARKLE_RHI_WITH_VULKAN}>
            SPARKLE_RHI_DEFAULT_BACKEND_${SPARKLE_RHI_DEFAULT_BACKEND_UPPER}=1
            "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>,$<CONFIG:DevelopmentEditor>,$<CONFIG:DevelopmentGame>>:ENGINE_GPU_VALIDATION=1>"
            "$<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>,$<CONFIG:DevelopmentEditor>,$<CONFIG:DevelopmentGame>>:ENGINE_REPORT_LIVE_OBJECTS=1>"
            "$<$<OR:$<CONFIG:ShippingEditor>,$<CONFIG:ShippingGame>>:ENGINE_GPU_VALIDATION=0>"
            "$<$<OR:$<CONFIG:ShippingEditor>,$<CONFIG:ShippingGame>>:ENGINE_REPORT_LIVE_OBJECTS=0>"
    )

    if(SPARKLE_BUILD_SHARED)
        target_compile_definitions(${target_name} PRIVATE SPARKLE_RHI_EXPORTS)
    else()
        target_compile_definitions(${target_name} PRIVATE SPARKLE_STATIC)
    endif()

    if(MSVC)
        target_compile_options(${target_name} PRIVATE /FS)
    endif()
endfunction()

sparkle_configure_rhi_implementation_target(SparkleRHICommon)

target_link_libraries(SparkleRHICommon
    PUBLIC
        SparkleCore
    PRIVATE
        SparklePlatform
)

sparkle_target_precompile_headers(SparkleRHICommon
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

set(SPARKLE_RHI_BACKEND_TARGETS "")

if(SPARKLE_RHI_WITH_D3D12)
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

    file(GLOB_RECURSE SPARKLE_RHI_D3D12_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/*.cpp
    )

    file(GLOB_RECURSE SPARKLE_RHI_D3D12_HEADERS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/*.h
    )

    set(SPARKLE_RHI_D3D12_THIRD_PARTY_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    )

    add_library(SparkleRHI_D3D12 STATIC
        ${SPARKLE_RHI_D3D12_SOURCES}
        ${SPARKLE_RHI_D3D12_HEADERS}
        ${SPARKLE_RHI_D3D12_THIRD_PARTY_SOURCES}
    )

    sparkle_configure_rhi_implementation_target(SparkleRHI_D3D12)

    target_link_libraries(SparkleRHI_D3D12
        PUBLIC
            SparkleCore
            imgui
            SparkleD3D12MA
            d3d12
            dxgi
            d3dcompiler
            dxguid
        PRIVATE
            SparklePlatform
    )

    sparkle_target_precompile_headers(SparkleRHI_D3D12
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/D3D12PCH.h
    )

    list(APPEND SPARKLE_RHI_BACKEND_TARGETS SparkleRHI_D3D12)
endif()

if(SPARKLE_RHI_WITH_VULKAN)
    add_library(SparkleRHI_Vulkan INTERFACE)
    target_compile_features(SparkleRHI_Vulkan INTERFACE cxx_std_20)
endif()

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleRHI SHARED
        ${SPARKLE_RHI_PUBLIC_HEADERS}
        $<TARGET_OBJECTS:SparkleRHICommon>
    )
    target_compile_definitions(SparkleRHI PRIVATE SPARKLE_RHI_EXPORTS)
else()
    add_library(SparkleRHI STATIC
        ${SPARKLE_RHI_PUBLIC_HEADERS}
        $<TARGET_OBJECTS:SparkleRHICommon>
    )
    target_compile_definitions(SparkleRHI PUBLIC SPARKLE_STATIC)
endif()

target_include_directories(SparkleRHI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

target_compile_features(SparkleRHI PUBLIC cxx_std_20)

# RHI publicly exposes Core-backed utility types only. Platform remains an
# implementation detail behind forward-declared window/timer types.
target_link_libraries(SparkleRHI
    PUBLIC
        SparkleCore
    PRIVATE
        SparklePlatform
        ${SPARKLE_RHI_BACKEND_TARGETS}
)

set_target_properties(SparkleRHI PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_RHI_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private\\Common" FILES ${SPARKLE_RHI_COMMON_PRIVATE_SOURCES} ${SPARKLE_RHI_COMMON_PRIVATE_HEADERS})

if(SPARKLE_RHI_WITH_D3D12)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12 PREFIX "Private\\D3D12" FILES ${SPARKLE_RHI_D3D12_SOURCES} ${SPARKLE_RHI_D3D12_HEADERS})
    source_group("ThirdParty\\ImGui\\D3D12" FILES ${SPARKLE_RHI_D3D12_THIRD_PARTY_SOURCES})
    source_group("ThirdParty\\D3D12MA" FILES ${SPARKLE_D3D12MA_SOURCES})
endif()