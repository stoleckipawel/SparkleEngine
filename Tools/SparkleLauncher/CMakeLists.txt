set(SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/BuildWorkspaceOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/BuildProfileCatalog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/CookOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/LaunchOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/MaintenanceOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/LauncherPaths.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/OperationModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ProcessRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ProjectDiscovery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/RepositoryLocator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ToolResolver.h
)

set(SPARKLE_LAUNCHER_CORE_FOUNDATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/BuildProfileCatalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/LauncherPaths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/OperationModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/ProcessRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/ProjectDiscovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/RepositoryLocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/ToolResolver.cpp
)

set(SPARKLE_LAUNCHER_CORE_BUILD_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildFilesFreshness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildFreshnessSignature.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildToolchainDetection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildWorkspaceExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildWorkspaceOperations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildWorkspacePlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildWorkspaceProcessRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildWorkspaceStateFiles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/CMakeWorkflowProcessRequests.cpp
)

set(SPARKLE_LAUNCHER_CORE_BUILD_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildFreshnessSignature.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildWorkspaceProcessRequests.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/BuildWorkspaceStateFiles.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow/CMakeWorkflowProcessRequests.h
)

set(SPARKLE_LAUNCHER_CORE_COOK_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cook/CookOperationExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cook/CookOperationProcessRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cook/CookOperations.cpp
)

set(SPARKLE_LAUNCHER_CORE_COOK_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cook/CookOperationProcessRequests.h
)

set(SPARKLE_LAUNCHER_CORE_MAINTENANCE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Maintenance/MaintenanceOperationExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Maintenance/MaintenanceOperationProcessRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Maintenance/MaintenanceOperations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Maintenance/Validation/ValidationGateCatalog.cpp
)

set(SPARKLE_LAUNCHER_CORE_MAINTENANCE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Maintenance/MaintenanceOperationProcessRequests.h
)

set(SPARKLE_LAUNCHER_CORE_LAUNCH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperationExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperationProcessRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/Smoke/RhiSmokeLaunchOperations.cpp
)

set(SPARKLE_LAUNCHER_CORE_LAUNCH_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperationProcessRequests.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/Smoke/RhiSmokeLaunchOperations.h
)

set(SPARKLE_LAUNCHER_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShell.cpp
)

set(SPARKLE_LAUNCHER_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShell.h
)

set(SPARKLE_CLI_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCli.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCliDispatcher.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCliOutput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCliParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCliValueOptions.cpp
)

set(SPARKLE_CLI_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCliArguments.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCli.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCliDispatcher.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCliOutput.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli/SparkleCliParser.h
)

add_library(SparkleLauncherCore STATIC
    ${SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS}
    ${SPARKLE_LAUNCHER_CORE_FOUNDATION_SOURCES}
    ${SPARKLE_LAUNCHER_CORE_BUILD_SOURCES}
    ${SPARKLE_LAUNCHER_CORE_BUILD_HEADERS}
    ${SPARKLE_LAUNCHER_CORE_COOK_SOURCES}
    ${SPARKLE_LAUNCHER_CORE_COOK_HEADERS}
    ${SPARKLE_LAUNCHER_CORE_MAINTENANCE_SOURCES}
    ${SPARKLE_LAUNCHER_CORE_MAINTENANCE_HEADERS}
    ${SPARKLE_LAUNCHER_CORE_LAUNCH_SOURCES}
    ${SPARKLE_LAUNCHER_CORE_LAUNCH_HEADERS}
)
sparkle_configure_host_tool_target(SparkleLauncherCore)

target_include_directories(SparkleLauncherCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildWorkflow
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cook
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Maintenance
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch
)

target_compile_features(SparkleLauncherCore PUBLIC cxx_std_20)

target_link_libraries(SparkleLauncherCore PRIVATE SparkleCore)

add_executable(SparkleLauncher
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
    ${SPARKLE_LAUNCHER_PRIVATE_SOURCES}
    ${SPARKLE_LAUNCHER_PRIVATE_HEADERS}
)
sparkle_configure_host_tool_target(SparkleLauncher)

target_include_directories(SparkleLauncher
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell
)

target_link_libraries(SparkleLauncher
    PRIVATE
        SparkleLauncherCore
)

target_compile_features(SparkleLauncher PRIVATE cxx_std_20)

set_target_properties(SparkleLauncher PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_executable(Sparkle
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/SparkleMain.cpp
    ${SPARKLE_CLI_PRIVATE_SOURCES}
    ${SPARKLE_CLI_PRIVATE_HEADERS}
)
sparkle_configure_host_tool_target(Sparkle)

target_include_directories(Sparkle
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cli
)

target_link_libraries(Sparkle
    PRIVATE
        SparkleLauncherCore
)

target_compile_features(Sparkle PRIVATE cxx_std_20)

set_target_properties(Sparkle PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

add_executable(SparkleLauncherProbe
    ${CMAKE_CURRENT_SOURCE_DIR}/Probe/SparkleLauncherProbe.cpp
)
sparkle_configure_host_tool_target(SparkleLauncherProbe)

target_link_libraries(SparkleLauncherProbe
    PRIVATE
        SparkleLauncherCore
)

target_compile_features(SparkleLauncherProbe PRIVATE cxx_std_20)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_LAUNCHER_CORE_FOUNDATION_SOURCES} ${SPARKLE_LAUNCHER_CORE_BUILD_SOURCES} ${SPARKLE_LAUNCHER_CORE_BUILD_HEADERS} ${SPARKLE_LAUNCHER_CORE_COOK_SOURCES} ${SPARKLE_LAUNCHER_CORE_COOK_HEADERS} ${SPARKLE_LAUNCHER_CORE_MAINTENANCE_SOURCES} ${SPARKLE_LAUNCHER_CORE_MAINTENANCE_HEADERS} ${SPARKLE_LAUNCHER_CORE_LAUNCH_SOURCES} ${SPARKLE_LAUNCHER_CORE_LAUNCH_HEADERS} ${SPARKLE_LAUNCHER_PRIVATE_SOURCES} ${SPARKLE_LAUNCHER_PRIVATE_HEADERS} ${SPARKLE_CLI_PRIVATE_SOURCES} ${SPARKLE_CLI_PRIVATE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Source/SparkleMain.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Probe PREFIX "Probe" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Probe/SparkleLauncherProbe.cpp)