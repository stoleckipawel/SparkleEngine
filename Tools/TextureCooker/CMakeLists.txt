set(TEXTURE_COOK_SHARED_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/TextureCookRequestList.h
)

set(TEXTURE_COOK_SHARED_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/TextureCookRequestList.cpp
)

set(TEXTURE_COOK_SHARED_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

set(TEXTURE_COOKER_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureAssetCooker.cpp
)

set(TEXTURE_COOKER_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureAssetCooker.h
)

add_library(TextureCookShared STATIC
    ${TEXTURE_COOK_SHARED_PUBLIC_HEADERS}
    ${TEXTURE_COOK_SHARED_PRIVATE_SOURCES}
    ${TEXTURE_COOK_SHARED_PRIVATE_HEADERS}
)

target_include_directories(TextureCookShared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
)

target_compile_features(TextureCookShared PUBLIC cxx_std_20)

target_precompile_headers(TextureCookShared PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(TextureCookShared
)

add_executable(TextureCooker
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
    ${TEXTURE_COOKER_PRIVATE_SOURCES}
    ${TEXTURE_COOKER_PRIVATE_HEADERS}
)

# TextureCooker owns source-texture loading plus cooked texture-asset emission.
# Keep the source-decode and serialization implementation private to this
# executable.
target_include_directories(TextureCooker
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(TextureCooker PRIVATE cxx_std_20)

target_precompile_headers(TextureCooker PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(TextureCooker
    PRIVATE
        TextureCookShared
        SparkleRHI
)

set_target_properties(TextureCooker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${TEXTURE_COOK_SHARED_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${TEXTURE_COOK_SHARED_PRIVATE_SOURCES} ${TEXTURE_COOK_SHARED_PRIVATE_HEADERS} ${TEXTURE_COOKER_PRIVATE_SOURCES} ${TEXTURE_COOKER_PRIVATE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)