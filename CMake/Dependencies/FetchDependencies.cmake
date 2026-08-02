# ============================================================================
# FetchDependencies.cmake - Unified Third-Party Dependency Management
# ============================================================================
#
# All third-party libraries are downloaded automatically at CMake configure
# time via FetchContent. Users just run cmake - no manual steps needed.
#
# Sources are cached in build/_deps/ (gitignored) and pinned to specific
# versions for reproducible builds.
#
# Managed dependencies:
#   Always-on:
#   - Dear ImGui     (v1.92.5)  - Immediate-mode GUI core + Win32 platform backend
#   - spdlog         (v1.14.1)  - Repo-wide logging backend (header-only)
#   - Font Awesome Free Solid (v6.7.1) - Editor/launcher icon font asset only
#   - NVIDIA Streamline SDK (v2.11.1) - DLSS headers, import library, and runtime DLLs
#
#   Optional content pipeline (SPARKLE_ENABLE_CONTENT_PIPELINE):
#   - cgltf          (v1.15)    - Single-header glTF 2.0 parser
#   - MikkTSpace     (pinned)   - Canonical glTF tangent-space generation
#   - stb            (master)   - stb_image + stb_image_resize2 (header-only)
#   - tinyexr        (v1.0.7)   - OpenEXR image loader (header-only)
#   - zlib           (v1.3.1)   - Compression backend for Assimp
#   - Assimp         (v5.4.3)   - FBX and general 3D asset import
#   - Compressonator (master)   - AMD BC1-BC7 block compression (CMP_Core only)
#
#   Optional KTX support (SPARKLE_ENABLE_KTX_SUPPORT):
#   - KTX-Software   (v4.3.2)   - KTX2 texture container read/write
#
#   Optional shader pipeline (SPARKLE_ENABLE_SHADER_COMPILER):
#   - SPIRV-Reflect  (vulkan-sdk-1.3.290.0) - SPIR-V reflection (offline tool only)
#
# ============================================================================

include(FetchContent)

# Suppress the CMP0169 warning in CMake 4.x - we need FetchContent_Populate()
# because several deps have no usable top-level CMakeLists.txt.
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

option(SPARKLE_VERBOSE_DEPENDENCIES "Show detailed source dependency configure output" OFF)
if(SPARKLE_VERBOSE_DEPENDENCIES)
    set(FETCHCONTENT_QUIET OFF)
else()
    set(FETCHCONTENT_QUIET ON)
endif()
option(SPARKLE_GIT_PROGRESS "Show raw Git object transfer progress while fetching third-party dependencies" OFF)
if(SPARKLE_GIT_PROGRESS)
    set(_sparkle_git_progress TRUE)
    set(_sparkle_git_progress_args --progress)
else()
    set(_sparkle_git_progress FALSE)
    set(_sparkle_git_progress_args)
endif()

function(sparkle_log_dependency_step step total name ref size purpose repository)
    if(SPARKLE_VERBOSE_DEPENDENCIES)
        message(STATUS "")
        message(STATUS "  [${step}/${total}] ${name} ${ref}")
        message(STATUS "        Purpose: ${purpose}")
        message(STATUS "        Source:  ${repository}")
        message(STATUS "        Expected download: ${size}")
    endif()
endfunction()

function(sparkle_log_dependency_ready name source size)
    if(SPARKLE_VERBOSE_DEPENDENCIES)
        message(STATUS "  ${name}: ${source} (${size})")
    endif()
endfunction()

function(sparkle_add_dependency_subdirectory source_dir binary_dir)
    if(DEFINED CMAKE_MESSAGE_LOG_LEVEL)
        set(_sparkle_previous_message_log_level "${CMAKE_MESSAGE_LOG_LEVEL}")
        set(_sparkle_had_message_log_level TRUE)
    else()
        set(_sparkle_had_message_log_level FALSE)
    endif()

    if(NOT SPARKLE_VERBOSE_DEPENDENCIES)
        set(CMAKE_MESSAGE_LOG_LEVEL WARNING)
    endif()

    if(ARGC GREATER 2 AND "${ARGV2}" STREQUAL "EXCLUDE_FROM_ALL")
        add_subdirectory(${source_dir} ${binary_dir} EXCLUDE_FROM_ALL)
    else()
        add_subdirectory(${source_dir} ${binary_dir})
    endif()

    if(_sparkle_had_message_log_level)
        set(CMAKE_MESSAGE_LOG_LEVEL "${_sparkle_previous_message_log_level}")
    else()
        unset(CMAKE_MESSAGE_LOG_LEVEL)
    endif()
endfunction()

# Skip Git LFS entirely - we only need source code, not test assets.
# Prevents multi-GB LFS pulls from compressonator and KTX repos.
# GIT_LFS_SKIP_SMUDGE prevents downloading LFS blobs.
# GIT_CONFIG_PARAMETERS overrides the LFS filter commands with empty strings
# so checkouts succeed even when git-lfs was previously configured but the
# binary is no longer in PATH (prevents "git-lfs: command not found").
# CMake's set(ENV{...} "") unsets variables, so GIT_CONFIG_COUNT can't set
# empty values - GIT_CONFIG_PARAMETERS uses a format that supports them.
set(ENV{GIT_LFS_SKIP_SMUDGE} 1)
set(ENV{GIT_CONFIG_PARAMETERS} "'filter.lfs.process=' 'filter.lfs.smudge=' 'filter.lfs.clean=' 'filter.lfs.required=false'")

# Locate Git once - used by FetchContent, recovery loops, and sparse checkout.
# Fresh Windows machines often have Git installed in the standard Program Files
# location without the current shell inheriting an updated PATH, so honor both
# CMake/launcher-provided overrides and common install roots before failing.
set(SPARKLE_GIT_EXE "" CACHE FILEPATH "Path to Git executable used by Sparkle dependency fetches.")
if(DEFINED GIT_EXECUTABLE AND NOT "${GIT_EXECUTABLE}" STREQUAL "" AND EXISTS "${GIT_EXECUTABLE}")
    set(_git_exe "${GIT_EXECUTABLE}")
elseif(NOT "${SPARKLE_GIT_EXE}" STREQUAL "" AND EXISTS "${SPARKLE_GIT_EXE}")
    set(_git_exe "${SPARKLE_GIT_EXE}")
elseif(DEFINED ENV{SPARKLE_GIT_EXE} AND NOT "$ENV{SPARKLE_GIT_EXE}" STREQUAL "" AND EXISTS "$ENV{SPARKLE_GIT_EXE}")
    set(_git_exe "$ENV{SPARKLE_GIT_EXE}")
else()
    find_program(_git_exe
        NAMES git git.exe
        PATHS
            "C:/Program Files/Git/cmd"
            "C:/Program Files/Git/bin"
            "C:/Program Files (x86)/Git/cmd"
            "C:/Program Files (x86)/Git/bin"
    )
endif()

if(NOT _git_exe OR NOT EXISTS "${_git_exe}")
    message(FATAL_ERROR
        "Git executable was not found. Install Git for Windows, add it to PATH, "
        "or configure with -DGIT_EXECUTABLE=<path-to-git.exe> / -DSPARKLE_GIT_EXE=<path-to-git.exe>.")
endif()
set(GIT_EXECUTABLE "${_git_exe}" CACHE FILEPATH "Git executable used by CMake/FetchContent." FORCE)
set(SPARKLE_GIT_EXE "${_git_exe}" CACHE FILEPATH "Path to Git executable used by Sparkle dependency fetches." FORCE)

if(SPARKLE_VERBOSE_DEPENDENCIES)
    message(STATUS "")
    message(STATUS "=== Third-Party Dependencies ===")
    message(STATUS "")
    message(STATUS "  Total download on fresh cache: ~314 MB with Streamline enabled (mostly shallow clones, LFS skipped)")
    message(STATUS "  Output mode: detailed dependency context")
    message(STATUS "  Git progress: ${SPARKLE_GIT_PROGRESS}")
    message(STATUS "")
else()
    message(STATUS "Third-party dependencies: checking cache (set SPARKLE_VERBOSE_DEPENDENCIES=ON for details)")
endif()

function(download_sparkle_editor_asset url output_path display_name)
    set(_needs_download TRUE)
    if(EXISTS "${output_path}")
        file(SIZE "${output_path}" _asset_size)
        if(_asset_size GREATER 0)
            set(_needs_download FALSE)
        endif()
    endif()

    if(_needs_download)
        get_filename_component(_asset_dir "${output_path}" DIRECTORY)
        file(MAKE_DIRECTORY "${_asset_dir}")
        message(STATUS "    Downloading ${display_name}...")
        file(DOWNLOAD
            "${url}"
            "${output_path}"
            STATUS _download_status
            TLS_VERIFY ON
            SHOW_PROGRESS
        )
        list(GET _download_status 0 _download_code)
        list(GET _download_status 1 _download_message)
        if(NOT _download_code EQUAL 0)
            file(REMOVE "${output_path}")
            message(FATAL_ERROR "Failed to download ${display_name}: ${_download_message}")
        endif()
    endif()
endfunction()

function(sparkle_collect_missing_relative_paths root_path out_missing_paths)
    set(missing_paths "")
    foreach(relative_path IN LISTS ARGN)
        if(NOT EXISTS "${root_path}/${relative_path}")
            list(APPEND missing_paths "${relative_path}")
        endif()
    endforeach()
    set(${out_missing_paths} "${missing_paths}" PARENT_SCOPE)
endfunction()

function(sparkle_prepare_git_fetchcontent_source dependency_name)
    set(options)
    set(oneValueArgs DISPLAY_NAME SOURCE_DIR BINARY_DIR SUBBUILD_DIR EXPECTED_REVISION)
    set(multiValueArgs REQUIRED_PATHS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if("${ARG_DISPLAY_NAME}" STREQUAL "")
        set(ARG_DISPLAY_NAME "${dependency_name}")
    endif()
    if("${ARG_SOURCE_DIR}" STREQUAL "")
        set(ARG_SOURCE_DIR "${FETCHCONTENT_BASE_DIR}/${dependency_name}-src")
    endif()
    if("${ARG_SUBBUILD_DIR}" STREQUAL "")
        set(ARG_SUBBUILD_DIR "${FETCHCONTENT_BASE_DIR}/${dependency_name}-subbuild")
    endif()

    string(TOUPPER "${dependency_name}" dependency_upper)
    set(fetchcontent_source_cache_key "FETCHCONTENT_SOURCE_DIR_${dependency_upper}")
    set(fetchcontent_source_cache_value "${${fetchcontent_source_cache_key}}")

    if(NOT EXISTS "${ARG_SOURCE_DIR}" AND NOT "${fetchcontent_source_cache_value}" STREQUAL "")
        if(SPARKLE_VERBOSE_DEPENDENCIES)
            message(STATUS "  Clearing stale FetchContent source override: ${dependency_name}")
        endif()
        set(${fetchcontent_source_cache_key} "" CACHE PATH "" FORCE)
        set(fetchcontent_source_cache_value "")
    endif()

    if(NOT EXISTS "${ARG_SOURCE_DIR}")
        return()
    endif()

    set(needs_recovery FALSE)
    set(recovery_reason "")

    if(NOT EXISTS "${ARG_SOURCE_DIR}/.git")
        set(needs_recovery TRUE)
        set(recovery_reason "no .git directory")
    else()
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --is-inside-work-tree
            WORKING_DIRECTORY "${ARG_SOURCE_DIR}"
            RESULT_VARIABLE git_rc
            OUTPUT_QUIET ERROR_QUIET
        )
        if(NOT git_rc EQUAL 0)
            set(needs_recovery TRUE)
            set(recovery_reason "git metadata is invalid")
        else()
            execute_process(
                COMMAND "${GIT_EXECUTABLE}" reset --hard HEAD
                WORKING_DIRECTORY "${ARG_SOURCE_DIR}"
                RESULT_VARIABLE git_reset_rc
                OUTPUT_QUIET ERROR_QUIET
            )
            if(NOT git_reset_rc EQUAL 0)
                set(needs_recovery TRUE)
                set(recovery_reason "git reset failed")
            endif()
        endif()
    endif()

    if(NOT needs_recovery AND NOT "${ARG_EXPECTED_REVISION}" STREQUAL "")
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse --verify "${ARG_EXPECTED_REVISION}^{commit}"
            WORKING_DIRECTORY "${ARG_SOURCE_DIR}"
            RESULT_VARIABLE expected_revision_rc
            OUTPUT_VARIABLE expected_revision
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        execute_process(
            COMMAND "${GIT_EXECUTABLE}" rev-parse HEAD
            WORKING_DIRECTORY "${ARG_SOURCE_DIR}"
            RESULT_VARIABLE current_revision_rc
            OUTPUT_VARIABLE current_revision
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET
        )
        if(NOT expected_revision_rc EQUAL 0 OR NOT current_revision_rc EQUAL 0 OR
           NOT current_revision STREQUAL expected_revision)
            set(needs_recovery TRUE)
            set(recovery_reason "revision does not match ${ARG_EXPECTED_REVISION}")
        endif()
    endif()

    if(NOT needs_recovery AND ARG_REQUIRED_PATHS)
        sparkle_collect_missing_relative_paths("${ARG_SOURCE_DIR}" missing_required_paths ${ARG_REQUIRED_PATHS})
        if(missing_required_paths)
            list(JOIN missing_required_paths ", " missing_required_paths_text)
            set(needs_recovery TRUE)
            set(recovery_reason "missing required paths: ${missing_required_paths_text}")
        endif()
    endif()

    if(needs_recovery)
        message(WARNING "Corrupt or incomplete ${ARG_DISPLAY_NAME} source cache detected (${recovery_reason}). Removing for re-download...")
        set(paths_to_remove "${ARG_SOURCE_DIR}" "${ARG_SUBBUILD_DIR}")
        if(NOT "${ARG_BINARY_DIR}" STREQUAL "")
            list(APPEND paths_to_remove "${ARG_BINARY_DIR}")
        endif()
        file(REMOVE_RECURSE ${paths_to_remove})
        set(${fetchcontent_source_cache_key} "" CACHE PATH "" FORCE)
        return()
    endif()

    if(SPARKLE_VERBOSE_DEPENDENCIES)
        message(STATUS "  Reusing existing clone: ${dependency_name}")
    endif()
    set(${fetchcontent_source_cache_key} "${ARG_SOURCE_DIR}" CACHE PATH "" FORCE)
endfunction()

function(sparkle_ensure_archive_dependency)
    set(options)
    set(oneValueArgs DISPLAY_NAME URL ARCHIVE_PATH ROOT_PATH EXPECTED_HASH)
    set(multiValueArgs REQUIRED_PATHS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    sparkle_collect_missing_relative_paths("${ARG_ROOT_PATH}" missing_paths ${ARG_REQUIRED_PATHS})
    if(missing_paths)
        file(MAKE_DIRECTORY "${FETCHCONTENT_BASE_DIR}")
        file(DOWNLOAD
            "${ARG_URL}"
            "${ARG_ARCHIVE_PATH}"
            EXPECTED_HASH "${ARG_EXPECTED_HASH}"
            STATUS download_status
            TLS_VERIFY ON
            SHOW_PROGRESS
        )
        list(GET download_status 0 download_code)
        list(GET download_status 1 download_message)
        if(NOT download_code EQUAL 0)
            file(REMOVE "${ARG_ARCHIVE_PATH}")
            message(FATAL_ERROR "Failed to download ${ARG_DISPLAY_NAME}: ${download_message}")
        endif()

        file(REMOVE_RECURSE "${ARG_ROOT_PATH}")
        file(MAKE_DIRECTORY "${ARG_ROOT_PATH}")
        file(ARCHIVE_EXTRACT INPUT "${ARG_ARCHIVE_PATH}" DESTINATION "${ARG_ROOT_PATH}")

        sparkle_collect_missing_relative_paths("${ARG_ROOT_PATH}" missing_paths ${ARG_REQUIRED_PATHS})
    endif()

    if(missing_paths)
        list(JOIN missing_paths ", " missing_paths_text)
        message(FATAL_ERROR "${ARG_DISPLAY_NAME} recovery is incomplete. Missing: ${missing_paths_text} under '${ARG_ROOT_PATH}'.")
    endif()
endfunction()

# ---------------------------------------------------------------------------
# Recovery: Remove partial/corrupt clones from interrupted downloads.
# If someone kills the process mid-clone, the src dir may exist but not be a
# valid git repo. FetchContent won't re-clone in that state - it tries an
# update step on a broken repo and fails with a confusing error.
# We detect this and nuke the broken directory + stamp files so the clone
# starts fresh.
#
# If the src dir IS a valid git repo (i.e., the clone completed successfully),
# we tell FetchContent to reuse it via FETCHCONTENT_SOURCE_DIR_<NAME>.
# This prevents FetchContent from trying to re-clone into an existing
# directory, which fails on Windows with "Error removing directory".
# ---------------------------------------------------------------------------
# Note: compressonator is handled separately below via sparse checkout.
foreach(_dep imgui cgltf mikktspace stb tinyexr spdlog zlib assimp ktx spirv_reflect)
    sparkle_prepare_git_fetchcontent_source("${_dep}" DISPLAY_NAME "${_dep}")
endforeach()

# ============================================================================
# Dear ImGui - Immediate-mode GUI library
# https://github.com/ocornut/imgui
#
# Target:  imgui (STATIC)
# Usage:   target_link_libraries(YourTarget PRIVATE imgui)
# ============================================================================
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        v1.92.5
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   ${_sparkle_git_progress}
)
sparkle_log_dependency_step(1 12 "Dear ImGui" "v1.92.5" "~7 MB" "Immediate-mode UI core and Win32 platform backend" "https://github.com/ocornut/imgui.git")
FetchContent_Populate(imgui)

add_library(imgui STATIC
    ${CMAKE_CURRENT_LIST_DIR}/ImGui/SparkleImGuiConfig.h
    ${CMAKE_CURRENT_LIST_DIR}/ImGui/SparkleImGuiContext.cpp
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    # Platform backend shared by editor/runtime UI. Renderer backends are owned by RHI implementations.
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

target_include_directories(imgui PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/ImGui
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    user32
    imm32
    winmm
)

target_compile_features(imgui PUBLIC cxx_std_20)

target_compile_definitions(imgui PUBLIC
    IMGUI_USER_CONFIG="SparkleImGuiConfig.h"
)

if(MSVC)
    target_compile_options(imgui PRIVATE /W0)
endif()

set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")
sparkle_log_dependency_ready("imgui" "${imgui_SOURCE_DIR}" "~7 MB")

if(SPARKLE_ENABLE_CONTENT_PIPELINE)
    # ============================================================================
    # cgltf - Single-header glTF 2.0 parser
    # https://github.com/jkuhlmann/cgltf
    #
    # Header-only library. Define CGLTF_IMPLEMENTATION in exactly one .cpp file
    # before including cgltf.h to generate the implementation.
    #
    # Target:  cgltf (INTERFACE)
    # Usage:   target_link_libraries(YourTarget PRIVATE cgltf)
    # ============================================================================
    FetchContent_Declare(cgltf
        GIT_REPOSITORY https://github.com/jkuhlmann/cgltf.git
        GIT_TAG        v1.15
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   ${_sparkle_git_progress}
    )
    sparkle_log_dependency_step(2 12 "cgltf" "v1.15" "~1 MB" "Single-header glTF 2.0 parser for source scene imports" "https://github.com/jkuhlmann/cgltf.git")
    FetchContent_Populate(cgltf)

    add_library(cgltf INTERFACE)
    target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})

    # Silence MSVC CRT deprecation warnings in cgltf implementation code.
    if(MSVC)
        target_compile_definitions(cgltf INTERFACE _CRT_SECURE_NO_WARNINGS)
        target_compile_options(cgltf INTERFACE /wd4996)
    endif()

    sparkle_log_dependency_ready("cgltf" "${cgltf_SOURCE_DIR}" "~1 MB")
endif()

if(SPARKLE_ENABLE_CONTENT_PIPELINE)
    # ============================================================================
    # MikkTSpace - canonical tangent-space generation used by glTF authoring tools
    # https://github.com/mmikk/MikkTSpace
    #
    # Target:  mikktspace (STATIC)
    # Usage:   target_link_libraries(YourTarget PRIVATE mikktspace)
    # ============================================================================
    FetchContent_Declare(mikktspace
        GIT_REPOSITORY https://github.com/mmikk/MikkTSpace.git
        GIT_TAG        3e895b49d05ea07e4c2133156cfa94369e19e409
        GIT_PROGRESS   ${_sparkle_git_progress}
    )
    sparkle_log_dependency_step(3 12 "MikkTSpace" "3e895b49" "<1 MB" "Canonical tangent generation for normal-mapped glTF imports" "https://github.com/mmikk/MikkTSpace.git")
    FetchContent_Populate(mikktspace)

    add_library(mikktspace STATIC
        ${mikktspace_SOURCE_DIR}/mikktspace.c
        ${mikktspace_SOURCE_DIR}/mikktspace.h
    )
    target_include_directories(mikktspace PUBLIC ${mikktspace_SOURCE_DIR})
    if(MSVC)
        target_compile_options(mikktspace PRIVATE /W0)
    endif()
    set_target_properties(mikktspace PROPERTIES FOLDER "ThirdParty")
    sparkle_log_dependency_ready("MikkTSpace" "${mikktspace_SOURCE_DIR}" "<1 MB")
endif()

if(SPARKLE_ENABLE_CONTENT_PIPELINE)
    # ============================================================================
    # stb - Header-only image loading and resizing
    # https://github.com/nothings/stb
    #
    # Provides: stb_image.h (image loading), stb_image_resize2.h (mip generation)
    #
    # Target:  stb (INTERFACE)
    # Usage:   target_link_libraries(YourTarget PRIVATE stb)
    #          #define STB_IMAGE_IMPLEMENTATION  (in exactly one .cpp)
    # ============================================================================
    FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG        master
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   ${_sparkle_git_progress}
    )
    sparkle_log_dependency_step(4 12 "stb" "master" "~5 MB" "Header-only image loading and mip resize helpers" "https://github.com/nothings/stb.git")
    FetchContent_Populate(stb)

    add_library(stb INTERFACE)
    target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

    sparkle_log_dependency_ready("stb" "${stb_SOURCE_DIR}" "~5 MB")

    # ============================================================================
    # tinyexr - Header-only OpenEXR image loader
    # https://github.com/syoyo/tinyexr
    #
    # Target:  tinyexr (INTERFACE)
    # Usage:   target_link_libraries(YourTarget PRIVATE tinyexr)
    #          #define TINYEXR_IMPLEMENTATION (in exactly one .cpp)
    # ============================================================================
    FetchContent_Declare(tinyexr
        GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
        GIT_TAG        v1.0.7
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   ${_sparkle_git_progress}
    )
    sparkle_log_dependency_step(5 12 "tinyexr" "v1.0.7" "~1 MB" "Header-only OpenEXR image loading support" "https://github.com/syoyo/tinyexr.git")
    FetchContent_Populate(tinyexr)

    add_library(tinyexr INTERFACE)
    target_include_directories(tinyexr INTERFACE
        ${tinyexr_SOURCE_DIR}
        ${tinyexr_SOURCE_DIR}/deps
        ${tinyexr_SOURCE_DIR}/deps/miniz
        ${tinyexr_SOURCE_DIR}/miniz
    )

    sparkle_log_dependency_ready("tinyexr" "${tinyexr_SOURCE_DIR}" "~1 MB")
endif()

# ============================================================================
# spdlog - Repo-owned logging backend
# https://github.com/gabime/spdlog
#
# SparkleCore owns logger bootstrap, named logger lifetime, and public
# spdlog-backed logging headers used by repo callsites.
#
# Target:  spdlog::spdlog_header_only
# Usage:   target_link_libraries(YourTarget PRIVATE spdlog::spdlog_header_only)
# ============================================================================
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   ${_sparkle_git_progress}
)
sparkle_log_dependency_step(6 12 "spdlog" "v1.14.1" "~3 MB" "Repo-wide logging backend" "https://github.com/gabime/spdlog.git")
FetchContent_Populate(spdlog)

set(SPDLOG_INSTALL OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_FMT_EXTERNAL OFF CACHE BOOL "" FORCE)

sparkle_add_dependency_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})

if(TARGET spdlog_header_only AND NOT TARGET spdlog::spdlog_header_only)
    add_library(spdlog::spdlog_header_only ALIAS spdlog_header_only)
endif()

if(TARGET spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER "ThirdParty")
    target_compile_definitions(spdlog PUBLIC FMT_CONSTEVAL=constexpr)
endif()

if(TARGET spdlog_header_only)
    set_target_properties(spdlog_header_only PROPERTIES FOLDER "ThirdParty")
    target_compile_definitions(spdlog_header_only INTERFACE FMT_CONSTEVAL=constexpr)
endif()

sparkle_log_dependency_ready("spdlog" "${spdlog_SOURCE_DIR}" "~3 MB")

if(SPARKLE_ENABLE_CONTENT_PIPELINE)
    # ============================================================================
    # zlib - Compression backend for Assimp
    # https://github.com/madler/zlib
    #
    # Assimp's bundled zlib is 1.2.13 and still uses K&R-style function
    # definitions that clang-cl warns about as non-prototype definitions.
    # Keep zlib as an explicit pinned dependency so Assimp links the modernized
    # 1.3.1 source instead of building its bundled copy.
    #
    # Target:  zlibstatic
    # Usage:   target_link_libraries(YourTarget PRIVATE zlibstatic)
    # ============================================================================
    FetchContent_Declare(zlib
        GIT_REPOSITORY https://github.com/madler/zlib.git
        GIT_TAG        v1.3.1
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   ${_sparkle_git_progress}
    )
    sparkle_log_dependency_step(7 12 "zlib" "v1.3.1" "~1 MB" "Compression backend used by Assimp" "https://github.com/madler/zlib.git")
    FetchContent_Populate(zlib)

    set(ZLIB_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    sparkle_add_dependency_subdirectory(${zlib_SOURCE_DIR} ${zlib_BINARY_DIR} EXCLUDE_FROM_ALL)

    if(TARGET zlibstatic AND NOT TARGET ZLIB::ZLIB)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    endif()

    if(TARGET zlib)
        set_target_properties(zlib PROPERTIES FOLDER "ThirdParty/zlib")
    endif()

    if(TARGET zlibstatic)
        set_target_properties(zlibstatic PROPERTIES FOLDER "ThirdParty/zlib")
    endif()

    set(ZLIB_FOUND TRUE)
    set(ZLIB_LIBRARIES zlibstatic)
    set(ZLIB_INCLUDE_DIR ${zlib_SOURCE_DIR} ${zlib_BINARY_DIR})
    set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})

    sparkle_log_dependency_ready("zlib" "${zlib_SOURCE_DIR}" "~1 MB")

    # ============================================================================
    # Assimp - Open Asset Import Library
    # https://github.com/assimp/assimp
    #
    # Provides FBX and other DCC format import through the GameFramework scene
    # import path. This remains a private dependency of GameFramework.
    #
    # Target:  assimp::assimp
    # Usage:   target_link_libraries(YourTarget PRIVATE assimp::assimp)
    # ============================================================================
    FetchContent_Declare(assimp
        GIT_REPOSITORY https://github.com/assimp/assimp.git
        GIT_TAG        v5.4.3
        GIT_SHALLOW    TRUE
        GIT_PROGRESS   ${_sparkle_git_progress}
    )
    sparkle_log_dependency_step(8 12 "Assimp" "v5.4.3" "~15 MB" "FBX and DCC scene import support" "https://github.com/assimp/assimp.git")
    FetchContent_Populate(assimp)

    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
    set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_FBX_IMPORTER ON CACHE BOOL "" FORCE)
    sparkle_add_dependency_subdirectory(${assimp_SOURCE_DIR} ${assimp_BINARY_DIR})

    if(TARGET assimp AND NOT TARGET assimp::assimp)
        add_library(assimp::assimp ALIAS assimp)
    endif()

    if(TARGET assimp)
        if(MSVC)
            target_compile_options(assimp PRIVATE /MP1 /FS)
        endif()
        set_target_properties(assimp PROPERTIES FOLDER "ThirdParty/Assimp")
    endif()

    sparkle_log_dependency_ready("assimp" "${assimp_SOURCE_DIR}" "~15 MB")
endif()

if(SPARKLE_ENABLE_CONTENT_PIPELINE)
    # ============================================================================
    # AMD Compressonator - BC1-BC7 texture block compression
    # https://github.com/GPUOpen-Tools/compressonator
    #
    # We build ONLY CMP_Core - the self-contained block compression library.
    # The full Compressonator project pollutes global CMake state with
    # add_compile_options(), global output dirs, etc. so we avoid their
    # top-level CMakeLists.txt and build CMP_Core from source ourselves.
    #
    # SPARSE CHECKOUT: The full repo is ~450 MB. We only need cmp_core/ and
    # applications/_libs/cmp_math/ (~5 MB). Using git's partial clone +
    # sparse checkout downloads only the trees and the blobs we need.
    #
    # Target:  CMP_Core (STATIC)
    # Usage:   target_link_libraries(YourTarget PRIVATE CMP_Core)
    #          #include "cmp_core.h"
    # API:     CompressBlockBC7(), DecompressBlockBC7(), etc.
    # ============================================================================
    sparkle_log_dependency_step(9 12 "Compressonator" "master (sparse)" "~5 MB" "AMD BC1-BC7 texture block compression; sparse checkout of cmp_core only" "https://github.com/GPUOpen-Tools/compressonator.git")

set(_comp_src "${FETCHCONTENT_BASE_DIR}/compressonator-src")

if(NOT EXISTS "${_comp_src}/cmp_core/source/cmp_core.cpp")
    # Fresh clone or incomplete checkout - (re)clone with sparse checkout.
    # --filter=blob:none  = partial clone (download trees only, fetch blobs on demand)
    # --sparse            = enable sparse checkout (only materialize listed paths)
    # --depth=1           = shallow (single commit, no history)
    file(REMOVE_RECURSE "${_comp_src}")

    if(SPARKLE_VERBOSE_DEPENDENCIES)
        message(STATUS "    Cloning (partial + sparse)...")
    endif()
    execute_process(
        COMMAND "${_git_exe}" clone
            --depth=1 --filter=blob:none --sparse ${_sparkle_git_progress_args}
            https://github.com/GPUOpen-Tools/compressonator.git
            "${_comp_src}"
        RESULT_VARIABLE _rc
    )
    if(_rc)
        message(FATAL_ERROR "Failed to clone compressonator (exit code ${_rc})")
    endif()

    if(SPARKLE_VERBOSE_DEPENDENCIES)
        message(STATUS "    Setting sparse checkout paths: cmp_core, applications/_libs/cmp_math")
    endif()
    execute_process(
        COMMAND "${_git_exe}" sparse-checkout set
            cmp_core
            applications/_libs/cmp_math
        WORKING_DIRECTORY "${_comp_src}"
        RESULT_VARIABLE _rc
    )
    if(_rc)
        message(FATAL_ERROR "Failed to set sparse checkout for compressonator (exit code ${_rc})")
    endif()
else()
    if(SPARKLE_VERBOSE_DEPENDENCIES)
        message(STATUS "    Reusing existing sparse clone")
    endif()
endif()

set(compressonator_SOURCE_DIR "${_comp_src}")

# --- CMP_Core: main block compression library ---
add_library(CMP_Core STATIC
    ${compressonator_SOURCE_DIR}/cmp_core/shaders/bc1_encode_kernel.cpp
    ${compressonator_SOURCE_DIR}/cmp_core/shaders/bc2_encode_kernel.cpp
    ${compressonator_SOURCE_DIR}/cmp_core/shaders/bc3_encode_kernel.cpp
    ${compressonator_SOURCE_DIR}/cmp_core/shaders/bc4_encode_kernel.cpp
    ${compressonator_SOURCE_DIR}/cmp_core/shaders/bc5_encode_kernel.cpp
    ${compressonator_SOURCE_DIR}/cmp_core/shaders/bc6_encode_kernel.cpp
    ${compressonator_SOURCE_DIR}/cmp_core/shaders/bc7_encode_kernel.cpp
    ${compressonator_SOURCE_DIR}/cmp_core/source/cmp_core.cpp
    ${compressonator_SOURCE_DIR}/applications/_libs/cmp_math/cpu_extensions.cpp
    ${compressonator_SOURCE_DIR}/applications/_libs/cmp_math/cmp_math_common.cpp
)

target_include_directories(CMP_Core PUBLIC
    ${compressonator_SOURCE_DIR}/cmp_core/source
    ${compressonator_SOURCE_DIR}/cmp_core/shaders
    ${compressonator_SOURCE_DIR}/applications/_libs/cmp_math
)

# --- SIMD acceleration targets (SSE2, AVX2, AVX-512) ---
# Each variant is compiled with its own arch flags, then linked into CMP_Core.

add_library(CMP_Core_SSE STATIC
    ${compressonator_SOURCE_DIR}/cmp_core/source/core_simd_sse.cpp
)
target_include_directories(CMP_Core_SSE PRIVATE
    ${compressonator_SOURCE_DIR}/cmp_core/source
    ${compressonator_SOURCE_DIR}/cmp_core/shaders
    ${compressonator_SOURCE_DIR}/applications/_libs/cmp_math
)

add_library(CMP_Core_AVX STATIC
    ${compressonator_SOURCE_DIR}/cmp_core/source/core_simd_avx.cpp
)
target_include_directories(CMP_Core_AVX PRIVATE
    ${compressonator_SOURCE_DIR}/cmp_core/source
    ${compressonator_SOURCE_DIR}/cmp_core/shaders
    ${compressonator_SOURCE_DIR}/applications/_libs/cmp_math
)

add_library(CMP_Core_AVX512 STATIC
    ${compressonator_SOURCE_DIR}/cmp_core/source/core_simd_avx512.cpp
)
target_include_directories(CMP_Core_AVX512 PRIVATE
    ${compressonator_SOURCE_DIR}/cmp_core/source
    ${compressonator_SOURCE_DIR}/cmp_core/shaders
    ${compressonator_SOURCE_DIR}/applications/_libs/cmp_math
)

# Architecture-specific compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(CMP_Core        PRIVATE -msse4.1)
    target_compile_options(CMP_Core_SSE    PRIVATE -msse4.1)
    target_compile_options(CMP_Core_AVX    PRIVATE -mavx2)
    target_compile_options(CMP_Core_AVX512 PRIVATE -mavx512f)
elseif(MSVC)
    # SSE2 is default on x64 MSVC - no flag needed
    target_compile_options(CMP_Core_AVX    PRIVATE /arch:AVX2)
    target_compile_options(CMP_Core_AVX512 PRIVATE /arch:AVX512)
endif()

target_link_libraries(CMP_Core PRIVATE CMP_Core_SSE CMP_Core_AVX CMP_Core_AVX512)

# Silence warnings in third-party code
if(MSVC)
    target_compile_options(CMP_Core        PRIVATE /W0)
    target_compile_options(CMP_Core_SSE    PRIVATE /W0)
    target_compile_options(CMP_Core_AVX    PRIVATE /W0)
    target_compile_options(CMP_Core_AVX512 PRIVATE /W0)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(CMP_Core        PRIVATE -w)
    target_compile_options(CMP_Core_SSE    PRIVATE -w)
    target_compile_options(CMP_Core_AVX    PRIVATE -w)
    target_compile_options(CMP_Core_AVX512 PRIVATE -w)
endif()

set_target_properties(CMP_Core CMP_Core_SSE CMP_Core_AVX CMP_Core_AVX512
    PROPERTIES FOLDER "ThirdParty/Compressonator"
)

    sparkle_log_dependency_ready("Compressonator" "${compressonator_SOURCE_DIR}/cmp_core" "~5 MB, sparse")
endif()

if(SPARKLE_ENABLE_KTX_SUPPORT)
    # ============================================================================
    # KTX-Software - KTX2 texture container read/write
    # https://github.com/KhronosGroup/KTX-Software
    #
    # v4.3.2 builds the ktx target from its root CMakeLists.txt. We disable
    # tests, tools, docs, and JNI/Python bindings to keep the build minimal.
    # We also skip the CTS git submodule (tests/cts) to avoid an extra clone.
    #
    # Target:  ktx (STATIC)
    # Usage:   target_link_libraries(YourTarget PRIVATE ktx)
    #          #include <ktx.h>
    # ============================================================================
    FetchContent_Declare(ktx
    GIT_REPOSITORY https://github.com/KhronosGroup/KTX-Software.git
    GIT_TAG        v4.3.2
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   ${_sparkle_git_progress}
    GIT_SUBMODULES ""
)
sparkle_log_dependency_step(10 12 "KTX-Software" "v4.3.2" "~46 MB (largest dependency)" "KTX2 texture container read/write, with tests/tools/docs/JNI/Python disabled" "https://github.com/KhronosGroup/KTX-Software.git")
FetchContent_Populate(ktx)

# Disable features we don't need
set(KTX_FEATURE_STATIC_LIBRARY ON  CACHE BOOL "" FORCE)
set(KTX_FEATURE_TESTS    OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_DOC      OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_TOOLS    OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_JNI      OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_PY       OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_LOADTEST_APPS "" CACHE STRING "" FORCE)
set(KTX_FEATURE_VK_UPLOAD OFF CACHE BOOL "" FORCE)
set(KTX_FEATURE_GL_UPLOAD OFF CACHE BOOL "" FORCE)

# CMake 4.2 can reach CTestTargets before the fetched binary tree has created
# its CMakeFiles directory on repeated configure passes. Seed the sub-build
# directories explicitly so the KTX configure remains stable during cook flows.
file(MAKE_DIRECTORY "${ktx_BINARY_DIR}")
file(MAKE_DIRECTORY "${ktx_BINARY_DIR}/CMakeFiles")

# Build from root CMakeLists.txt (v4.3.2 has no lib/CMakeLists.txt)
sparkle_add_dependency_subdirectory(${ktx_SOURCE_DIR} ${ktx_BINARY_DIR})

if(TARGET ktx)
    set_target_properties(ktx PROPERTIES FOLDER "ThirdParty/KTX")
endif()
if(TARGET ktx_read)
    set_target_properties(ktx_read PROPERTIES FOLDER "ThirdParty/KTX")
endif()

    sparkle_log_dependency_ready("KTX-Software" "${ktx_SOURCE_DIR}" "~46 MB")
endif()

if(SPARKLE_ENABLE_SHADER_COMPILER)
    # ============================================================================
    # SPIRV-Reflect - SPIR-V reflection (Khronos)
    # https://github.com/KhronosGroup/SPIRV-Reflect
    #
    # Used by Tools/Shaders/ShaderCompiler/Backends/Dxc to extract reflection from the
    # SPIR-V blobs DXC emits in `-spirv` mode. Single .c + headers; we build it
    # as a small static library so its symbols stay out of every translation
    # unit that includes the header.
    #
    # Target:  spirv_reflect (STATIC)
    # Usage:   target_link_libraries(YourTarget PRIVATE spirv_reflect)
    #          #include <spirv_reflect.h>
    # ============================================================================
    FetchContent_Declare(spirv_reflect
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Reflect.git
    GIT_TAG        vulkan-sdk-1.3.290.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   ${_sparkle_git_progress}
)
sparkle_log_dependency_step(11 12 "SPIRV-Reflect" "vulkan-sdk-1.3.290.0" "~2 MB" "SPIR-V reflection for offline shader compiler backends" "https://github.com/KhronosGroup/SPIRV-Reflect.git")
FetchContent_Populate(spirv_reflect)

add_library(spirv_reflect STATIC
    ${spirv_reflect_SOURCE_DIR}/spirv_reflect.c
    ${spirv_reflect_SOURCE_DIR}/spirv_reflect.h
)

target_include_directories(spirv_reflect PUBLIC
    ${spirv_reflect_SOURCE_DIR}
    ${spirv_reflect_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(spirv_reflect PRIVATE /W0)
endif()

    set_target_properties(spirv_reflect PROPERTIES FOLDER "ThirdParty")
    sparkle_log_dependency_ready("SPIRV-Reflect" "${spirv_reflect_SOURCE_DIR}" "~2 MB")
endif()

# ============================================================================
# Font Awesome Free Solid - Editor icon font asset only
# https://github.com/FortAwesome/Font-Awesome
#
# We intentionally download only the solid TTF and license instead of adding a
# code dependency or cloning the full icon repository. Sparkle owns the small
# semantic icon mapping in editor code.
# ============================================================================
sparkle_log_dependency_step(12 12 "Font Awesome Free Solid" "v6.7.1" "~0.5 MB" "Editor icon font asset and license only" "https://github.com/FortAwesome/Font-Awesome")

set(_sparkle_editor_icons_dir "${FETCHCONTENT_BASE_DIR}/editor-icons/fontawesome-6.7.1")
set(SPARKLE_FONT_AWESOME_SOLID_TTF "${_sparkle_editor_icons_dir}/fa-solid-900.ttf" CACHE FILEPATH "Font Awesome Free Solid TTF path" FORCE)
set(SPARKLE_FONT_AWESOME_LICENSE "${_sparkle_editor_icons_dir}/LICENSE.txt" CACHE FILEPATH "Font Awesome Free license path" FORCE)

download_sparkle_editor_asset(
    "https://raw.githubusercontent.com/FortAwesome/Font-Awesome/6.7.1/webfonts/fa-solid-900.ttf"
    "${SPARKLE_FONT_AWESOME_SOLID_TTF}"
    "Font Awesome Free Solid TTF"
)
download_sparkle_editor_asset(
    "https://raw.githubusercontent.com/FortAwesome/Font-Awesome/6.7.1/LICENSE.txt"
    "${SPARKLE_FONT_AWESOME_LICENSE}"
    "Font Awesome Free license"
)

sparkle_log_dependency_ready("Font Awesome" "${SPARKLE_FONT_AWESOME_SOLID_TTF}" "~0.5 MB asset-only")

# ============================================================================
# NVIDIA Streamline SDK - DLSS provider runtime
# https://github.com/NVIDIA-RTX/Streamline
#
# The SDK release zip contains headers, the Streamline interposer import
# library, signed Streamline plugin DLLs, and NVIDIA DLSS runtime DLLs. Keep it
# out of source and cache it under build/_deps so fresh syncs are reproducible.
# ============================================================================
if(SPARKLE_ENABLE_NVIDIA_STREAMLINE)
    sparkle_log_dependency_step(13 13 "NVIDIA Streamline SDK" "v2.11.1" "~217 MB" "DLSS external provider SDK headers, import library, and runtime DLLs" "https://github.com/NVIDIA-RTX/Streamline/releases")

    set(_sparkle_streamline_url "https://github.com/NVIDIA-RTX/Streamline/releases/download/v2.11.1/streamline-sdk-v2.11.1.zip")
    set(_sparkle_streamline_zip "${FETCHCONTENT_BASE_DIR}/streamline-sdk-v2.11.1.zip")
    set(_sparkle_streamline_root "${FETCHCONTENT_BASE_DIR}/streamline-sdk-src")
    set(_sparkle_streamline_required_paths
        "include/sl.h"
        "lib/x64/sl.interposer.lib"
        "bin/x64/sl.interposer.dll"
        "bin/x64/sl.common.dll"
        "bin/x64/sl.dlss.dll"
        "bin/x64/sl.dlss_d.dll"
        "bin/x64/sl.pcl.dll"
        "bin/x64/sl.reflex.dll"
        "bin/x64/nvngx_dlss.dll"
        "bin/x64/nvngx_dlssd.dll"
    )

    sparkle_ensure_archive_dependency(
        DISPLAY_NAME "NVIDIA Streamline SDK"
        URL "${_sparkle_streamline_url}"
        ARCHIVE_PATH "${_sparkle_streamline_zip}"
        ROOT_PATH "${_sparkle_streamline_root}"
        EXPECTED_HASH "SHA256=0C1D562E59557434CABFB8997157CB8C04FC7D23F077C8BDF5260975B73DFB89"
        REQUIRED_PATHS ${_sparkle_streamline_required_paths}
    )

    set(SPARKLE_NVIDIA_STREAMLINE_ROOT "${_sparkle_streamline_root}" CACHE PATH "NVIDIA Streamline SDK root used by Sparkle DLSS integration." FORCE)
    set(SPARKLE_NVIDIA_STREAMLINE_BIN_DIR "${_sparkle_streamline_root}/bin/x64" CACHE PATH "NVIDIA Streamline runtime DLL directory." FORCE)
    set(SPARKLE_NVIDIA_STREAMLINE_INCLUDE_DIR "${_sparkle_streamline_root}/include" CACHE PATH "NVIDIA Streamline include directory." FORCE)
    set(SPARKLE_NVIDIA_STREAMLINE_INTERPOSER_LIB "${_sparkle_streamline_root}/lib/x64/sl.interposer.lib" CACHE FILEPATH "NVIDIA Streamline interposer import library." FORCE)

    if(NOT TARGET NVIDIA::Streamline)
        add_library(NVIDIA::Streamline INTERFACE IMPORTED GLOBAL)
        set_target_properties(NVIDIA::Streamline PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${SPARKLE_NVIDIA_STREAMLINE_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES "${SPARKLE_NVIDIA_STREAMLINE_INTERPOSER_LIB}"
        )
    endif()

    set(SPARKLE_NVIDIA_STREAMLINE_RUNTIME_DLLS
        "${SPARKLE_NVIDIA_STREAMLINE_BIN_DIR}/sl.interposer.dll"
        "${SPARKLE_NVIDIA_STREAMLINE_BIN_DIR}/sl.common.dll"
        "${SPARKLE_NVIDIA_STREAMLINE_BIN_DIR}/sl.dlss.dll"
        "${SPARKLE_NVIDIA_STREAMLINE_BIN_DIR}/sl.dlss_d.dll"
        "${SPARKLE_NVIDIA_STREAMLINE_BIN_DIR}/sl.pcl.dll"
        "${SPARKLE_NVIDIA_STREAMLINE_BIN_DIR}/sl.reflex.dll"
        "${SPARKLE_NVIDIA_STREAMLINE_BIN_DIR}/nvngx_dlss.dll"
        "${SPARKLE_NVIDIA_STREAMLINE_BIN_DIR}/nvngx_dlssd.dll"
        CACHE STRING "NVIDIA Streamline DLSS SR and Ray Reconstruction runtime DLLs staged beside Sparkle products."
        FORCE
    )

    sparkle_log_dependency_ready("NVIDIA Streamline SDK" "${SPARKLE_NVIDIA_STREAMLINE_ROOT}" "~217 MB release SDK")
endif()

# ============================================================================
# Restore LFS behavior
# ============================================================================
unset(ENV{GIT_LFS_SKIP_SMUDGE})
unset(ENV{GIT_CONFIG_PARAMETERS})

if(SPARKLE_VERBOSE_DEPENDENCIES)
    message(STATUS "")
    message(STATUS "=== Third-Party Dependencies Ready ===")
    message(STATUS "")
else()
    message(STATUS "Third-party dependencies: ready")
endif()
