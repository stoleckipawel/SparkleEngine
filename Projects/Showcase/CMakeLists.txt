# Showcase project CMake configuration

# Explicit editor and runtime launch targets keep the host split visible at the
# project boundary instead of hiding it behind a single mixed executable.
add_executable(ShowcaseEditor
    Src/EditorMain.cpp
)

add_executable(ShowcaseRuntime
    Src/RuntimeMain.cpp
)

target_compile_features(ShowcaseEditor PRIVATE cxx_std_20)
target_compile_features(ShowcaseRuntime PRIVATE cxx_std_20)

target_link_libraries(ShowcaseEditor PRIVATE
    SparkleApplicationEditor
)

target_link_libraries(ShowcaseRuntime PRIVATE
    SparkleApplication
)

sparkle_configure_project_artifacts(ShowcaseEditor Showcase editor)
sparkle_configure_project_artifacts(ShowcaseRuntime Showcase runtime)
sparkle_declare_runtime_dll_owner(
    ShowcaseEditor
    SparkleApplicationEditor
    SparkleApplication
    SparkleEditor
    SparkleRenderer
    SparkleRHI
    SparkleGameFramework
    SparklePlatform
    SparkleCore
)
sparkle_declare_runtime_dll_owner(
    ShowcaseRuntime
    SparkleApplication
    SparkleRenderer
    SparkleRHI
    SparkleGameFramework
    SparklePlatform
    SparkleCore
)

sparkle_stage_nvidia_streamline_runtime(ShowcaseEditor)
sparkle_stage_nvidia_streamline_runtime(ShowcaseRuntime)

if(MSVC)
    set_target_properties(ShowcaseEditor PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    set_target_properties(ShowcaseRuntime PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()
