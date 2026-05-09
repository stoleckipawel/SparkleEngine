file(GLOB_RECURSE SOURCE_IMPORT_ADAPTER_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SOURCE_IMPORT_ADAPTER_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SOURCE_IMPORT_ADAPTER_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

add_library(SourceImportAdapters STATIC
    ${SOURCE_IMPORT_ADAPTER_PUBLIC_HEADERS}
    ${SOURCE_IMPORT_ADAPTER_PRIVATE_SOURCES}
    ${SOURCE_IMPORT_ADAPTER_PRIVATE_HEADERS}
)

target_include_directories(SourceImportAdapters
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(SourceImportAdapters PUBLIC cxx_std_20)

target_precompile_headers(SourceImportAdapters PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(SourceImportAdapters
    PUBLIC
        SparkleCore
        SparkleGameFramework
    PRIVATE
        cgltf
        assimp::assimp
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SOURCE_IMPORT_ADAPTER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SOURCE_IMPORT_ADAPTER_PRIVATE_SOURCES} ${SOURCE_IMPORT_ADAPTER_PRIVATE_HEADERS})