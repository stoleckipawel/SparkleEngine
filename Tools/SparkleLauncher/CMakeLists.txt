set(SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/BuildProfileCatalog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/LauncherPaths.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/OperationModel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ProcessRunner.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ProjectDiscovery.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/RepositoryLocator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/SparkleLauncher/ToolResolver.h
)

set(SPARKLE_LAUNCHER_CORE_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/BuildProfileCatalog.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/LauncherPaths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/OperationModel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/ProcessRunner.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/ProjectDiscovery.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/RepositoryLocator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/ToolResolver.cpp
)

add_library(SparkleLauncherCore STATIC
    ${SPARKLE_LAUNCHER_CORE_PUBLIC_HEADERS}
    ${SPARKLE_LAUNCHER_CORE_PRIVATE_SOURCES}
)
sparkle_configure_host_tool_target(SparkleLauncherCore)

target_include_directories(SparkleLauncherCore
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

target_compile_features(SparkleLauncherCore PUBLIC cxx_std_20)

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
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_LAUNCHER_CORE_PRIVATE_SOURCES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Probe PREFIX "Probe" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Probe/SparkleLauncherProbe.cpp)