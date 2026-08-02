cmake_minimum_required(VERSION 3.20)
project(Sparkle LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
enable_testing()

include(${CMAKE_SOURCE_DIR}/CMake/SparkleBuildProfiles.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/SparkleQtDiscovery.cmake)
include(${CMAKE_SOURCE_DIR}/CMake/SparkleArtifactContract.cmake)

# ---------------------------
# Optional: sanitizer support
# ---------------------------
option(ENABLE_SANITIZERS "Enable sanitizer instrumentation (requires clang/clang++)" OFF)
set(SANITIZER_TYPE "ASAN" CACHE STRING "Sanitizer to enable: ASAN, UBSAN, TSAN, MSAN, LSAN")

if(ENABLE_SANITIZERS)
    message(STATUS "Sanitizers enabled: ${SANITIZER_TYPE}")
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        if(SANITIZER_TYPE STREQUAL "ASAN")
            set(SAN_COMPILE_FLAGS "-fsanitize=address,leak -fno-omit-frame-pointer -g -O1")
            set(SAN_LINK_FLAGS "-fsanitize=address,leak")
        elseif(SANITIZER_TYPE STREQUAL "UBSAN")
            set(SAN_COMPILE_FLAGS "-fsanitize=undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer -g -O1")
            set(SAN_LINK_FLAGS "-fsanitize=undefined")
        elseif(SANITIZER_TYPE STREQUAL "TSAN")
            set(SAN_COMPILE_FLAGS "-fsanitize=thread -fno-omit-frame-pointer -g -O1")
            set(SAN_LINK_FLAGS "-fsanitize=thread")
        elseif(SANITIZER_TYPE STREQUAL "MSAN")
            set(SAN_COMPILE_FLAGS "-fsanitize=memory -fno-omit-frame-pointer -g -O1")
            set(SAN_LINK_FLAGS "-fsanitize=memory")
        elseif(SANITIZER_TYPE STREQUAL "LSAN")
            set(SAN_COMPILE_FLAGS "-fsanitize=leak -fno-omit-frame-pointer -g -O1")
            set(SAN_LINK_FLAGS "-fsanitize=leak")
        else()
            message(FATAL_ERROR "Unknown SANITIZER_TYPE: ${SANITIZER_TYPE}")
        endif()

        add_compile_options(${SAN_COMPILE_FLAGS})
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SAN_LINK_FLAGS}")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SAN_LINK_FLAGS}")
    else()
        message(WARNING "Sanitizers requested but compiler is not Clang/GNU. Behavior may be unsupported on this toolchain.")
    endif()
endif()

# ---------------------------
# Require C++20
# ---------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/Zc:__cplusplus)
endif()

function(sparkle_target_precompile_headers target_name)
    target_precompile_headers(${target_name} PRIVATE ${ARGN})

    if(MSVC AND CMAKE_GENERATOR MATCHES "^Visual Studio ")
        target_compile_options(${target_name}
            PRIVATE
                $<$<COMPILE_LANGUAGE:CXX>:/Fp${CMAKE_CURRENT_BINARY_DIR}/${target_name}.dir/$<CONFIG>/cmake_pch.pch>
        )
    endif()
endfunction()

# ---------------------------
# Optional: strict warnings
# ---------------------------
option(ENABLE_STRICT_WARNINGS "Enable strict compiler warnings and treat warnings as errors" OFF)
if(ENABLE_STRICT_WARNINGS)
    message(STATUS "Strict warnings enabled: adding compiler warning flags and Werror")
    if(MSVC)
        add_compile_options(/W4 /WX)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        add_compile_options(-Wall -Wextra -Wpedantic -Werror -Wconversion -Wsign-conversion -Wshadow -Wformat-security)
    else()
        message(WARNING "ENABLE_STRICT_WARNINGS requested but compiler is unsupported: ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif()

# ---------------------------
# Optional dependency groups
# ---------------------------
option(SPARKLE_ENABLE_CONTENT_PIPELINE "Build source import, conversion, and asset/texture cooking tools" ON)
option(SPARKLE_ENABLE_SHADER_COMPILER "Build offline shader compiler tooling" ON)
option(SPARKLE_ENABLE_KTX_SUPPORT "Fetch and build KTX tooling support for the content pipeline" OFF)
option(SPARKLE_ENABLE_NVIDIA_STREAMLINE "Fetch and stage NVIDIA Streamline SDK runtime for DLSS providers" ON)

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
    COMMENT "Checking Renderer/RHI architecture boundaries..."
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
