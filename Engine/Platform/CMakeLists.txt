# SparklePlatform Module
# OS abstraction: Window, Input system, File system
# Dependencies: SparkleCore

file(GLOB_RECURSE SPARKLE_PLATFORM_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_PLATFORM_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_PLATFORM_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparklePlatform SHARED
        ${SPARKLE_PLATFORM_PUBLIC_HEADERS}
        ${SPARKLE_PLATFORM_PRIVATE_SOURCES}
        ${SPARKLE_PLATFORM_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparklePlatform PRIVATE SPARKLE_PLATFORM_EXPORTS)
else()
    add_library(SparklePlatform STATIC
        ${SPARKLE_PLATFORM_PUBLIC_HEADERS}
        ${SPARKLE_PLATFORM_PRIVATE_SOURCES}
        ${SPARKLE_PLATFORM_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparklePlatform PUBLIC SPARKLE_STATIC)
endif()

target_include_directories(SparklePlatform
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        # Engine root for module-prefixed includes (e.g., "Platform/...")
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_compile_features(SparklePlatform PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
sparkle_target_precompile_headers(SparklePlatform
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# Platform depends on Core
target_link_libraries(SparklePlatform 
    PUBLIC 
        SparkleCore
    PRIVATE
        imgui
        dwmapi
)

set_target_properties(SparklePlatform PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_PLATFORM_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_PLATFORM_PRIVATE_SOURCES} ${SPARKLE_PLATFORM_PRIVATE_HEADERS})
