# SparkleAnimation Module (Placeholder)
# Future: Skeletal animation, blend trees, IK solvers
#
# This module is scaffolded but not yet implemented.
# It builds as an INTERFACE library so other modules can reference it
# without breaking the build.

add_library(SparkleAnimation INTERFACE)

target_include_directories(SparkleAnimation
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)
