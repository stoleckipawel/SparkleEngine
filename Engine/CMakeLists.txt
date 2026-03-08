# Engine CMake configuration
# Layered DLL Architecture (Unreal-style)
#
# Module Dependency Hierarchy (strict layering - each layer only depends on layers below):
#
#   ┌─────────────────────────────────────────────────────────────┐
#   │                 SparkleApplication.lib                      │
#   │              (App bootstrap and main loop)                  │
#   └─────────────────────────────────────────────────────────────┘
#                               │
#                ┌──────────────┴──────────────┐
#                ▼                             ▼
#   ┌───────────────────────────┐   ┌─────────────────────────────┐
#   │    SparkleGameFramework   │   │        SparkleUI.dll        │
#   │   (World, Entities, Assets)  │   │   (Widgets and overlays)   │
#   └───────────────────────────┘   └─────────────────────────────┘
#                │                             │
#                └──────────────┬──────────────┘
#                               │
#   ┌─────────────────────────────────────────────────────────────┐
#   │                   SparkleRenderer.dll                       │
#   └─────────────────────────────────────────────────────────────┘
#                               │
#   ┌─────────────────────────────────────────────────────────────┐
#   │                     SparkleRHI.dll                          │
#   └─────────────────────────────────────────────────────────────┘
#                               │
#   ┌─────────────────────────────────────────────────────────────┐
#   │                   SparklePlatform.dll                       │
#   └─────────────────────────────────────────────────────────────┘
#                               │
#   ┌─────────────────────────────────────────────────────────────┐
#   │                    SparkleCore.dll                          │
#   └─────────────────────────────────────────────────────────────┘
#

# Option to build as shared libraries (DLLs) or static libraries
option(SPARKLE_BUILD_SHARED "Build Sparkle modules as shared libraries (DLLs)" OFF)

# All third-party dependencies are fetched automatically via FetchContent.
# See Scripts/FetchDependencies.cmake for the full list and pinned versions.
# Sources are cached in build/_deps/ — no manual download or vendoring needed.
include(${CMAKE_SOURCE_DIR}/Scripts/FetchDependencies.cmake)

# ============================================================================
# CORE MODULES (always built - the foundation of the engine)
# ============================================================================
add_subdirectory(Core)           # SparkleCore.dll     - Math, Memory, Events, Logging
add_subdirectory(Platform)       # SparklePlatform.dll - Window, Input, FileSystem
add_subdirectory(RHI)            # SparkleRHI.dll      - D3D12/Vulkan abstraction
add_subdirectory(Renderer)       # SparkleRenderer.dll - Rendering pipeline
add_subdirectory(GameFramework)  # SparkleEngine.dll   - World, Entities, Components (NO rendering deps)
add_subdirectory(Application)    # SparkleApplication  - App class (ties GameFramework + Renderer)
add_subdirectory(UI)             # SparkleUI.dll       - Widget system, Layout

# ============================================================================
# MODULE LINKING GUIDE
# ============================================================================
# Projects should explicitly link only the modules they need:
#
#   SparkleCore         - Math, Memory, Events, Logging (required by all)
#   SparklePlatform     - Window, Input (required for windowed apps)
#   SparkleRHI          - D3D12/Vulkan abstraction (required for rendering)
#   SparkleRenderer     - High-level rendering (requires RHI)
#   SparkleGameFramework - Scene, Camera, Primitives (NO rendering deps)
#   SparkleApplication  - App class (ties GameFramework + Renderer together)
#   SparkleUI           - ImGui integration (requires Renderer)
#
# Example for a typical game project:
#   target_link_libraries(MyGame PRIVATE
#       SparkleApplication  # Brings in GameFramework + Renderer + all deps
#       SparkleUI
#   )
#
# Example for a headless tool (no rendering):
#   target_link_libraries(MyTool PRIVATE
#       SparkleCore
#   )

# Set compiler flags for debug builds
if (MSVC)
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} /Od /Zi")
else()
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
endif()