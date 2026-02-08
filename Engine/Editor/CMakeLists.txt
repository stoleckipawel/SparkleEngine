# SparkleEditor Module (Placeholder)
# Future: Editor tools, viewport, property inspector, asset browser
#
# This module is scaffolded but not yet implemented.
# It builds as an INTERFACE library so other modules can reference it
# without breaking the build.

add_library(SparkleEditor INTERFACE)

target_include_directories(SparkleEditor
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)
