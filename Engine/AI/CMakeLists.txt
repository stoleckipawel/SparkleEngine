# SparkleAI Module (Placeholder)
# Future: Navigation (Recast/Detour), behavior trees, perception
#
# This module is scaffolded but not yet implemented.
# It builds as an INTERFACE library so other modules can reference it
# without breaking the build.

add_library(SparkleAI INTERFACE)

target_include_directories(SparkleAI
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)
