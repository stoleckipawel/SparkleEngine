if(NOT DEFINED LOGGING_BOUNDARY_SOURCE_DIR)
    set(LOGGING_BOUNDARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    LOGGING_BOUNDARY_SOURCE_DIR
    "${LOGGING_BOUNDARY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH LOGGING_BOUNDARY_SOURCE_DIR)

set(LOGGING_BOUNDARY_SOURCE_ROOTS
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Tools"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Projects"
)

set(LOGGING_BOUNDARY_CMAKE_FILES
    "${LOGGING_BOUNDARY_SOURCE_DIR}/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Core/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Application/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/GameFramework/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Renderer/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/RHI/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Editor/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Platform/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Tools/AssetConverter/CMakeLists.txt"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Tools/ShaderCompiler/CMakeLists.txt"
)

set(LOGGING_BOUNDARY_FORBIDDEN_FILES
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Core/Public/Diagnostics/Log.h"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Core/Private/Diagnostics/Log.cpp"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Core/Private/Diagnostics/Logging/LogBuffer.h"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Core/Private/Diagnostics/Logging/LogSink.h"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Core/Private/Diagnostics/Logging/LogSink.cpp"
    "${LOGGING_BOUNDARY_SOURCE_DIR}/Engine/Core/Private/Diagnostics/Logging/LogFormatting.h"
)

set(FORBIDDEN_DELETED_LOGGING_SOURCE_TOKENS
    "Diagnostics/Log.h"
    "#include \"Log.h\""
    "LogLevel"
    "LE_LOG("
    "LE_LOG_LEVEL_"
    "LOG_TRACE("
    "LOG_DEBUG("
    "LOG_INFO("
    "LOG_WARNING("
    "LOG_ERROR("
    "LOG_FATAL("
)

set(FORBIDDEN_DIRECT_INCLUDE_SOURCE_TOKENS
    "#include <spdlog/spdlog.h>"
)

set(FORBIDDEN_DEFAULT_LOGGER_SOURCE_TOKENS
    "SPDLOG_TRACE("
    "SPDLOG_DEBUG("
    "SPDLOG_INFO("
    "SPDLOG_WARN("
    "SPDLOG_ERROR("
    "SPDLOG_CRITICAL("
    "spdlog::trace("
    "spdlog::debug("
    "spdlog::info("
    "spdlog::warn("
    "spdlog::error("
    "spdlog::critical("
    "spdlog::default_logger("
    "spdlog::default_logger_raw("
    "spdlog::get("
)

set(FORBIDDEN_BOOTSTRAP_SOURCE_TOKENS
    "Engine::Logging::Initialize("
    "spdlog/sinks/"
    "std::make_shared<spdlog::logger>"
    "spdlog::set_default_logger"
    "spdlog::register_logger"
    "spdlog::drop("
    "spdlog::drop_all("
    "spdlog::shutdown("
    "spdlog::apply_all("
)

set(FORBIDDEN_DELETED_LOGGING_CMAKE_TOKENS
    "Diagnostics/Log.h"
    "Diagnostics/Log.cpp"
    "Diagnostics/Logging/LogBuffer.h"
    "Diagnostics/Logging/LogSink.h"
    "Diagnostics/Logging/LogSink.cpp"
    "Diagnostics/Logging/LogFormatting.h"
)

set(LOGGING_BOUNDARY_VIOLATIONS "")

function(check_file_for_tokens file_path)
    set(options)
    set(one_value_args)
    set(multi_value_args TOKENS ALLOWED_RELATIVE_PATHS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${LOGGING_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)

    foreach(allowed_relative_path IN LISTS CHECK_ALLOWED_RELATIVE_PATHS)
        if(relative_path STREQUAL allowed_relative_path)
            return()
        endif()
    endforeach()

    file(READ "${file_path}" file_text)

    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            set(LOGGING_BOUNDARY_VIOLATIONS
                "${LOGGING_BOUNDARY_VIOLATIONS}${relative_path}: found forbidden token '${token}'\n"
                PARENT_SCOPE)
        endif()
    endforeach()
endfunction()

foreach(forbidden_file IN LISTS LOGGING_BOUNDARY_FORBIDDEN_FILES)
    if(EXISTS "${forbidden_file}")
        cmake_path(RELATIVE_PATH forbidden_file BASE_DIRECTORY "${LOGGING_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        set(LOGGING_BOUNDARY_VIOLATIONS
            "${LOGGING_BOUNDARY_VIOLATIONS}${relative_path}: forbidden deleted logging facade file exists\n")
    endif()
endforeach()

foreach(source_root IN LISTS LOGGING_BOUNDARY_SOURCE_ROOTS)
    if(NOT EXISTS "${source_root}")
        continue()
    endif()

    file(GLOB_RECURSE logging_source_files
        "${source_root}/*.h"
        "${source_root}/*.hpp"
        "${source_root}/*.cpp"
        "${source_root}/*.cxx"
    )

    foreach(logging_source_file IN LISTS logging_source_files)
        check_file_for_tokens(
            "${logging_source_file}"
            TOKENS ${FORBIDDEN_DELETED_LOGGING_SOURCE_TOKENS}
        )

        check_file_for_tokens(
            "${logging_source_file}"
            TOKENS ${FORBIDDEN_DIRECT_INCLUDE_SOURCE_TOKENS}
            ALLOWED_RELATIVE_PATHS
                "Engine/Core/Public/Diagnostics/Logger.h"
        )

        check_file_for_tokens(
            "${logging_source_file}"
            TOKENS ${FORBIDDEN_DEFAULT_LOGGER_SOURCE_TOKENS}
            ALLOWED_RELATIVE_PATHS
                "Engine/Core/Private/Diagnostics/Logger.cpp"
        )

        check_file_for_tokens(
            "${logging_source_file}"
            TOKENS ${FORBIDDEN_BOOTSTRAP_SOURCE_TOKENS}
            ALLOWED_RELATIVE_PATHS
                "Engine/Core/Private/Diagnostics/Logger.cpp"
        )
    endforeach()
endforeach()

foreach(logging_cmake_file IN LISTS LOGGING_BOUNDARY_CMAKE_FILES)
    if(EXISTS "${logging_cmake_file}")
        check_file_for_tokens(
            "${logging_cmake_file}"
            TOKENS ${FORBIDDEN_DELETED_LOGGING_CMAKE_TOKENS}
            ALLOWED_RELATIVE_PATHS
                "Engine/Core/CMakeLists.txt"
        )
    endif()
endforeach()

if(LOGGING_BOUNDARY_VIOLATIONS)
    string(PREPEND LOGGING_BOUNDARY_VIOLATIONS
        "Logging boundary validation failed. SparkleCore must remain the only bootstrap owner, repo callsites must keep using named engine-owned loggers with native spdlog logging macros, and the deleted logging facade must not return.\n")
    message(FATAL_ERROR "${LOGGING_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "Logging boundary check passed for Engine, Tools, Projects, and logging-related CMake wiring.")