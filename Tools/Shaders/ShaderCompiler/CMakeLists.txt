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

set(DXC_SDK_ROOT "$ENV{VULKAN_SDK}" CACHE PATH "DXC SDK root")
set(SLANG_SDK_ROOT "$ENV{VULKAN_SDK}" CACHE PATH "Slang SDK root")
find_path(DXC_INCLUDE_DIR NAMES dxcapi.h PATHS "${DXC_SDK_ROOT}/Include/dxc" REQUIRED)
find_library(DXC_IMPORT_LIBRARY NAMES dxcompiler PATHS "${DXC_SDK_ROOT}/Lib" REQUIRED)
find_file(DXC_RUNTIME_LIBRARY NAMES dxcompiler.dll PATHS "${DXC_SDK_ROOT}/Bin" REQUIRED)
find_path(SLANG_INCLUDE_DIR NAMES slang.h PATHS "${SLANG_SDK_ROOT}/Include/slang" REQUIRED)
find_library(SLANG_IMPORT_LIBRARY NAMES slang PATHS "${SLANG_SDK_ROOT}/Lib" REQUIRED)
find_file(SLANG_RUNTIME_LIBRARY NAMES slang.dll PATHS "${SLANG_SDK_ROOT}/Bin" REQUIRED)
find_file(SLANG_COMPILER_RUNTIME_LIBRARY NAMES slang-compiler.dll PATHS "${SLANG_SDK_ROOT}/Bin" REQUIRED)
find_file(SLANG_GLSL_MODULE_RUNTIME_LIBRARY NAMES slang-glsl-module.dll PATHS "${SLANG_SDK_ROOT}/Bin" REQUIRED)
find_file(SLANG_GLSLANG_RUNTIME_LIBRARY NAMES slang-glslang.dll PATHS "${SLANG_SDK_ROOT}/Bin" REQUIRED)
find_file(SLANG_RT_RUNTIME_LIBRARY NAMES slang-rt.dll PATHS "${SLANG_SDK_ROOT}/Bin" REQUIRED)
find_file(SLANG_STANDARD_LIBRARY_FILE NAMES slang.slang PATHS "${SLANG_SDK_ROOT}/Bin" REQUIRED)
file(GLOB SLANG_STANDARD_MODULE_DIRECTORIES LIST_DIRECTORIES true "${SLANG_SDK_ROOT}/Bin/slang-standard-module-*")
list(SORT SLANG_STANDARD_MODULE_DIRECTORIES)
if(NOT SLANG_STANDARD_MODULE_DIRECTORIES)
    message(FATAL_ERROR "Slang standard modules were not found under ${SLANG_SDK_ROOT}/Bin.")
endif()

set(SLANG_RUNTIME_SUPPORT_FILES
    "${SLANG_COMPILER_RUNTIME_LIBRARY}"
    "${SLANG_GLSL_MODULE_RUNTIME_LIBRARY}"
    "${SLANG_GLSLANG_RUNTIME_LIBRARY}"
    "${SLANG_RT_RUNTIME_LIBRARY}"
    "${SLANG_STANDARD_LIBRARY_FILE}")

add_library(slang::slang SHARED IMPORTED GLOBAL)
set_target_properties(slang::slang PROPERTIES
    IMPORTED_IMPLIB "${SLANG_IMPORT_LIBRARY}"
    IMPORTED_LOCATION "${SLANG_RUNTIME_LIBRARY}"
    INTERFACE_INCLUDE_DIRECTORIES "${SLANG_INCLUDE_DIR}"
)

target_include_directories(ShaderCompiler
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/Backends
        ${DXC_INCLUDE_DIR}
        ${SLANG_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/Engine
        ${CMAKE_SOURCE_DIR}/Engine/Core/Public/Diagnostics
)

target_compile_definitions(ShaderCompiler
    PRIVATE
        SPARKLE_SHADER_COMPILER_DXC_INCLUDE_DIR="${DXC_INCLUDE_DIR}"
        SPARKLE_SHADER_COMPILER_DXC_IMPORT_LIBRARY="${DXC_IMPORT_LIBRARY}"
        SPARKLE_SHADER_COMPILER_DXC_RUNTIME_LIBRARY="${DXC_RUNTIME_LIBRARY}"
        SPARKLE_SHADER_COMPILER_SLANG_INCLUDE_DIR="${SLANG_INCLUDE_DIR}"
        SPARKLE_SHADER_COMPILER_SLANG_IMPORT_LIBRARY="${SLANG_IMPORT_LIBRARY}"
        SPARKLE_SHADER_COMPILER_SLANG_RUNTIME_LIBRARY="${SLANG_RUNTIME_LIBRARY}"
)

target_compile_features(ShaderCompiler PRIVATE cxx_std_20)

sparkle_target_precompile_headers(ShaderCompiler
    ${CMAKE_CURRENT_SOURCE_DIR}/Private/PCH.h
)

target_link_libraries(ShaderCompiler
    PRIVATE
        ToolConsoleSupport
        ShaderContracts
        SparkleCore
        SparkleRHI
        SparkleRendererShaderRegistrations
        ${DXC_IMPORT_LIBRARY}
        spirv_reflect
        slang::slang
)

add_custom_command(TARGET ShaderCompiler POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DXC_RUNTIME_LIBRARY}"
        "$<TARGET_FILE_DIR:ShaderCompiler>/dxcompiler.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SLANG_RUNTIME_LIBRARY}"
        "$<TARGET_FILE_DIR:ShaderCompiler>/slang.dll"
)

foreach(slang_runtime_support_file IN LISTS SLANG_RUNTIME_SUPPORT_FILES)
    get_filename_component(slang_runtime_support_name "${slang_runtime_support_file}" NAME)
    add_custom_command(TARGET ShaderCompiler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${slang_runtime_support_file}"
            "$<TARGET_FILE_DIR:ShaderCompiler>/${slang_runtime_support_name}"
    )
endforeach()

foreach(slang_standard_module_directory IN LISTS SLANG_STANDARD_MODULE_DIRECTORIES)
    get_filename_component(slang_standard_module_name "${slang_standard_module_directory}" NAME)
    add_custom_command(TARGET ShaderCompiler POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E remove_directory
            "$<TARGET_FILE_DIR:ShaderCompiler>/${slang_standard_module_name}"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${slang_standard_module_directory}"
            "$<TARGET_FILE_DIR:ShaderCompiler>/${slang_standard_module_name}"
    )
endforeach()

sparkle_configure_development_tool_artifacts(ShaderCompiler)
sparkle_declare_runtime_dll_owner(ShaderCompiler SparkleCore)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Source PREFIX "Source" FILES ${CMAKE_CURRENT_SOURCE_DIR}/Source/main.cpp)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR} PREFIX "" FILES ${SHADER_COMPILER_PRIVATE_SOURCES} ${SHADER_COMPILER_PRIVATE_HEADERS})
