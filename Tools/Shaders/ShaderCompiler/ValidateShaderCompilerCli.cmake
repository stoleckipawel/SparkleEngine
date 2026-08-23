if(NOT DEFINED SHADER_COMPILER_EXECUTABLE OR SHADER_COMPILER_EXECUTABLE STREQUAL "")
    message(FATAL_ERROR "SHADER_COMPILER_EXECUTABLE is required.")
endif()

if(NOT DEFINED WORKING_DIRECTORY OR WORKING_DIRECTORY STREQUAL "")
    message(FATAL_ERROR "WORKING_DIRECTORY is required.")
endif()

if(NOT DEFINED REPRESENTATIVE_SHADER OR REPRESENTATIVE_SHADER STREQUAL "")
    set(REPRESENTATIVE_SHADER "ComputeClearCS")
endif()

if(NOT DEFINED REPRESENTATIVE_VIRTUAL_SOURCE OR REPRESENTATIVE_VIRTUAL_SOURCE STREQUAL "")
    set(REPRESENTATIVE_VIRTUAL_SOURCE "/Engine/Passes/Compute/ComputeClear.hlsl")
endif()

if(NOT DEFINED REPRESENTATIVE_PROJECT OR REPRESENTATIVE_PROJECT STREQUAL "")
    set(REPRESENTATIVE_PROJECT "Shared")
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

function(run_shader_compiler_capture output_variable)
    execute_process(
        COMMAND "${SHADER_COMPILER_EXECUTABLE}" ${ARGN}
        WORKING_DIRECTORY "${WORKING_DIRECTORY}"
        RESULT_VARIABLE command_result
        OUTPUT_VARIABLE command_output
        ERROR_VARIABLE command_error)
    if(NOT command_result EQUAL 0)
        string(REPLACE ";" " " command_line "${ARGN}")
        message(FATAL_ERROR
            "ShaderCompiler ${command_line} failed with exit code ${command_result}.\n${command_output}${command_error}")
    endif()
    set(${output_variable} "${command_output}${command_error}" PARENT_SCOPE)
endfunction()

function(expect_shader_compiler_failure)
    execute_process(
        COMMAND "${SHADER_COMPILER_EXECUTABLE}" ${ARGV}
        WORKING_DIRECTORY "${WORKING_DIRECTORY}"
        RESULT_VARIABLE command_result
        OUTPUT_QUIET
        ERROR_QUIET)
    if(command_result EQUAL 0)
        string(REPLACE ";" " " command_line "${ARGV}")
        message(FATAL_ERROR "ShaderCompiler ${command_line} unexpectedly succeeded.")
    endif()
endfunction()

function(require_output_match output pattern claim)
    if(NOT "${output}" MATCHES "${pattern}")
        message(FATAL_ERROR "ShaderCompiler output did not prove ${claim}.\n${output}")
    endif()
endfunction()

function(read_artifact_hashes map_output library_output)
    if(NOT EXISTS "${global_shader_map_path}" OR NOT EXISTS "${cooked_shader_library_path}")
        message(FATAL_ERROR "Expected the cooked shader map and code library for project ${REPRESENTATIVE_PROJECT}.")
    endif()
    file(SHA256 "${global_shader_map_path}" map_hash)
    file(SHA256 "${cooked_shader_library_path}" library_hash)
    set(${map_output} "${map_hash}" PARENT_SCOPE)
    set(${library_output} "${library_hash}" PARENT_SCOPE)
endfunction()

function(require_artifact_hashes expected_map expected_library claim)
    read_artifact_hashes(actual_map actual_library)
    if(NOT actual_map STREQUAL expected_map OR NOT actual_library STREQUAL expected_library)
        message(FATAL_ERROR "Shader artifacts changed during ${claim}.")
    endif()
endfunction()

set(global_shader_map_path
    "${WORKING_DIRECTORY}/artifacts/dev/projects/${REPRESENTATIVE_PROJECT}/cooked/Shaders/GlobalShaderMap.smap")
set(cooked_shader_library_path
    "${WORKING_DIRECTORY}/artifacts/dev/projects/${REPRESENTATIVE_PROJECT}/cooked/Shaders/CookedShaderLibrary.slib")

run_shader_compiler(list-backends)
run_shader_compiler(list-targets)
run_shader_compiler(list-shaders --validate)
run_shader_compiler_capture(
    full_cook_output
    cook --target DxilSm66 --target SpirV16 --analysis cooked-shader-stats)
require_output_match("${full_cook_output}" "shaderTypes=[1-9][0-9]*" "a non-empty typed shader catalog")
require_output_match("${full_cook_output}" "mapEntries=[1-9][0-9]*" "a non-empty global shader map")
require_output_match("${full_cook_output}" "uniqueCodeRecords=[1-9][0-9]*" "a non-empty cooked shader code library")
read_artifact_hashes(full_map_hash full_library_hash)

run_shader_compiler_capture(inspect_output inspect-shader "${REPRESENTATIVE_SHADER}")
require_output_match("${inspect_output}" "target=DxilSm66.*codeHash=.*parameterSignature=.*compileInputHash=" "typed DXIL map lookup")
require_output_match("${inspect_output}" "target=SpirV16.*codeHash=.*parameterSignature=.*compileInputHash=" "typed SPIR-V map lookup")

run_shader_compiler_capture(
    changed_cook_output
    cook --changed "${REPRESENTATIVE_VIRTUAL_SOURCE}" --target DxilSm66 --target SpirV16)
require_output_match("${changed_cook_output}" "shaderTypes=1" "exact representative changed-source selection")
require_output_match("${changed_cook_output}" "compileJobs=2" "one representative compile request per selected target")
require_artifact_hashes("${full_map_hash}" "${full_library_hash}" "deterministic changed-source republication")

run_shader_compiler_capture(
    first_repeat_output
    cook --shader-id "${REPRESENTATIVE_SHADER}" --target DxilSm66 --target SpirV16)
require_output_match("${first_repeat_output}" "compileJobs=2" "the first explicit repeated operation")
require_artifact_hashes("${full_map_hash}" "${full_library_hash}" "the first explicit repeated operation")
run_shader_compiler_capture(
    second_repeat_output
    cook --shader-id "${REPRESENTATIVE_SHADER}" --target DxilSm66 --target SpirV16)
require_output_match("${second_repeat_output}" "compileJobs=2" "the second explicit repeated operation")
require_artifact_hashes("${full_map_hash}" "${full_library_hash}" "the second explicit repeated operation")

set(cancellation_signal
    "${WORKING_DIRECTORY}/artifacts/dev/projects/${REPRESENTATIVE_PROJECT}/cooked/Shaders/ShaderCompilerCliValidation.cancel")
file(WRITE "${cancellation_signal}" "cancel")
expect_shader_compiler_failure(
    cook --shader-id "${REPRESENTATIVE_SHADER}" --target DxilSm66 --target SpirV16
    --cancellation-signal "${cancellation_signal}")
file(REMOVE "${cancellation_signal}")
require_artifact_hashes("${full_map_hash}" "${full_library_hash}" "cancelled cooking")

expect_shader_compiler_failure(cook --changed "/Engine/../Outside.hlsl" --target DxilSm66)
require_artifact_hashes("${full_map_hash}" "${full_library_hash}" "traversal rejection")
expect_shader_compiler_failure(cook --shader-id "__MissingShaderValidationSentinel" --target DxilSm66)
require_artifact_hashes("${full_map_hash}" "${full_library_hash}" "unknown shader rejection")
