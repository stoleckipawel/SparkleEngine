set(TOOL_CONSOLE_SUPPORT_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/ToolConsole.h
)

set(TOOL_CONSOLE_SUPPORT_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/ToolConsole.cpp
)

add_library(ToolConsoleSupport STATIC
    ${TOOL_CONSOLE_SUPPORT_PUBLIC_HEADERS}
    ${TOOL_CONSOLE_SUPPORT_PRIVATE_SOURCES}
)
sparkle_configure_host_tool_target(ToolConsoleSupport)

target_include_directories(ToolConsoleSupport
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

target_compile_features(ToolConsoleSupport PUBLIC cxx_std_20)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${TOOL_CONSOLE_SUPPORT_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${TOOL_CONSOLE_SUPPORT_PRIVATE_SOURCES})
