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
sparkle_configure_host_tool_target(AssetConverter)
sparkle_configure_development_tool_artifacts(AssetConverter)

# AssetConverter remains a direct debug command surface. Source import and
# cooked output category work are owned by the focused tool modules it links.
target_include_directories(AssetConverter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(AssetConverter PRIVATE cxx_std_20)

sparkle_target_precompile_headers(AssetConverter
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(AssetConverter
    PRIVATE
        ole32
    SourceImportAdapters
    MeshCooker
    MaterialCooker
    SceneCooker
        TextureCookShared
        SparkleCore
        SparkleGameFramework
)

sparkle_declare_runtime_dll_owner(AssetConverter SparkleCore SparkleGameFramework)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${ASSET_CONVERTER_PRIVATE_SOURCES} ${ASSET_CONVERTER_PRIVATE_HEADERS})
