# Engine CMake configuration
# Layered DLL Architecture (Unreal-style)

# Option to build as shared libraries (DLLs) or static libraries
option(SPARKLE_BUILD_SHARED "Build Sparkle modules as shared libraries (DLLs)" OFF)

# All third-party dependencies are fetched automatically via FetchContent.
# See CMake/Dependencies/FetchDependencies.cmake for the full list and pinned versions.
# Sources are cached in build/_deps/ - no manual download or vendoring needed.
include(${CMAKE_SOURCE_DIR}/CMake/Dependencies/FetchDependencies.cmake)

# ============================================================================
# CORE MODULES (always built - the foundation of the engine)
# ============================================================================
add_subdirectory(Core)           # SparkleCore.dll     - Math, Memory, Events, Logging
add_subdirectory(Tasks)          # SparkleTasks.dll    - Deterministic task topology and execution
add_subdirectory(Platform)       # SparklePlatform.dll - Window, Input, FileSystem
add_subdirectory(RHI)            # SparkleRHI.dll      - D3D12/Vulkan abstraction
add_subdirectory(Renderer)       # SparkleRenderer.dll - Rendering pipeline
add_subdirectory(GameFramework)  # SparkleEngine.dll   - World, Entities, Components (NO rendering deps)
add_subdirectory(Editor)         # SparkleEditor.dll   - Editor UI, panels, tools
add_subdirectory(Application)    # SparkleApplication/SparkleApplicationEditor - Runtime and editor hosts

# ============================================================================
# MODULE LINKING GUIDE
# ============================================================================
# Projects should explicitly link only the modules they need:
#
#   SparkleCore         - Math, Memory, Events, Logging (required by all)
#   SparkleTasks        - Engine task topology/execution (independent of product modules)
#   SparklePlatform     - Window, Input (required for windowed apps)
#   SparkleRHI          - D3D12/Vulkan abstraction (required for rendering)
#   SparkleRenderer     - High-level rendering (requires RHI)
#   SparkleGameFramework - Scene, Camera, Primitives (NO rendering deps)
#   SparkleApplication  - Runtime host class (ties GameFramework + Renderer together)
#   SparkleEditor       - Editor UI and tools, without launch ownership
#   SparkleApplicationEditor - Editor host launch class (ties runtime host + editor UI together)
#
# Example for a typical runtime project:
#   target_link_libraries(MyGame PRIVATE
#       SparkleApplication  # Brings in GameFramework + Renderer + all deps
#   )
#
# Example for an editor host:
#   target_link_libraries(MyGameEditor PRIVATE
#       SparkleApplicationEditor
#   )
#
# Example for a headless tool (no rendering):
#   target_link_libraries(MyTool PRIVATE
#       SparkleCore
#   )

