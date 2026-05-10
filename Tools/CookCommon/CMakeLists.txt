set(COOK_COMMON_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/ToolConsole.h
)

set(COOK_COMMON_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/ToolConsole.cpp
)

add_library(CookCommon STATIC
    ${COOK_COMMON_PUBLIC_HEADERS}
    ${COOK_COMMON_PRIVATE_SOURCES}
)

target_include_directories(CookCommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
)

target_compile_features(CookCommon PUBLIC cxx_std_20)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${COOK_COMMON_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${COOK_COMMON_PRIVATE_SOURCES})