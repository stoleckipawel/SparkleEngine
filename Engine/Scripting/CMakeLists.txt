# SparkleScripting Module (Placeholder)
# Future: Lua/Mono scripting integration
#
# This module is scaffolded but not yet implemented.
# It builds as an INTERFACE library so other modules can reference it
# without breaking the build.

add_library(SparkleScripting INTERFACE)

target_include_directories(SparkleScripting
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)
