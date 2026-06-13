add_library(ShaderContracts INTERFACE)
sparkle_configure_host_tool_target(ShaderContracts)

target_include_directories(ShaderContracts
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
        ${CMAKE_SOURCE_DIR}/Engine
)

target_compile_features(ShaderContracts INTERFACE cxx_std_20)

target_link_libraries(ShaderContracts
    INTERFACE
        SparkleRHI
)
