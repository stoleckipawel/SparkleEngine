cmake_minimum_required(VERSION 3.20)
include(${CMAKE_CURRENT_LIST_DIR}/CMake/SparkleArtifactContract.cmake)
project(Sparkle LANGUAGES C CXX)

include(${CMAKE_SOURCE_DIR}/CMake/SparkleBuildProfiles.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/SparkleQtDiscovery.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/SparkleProjectProduct.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/SparkleWorkspaceFeatures.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/SparkleBuildFoundation.cmake)

set(SPARKLE_SYNC_SOURCE_DEPENDENCY "" CACHE STRING "Populate exactly one source dependency cache and skip workspace generation")
if(NOT SPARKLE_SYNC_SOURCE_DEPENDENCY STREQUAL "")
    include(${CMAKE_SOURCE_DIR}/CMake/Dependencies/FetchDependencies.cmake)
    return()
endif()

# ---------------------------
# Output directories
# ---------------------------
# For multi-config generators (Visual Studio), set per-configuration output dirs.
# For single-config generators (Ninja, Makefiles), the non-suffixed vars are used.
if(CMAKE_CONFIGURATION_TYPES)
    # Multi-config generator (Visual Studio, Xcode, Ninja Multi-Config)
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${CONFIG_TYPE}" CONFIG_UPPER)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/private/runtime/${CONFIG_TYPE}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/private/lib/${CONFIG_TYPE}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_BINARY_DIR}/private/runtime/${CONFIG_TYPE}")
    endforeach()
else()
    # Single-config generator (Ninja, Makefiles)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/private/runtime/${CMAKE_BUILD_TYPE}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/private/lib/${CMAKE_BUILD_TYPE}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/private/runtime/${CMAKE_BUILD_TYPE}")
endif()

# ---------------------------
# Add subdirectories
# ---------------------------
add_subdirectory(Engine)
add_subdirectory(Tools)

# ---------------------------
# Auto-discover projects via .sparkle-project marker
# ---------------------------
# Scans Projects/ directory for project markers
set(PROJECT_SEARCH_DIRS
    "${CMAKE_SOURCE_DIR}/Projects"
)

set(DISCOVERED_PROJECTS "")
foreach(SEARCH_DIR ${PROJECT_SEARCH_DIRS})
    if(EXISTS "${SEARCH_DIR}")
        file(GLOB PROJECT_CANDIDATES "${SEARCH_DIR}/*")
        foreach(CANDIDATE ${PROJECT_CANDIDATES})
            if(IS_DIRECTORY "${CANDIDATE}")
                get_filename_component(PROJECT_NAME "${CANDIDATE}" NAME)
                # Skip TemplateProject - it's only used for project generation
                if(PROJECT_NAME STREQUAL "TemplateProject")
                    continue()
                endif()
                if(EXISTS "${CANDIDATE}/.sparkle-project")
                    message(STATUS "Discovered project: ${PROJECT_NAME}")
                    add_subdirectory("${CANDIDATE}")
                    list(APPEND DISCOVERED_PROJECTS "${PROJECT_NAME}")
                endif()
            endif()
        endforeach()
    endif()
endforeach()

if(NOT DISCOVERED_PROJECTS)
    message(WARNING "No projects discovered. Add a runnable project under Projects/ with a .sparkle-project marker")
endif()

# ---------------------------
# Architecture boundary checks
# ---------------------------
add_custom_target(
    architecture_boundary_check
    COMMAND ${CMAKE_COMMAND} -DSPARKLE_REPO_ROOT=${CMAKE_SOURCE_DIR} -P ${CMAKE_SOURCE_DIR}/CMake/ArchitectureBoundaryCheck.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking repository architecture boundaries..."
)

# ---------------------------
# Code-style checks
# ---------------------------
find_program(SPARKLE_POWERSHELL_EXECUTABLE NAMES pwsh powershell REQUIRED)
set(SPARKLE_CLANG_FORMAT_EXECUTABLE "" CACHE FILEPATH "Optional path to the required clang-format 22.1.3 executable")
set(SPARKLE_CLANG_TIDY_EXECUTABLE "" CACHE FILEPATH "Optional path to the required clang-tidy 22.1.3 executable")

add_custom_target(
    code_style_check
    COMMAND ${SPARKLE_POWERSHELL_EXECUTABLE}
        -NoProfile
        -ExecutionPolicy Bypass
        -File ${CMAKE_SOURCE_DIR}/CMake/CodeStyle.ps1
        -Mode Check
        -ClangFormatPath "${SPARKLE_CLANG_FORMAT_EXECUTABLE}"
        -ClangTidyPath "${SPARKLE_CLANG_TIDY_EXECUTABLE}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Checking tracked owned source against the Sparkle code-style policy..."
    USES_TERMINAL
    VERBATIM
)

add_custom_target(
    code_style_format
    COMMAND ${SPARKLE_POWERSHELL_EXECUTABLE}
        -NoProfile
        -ExecutionPolicy Bypass
        -File ${CMAKE_SOURCE_DIR}/CMake/CodeStyle.ps1
        -Mode Format
        -ClangFormatPath "${SPARKLE_CLANG_FORMAT_EXECUTABLE}"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Formatting tracked owned source with the Sparkle code-style policy..."
    USES_TERMINAL
    VERBATIM
)

# ---------------------------
# Default startup project for VS
# ---------------------------
# Prefer the editor-enabled Showcase launch target when it is present.
if(DISCOVERED_PROJECTS)
    set(STARTUP_PROJECT "")

    if(TARGET ShowcaseEditor)
        set(STARTUP_PROJECT "ShowcaseEditor")
    else()
        list(GET DISCOVERED_PROJECTS 0 FIRST_DISCOVERED_PROJECT)
        if(TARGET "${FIRST_DISCOVERED_PROJECT}Editor")
            set(STARTUP_PROJECT "${FIRST_DISCOVERED_PROJECT}Editor")
        elseif(TARGET "${FIRST_DISCOVERED_PROJECT}")
            set(STARTUP_PROJECT "${FIRST_DISCOVERED_PROJECT}")
        else()
            set(STARTUP_PROJECT "${FIRST_DISCOVERED_PROJECT}")
        endif()
    endif()

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${STARTUP_PROJECT})
    message(STATUS "VS startup project: ${STARTUP_PROJECT}")
else()
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SparkleEngine)
endif()
