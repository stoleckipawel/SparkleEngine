# ============================================================================
# FetchDependencies.cmake — Unified Third-Party Dependency Management
# ============================================================================
#
# All third-party libraries are downloaded automatically at CMake configure
# time via FetchContent. Users just run cmake — no manual steps needed.
#
# Sources are cached in build/_deps/ (gitignored) and pinned to specific
# versions for reproducible builds.
#
# Managed dependencies:
#   - Dear ImGui     (v1.92.5)  — Immediate-mode GUI (DX12 + Win32 backends)
#   - cgltf          (v1.15)    — Single-header glTF 2.0 parser
#   - stb            (master)   — stb_image + stb_image_resize2 (header-only)
#   - Compressonator (master)   — AMD BC1-BC7 block compression (CMP_Core only)
#   - KTX-Software   (v4.3.2)  — KTX2 texture container read/write
#
# ============================================================================

include(FetchContent)

# Suppress the CMP0169 warning in CMake 4.x — we need FetchContent_Populate()
# because several deps have no usable top-level CMakeLists.txt.
if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

set(FETCHCONTENT_QUIET OFF)

# Skip Git LFS entirely — we only need source code, not test assets.
# Prevents multi-GB LFS pulls from compressonator and KTX repos.
# GIT_LFS_SKIP_SMUDGE prevents downloading LFS blobs.
# GIT_CONFIG_PARAMETERS overrides the LFS filter commands with empty strings
# so checkouts succeed even when git-lfs was previously configured but the
# binary is no longer in PATH (prevents "git-lfs: command not found").
# CMake's set(ENV{...} "") unsets variables, so GIT_CONFIG_COUNT can't set
# empty values — GIT_CONFIG_PARAMETERS uses a format that supports them.
set(ENV{GIT_LFS_SKIP_SMUDGE} 1)
set(ENV{GIT_CONFIG_PARAMETERS} "'filter.lfs.process=' 'filter.lfs.smudge=' 'filter.lfs.clean=' 'filter.lfs.required=false'")

message(STATUS "")
message(STATUS "=== Fetching Third-Party Dependencies ===")
message(STATUS "")
message(STATUS "  Total download: ~64 MB (shallow clones, LFS skipped)")
message(STATUS "  Expected time:  1-3 minutes depending on connection")
message(STATUS "")
message(STATUS "  Dependency sizes:")
message(STATUS "    imgui            ~7 MB")
message(STATUS "    cgltf            ~1 MB")
message(STATUS "    stb              ~5 MB")
message(STATUS "    Compressonator   ~5 MB  (sparse checkout — cmp_core only)")
message(STATUS "    KTX-Software    ~46 MB  (largest)")
message(STATUS "")

# ---------------------------------------------------------------------------
# Recovery: Remove partial/corrupt clones from interrupted downloads.
# If someone kills the process mid-clone, the src dir may exist but not be a
# valid git repo. FetchContent won't re-clone in that state — it tries an
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
foreach(_dep imgui cgltf stb ktx)
    set(_src_dir "${FETCHCONTENT_BASE_DIR}/${_dep}-src")
    set(_subbuild_dir "${FETCHCONTENT_BASE_DIR}/${_dep}-subbuild")
    if(EXISTS "${_src_dir}" AND NOT EXISTS "${_src_dir}/.git")
        message(WARNING "Corrupt/partial clone detected: ${_dep}-src (no .git directory). Removing for re-download...")
        file(REMOVE_RECURSE "${_src_dir}")
        file(REMOVE_RECURSE "${_subbuild_dir}")
    elseif(EXISTS "${_src_dir}/.git")
        # Valid clone exists — tell FetchContent to reuse it instead of re-cloning.
        # FetchContent creates empty FETCHCONTENT_SOURCE_DIR_<NAME> cache entries
        # by default, so we must check the value, not just DEFINED.
        string(TOUPPER "${_dep}" _dep_upper)
        if("${FETCHCONTENT_SOURCE_DIR_${_dep_upper}}" STREQUAL "")
            # Ensure checkout is complete (may have been interrupted by LFS errors).
            find_program(_git_exe git REQUIRED)
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
# Dear ImGui — Immediate-mode GUI library
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
message(STATUS "  [1/5] Fetching Dear ImGui v1.92.5 (~7 MB)...")
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
# cgltf — Single-header glTF 2.0 parser
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
message(STATUS "  [2/5] Fetching cgltf v1.15 (~1 MB)...")
FetchContent_Populate(cgltf)

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE ${cgltf_SOURCE_DIR})

# Silence common warnings in third-party C code when included
if(MSVC)
    target_compile_options(cgltf INTERFACE /wd4996)
endif()

message(STATUS "  cgltf:          ${cgltf_SOURCE_DIR} (~1 MB)")

# ============================================================================
# stb — Header-only image loading and resizing
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
message(STATUS "  [3/5] Fetching stb (~5 MB)...")
FetchContent_Populate(stb)

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

message(STATUS "  stb:            ${stb_SOURCE_DIR} (~5 MB)")

# ============================================================================
# AMD Compressonator — BC1-BC7 texture block compression
# https://github.com/GPUOpen-Tools/compressonator
#
# We build ONLY CMP_Core — the self-contained block compression library.
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
message(STATUS "  [4/5] Fetching Compressonator (sparse checkout, ~5 MB)...")

set(_comp_src "${FETCHCONTENT_BASE_DIR}/compressonator-src")

if(NOT EXISTS "${_comp_src}/cmp_core/source/cmp_core.cpp")
    # Fresh clone or incomplete checkout — (re)clone with sparse checkout.
    # --filter=blob:none  = partial clone (download trees only, fetch blobs on demand)
    # --sparse            = enable sparse checkout (only materialize listed paths)
    # --depth=1           = shallow (single commit, no history)
    file(REMOVE_RECURSE "${_comp_src}")
    find_program(_git_exe git REQUIRED)

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
    # SSE2 is default on x64 MSVC — no flag needed
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
# KTX-Software — KTX2 texture container read/write
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
message(STATUS "  [5/5] Fetching KTX-Software v4.3.2 (~46 MB) — largest dependency...")
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
# Restore LFS behavior
# ============================================================================
unset(ENV{GIT_LFS_SKIP_SMUDGE})
unset(ENV{GIT_CONFIG_PARAMETERS})

message(STATUS "")
message(STATUS "=== Third-Party Dependencies Ready ===")
message(STATUS "")
