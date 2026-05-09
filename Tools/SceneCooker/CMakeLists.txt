file(GLOB_RECURSE SCENE_COOKER_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SCENE_COOKER_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SCENE_COOKER_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

add_library(SceneCooker STATIC
    ${SCENE_COOKER_PUBLIC_HEADERS}
    ${SCENE_COOKER_PRIVATE_SOURCES}
    ${SCENE_COOKER_PRIVATE_HEADERS}
)

target_include_directories(SceneCooker
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(SceneCooker PUBLIC cxx_std_20)

target_precompile_headers(SceneCooker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(SceneCooker
    PUBLIC
        MeshCooker
        MaterialCooker
        SourceImportAdapters
        SparkleCore
        SparkleGameFramework
    PRIVATE
        CookCommon
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SCENE_COOKER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SCENE_COOKER_PRIVATE_SOURCES} ${SCENE_COOKER_PRIVATE_HEADERS})