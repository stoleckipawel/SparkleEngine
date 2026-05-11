file(GLOB_RECURSE MATERIAL_COOKER_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE MATERIAL_COOKER_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE MATERIAL_COOKER_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

add_library(MaterialCooker STATIC
    ${MATERIAL_COOKER_PUBLIC_HEADERS}
    ${MATERIAL_COOKER_PRIVATE_SOURCES}
    ${MATERIAL_COOKER_PRIVATE_HEADERS}
)
sparkle_configure_host_tool_target(MaterialCooker)

target_include_directories(MaterialCooker
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(MaterialCooker PUBLIC cxx_std_20)

sparkle_target_precompile_headers(MaterialCooker
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(MaterialCooker
    PUBLIC
        SourceImportAdapters
        TextureCookShared
        SparkleCore
        SparkleGameFramework
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${MATERIAL_COOKER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${MATERIAL_COOKER_PRIVATE_SOURCES} ${MATERIAL_COOKER_PRIVATE_HEADERS})