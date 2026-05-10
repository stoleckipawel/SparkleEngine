# SparkleCore Module
# Foundation module: Math, Memory, Containers, Logging, Events, Time, Input types

# Collect source files from new module structure
file(GLOB_RECURSE SPARKLE_CORE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SPARKLE_CORE_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SPARKLE_CORE_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

set(SPARKLE_CORE_FORBIDDEN_LOGGING_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/Diagnostics/Log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/Log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/Logging/LogBuffer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/Logging/LogSink.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/Logging/LogSink.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics/Logging/LogFormatting.h
)

foreach(forbidden_logging_file IN LISTS SPARKLE_CORE_FORBIDDEN_LOGGING_FILES)
    if(EXISTS "${forbidden_logging_file}")
        cmake_path(RELATIVE_PATH forbidden_logging_file BASE_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE forbidden_logging_relative_path)
        message(FATAL_ERROR
            "Legacy logging facade file '${forbidden_logging_relative_path}' was reintroduced. Keep Sparkle logging on Diagnostics/Logger.h and Diagnostics/Verify.h instead.")
    endif()
endforeach()

# Create the library (SHARED for DLL, STATIC for static lib)
# Use BUILD_SHARED_LIBS option or explicit SHARED/STATIC
if(SPARKLE_BUILD_SHARED)
    add_library(SparkleCore SHARED
        ${SPARKLE_CORE_PUBLIC_HEADERS}
        ${SPARKLE_CORE_PRIVATE_SOURCES}
        ${SPARKLE_CORE_PRIVATE_HEADERS}
    )
    # Define export macro for DLL builds
    target_compile_definitions(SparkleCore PRIVATE SPARKLE_CORE_EXPORTS)
else()
    add_library(SparkleCore STATIC
        ${SPARKLE_CORE_PUBLIC_HEADERS}
        ${SPARKLE_CORE_PRIVATE_SOURCES}
        ${SPARKLE_CORE_PRIVATE_HEADERS}
    )
    # Define static build macro
    target_compile_definitions(SparkleCore PUBLIC SPARKLE_STATIC)
endif()

# Set include directories
# PUBLIC: Headers that consumers of this module need
# PRIVATE: Headers only used internally (plus Public for internal compilation)
target_include_directories(SparkleCore
    PUBLIC
        # Engine root for module-prefixed includes (e.g., "Core/...")
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        # Subdirectories for easier includes
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Math
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Events
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Diagnostics
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Hash
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Strings
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Time
        ${CMAKE_CURRENT_SOURCE_DIR}/Public/Input
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Diagnostics
        ${CMAKE_CURRENT_SOURCE_DIR}/Private/Input
)

# Require C++20
target_compile_features(SparkleCore PUBLIC cxx_std_20)

# Precompiled header (per-module PCH)
target_precompile_headers(SparkleCore PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

# Core module has no engine dependencies (it's the base)
# Add any system dependencies here if needed
# SparkleCore owns spdlog bootstrap, sink policy, and named logger lifetime.
target_link_libraries(SparkleCore
    PUBLIC
        spdlog::spdlog_header_only
)

target_compile_definitions(SparkleCore
    PUBLIC
    $<$<OR:$<CONFIG:DebugEditor>,$<CONFIG:DebugGame>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE>
    $<$<OR:$<CONFIG:DevelopmentEditor>,$<CONFIG:DevelopmentGame>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
    $<$<OR:$<CONFIG:ShippingEditor>,$<CONFIG:ShippingGame>>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_WARN>
)

# Set output directories
set_target_properties(SparkleCore PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# IDE organization
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SPARKLE_CORE_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SPARKLE_CORE_PRIVATE_SOURCES} ${SPARKLE_CORE_PRIVATE_HEADERS})
