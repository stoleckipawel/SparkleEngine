file(GLOB_RECURSE SHADER_COMPILER_PRIVATE_SOURCES CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/Backends/*.cpp
)

file(GLOB_RECURSE SHADER_COMPILER_PRIVATE_HEADERS CONFIGURE_DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Backends/*.h
)

add_executable(ShaderCompiler
    ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp
    ${SHADER_COMPILER_PRIVATE_SOURCES}
    ${SHADER_COMPILER_PRIVATE_HEADERS}
)
sparkle_configure_host_tool_target(ShaderCompiler)

set(SLANG_SDK_ROOT "$ENV{VULKAN_SDK}" CACHE PATH "Slang SDK root")
find_path(SLANG_INCLUDE_DIR NAMES slang.h PATHS "${SLANG_SDK_ROOT}/Include/slang" REQUIRED)
find_library(SLANG_IMPORT_LIBRARY NAMES slang PATHS "${SLANG_SDK_ROOT}/Lib" REQUIRED)
find_file(SLANG_RUNTIME_LIBRARY NAMES slang.dll PATHS "${SLANG_SDK_ROOT}/Bin" REQUIRED)

add_library(slang::slang SHARED IMPORTED GLOBAL)
set_target_properties(slang::slang PROPERTIES
    IMPORTED_IMPLIB "${SLANG_IMPORT_LIBRARY}"
    IMPORTED_LOCATION "${SLANG_RUNTIME_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SLANG_INCLUDE_DIR}"
)

# ShaderCompiler owns offline shader manifest validation and cooked package
# emission. Keep shader-cooking implementation private to this executable
# target so scene-cook tooling cannot accidentally absorb it again.
target_include_directories(ShaderCompiler
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/Backends
        ${SLANG_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_features(ShaderCompiler PRIVATE cxx_std_20)

sparkle_target_precompile_headers(ShaderCompiler
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(ShaderCompiler
    PRIVATE
        CookCommon
        SparkleCore
    # ShaderCompiler is allowed to depend only on the shared cooked-shader
    # contract surface from SparkleRHI. Offline DXC ownership stays inside
    # this tool target and is enforced separately by CMake validation.
        SparkleRHI
    dxcompiler
    # SPIRV-Reflect: SPIR-V → ShaderReflection extractor. Confined to
    # Backends/Dxc/ by the boundary validator.
    spirv_reflect
    slang::slang
)

add_custom_command(TARGET ShaderCompiler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SLANG_RUNTIME_LIBRARY}"
        "$<TARGET_FILE_DIR:ShaderCompiler>/slang.dll"
)

set_target_properties(ShaderCompiler PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "" FILES ${SHADER_COMPILER_PRIVATE_SOURCES} ${SHADER_COMPILER_PRIVATE_HEADERS})