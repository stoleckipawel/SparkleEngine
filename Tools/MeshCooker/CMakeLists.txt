file(GLOB_RECURSE MESH_COOKER_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE MESH_COOKER_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE MESH_COOKER_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

add_library(MeshCooker STATIC
    ${MESH_COOKER_PUBLIC_HEADERS}
    ${MESH_COOKER_PRIVATE_SOURCES}
    ${MESH_COOKER_PRIVATE_HEADERS}
)

target_include_directories(MeshCooker
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(MeshCooker PUBLIC cxx_std_20)

target_precompile_headers(MeshCooker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(MeshCooker
    PUBLIC
        SourceImportAdapters
        SparkleCore
        SparkleGameFramework
    PRIVATE
        CookCommon
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${MESH_COOKER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${MESH_COOKER_PRIVATE_SOURCES} ${MESH_COOKER_PRIVATE_HEADERS})