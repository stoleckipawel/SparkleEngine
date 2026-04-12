# SparkleApplication Module
# Top-level application framework that ties together GameFramework and Renderer
# Dependencies: SparkleCore, SparklePlatform, SparkleGameFramework, SparkleRenderer
#
# This module contains App and related classes that need both game logic and rendering.
# It sits at the top of the dependency hierarchy.

file(GLOB_RECURSE SPARKLE_APPLICATION_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_APPLICATION_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_APPLICATION_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleApplication SHARED
        ${SPARKLE_APPLICATION_PUBLIC_HEADERS}
        ${SPARKLE_APPLICATION_PRIVATE_SOURCES}
        ${SPARKLE_APPLICATION_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleApplication PRIVATE SPARKLE_APPLICATION_EXPORTS)
else()
    add_library(SparkleApplication STATIC
        ${SPARKLE_APPLICATION_PUBLIC_HEADERS}
        ${SPARKLE_APPLICATION_PRIVATE_SOURCES}
        ${SPARKLE_APPLICATION_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleApplication PUBLIC SPARKLE_STATIC)
endif()

target_include_directories(SparkleApplication
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        # Engine root for module-prefixed includes
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        # Core module subdirectories
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Time
        # Platform module subdirectories
        ${CMAKE_CURRENT_SOURCE_DIR}/../Platform/Public
        # GameFramework module subdirectories
        ${CMAKE_CURRENT_SOURCE_DIR}/../GameFramework/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/../GameFramework/Public/Scene
        ${CMAKE_CURRENT_SOURCE_DIR}/../GameFramework/Public/Scene/Camera
        # Renderer module
        ${CMAKE_CURRENT_SOURCE_DIR}/../Renderer/Public
)

target_compile_features(SparkleApplication PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
target_precompile_headers(SparkleApplication PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# Application depends on all lower-level modules
target_link_libraries(SparkleApplication 
    PUBLIC 
        SparkleCore
        SparklePlatform
        SparkleGameFramework
        SparkleRenderer
    PRIVATE
        SparkleEditor
)

set_target_properties(SparkleApplication PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_APPLICATION_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_APPLICATION_PRIVATE_SOURCES} ${SPARKLE_APPLICATION_PRIVATE_HEADERS})
