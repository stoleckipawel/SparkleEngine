set(SPARKLE_ASSET_CONVERTER_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CookedAssetFormatBuilder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/CookedAssetWriter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/GltfImporter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneImporter.cpp
    ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Private/Assets/FbxImporter.cpp
    ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Private/Assets/GltfLoader.cpp
    ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Private/Assets/SceneImportPostProcessor.cpp
    ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Private/Assets/SceneImportUtilities.cpp
    ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Private/Scene/Transform.cpp
)

set(SPARKLE_ASSET_CONVERTER_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/CookedAssetFormatBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/CookedAssetWriter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/GltfImporter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/SceneImporter.h
)

add_executable(SparkleAssetConverter
    ${SPARKLE_ASSET_CONVERTER_SOURCES}
    ${SPARKLE_ASSET_CONVERTER_HEADERS}
)

target_compile_features(SparkleAssetConverter PRIVATE cxx_std_20)
target_compile_definitions(SparkleAssetConverter PRIVATE SPARKLE_STATIC)

target_include_directories(SparkleAssetConverter
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Hash
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Math
        ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Public
        ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Public/Scene
        ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Public/Scene/Camera
        ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Private
        ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Private/Assets
        ${CMAKE_SOURCE_DIR}/Engine/GameFramework/Private/Scene
)

target_link_libraries(SparkleAssetConverter
    PRIVATE
        SparkleCore
        cgltf
        assimp::assimp
)

if(CMAKE_CONFIGURATION_TYPES)
    foreach(CONFIG_TYPE ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${CONFIG_TYPE}" CONFIG_UPPER)
        set_target_properties(SparkleAssetConverter PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_SOURCE_DIR}/bin/${CONFIG_TYPE}"
            ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_SOURCE_DIR}/bin/${CONFIG_TYPE}"
            LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} "${CMAKE_SOURCE_DIR}/bin/${CONFIG_TYPE}"
        )
    endforeach()
else()
    set_target_properties(SparkleAssetConverter PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}"
    )
endif()

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "Tool" FILES ${SPARKLE_ASSET_CONVERTER_HEADERS})
