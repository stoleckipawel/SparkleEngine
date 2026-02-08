# SparklePhysics Module (Placeholder)
# Future: PhysX/Jolt integration — collision, rigid bodies, raycasting
#
# This module is scaffolded but not yet implemented.
# It builds as an INTERFACE library so other modules can reference it
# without breaking the build.

add_library(SparklePhysics INTERFACE)

target_include_directories(SparklePhysics
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

# When implementation is added, switch to STATIC/SHARED:
# file(GLOB_RECURSE PHYSICS_SOURCES Private/*.cpp)
# add_library(SparklePhysics STATIC ${PHYSICS_SOURCES})
# target_link_libraries(SparklePhysics PRIVATE SparkleCore)
