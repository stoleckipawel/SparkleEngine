file(GLOB_RECURSE ASSET_CONVERTER_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE ASSET_CONVERTER_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

add_executable(AssetConverter
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
    ${ASSET_CONVERTER_PRIVATE_SOURCES}
    ${ASSET_CONVERTER_PRIVATE_HEADERS}
)

# AssetConverter is the tool-side seam for source import and scene cooking.
# Keep scene-authoring implementation and headers private to this executable
# target.
target_include_directories(AssetConverter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(AssetConverter PRIVATE cxx_std_20)

target_precompile_headers(AssetConverter PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(AssetConverter
    PRIVATE
        ole32
        TextureCookShared
        SparkleCore
        SparkleGameFramework
        cgltf
        assimp::assimp
)

set_target_properties(AssetConverter PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${ASSET_CONVERTER_PRIVATE_SOURCES} ${ASSET_CONVERTER_PRIVATE_HEADERS})