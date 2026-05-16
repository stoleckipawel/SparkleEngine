function(add_sparkle_validation_dependency target_name validation_target)
    if(SPARKLE_BUILD_VALIDATION_ON_BUILD AND TARGET ${target_name} AND TARGET ${validation_target})
        add_dependencies(${target_name} ${validation_target})
    endif()
endfunction()

add_custom_target(
    runtime_cooked_boundary_check
    COMMAND ${CMAKE_COMMAND}
        -DRUNTIME_BOUNDARY_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_SOURCE_DIR}/CMake/Validation/ValidateRuntimeCookedBoundary.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating cooked-only runtime boundaries for Application, GameFramework, and Renderer..."
)

add_sparkle_validation_dependency(SparkleApplication runtime_cooked_boundary_check)
add_sparkle_validation_dependency(SparkleGameFramework runtime_cooked_boundary_check)
add_sparkle_validation_dependency(SparkleRenderer runtime_cooked_boundary_check)

add_custom_target(
    framegraph_boundary_check
    COMMAND ${CMAKE_COMMAND}
        -DFRAMEGRAPH_BOUNDARY_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_SOURCE_DIR}/CMake/Validation/ValidateFrameGraphBoundary.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating Renderer FrameGraph public/private and compiler/execution boundaries..."
)

add_sparkle_validation_dependency(SparkleRenderer framegraph_boundary_check)

add_custom_target(
    shader_compiler_boundary_check
    COMMAND ${CMAKE_COMMAND}
        -DSHADER_COMPILER_BOUNDARY_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_SOURCE_DIR}/CMake/Validation/ValidateShaderCompilerBoundary.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating runtime and ShaderCompiler boundary ownership..."
)

add_sparkle_validation_dependency(SparkleRHI shader_compiler_boundary_check)
add_sparkle_validation_dependency(SparkleApplication shader_compiler_boundary_check)
add_sparkle_validation_dependency(SparkleGameFramework shader_compiler_boundary_check)
add_sparkle_validation_dependency(SparkleRenderer shader_compiler_boundary_check)
add_sparkle_validation_dependency(ShaderCompiler shader_compiler_boundary_check)

add_custom_target(
    texture_cooker_boundary_check
    COMMAND ${CMAKE_COMMAND}
        -DTEXTURE_COOKER_BOUNDARY_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_SOURCE_DIR}/CMake/Validation/ValidateTextureCookerBoundary.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating AssetConverter and TextureCooker boundary ownership..."
)

add_sparkle_validation_dependency(SparkleRHI texture_cooker_boundary_check)
add_sparkle_validation_dependency(SparkleApplication texture_cooker_boundary_check)
add_sparkle_validation_dependency(SparkleGameFramework texture_cooker_boundary_check)
add_sparkle_validation_dependency(SparkleRenderer texture_cooker_boundary_check)
add_sparkle_validation_dependency(AssetConverter texture_cooker_boundary_check)
add_sparkle_validation_dependency(MaterialCooker texture_cooker_boundary_check)
add_sparkle_validation_dependency(TextureCookShared texture_cooker_boundary_check)
add_sparkle_validation_dependency(TextureCooker texture_cooker_boundary_check)

add_custom_target(
    tools_architecture_boundary_check
    COMMAND ${CMAKE_COMMAND}
        -DTOOLS_ARCHITECTURE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_SOURCE_DIR}/CMake/Validation/ValidateToolsArchitectureBoundary.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating Tools public seams and private implementation boundaries..."
)

add_sparkle_validation_dependency(AssetCookerCore tools_architecture_boundary_check)
add_sparkle_validation_dependency(AssetCooker tools_architecture_boundary_check)
add_sparkle_validation_dependency(SourceImportAdapters tools_architecture_boundary_check)
add_sparkle_validation_dependency(MeshCooker tools_architecture_boundary_check)
add_sparkle_validation_dependency(MaterialCooker tools_architecture_boundary_check)
add_sparkle_validation_dependency(SceneCooker tools_architecture_boundary_check)
add_sparkle_validation_dependency(TextureCookShared tools_architecture_boundary_check)
add_sparkle_validation_dependency(TextureCooker tools_architecture_boundary_check)
add_sparkle_validation_dependency(AssetConverter tools_architecture_boundary_check)
add_sparkle_validation_dependency(ShaderCompiler tools_architecture_boundary_check)

add_custom_target(
    logging_boundary_check
    COMMAND ${CMAKE_COMMAND}
        -DLOGGING_BOUNDARY_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -P ${CMAKE_SOURCE_DIR}/CMake/Validation/ValidateLoggingBoundary.cmake
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Validating repo-wide logging ownership, logger acquisition, and deleted logging facade guard..."
)

function(add_logging_boundary_dependency_if_target target_name)
    add_sparkle_validation_dependency(${target_name} logging_boundary_check)
endfunction()

add_logging_boundary_dependency_if_target(SparkleCore)
add_logging_boundary_dependency_if_target(SparkleApplication)
add_logging_boundary_dependency_if_target(SparkleGameFramework)
add_logging_boundary_dependency_if_target(SparkleRenderer)
add_logging_boundary_dependency_if_target(SparkleRHI)
add_logging_boundary_dependency_if_target(SparkleEditor)
add_logging_boundary_dependency_if_target(SparklePlatform)
add_logging_boundary_dependency_if_target(AssetConverter)
add_logging_boundary_dependency_if_target(AssetCookerCore)
add_logging_boundary_dependency_if_target(AssetCooker)
add_logging_boundary_dependency_if_target(SourceImportAdapters)
add_logging_boundary_dependency_if_target(MeshCooker)
add_logging_boundary_dependency_if_target(MaterialCooker)
add_logging_boundary_dependency_if_target(SceneCooker)
add_logging_boundary_dependency_if_target(TextureCookShared)
add_logging_boundary_dependency_if_target(TextureCooker)
add_logging_boundary_dependency_if_target(ShaderCompiler)

add_custom_target(
    sparkle_validation_check
    DEPENDS
        runtime_cooked_boundary_check
        framegraph_boundary_check
        shader_compiler_boundary_check
        texture_cooker_boundary_check
        tools_architecture_boundary_check
        logging_boundary_check
)
