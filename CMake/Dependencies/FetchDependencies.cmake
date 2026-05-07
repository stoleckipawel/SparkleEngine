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
#   - Dear ImGui     (v1.92.5)  - Immediate-mode GUI (DX12 + Win32 backends)
#   - cgltf          (v1.15)    - Single-header glTF 2.0 parser
#   - stb            (master)   - stb_image + stb_image_resize2 (header-only)
#   - tinyexr        (v1.0.7)   - OpenEXR image loader (header-only)
#   - spdlog         (v1.14.1)  - Repo-wide logging backend (header-only)
#   - Assimp         (v5.4.3)   - FBX and general 3D asset import
#   - Compressonator (master)   - AMD BC1-BC7 block compression (CMP_Core only)
#   - KTX-Software   (v4.3.2)  - KTX2 texture container read/write
#   - SPIRV-Reflect  (vulkan-sdk-1.3.290.0) - SPIR-V reflection (offline tool only)
#   - Font Awesome Free Solid (v6.7.1) - Editor icon font asset only
#
# ============================================================================

include(FetchContent)

# Suppress the CMP0169 warning in CMake 4.x - we need FetchContent_Populate()
# because several deps have no usable top-level CMakeLists.txt.
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

set(FETCHCONTENT_QUIET OFF)

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

# Locate git once - used by recovery loop and compressonator sparse checkout.
find_program(_git_exe git REQUIRED)

message(STATUS "")
message(STATUS "=== Fetching Third-Party Dependencies ===")
message(STATUS "")
message(STATUS "  Total download: ~86 MB (shallow clones, LFS skipped)")
message(STATUS "  Expected time:  1-3 minutes depending on connection")
message(STATUS "")
message(STATUS "  Dependency sizes:")
message(STATUS "    imgui            ~7 MB")
message(STATUS "    cgltf            ~1 MB")
message(STATUS "    stb              ~5 MB")
message(STATUS "    tinyexr          ~1 MB")
message(STATUS "    spdlog           ~3 MB")
message(STATUS "    assimp          ~15 MB")
message(STATUS "    Compressonator   ~5 MB  (sparse checkout - cmp_core only)")
message(STATUS "    KTX-Software    ~46 MB  (largest)")
message(STATUS "    SPIRV-Reflect   ~2 MB")
message(STATUS "    Editor Icons   ~0.5 MB  (Font Awesome Solid font asset only)")
message(STATUS "")

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
foreach(_dep imgui cgltf stb tinyexr spdlog assimp ktx spirv_reflect)
    set(_src_dir "${FETCHCONTENT_BASE_DIR}/${_dep}-src")
    set(_subbuild_dir "${FETCHCONTENT_BASE_DIR}/${_dep}-subbuild")
    if(EXISTS "${_src_dir}" AND NOT EXISTS "${_src_dir}/.git")
        message(WARNING "Corrupt/partial clone detected: ${_dep}-src (no .git directory). Removing for re-download...")
        file(REMOVE_RECURSE "${_src_dir}")
        file(REMOVE_RECURSE "${_subbuild_dir}")
    elseif(EXISTS "${_src_dir}/.git")
        # Valid clone exists - tell FetchContent to reuse it instead of re-cloning.
        # FetchContent creates empty FETCHCONTENT_SOURCE_DIR_<NAME> cache entries
        # by default, so we must check the value, not just DEFINED.
        string(TOUPPER "${_dep}" _dep_upper)
        if("${FETCHCONTENT_SOURCE_DIR_${_dep_upper}}" STREQUAL "")
            # Ensure checkout is complete (may have been interrupted by LFS errors).
            execute_process(
                COMMAND "${_git_exe}" reset --hard HEAD
                WORKING_DIRECTORY "${_src_dir}"
                RESULT_VARIABLE _git_rc
                OUTPUT_QUIET ERROR_QUIET
            )
            message(STATUS "  Reusing existing clone: ${_dep}-src")
            set(FETCHCONTENT_SOURCE_DIR_${_dep_upper} "${_src_dir}" CACHE PATH "" FORCE)
        endif()
    endif()
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
    GIT_PROGRESS   TRUE
)
message(STATUS "  [1/9] Fetching Dear ImGui v1.92.5 (~7 MB)...")
FetchContent_Populate(imgui)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    # DX12 + Win32 backends only (the full repo has many more)
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx12.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    d3d12
    dxgi
    dxguid
    user32
    imm32
    winmm
)

target_compile_features(imgui PUBLIC cxx_std_20)

if(MSVC)
    target_compile_options(imgui PRIVATE /W0)
endif()

set_target_properties(imgui PROPERTIES FOLDER "ThirdParty")
message(STATUS "  imgui:          ${imgui_SOURCE_DIR} (~7 MB)")

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
    GIT_PROGRESS   TRUE
)
message(STATUS "  [2/9] Fetching cgltf v1.15 (~1 MB)...")
FetchContent_Populate(cgltf)

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})

# Silence MSVC CRT deprecation warnings in cgltf implementation code.
if(MSVC)
    target_compile_definitions(cgltf INTERFACE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(cgltf INTERFACE /wd4996)
endif()

message(STATUS "  cgltf:          ${cgltf_SOURCE_DIR} (~1 MB)")

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
    GIT_PROGRESS   TRUE
)
message(STATUS "  [3/10] Fetching stb (~5 MB)...")
FetchContent_Populate(stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

message(STATUS "  stb:            ${stb_SOURCE_DIR} (~5 MB)")

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
    GIT_PROGRESS   TRUE
)
message(STATUS "  [4/10] Fetching tinyexr v1.0.7 (~1 MB)...")
FetchContent_Populate(tinyexr)

add_library(tinyexr INTERFACE)
target_include_directories(tinyexr INTERFACE
    ${tinyexr_SOURCE_DIR}
    ${tinyexr_SOURCE_DIR}/deps
    ${tinyexr_SOURCE_DIR}/deps/miniz
    ${tinyexr_SOURCE_DIR}/miniz
)

message(STATUS "  tinyexr:        ${tinyexr_SOURCE_DIR} (~1 MB)")

# ============================================================================
# spdlog - Repo-owned logging backend
# https://github.com/gabime/spdlog
#
# SparkleCore owns logger bootstrap and named logger lifetime, while repo
# callsites gradually migrate toward direct spdlog usage after logger lookup.
# That means SparkleCore now exposes spdlog-backed public logging headers.
#
# Target:  spdlog::spdlog_header_only
# Usage:   target_link_libraries(YourTarget PRIVATE spdlog::spdlog_header_only)
# ============================================================================
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG        v1.14.1
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
message(STATUS "  [5/10] Fetching spdlog v1.14.1 (~3 MB)...")
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

add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})

if(TARGET spdlog_header_only AND NOT TARGET spdlog::spdlog_header_only)
    add_library(spdlog::spdlog_header_only ALIAS spdlog_header_only)
endif()

if(TARGET spdlog)
    set_target_properties(spdlog PROPERTIES FOLDER "ThirdParty")
endif()

if(TARGET spdlog_header_only)
    set_target_properties(spdlog_header_only PROPERTIES FOLDER "ThirdParty")
endif()

message(STATUS "  spdlog:         ${spdlog_SOURCE_DIR} (~3 MB)")

# ============================================================================
# Assimp - Open Asset Import Library
# https://github.com/assimp/assimp
#
# Provides FBX and other DCC format import for the transitional runtime path.
# This remains a private dependency of GameFramework and should move to the
# converter tool in the final pipeline.
#
# Target:  assimp::assimp
# Usage:   target_link_libraries(YourTarget PRIVATE assimp::assimp)
# ============================================================================
FetchContent_Declare(assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG        v5.4.3
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
message(STATUS "  [5/9] Fetching Assimp v5.4.3 (~15 MB)...")
FetchContent_Populate(assimp)

set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
add_subdirectory(${assimp_SOURCE_DIR} ${assimp_BINARY_DIR})

if(TARGET assimp AND NOT TARGET assimp::assimp)
    add_library(assimp::assimp ALIAS assimp)
endif()

if(TARGET assimp)
    set_target_properties(assimp PROPERTIES FOLDER "ThirdParty/Assimp")
endif()

message(STATUS "  assimp:         ${assimp_SOURCE_DIR} (~15 MB)")

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
message(STATUS "")
message(STATUS "  [6/9] Fetching Compressonator (sparse checkout, ~5 MB)...")

set(_comp_src "${FETCHCONTENT_BASE_DIR}/compressonator-src")

if(NOT EXISTS "${_comp_src}/cmp_core/source/cmp_core.cpp")
    # Fresh clone or incomplete checkout - (re)clone with sparse checkout.
    # --filter=blob:none  = partial clone (download trees only, fetch blobs on demand)
    # --sparse            = enable sparse checkout (only materialize listed paths)
    # --depth=1           = shallow (single commit, no history)
    file(REMOVE_RECURSE "${_comp_src}")

    message(STATUS "    Cloning (partial + sparse)...")
    execute_process(
        COMMAND "${_git_exe}" clone
            --depth=1 --filter=blob:none --sparse --progress
            https://github.com/GPUOpen-Tools/compressonator.git
            "${_comp_src}"
        RESULT_VARIABLE _rc
    )
    if(_rc)
        message(FATAL_ERROR "Failed to clone compressonator (exit code ${_rc})")
    endif()

    message(STATUS "    Setting sparse checkout paths: cmp_core, applications/_libs/cmp_math")
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
    message(STATUS "    Reusing existing sparse clone")
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
if(MSVC)
    # SSE2 is default on x64 MSVC - no flag needed
    target_compile_options(CMP_Core_AVX    PRIVATE /arch:AVX2)
    target_compile_options(CMP_Core_AVX512 PRIVATE /arch:AVX512)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(CMP_Core_SSE    PRIVATE -msse2)
    target_compile_options(CMP_Core_AVX    PRIVATE -mavx2)
    target_compile_options(CMP_Core_AVX512 PRIVATE -mavx512f)
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

message(STATUS "  Compressonator: ${compressonator_SOURCE_DIR}/cmp_core (~5 MB, sparse)")

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
    GIT_PROGRESS   TRUE
    GIT_SUBMODULES ""
)
message(STATUS "")
message(STATUS "  [7/9] Fetching KTX-Software v4.3.2 (~46 MB) - largest dependency...")
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
add_subdirectory(${ktx_SOURCE_DIR} ${ktx_BINARY_DIR})

if(TARGET ktx)
    set_target_properties(ktx PROPERTIES FOLDER "ThirdParty/KTX")
endif()
if(TARGET ktx_read)
    set_target_properties(ktx_read PROPERTIES FOLDER "ThirdParty/KTX")
endif()

message(STATUS "  KTX-Software:   ${ktx_SOURCE_DIR} (~46 MB)")

# ============================================================================
# SPIRV-Reflect - SPIR-V reflection (Khronos)
# https://github.com/KhronosGroup/SPIRV-Reflect
#
# Used by Tools/ShaderCompiler/Backends/Dxc to extract reflection from the
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
    GIT_PROGRESS   TRUE
)
message(STATUS "  [8/9] Fetching SPIRV-Reflect vulkan-sdk-1.3.290.0 (~2 MB)...")
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
message(STATUS "  SPIRV-Reflect:  ${spirv_reflect_SOURCE_DIR} (~2 MB)")

# ============================================================================
# Font Awesome Free Solid - Editor icon font asset only
# https://github.com/FortAwesome/Font-Awesome
#
# We intentionally download only the solid TTF and license instead of adding a
# code dependency or cloning the full icon repository. Sparkle owns the small
# semantic icon mapping in editor code.
# ============================================================================
message(STATUS "")
message(STATUS "  [9/9] Fetching Font Awesome Free Solid v6.7.1 icon assets (~0.5 MB)...")

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

message(STATUS "  Font Awesome:   ${SPARKLE_FONT_AWESOME_SOLID_TTF} (~0.5 MB asset-only)")

# ============================================================================
# Restore LFS behavior
# ============================================================================
unset(ENV{GIT_LFS_SKIP_SMUDGE})
unset(ENV{GIT_CONFIG_PARAMETERS})

message(STATUS "")
message(STATUS "=== Third-Party Dependencies Ready ===")
message(STATUS "")