# SparkleApplication Module
# Runtime/editor host layer built around ProjectApp and EditorApp.
# Dependencies: SparkleCore, SparklePlatform, SparkleGameFramework, SparkleRenderer
# Runtime host code must stay cooked-only. Source import, conversion, and authoring
# workflows belong to tool-side modules and validation should fail if they leak here.
#
# This module owns host orchestration above the runtime world and renderer while
# keeping editor policy isolated to EditorApp.

file(GLOB_RECURSE SPARKLE_APPLICATION_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_APPLICATION_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_APPLICATION_PRIVATE_HEADERS CONFIGURE_DEPENDS
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
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Time
)

target_compile_features(SparkleApplication PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
target_precompile_headers(SparkleApplication PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# Application publicly exposes renderer viewport contracts only. Core, platform,
# game runtime, and editor dependencies stay implementation-only.
target_link_libraries(SparkleApplication 
    PUBLIC
        SparkleRenderer
    PRIVATE
        SparkleCore
        SparklePlatform
        SparkleGameFramework
        SparkleEditor
        imgui
)

set_target_properties(SparkleApplication PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_APPLICATION_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_APPLICATION_PRIVATE_SOURCES} ${SPARKLE_APPLICATION_PRIVATE_HEADERS})
