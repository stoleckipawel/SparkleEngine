file(GLOB_RECURSE SOURCE_IMPORTER_PUBLIC_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/*.h
)

file(GLOB_RECURSE SOURCE_IMPORTER_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
)

file(GLOB_RECURSE SOURCE_IMPORTER_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
)

add_library(SourceImporters STATIC
    ${SOURCE_IMPORTER_PUBLIC_HEADERS}
    ${SOURCE_IMPORTER_PRIVATE_SOURCES}
    ${SOURCE_IMPORTER_PRIVATE_HEADERS}
)
sparkle_configure_host_tool_target(SourceImporters)

target_include_directories(SourceImporters
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(SourceImporters PUBLIC cxx_std_20)

sparkle_target_precompile_headers(SourceImporters
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(SourceImporters
    PUBLIC
        SparkleCore
        SparkleGameFramework
    PRIVATE
        cgltf
        mikktspace
        assimp::assimp
)

add_executable(WorldCoordinateImportTests
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/WorldCoordinateImportTests.cpp
)
target_include_directories(WorldCoordinateImportTests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
)
target_link_libraries(WorldCoordinateImportTests
    PRIVATE
        SourceImporters
        assimp::assimp
        cgltf
)
target_compile_features(WorldCoordinateImportTests PRIVATE cxx_std_20)
add_test(NAME world_coordinate_import COMMAND WorldCoordinateImportTests ${CMAKE_SOURCE_DIR})
set_tests_properties(world_coordinate_import PROPERTIES LABELS "Import;Coordinates;Animation")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${SOURCE_IMPORTER_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${SOURCE_IMPORTER_PRIVATE_SOURCES} ${SOURCE_IMPORTER_PRIVATE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Tests PREFIX "Tests" FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/Tests/WorldCoordinateImportTests.cpp
)
