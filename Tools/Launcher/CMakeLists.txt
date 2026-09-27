cmake_minimum_required(VERSION 3.20)

get_filename_component(SPARKLE_REPOSITORY_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
include(${SPARKLE_REPOSITORY_ROOT}/CMake/SparkleArtifactContract.cmake)
project(SparkleLauncherProduct LANGUAGES C CXX)

include(${SPARKLE_REPOSITORY_ROOT}/CMake/SparkleBuildProfiles.cmake)
include(${SPARKLE_REPOSITORY_ROOT}/CMake/SparkleQtDiscovery.cmake)
include(${SPARKLE_REPOSITORY_ROOT}/CMake/SparkleWorkspaceFeatures.cmake)
include(${SPARKLE_REPOSITORY_ROOT}/CMake/SparkleBuildFoundation.cmake)
include(${SPARKLE_REPOSITORY_ROOT}/CMake/SparkleHostTool.cmake)

# This entry point owns the complete source dependency closure needed to build
# and run SparkleLauncher. Optional workspace capabilities are detected by the
# Launcher at runtime and are not prerequisites of the Launcher product itself.
set(SPARKLE_SOURCE_DEPENDENCY_ALLOWLIST
    spdlog
    editor-icons
)
include(${SPARKLE_REPOSITORY_ROOT}/CMake/Dependencies/FetchDependencies.cmake NO_POLICY_SCOPE)

add_subdirectory(${SPARKLE_REPOSITORY_ROOT}/Engine/Core ${CMAKE_BINARY_DIR}/Engine/Core)
add_subdirectory(${SPARKLE_REPOSITORY_ROOT}/Engine/Tasks ${CMAKE_BINARY_DIR}/Engine/Tasks)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/SparkleLauncher ${CMAKE_BINARY_DIR}/SparkleLauncher)
