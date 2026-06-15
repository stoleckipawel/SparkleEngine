# SparkleRHI Module
# Graphics API abstraction: common RHI contracts plus backend-private implementations.
# Dependencies: SparkleCore, SparklePlatform
# Shared shader-parameter layout types live here so RHI does not reach upward into Renderer/GameFramework.

include(FetchContent)

option(SPARKLE_RHI_WITH_D3D12 "Enable the D3D12 RHI backend" ON)
option(SPARKLE_RHI_WITH_D3D12_NVAPI "Enable NVIDIA NVAPI integration inside the D3D12 RHI backend" ON)
option(SPARKLE_RHI_D3D12_NVAPI_FETCH_FROM_GITHUB "Fetch the official NVIDIA NVAPI SDK when D3D12 NVAPI is enabled and no local SDK paths are provided" ON)
set(SPARKLE_RHI_D3D12_NVAPI_GIT_TAG "9b181ea572f680327fe01a14a0f1f41c78034104" CACHE STRING "Official NVIDIA NVAPI repository revision used when fetching NVAPI")
set(SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR "" CACHE PATH "Directory containing NVIDIA NVAPI headers, used only by SparkleRHI_D3D12")
set(SPARKLE_RHI_D3D12_NVAPI_LIBRARY "" CACHE FILEPATH "NVIDIA NVAPI import library, used only by SparkleRHI_D3D12")
find_package(Vulkan QUIET)
if(Vulkan_FOUND)
    set(SPARKLE_RHI_WITH_VULKAN_DEFAULT ON)
else()
    set(SPARKLE_RHI_WITH_VULKAN_DEFAULT OFF)
endif()
option(SPARKLE_RHI_WITH_VULKAN "Enable the Vulkan RHI backend target" ${SPARKLE_RHI_WITH_VULKAN_DEFAULT})
option(SPARKLE_RHI_REQUIRE_VULKAN_SDK "Fail configure when the Vulkan RHI backend is enabled but the Vulkan SDK is missing" OFF)
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

if(SPARKLE_RHI_WITH_VULKAN AND NOT Vulkan_FOUND)
    if(SPARKLE_RHI_REQUIRE_VULKAN_SDK OR SPARKLE_RHI_DEFAULT_BACKEND_UPPER STREQUAL "VULKAN")
        message(FATAL_ERROR "SPARKLE_RHI_WITH_VULKAN is ON, but the Vulkan SDK was not found. Install the Vulkan SDK or set SPARKLE_RHI_WITH_VULKAN=OFF.")
    endif()

    message(WARNING "Vulkan SDK was not found. Disabling SparkleRHI_Vulkan for this configure. Set SPARKLE_RHI_WITH_VULKAN=OFF to silence this warning.")
    set(SPARKLE_RHI_WITH_VULKAN OFF CACHE BOOL "Enable the Vulkan RHI backend target" FORCE)
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
list(FILTER SPARKLE_RHI_COMMON_PRIVATE_SOURCES EXCLUDE REGEX "/Private/Vulkan/")
list(FILTER SPARKLE_RHI_COMMON_PRIVATE_HEADERS EXCLUDE REGEX "/Private/Vulkan/")

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
    if(SPARKLE_RHI_WITH_D3D12_NVAPI)
        if(SPARKLE_RHI_D3D12_NVAPI_FETCH_FROM_GITHUB AND
           (NOT SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR OR NOT SPARKLE_RHI_D3D12_NVAPI_LIBRARY))
            FetchContent_Declare(sparkle_nvapi
                GIT_REPOSITORY https://github.com/NVIDIA/nvapi.git
                GIT_TAG        ${SPARKLE_RHI_D3D12_NVAPI_GIT_TAG}
                GIT_SHALLOW    TRUE
                GIT_PROGRESS   ${SPARKLE_GIT_PROGRESS}
            )

            FetchContent_MakeAvailable(sparkle_nvapi)

            if(NOT SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR)
                set(SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR "${sparkle_nvapi_SOURCE_DIR}" CACHE PATH "Directory containing NVIDIA NVAPI headers, used only by SparkleRHI_D3D12" FORCE)
            endif()
            if(NOT SPARKLE_RHI_D3D12_NVAPI_LIBRARY)
                set(SPARKLE_RHI_D3D12_NVAPI_LIBRARY "${sparkle_nvapi_SOURCE_DIR}/amd64/nvapi64.lib" CACHE FILEPATH "NVIDIA NVAPI import library, used only by SparkleRHI_D3D12" FORCE)
            endif()
        endif()

        if(NOT SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR)
            message(FATAL_ERROR "SPARKLE_RHI_WITH_D3D12_NVAPI is ON, but SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR is empty.")
        endif()
        if(NOT SPARKLE_RHI_D3D12_NVAPI_LIBRARY)
            message(FATAL_ERROR "SPARKLE_RHI_WITH_D3D12_NVAPI is ON, but SPARKLE_RHI_D3D12_NVAPI_LIBRARY is empty.")
        endif()
        if(NOT EXISTS "${SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR}/nvapi.h")
            message(FATAL_ERROR "SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR does not contain nvapi.h: ${SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR}")
        endif()
        if(NOT EXISTS "${SPARKLE_RHI_D3D12_NVAPI_LIBRARY}")
            message(FATAL_ERROR "SPARKLE_RHI_D3D12_NVAPI_LIBRARY does not exist: ${SPARKLE_RHI_D3D12_NVAPI_LIBRARY}")
        endif()
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

    target_compile_definitions(SparkleRHI_D3D12
        PRIVATE
            SPARKLE_RHI_WITH_D3D12_NVAPI=$<BOOL:${SPARKLE_RHI_WITH_D3D12_NVAPI}>
    )

    if(SPARKLE_RHI_WITH_D3D12_NVAPI)
        target_include_directories(SparkleRHI_D3D12
            PRIVATE
                ${SPARKLE_RHI_D3D12_NVAPI_INCLUDE_DIR}
        )
        target_link_libraries(SparkleRHI_D3D12
            PRIVATE
                ${SPARKLE_RHI_D3D12_NVAPI_LIBRARY}
        )
    endif()

    sparkle_target_precompile_headers(SparkleRHI_D3D12
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12/D3D12PCH.h
    )

    list(APPEND SPARKLE_RHI_BACKEND_TARGETS SparkleRHI_D3D12)
endif()

if(SPARKLE_RHI_WITH_VULKAN)
    FetchContent_Declare(sparkle_vma
        GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
        GIT_TAG        v3.1.0
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   ${SPARKLE_GIT_PROGRESS}
    )

    FetchContent_GetProperties(sparkle_vma)
    if(NOT sparkle_vma_POPULATED)
        FetchContent_Populate(sparkle_vma)
    endif()

    add_library(SparkleVMA INTERFACE)
    target_include_directories(SparkleVMA
        INTERFACE
            ${sparkle_vma_SOURCE_DIR}/include
    )
    target_link_libraries(SparkleVMA
        INTERFACE
            Vulkan::Vulkan
    )
    set_target_properties(SparkleVMA PROPERTIES FOLDER "ThirdParty")

    file(GLOB_RECURSE SPARKLE_RHI_VULKAN_SOURCES CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Vulkan/*.cpp
    )

    file(GLOB_RECURSE SPARKLE_RHI_VULKAN_HEADERS CONFIGURE_DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Vulkan/*.h
    )

    set(SPARKLE_RHI_VULKAN_THIRD_PARTY_SOURCES
        ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
    )

    add_library(SparkleRHI_Vulkan STATIC
        ${SPARKLE_RHI_VULKAN_SOURCES}
        ${SPARKLE_RHI_VULKAN_HEADERS}
        ${SPARKLE_RHI_VULKAN_THIRD_PARTY_SOURCES}
    )

    sparkle_configure_rhi_implementation_target(SparkleRHI_Vulkan)

    target_link_libraries(SparkleRHI_Vulkan
        PUBLIC
            SparkleCore
            imgui
        PRIVATE
            SparklePlatform
            Vulkan::Vulkan
            SparkleVMA
    )

    sparkle_target_precompile_headers(SparkleRHI_Vulkan
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Vulkan/VulkanPCH.h
    )

    list(APPEND SPARKLE_RHI_BACKEND_TARGETS SparkleRHI_Vulkan)
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

sparkle_configure_runtime_support_artifacts(SparkleRHI)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_RHI_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private\\Common" FILES ${SPARKLE_RHI_COMMON_PRIVATE_SOURCES} ${SPARKLE_RHI_COMMON_PRIVATE_HEADERS})

if(SPARKLE_RHI_WITH_D3D12)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private/D3D12 PREFIX "Private\\D3D12" FILES ${SPARKLE_RHI_D3D12_SOURCES} ${SPARKLE_RHI_D3D12_HEADERS})
    source_group("ThirdParty\\ImGui\\D3D12" FILES ${SPARKLE_RHI_D3D12_THIRD_PARTY_SOURCES})
    source_group("ThirdParty\\D3D12MA" FILES ${SPARKLE_D3D12MA_SOURCES})
endif()

if(SPARKLE_RHI_WITH_VULKAN)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private/Vulkan PREFIX "Private\\Vulkan" FILES ${SPARKLE_RHI_VULKAN_SOURCES} ${SPARKLE_RHI_VULKAN_HEADERS})
    source_group("ThirdParty\\ImGui\\Vulkan" FILES ${SPARKLE_RHI_VULKAN_THIRD_PARTY_SOURCES})
endif()

if(SPARKLE_RHI_WITH_VULKAN)
    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private/Vulkan PREFIX "Private\\Vulkan" FILES ${SPARKLE_RHI_VULKAN_SOURCES} ${SPARKLE_RHI_VULKAN_HEADERS})
endif()
