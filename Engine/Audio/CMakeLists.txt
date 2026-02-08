# SparkleAudio Module (Placeholder)
# Future: FMOD/Wwise integration — spatial audio, mixing, DSP
#
# This module is scaffolded but not yet implemented.
# It builds as an INTERFACE library so other modules can reference it
# without breaking the build.

add_library(SparkleAudio INTERFACE)

target_include_directories(SparkleAudio
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)
