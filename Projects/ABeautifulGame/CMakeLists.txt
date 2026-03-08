# ABeautifulGame project CMake configuration

# Add executable target
add_executable(ABeautifulGame
    src/main.cpp
)

# Require C++20
target_compile_features(ABeautifulGame PRIVATE cxx_std_20)

# Link against engine modules explicitly
# SparkleApplication brings in GameFramework + Renderer + lower-level dependencies.
target_link_libraries(ABeautifulGame PRIVATE
    SparkleApplication
    SparkleUI
)

# Include third-party headers if needed directly
target_include_directories(ABeautifulGame PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/third_party
)

# Configure Visual Studio debugger to launch from project directory.
# Path discovery uses marker files, so we just need the working directory set correctly.
if(MSVC)
    set_target_properties(ABeautifulGame PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()
