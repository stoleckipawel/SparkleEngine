# __PROJECT_NAME__ project CMake configuration

# Add explicit editor and runtime launch targets so new projects inherit the
# host split directly instead of a mixed single-executable entrypoint.
add_executable(__PROJECT_NAME__Editor
    Src/EditorMain.cpp
)

add_executable(__PROJECT_NAME__Runtime
    Src/RuntimeMain.cpp
)

# Require C++20
target_compile_features(__PROJECT_NAME__Editor PRIVATE cxx_std_20)
target_compile_features(__PROJECT_NAME__Runtime PRIVATE cxx_std_20)

# Link against engine modules explicitly.
# Editor launch stays above runtime modules; runtime launch remains editorless.
target_link_libraries(__PROJECT_NAME__Editor PRIVATE
    SparkleApplication
    SparkleEditor
)

target_link_libraries(__PROJECT_NAME__Runtime PRIVATE
    SparkleApplication
)

# Include third-party headers if needed directly.
target_include_directories(__PROJECT_NAME__Editor PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/third_party
)

target_include_directories(__PROJECT_NAME__Runtime PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/third_party
)

# Configure Visual Studio debugger to launch from project directory.
# Path discovery uses marker files, so we just need the working directory set correctly.
if(MSVC)
    set_target_properties(__PROJECT_NAME__Editor PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )

    set_target_properties(__PROJECT_NAME__Runtime PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()
