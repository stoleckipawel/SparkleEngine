file(GLOB_RECURSE SPARKLE_ASSET_AUTHORING_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_ASSET_AUTHORING_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_ASSET_AUTHORING_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

add_library(SparkleAssetAuthoring STATIC
    ${SPARKLE_ASSET_AUTHORING_PUBLIC_HEADERS}
    ${SPARKLE_ASSET_AUTHORING_PRIVATE_SOURCES}
    ${SPARKLE_ASSET_AUTHORING_PRIVATE_HEADERS}
)

target_include_directories(SparkleAssetAuthoring
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(SparkleAssetAuthoring PUBLIC cxx_std_20)

target_precompile_headers(SparkleAssetAuthoring PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(SparkleAssetAuthoring
    PUBLIC
        SparkleCore
        SparkleGameFramework
    PRIVATE
        SparkleRHI
        cgltf
        assimp::assimp
        ktx
)

add_executable(AssetConverter
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
)

target_include_directories(AssetConverter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_compile_features(AssetConverter PRIVATE cxx_std_20)

target_link_libraries(AssetConverter
    PRIVATE
    ole32
        SparkleAssetAuthoring
)

set_target_properties(SparkleAssetAuthoring PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

set_target_properties(AssetConverter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_ASSET_AUTHORING_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_ASSET_AUTHORING_PRIVATE_SOURCES} ${SPARKLE_ASSET_AUTHORING_PRIVATE_HEADERS})