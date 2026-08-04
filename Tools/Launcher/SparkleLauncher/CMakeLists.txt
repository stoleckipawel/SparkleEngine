set(SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/BuildWorkspaceOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ArtifactNaming.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/BuildProfileCatalog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/CookOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/LaunchOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/LauncherContentDefaults.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/MaintenanceOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/LauncherPaths.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/OperationModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ProcessRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ContentDiscovery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/RepositoryLocator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/SourceDependencyState.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ToolResolver.h
)

set(SPARKLE_LAUNCHER_CORE_FOUNDATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/CapabilityGraph.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/BuildProfileCatalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/LauncherPaths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/OperationModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/ProcessRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/ContentDiscovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/RepositoryLocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/SourceDependencyState.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/HostGraphicsCapabilities.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core/ToolResolver.cpp
)

set(SPARKLE_LAUNCHER_CORE_BUILD_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildFilesFreshness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildFreshnessSignature.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildToolchainDetection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/HostToolInstaller.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/QtToolchainDiscovery.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/ShaderCompilerSdkDiscovery.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/VisualStudioToolchainDiscovery.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/VulkanSdkDiscovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceOperations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspacePlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceProcessRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceStateFiles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/CMakeGeneratorModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/CMakeWorkflowProcessRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/AssetPackSyncPlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/AssetPackSyncProcessRequests.cpp
)

set(SPARKLE_LAUNCHER_CORE_BUILD_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildFreshnessSignature.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceProcessRequests.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceStateFiles.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/CMakeGeneratorModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/CMakeWorkflowProcessRequests.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/HostToolInstaller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/AssetPackSyncPlanner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/AssetPackSyncProcessRequests.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/QtToolchainDiscovery.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/ShaderCompilerSdkDiscovery.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/VisualStudioToolchainDiscovery.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/VulkanSdkDiscovery.h
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
)

set(SPARKLE_LAUNCHER_CORE_MAINTENANCE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Maintenance/MaintenanceOperationProcessRequests.h
)

set(SPARKLE_LAUNCHER_CORE_LAUNCH_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperationExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperationProcessRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperations.cpp
)

set(SPARKLE_LAUNCHER_CORE_LAUNCH_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperationProcessRequests.h
)

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets)

set(SPARKLE_LAUNCHER_GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/App/LauncherBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/App/LauncherGuiApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/App/LauncherSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Operations/LauncherOperationRequestMapping.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Operations/LauncherOperationExecution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Operations/LauncherOperationService.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherActionHistoryModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherCleanUiModel.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherContextUiModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherDependencyUiModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherLevelUiModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherOperationRequestFactory.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherCapabilityProviders.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherHostToolCapabilityProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherSourceDependencyCapabilityProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherWorkspaceCapabilityProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherLevelCapabilityProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherCookCapabilityProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherLaunchCapabilityProvider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherQuickStartExecution.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherQuickStartPlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherContentModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherToolchainUiModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherUiModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherWorkflowCatalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowActivity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherPageUtilities.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowHomePage.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowLevelSync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowOptionPages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowOperations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowPagePrimitives.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowQuickStart.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowShell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowStatusPages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindowSyncPages.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Style/LauncherIconLibrary.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Style/LauncherUiDesign.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Style/LauncherVisualStyle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherActionWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherArtworkWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherHomeWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherLayoutWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherOutputWidgets.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherSelectionWidgets.cpp
)

set(SPARKLE_LAUNCHER_GUI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/App/LauncherBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/App/LauncherGuiApp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/App/LauncherSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Operations/LauncherOperationRequest.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Operations/LauncherOperationRequestMapping.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Operations/LauncherOperationExecution.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Operations/LauncherOperationService.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherActionHistoryModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherCleanUiModel.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherContextUiModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherDependencyUiModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherLevelUiModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherOperationRequestFactory.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherCapabilityRegistry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherCapabilityProviders.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherQuickStartExecution.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherQuickStartPlanner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherContentModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherToolchainUiModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherUiModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherWorkflowCatalog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherMainWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell/LauncherPageUtilities.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Style/LauncherIconLibrary.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Style/LauncherUiDesign.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Style/LauncherVisualStyle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherActionWidgets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherArtworkWidgets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherHomeWidgets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherLayoutWidgets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherOutputWidgets.h
	${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets/LauncherSelectionWidgets.h
)

set(SPARKLE_LAUNCHER_PRIVATE_SOURCES
    ${SPARKLE_LAUNCHER_GUI_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShellArguments.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShell.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShellModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShellOperations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShellPresentation.cpp
)

set(SPARKLE_LAUNCHER_PRIVATE_HEADERS
    ${SPARKLE_LAUNCHER_GUI_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShellArguments.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShell.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShellModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShellOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShellPresentation.h
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
sparkle_configure_launcher_artifacts(SparkleLauncherCore)

target_include_directories(SparkleLauncherCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cook
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Maintenance
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch
)

target_compile_features(SparkleLauncherCore PUBLIC cxx_std_20)

target_compile_definitions(SparkleLauncherCore
    PUBLIC
        SPARKLE_ENABLE_CONTENT_PIPELINE=$<BOOL:${SPARKLE_ENABLE_CONTENT_PIPELINE}>
        SPARKLE_ENABLE_SHADER_COMPILER=$<BOOL:${SPARKLE_ENABLE_SHADER_COMPILER}>
        SPARKLE_ENABLE_KTX_SUPPORT=$<BOOL:${SPARKLE_ENABLE_KTX_SUPPORT}>
        SPARKLE_ENABLE_NVIDIA_STREAMLINE=$<BOOL:${SPARKLE_ENABLE_NVIDIA_STREAMLINE}>
)

target_link_libraries(SparkleLauncherCore PUBLIC SparkleCore)

add_executable(SparkleLauncher WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
    ${SPARKLE_LAUNCHER_PRIVATE_SOURCES}
    ${SPARKLE_LAUNCHER_PRIVATE_HEADERS}
)
sparkle_configure_host_tool_target(SparkleLauncher)

target_include_directories(SparkleLauncher
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/App
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Operations
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Shell
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Style
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Widgets
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell
)

target_link_libraries(SparkleLauncher
    PRIVATE
        SparkleLauncherCore
        SparkleCore
        SparkleTasks
        Qt6::Widgets
)

target_compile_features(SparkleLauncher PRIVATE cxx_std_20)

if(DEFINED SPARKLE_FONT_AWESOME_SOLID_TTF AND EXISTS "${SPARKLE_FONT_AWESOME_SOLID_TTF}")
    file(TO_CMAKE_PATH "${SPARKLE_FONT_AWESOME_SOLID_TTF}" SPARKLE_LAUNCHER_FONT_AWESOME_SOLID_TTF_NORMALIZED)
    target_compile_definitions(SparkleLauncher PRIVATE
        SPARKLE_FONT_AWESOME_SOLID_TTF="${SPARKLE_LAUNCHER_FONT_AWESOME_SOLID_TTF_NORMALIZED}"
    )
endif()

set_target_properties(SparkleLauncher PROPERTIES
    AUTOMOC ON
)
sparkle_configure_launcher_artifacts(SparkleLauncher)
sparkle_declare_runtime_dll_owner(SparkleLauncher SparkleCore)

add_custom_command(TARGET SparkleLauncher POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/Assets/Visuals"
        "$<TARGET_FILE_DIR:SparkleLauncher>/Visuals"
    COMMENT "Deploying SparkleLauncher visual artwork"
)

if(WIN32)
    get_target_property(SPARKLE_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
    get_filename_component(SPARKLE_QT_BIN_DIR "${SPARKLE_QMAKE_EXECUTABLE}" DIRECTORY)

    find_program(SPARKLE_WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt windeployqt6
        HINTS "${SPARKLE_QT_BIN_DIR}"
        REQUIRED
    )

    add_custom_command(TARGET SparkleLauncher POST_BUILD
        COMMAND "${SPARKLE_WINDEPLOYQT_EXECUTABLE}"
            --verbose 0
            --dir "$<TARGET_FILE_DIR:SparkleLauncher>"
            "$<TARGET_FILE:SparkleLauncher>"
        COMMENT "Deploying Qt runtime for SparkleLauncher artifact"
    )
endif()

add_executable(SparkleLauncherProbe
    ${CMAKE_CURRENT_SOURCE_DIR}/Probe/SparkleLauncherProbe.cpp
)
sparkle_configure_host_tool_target(SparkleLauncherProbe)
sparkle_configure_launcher_artifacts(SparkleLauncherProbe)

target_link_libraries(SparkleLauncherProbe
    PRIVATE
        SparkleLauncherCore
        SparkleCore
)
target_include_directories(SparkleLauncherProbe PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build)

target_compile_features(SparkleLauncherProbe PRIVATE cxx_std_20)
add_test(NAME sparkle_launcher_probe COMMAND SparkleLauncherProbe "${CMAKE_SOURCE_DIR}")
set_tests_properties(sparkle_launcher_probe PROPERTIES LABELS "Launcher;Content;Integration")

add_test(
    NAME asset_pack_sync
    COMMAND "${CMAKE_COMMAND}"
        "-DSPARKLE_TEST_BINARY_ROOT=${CMAKE_CURRENT_BINARY_DIR}/AssetPackSyncTests"
        "-DSPARKLE_SYNC_SCRIPT=${CMAKE_CURRENT_SOURCE_DIR}/Scripts/SyncAssetPack.cmake"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/Tests/AssetPackSyncTests.cmake"
)
set_tests_properties(asset_pack_sync PROPERTIES LABELS "Launcher;Content")

add_executable(AssetPackSyncPlannerTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AssetPackSyncPlannerTests.cpp
)
sparkle_configure_host_tool_target(AssetPackSyncPlannerTests)
target_include_directories(AssetPackSyncPlannerTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build)
target_link_libraries(AssetPackSyncPlannerTests PRIVATE SparkleLauncherCore SparkleCore)
target_compile_features(AssetPackSyncPlannerTests PRIVATE cxx_std_20)
add_test(NAME asset_pack_sync_planner COMMAND AssetPackSyncPlannerTests)
set_tests_properties(asset_pack_sync_planner PROPERTIES LABELS "Launcher;Content")

add_executable(ContentDiscoveryTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ContentDiscoveryTests.cpp
)
sparkle_configure_host_tool_target(ContentDiscoveryTests)
target_link_libraries(ContentDiscoveryTests PRIVATE SparkleLauncherCore SparkleCore)
target_compile_features(ContentDiscoveryTests PRIVATE cxx_std_20)
add_test(NAME content_discovery COMMAND ContentDiscoveryTests)
set_tests_properties(content_discovery PROPERTIES LABELS "Launcher;Content")

add_executable(CapabilityGraphTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/CapabilityGraphTests.cpp
)
sparkle_configure_host_tool_target(CapabilityGraphTests)
target_include_directories(CapabilityGraphTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core)
target_compile_features(CapabilityGraphTests PRIVATE cxx_std_20)
add_test(NAME capability_graph COMMAND CapabilityGraphTests)
set_tests_properties(capability_graph PROPERTIES LABELS "Launcher;Workflow")

add_executable(LauncherQuickStartExecutionTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/LauncherQuickStartExecutionTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherQuickStartExecution.cpp
)
sparkle_configure_host_tool_target(LauncherQuickStartExecutionTests)
target_include_directories(LauncherQuickStartExecutionTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Core
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/App
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models
)
target_link_libraries(LauncherQuickStartExecutionTests PRIVATE SparkleCore Qt6::Core)
target_compile_features(LauncherQuickStartExecutionTests PRIVATE cxx_std_20)
if(WIN32)
    add_custom_command(TARGET LauncherQuickStartExecutionTests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:Qt6::Core>"
            "$<TARGET_FILE_DIR:LauncherQuickStartExecutionTests>"
        COMMENT "Deploying Qt Core for LauncherQuickStartExecutionTests"
    )
endif()
add_test(NAME launcher_quick_start_execution COMMAND LauncherQuickStartExecutionTests)
set_tests_properties(launcher_quick_start_execution PROPERTIES LABELS "Launcher;Workflow")

add_executable(LauncherContextUiModelTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/LauncherContextUiModelTests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models/LauncherContextUiModel.cpp
)
sparkle_configure_host_tool_target(LauncherContextUiModelTests)
target_include_directories(LauncherContextUiModelTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/Models
)
target_link_libraries(LauncherContextUiModelTests PRIVATE SparkleLauncherCore Qt6::Core)
target_compile_features(LauncherContextUiModelTests PRIVATE cxx_std_20)
if(WIN32)
    add_custom_command(TARGET LauncherContextUiModelTests POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "$<TARGET_FILE:Qt6::Core>"
            "$<TARGET_FILE_DIR:LauncherContextUiModelTests>"
        COMMENT "Deploying Qt Core for LauncherContextUiModelTests"
    )
endif()
add_test(NAME launcher_context_ui_model COMMAND LauncherContextUiModelTests)
set_tests_properties(launcher_context_ui_model PROPERTIES LABELS "Launcher;UI")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_LAUNCHER_CORE_FOUNDATION_SOURCES} ${SPARKLE_LAUNCHER_CORE_BUILD_SOURCES} ${SPARKLE_LAUNCHER_CORE_BUILD_HEADERS} ${SPARKLE_LAUNCHER_CORE_COOK_SOURCES} ${SPARKLE_LAUNCHER_CORE_COOK_HEADERS} ${SPARKLE_LAUNCHER_CORE_MAINTENANCE_SOURCES} ${SPARKLE_LAUNCHER_CORE_MAINTENANCE_HEADERS} ${SPARKLE_LAUNCHER_CORE_LAUNCH_SOURCES} ${SPARKLE_LAUNCHER_CORE_LAUNCH_HEADERS} ${SPARKLE_LAUNCHER_PRIVATE_SOURCES} ${SPARKLE_LAUNCHER_PRIVATE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Probe PREFIX "Probe" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Probe/SparkleLauncherProbe.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Tests PREFIX "Tests" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Tests/AssetPackSyncPlannerTests.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Tests/ContentDiscoveryTests.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Tests/CapabilityGraphTests.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Tests/LauncherQuickStartExecutionTests.cpp ${CMAKE_CURRENT_SOURCE_DIR}/Tests/LauncherContextUiModelTests.cpp)
