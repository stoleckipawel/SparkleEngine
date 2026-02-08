# SparkleNetworking Module (Placeholder)
# Future: Replication, RPCs, transport layer
#
# This module is scaffolded but not yet implemented.
# It builds as an INTERFACE library so other modules can reference it
# without breaking the build.

add_library(SparkleNetworking INTERFACE)

target_include_directories(SparkleNetworking
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)
