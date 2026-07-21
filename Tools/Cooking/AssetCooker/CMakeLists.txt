set(ASSET_COOKER_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Api/AssetCookerService.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/DefaultTextureCookRequestBuilder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/ImportedSceneCooker.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureRequestPlanBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/AssetCookerDiagnostics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Discovery/AssetCookerDiscovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerDispatcher.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerStageExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerToolProcess.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Inspection/AssetCookerSourceInspection.cpp
)

set(ASSET_COOKER_CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Api/AssetCookerService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Api/AssetCookerTypes.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/DefaultTextureCookRequestBuilder.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/ImportedSceneCooker.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureRequestPlanBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/AssetCookerDiagnostics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Discovery/AssetCookerDiscovery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerDispatcher.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerStageExecutor.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Dispatch/AssetCookerToolProcess.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Inspection/AssetCookerSourceInspection.h
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
    ${ASSET_COOKER_CORE_SOURCES}
    ${ASSET_COOKER_CORE_HEADERS}
)
sparkle_configure_host_tool_target(AssetCookerCore)

target_include_directories(AssetCookerCore
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(AssetCookerCore PRIVATE cxx_std_20)
set(ASSET_COOKER_COOKING_LINK_LIBRARIES
    ole32
    SourceImporters
    MeshCooker
    MaterialCooker
    SceneCooker
    TextureCookShared
    ToolConsoleSupport
    SparkleCore
    SparkleGameFramework
)

target_link_libraries(AssetCookerCore
    PRIVATE
        ${ASSET_COOKER_COOKING_LINK_LIBRARIES}
)

add_executable(AssetCooker
    ${ASSET_COOKER_CLI_SOURCES}
    ${ASSET_COOKER_CLI_HEADERS}
)
sparkle_configure_host_tool_target(AssetCooker)
sparkle_configure_development_tool_artifacts(AssetCooker)

target_include_directories(AssetCooker
    PRIVATE
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
sparkle_declare_runtime_dll_owner(AssetCooker SparkleCore SparkleGameFramework SparkleRHI)

if(TARGET assimp)
    add_custom_command(TARGET AssetCooker POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:assimp>"
            "$<TARGET_FILE_DIR:AssetCooker>"
        COMMENT "Copying Assimp runtime DLL for AssetCooker"
        VERBATIM
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${ASSET_COOKER_CORE_SOURCES} ${ASSET_COOKER_CORE_HEADERS} ${ASSET_COOKER_CLI_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)
