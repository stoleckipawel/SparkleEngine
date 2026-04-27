set(COOK_COMMON_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/CookArtifactCache.h
)

set(COOK_COMMON_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/CookArtifactCache.cpp
)

add_library(CookCommon STATIC
    ${COOK_COMMON_PUBLIC_HEADERS}
    ${COOK_COMMON_PRIVATE_SOURCES}
)

target_include_directories(CookCommon
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Engine
)

target_compile_features(CookCommon PRIVATE cxx_std_20)

target_link_libraries(CookCommon
    PRIVATE
        SparkleCore
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${COOK_COMMON_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${COOK_COMMON_PRIVATE_SOURCES})
