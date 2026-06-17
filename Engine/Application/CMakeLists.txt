# SparkleApplication Module
# Runtime/editor host layer built around RuntimeApplication and EditorApplication.
# Dependencies: SparkleCore, SparklePlatform, SparkleGameFramework, SparkleRenderer
# Runtime host code must stay cooked-only. Source import, conversion, and authoring
# workflows belong to tool-side modules and validation should fail if they leak here.
#
# This module owns host orchestration above the runtime world and renderer while
# keeping editor policy isolated to EditorApplication.

file(GLOB_RECURSE SPARKLE_APPLICATION_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_APPLICATION_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_APPLICATION_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

set(SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES ${SPARKLE_APPLICATION_PRIVATE_SOURCES})
list(FILTER SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES EXCLUDE REGEX "/Private/EditorApplication\\.cpp$")
list(FILTER SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES EXCLUDE REGEX "/Private/EditorApplicationLaunch\\.cpp$")
list(FILTER SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES EXCLUDE REGEX "/Private/Validation/RhiSmokeCaptureArtifacts\\.cpp$")
list(FILTER SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES EXCLUDE REGEX "/Private/Validation/RhiSmokeEditorValidation\\.cpp$")
list(FILTER SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES EXCLUDE REGEX "/Private/Validation/RhiSmokeViewportCapture\\.cpp$")
list(FILTER SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES EXCLUDE REGEX "/Private/Validation/RhiSmokeRenderViewModeNames\\.cpp$")
list(FILTER SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES EXCLUDE REGEX "/Private/ShaderRecook/")

file(GLOB_RECURSE SPARKLE_APPLICATION_EDITOR_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/ShaderRecook/*.cpp
)
list(APPEND SPARKLE_APPLICATION_EDITOR_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/EditorApplication.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/EditorApplicationLaunch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Validation/RhiSmokeCameraMotion.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Validation/RhiSmokeCaptureArtifacts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Validation/RhiSmokeEditorValidation.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Validation/RhiSmokeRenderViewModeNames.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Validation/RhiSmokeViewportCapture.cpp
)

file(GLOB_RECURSE SPARKLE_APPLICATION_EDITOR_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/ShaderRecook/*.h
)
list(APPEND SPARKLE_APPLICATION_EDITOR_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Validation/RhiSmokeCaptureArtifacts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Validation/RhiSmokeRenderViewModeNames.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Validation/RhiSmokeViewportCapture.h
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleApplication SHARED
        ${SPARKLE_APPLICATION_PUBLIC_HEADERS}
        ${SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES}
        ${SPARKLE_APPLICATION_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleApplication PRIVATE SPARKLE_APPLICATION_EXPORTS)
else()
    add_library(SparkleApplication STATIC
        ${SPARKLE_APPLICATION_PUBLIC_HEADERS}
        ${SPARKLE_APPLICATION_RUNTIME_PRIVATE_SOURCES}
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
sparkle_target_precompile_headers(SparkleApplication
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
        imgui
)

if(SPARKLE_BUILD_SHARED)
    add_library(SparkleApplicationEditor SHARED
        ${SPARKLE_APPLICATION_EDITOR_PRIVATE_SOURCES}
        ${SPARKLE_APPLICATION_EDITOR_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleApplicationEditor PRIVATE SPARKLE_APPLICATION_EXPORTS)
else()
    add_library(SparkleApplicationEditor STATIC
        ${SPARKLE_APPLICATION_EDITOR_PRIVATE_SOURCES}
        ${SPARKLE_APPLICATION_EDITOR_PRIVATE_HEADERS}
    )
    target_compile_definitions(SparkleApplicationEditor PUBLIC SPARKLE_STATIC)
endif()

target_include_directories(SparkleApplicationEditor
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Diagnostics
        ${CMAKE_CURRENT_SOURCE_DIR}/../Core/Public/Time
)

target_compile_features(SparkleApplicationEditor PUBLIC cxx_std_20)

sparkle_target_precompile_headers(SparkleApplicationEditor
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(SparkleApplicationEditor
    PUBLIC
        SparkleApplication
    PRIVATE
        SparkleCore
        SparklePlatform
        SparkleGameFramework
        SparkleEditor
        imgui
)

sparkle_configure_runtime_support_artifacts(SparkleApplication)
sparkle_configure_runtime_support_artifacts(SparkleApplicationEditor)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_APPLICATION_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_APPLICATION_PRIVATE_SOURCES} ${SPARKLE_APPLICATION_PRIVATE_HEADERS})
