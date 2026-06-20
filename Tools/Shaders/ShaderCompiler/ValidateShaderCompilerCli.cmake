if(NOT DEFINED SHADER_COMPILER_EXECUTABLE OR SHADER_COMPILER_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "SHADER_COMPILER_EXECUTABLE is required.")
endif()

if(NOT DEFINED WORKING_DIRECTORY OR WORKING_DIRECTORY STREQUAL "")
    message(FATAL_ERROR "WORKING_DIRECTORY is required.")
endif()

if(NOT DEFINED REPRESENTATIVE_PACKAGE OR REPRESENTATIVE_PACKAGE STREQUAL "")
    set(REPRESENTATIVE_PACKAGE "ComputeClear")
endif()

if(NOT DEFINED REPRESENTATIVE_PACKAGE_KEY OR REPRESENTATIVE_PACKAGE_KEY STREQUAL "")
    message(FATAL_ERROR "REPRESENTATIVE_PACKAGE_KEY is required so package identity changes are intentional.")
endif()

function(run_shader_compiler)
    execute_process(
        COMMAND "${SHADER_COMPILER_EXECUTABLE}" ${ARGV}
        WORKING_DIRECTORY "${WORKING_DIRECTORY}"
        RESULT_VARIABLE command_result)
    if(NOT command_result EQUAL 0)
        string(REPLACE ";" " " command_line "${ARGV}")
        message(FATAL_ERROR "ShaderCompiler ${command_line} failed with exit code ${command_result}.")
    endif()
endfunction()

run_shader_compiler(list-backends)
run_shader_compiler(list-targets)
run_shader_compiler(list-shaders --validate)
run_shader_compiler(inspect-shader "${REPRESENTATIVE_PACKAGE}")
run_shader_compiler(cook --package "${REPRESENTATIVE_PACKAGE}" --target DxilSm66 --target SpirV16 --analysis cooked-shader-stats)

file(GLOB_RECURSE representative_packages
    "${WORKING_DIRECTORY}/artifacts/dev/projects/*/cooked/Shaders/Packages/${REPRESENTATIVE_PACKAGE_KEY}.sparkshader")
list(LENGTH representative_packages representative_package_count)
if(NOT representative_package_count EQUAL 1)
    message(FATAL_ERROR
        "Expected one cooked package for ${REPRESENTATIVE_PACKAGE} key ${REPRESENTATIVE_PACKAGE_KEY}, found ${representative_package_count}.")
endif()

list(GET representative_packages 0 representative_package_path)
run_shader_compiler(inspect-package "${representative_package_path}")
