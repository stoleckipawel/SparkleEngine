# Showcase project CMake configuration

add_executable(Showcase
    src/main.cpp
)

target_compile_features(Showcase PRIVATE cxx_std_20)

target_link_libraries(Showcase PRIVATE
    SparkleApplication
    SparkleUI
)

target_include_directories(Showcase PRIVATE
    ${CMAKE_SOURCE_DIR}/Engine/third_party
)

if(MSVC)
    set_target_properties(Showcase PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
endif()