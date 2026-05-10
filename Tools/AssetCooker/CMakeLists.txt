set(ASSET_COOKER_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/AssetCookRequest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/AssetCookResult.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/AssetCookerTypes.h
)

set(ASSET_COOKER_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Api/AssetCookerService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/AssetCookerDiagnostics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Discovery/AssetCookerDiscovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerToolProcess.cpp
)

set(ASSET_COOKER_CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Api/AssetCookerService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/AssetCookerDiagnostics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Discovery/AssetCookerDiscovery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerDispatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerToolProcess.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Planning/ProjectCookPlan.h
)

set(ASSET_COOKER_CLI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/AssetCookerCli.cpp
)

set(ASSET_COOKER_CLI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/AssetCookerCli.h
)

add_library(AssetCookerCore STATIC
    ${ASSET_COOKER_PUBLIC_HEADERS}
    ${ASSET_COOKER_CORE_SOURCES}
    ${ASSET_COOKER_CORE_HEADERS}
)

target_include_directories(AssetCookerCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(AssetCookerCore PUBLIC cxx_std_20)
set(ASSET_COOKER_COOKING_LINK_LIBRARIES
    ole32
    CookCommon
    SourceImportAdapters
    MeshCooker
    MaterialCooker
    SceneCooker
    TextureCookShared
    SparkleCore
    SparkleGameFramework
)

target_link_libraries(AssetCookerCore
    PRIVATE
        ${ASSET_COOKER_COOKING_LINK_LIBRARIES}
)

add_executable(AssetCooker
    ${ASSET_COOKER_PUBLIC_HEADERS}
    ${ASSET_COOKER_CLI_SOURCES}
    ${ASSET_COOKER_CLI_HEADERS}
)

target_include_directories(AssetCooker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(AssetCooker PRIVATE cxx_std_20)
target_link_libraries(AssetCooker
    PRIVATE
        AssetCookerCore
        ${ASSET_COOKER_COOKING_LINK_LIBRARIES}
)
set_target_properties(AssetCooker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${ASSET_COOKER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${ASSET_COOKER_CORE_SOURCES} ${ASSET_COOKER_CORE_HEADERS} ${ASSET_COOKER_CLI_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)
