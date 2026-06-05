set(SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/BuildWorkspaceOperations.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ArtifactNaming.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildFilesFreshness.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildFreshnessSignature.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildToolchainDetection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceExecutor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceOperations.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspacePlanner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceProcessRequests.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceStateFiles.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/CMakeWorkflowProcessRequests.cpp
)

set(SPARKLE_LAUNCHER_CORE_BUILD_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildFreshnessSignature.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceProcessRequests.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/BuildWorkspaceStateFiles.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Build/CMakeWorkflowProcessRequests.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/Smoke/RhiSmokeLaunchOperations.cpp
)

set(SPARKLE_LAUNCHER_CORE_LAUNCH_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/LaunchOperationProcessRequests.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Launch/Smoke/RhiSmokeLaunchOperations.h
)

find_package(Qt6 6.8 REQUIRED COMPONENTS Widgets)

set(SPARKLE_LAUNCHER_GUI_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherActionWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherArtworkWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherBackend.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherDependencyUiModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherGuiApp.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherLayoutWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherMainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherOutputWidgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherProjectModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherRecoveryUiModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherSettings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherUiModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherVisualStyle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherWorkflowCatalog.cpp
)

set(SPARKLE_LAUNCHER_GUI_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherActionWidgets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherArtworkWidgets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherDependencyUiModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherLayoutWidgets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherMainWindow.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherOutputWidgets.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherProjectModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherRecoveryUiModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherSettings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherUiDesign.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherUiModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherVisualStyle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherWorkflowCatalog.h
)

set(SPARKLE_LAUNCHER_PRIVATE_SOURCES
    ${SPARKLE_LAUNCHER_GUI_SOURCES}
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShell.cpp
)

set(SPARKLE_LAUNCHER_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui/LauncherGuiApp.h
    ${SPARKLE_LAUNCHER_GUI_HEADERS}
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell/LauncherShell.h
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
)

target_link_libraries(SparkleLauncherCore PRIVATE SparkleCore)

add_executable(SparkleLauncher WIN32
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
    ${SPARKLE_LAUNCHER_PRIVATE_SOURCES}
    ${SPARKLE_LAUNCHER_PRIVATE_HEADERS}
)
sparkle_configure_host_tool_target(SparkleLauncher)

target_include_directories(SparkleLauncher
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Gui
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Shell
)

target_link_libraries(SparkleLauncher
    PRIVATE
        SparkleLauncherCore
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
)

target_compile_features(SparkleLauncherProbe PRIVATE cxx_std_20)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_LAUNCHER_CORE_FOUNDATION_SOURCES} ${SPARKLE_LAUNCHER_CORE_BUILD_SOURCES} ${SPARKLE_LAUNCHER_CORE_BUILD_HEADERS} ${SPARKLE_LAUNCHER_CORE_COOK_SOURCES} ${SPARKLE_LAUNCHER_CORE_COOK_HEADERS} ${SPARKLE_LAUNCHER_CORE_MAINTENANCE_SOURCES} ${SPARKLE_LAUNCHER_CORE_MAINTENANCE_HEADERS} ${SPARKLE_LAUNCHER_CORE_LAUNCH_SOURCES} ${SPARKLE_LAUNCHER_CORE_LAUNCH_HEADERS} ${SPARKLE_LAUNCHER_PRIVATE_SOURCES} ${SPARKLE_LAUNCHER_PRIVATE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Probe PREFIX "Probe" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Probe/SparkleLauncherProbe.cpp)
