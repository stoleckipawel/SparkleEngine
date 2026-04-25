add_executable(ConsoleDiagnostics
    Private/Main.cpp
)

target_link_libraries(ConsoleDiagnostics
    PRIVATE
        SparkleCore
)

target_compile_features(ConsoleDiagnostics PRIVATE cxx_std_20)

set_target_properties(ConsoleDiagnostics PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
