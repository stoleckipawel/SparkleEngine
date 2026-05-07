set(TEXTURE_COOK_SHARED_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Public/TextureCookRequestList.h
)

set(TEXTURE_COOK_SHARED_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/TextureCookRequestList.cpp
)

set(TEXTURE_COOKER_PRIVATE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/DdsTextureSourceLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/ExrTextureSourceLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/HdrTextureSourceLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/RasterTextureSourceLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureSourceLoader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureSourceLoaderUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureAssetCooker.cpp
)

set(TEXTURE_COOKER_PRIVATE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/DdsTextureSourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/ExrTextureSourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/HdrTextureSourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/RasterTextureSourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureSourceLoader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureSourceLoaderBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureSourceLoaderUtils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/Cooking/TextureAssetCooker.h
)

add_library(TextureCookShared STATIC
    ${TEXTURE_COOK_SHARED_PUBLIC_HEADERS}
    ${TEXTURE_COOK_SHARED_PRIVATE_SOURCES}
)

target_include_directories(TextureCookShared
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/Public
    PRIVATE
        ${CMAKE_SOURCE_DIR}/Engine
)

target_compile_features(TextureCookShared PUBLIC cxx_std_20)

target_link_libraries(TextureCookShared
    PRIVATE
        SparkleCore
)

add_library(TextureCookerMiniz STATIC
    ${FETCHCONTENT_BASE_DIR}/tinyexr-src/deps/miniz/miniz.c
)

target_include_directories(TextureCookerMiniz
    PUBLIC
        ${FETCHCONTENT_BASE_DIR}/tinyexr-src/deps/miniz
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
    ${stb_SOURCE_DIR}
    ${tinyexr_SOURCE_DIR}
)

target_compile_features(TextureCooker PRIVATE cxx_std_20)

target_link_libraries(TextureCooker
    PRIVATE
    CookCommon
        TextureCookShared
        TextureCookerMiniz
        SparkleRHI
    stb
    tinyexr
)

set_target_properties(TextureCooker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Public PREFIX "Public" FILES ${TEXTURE_COOK_SHARED_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Private PREFIX "Private" FILES ${TEXTURE_COOK_SHARED_PRIVATE_SOURCES} ${TEXTURE_COOKER_PRIVATE_SOURCES} ${TEXTURE_COOKER_PRIVATE_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)