cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED SPARKLE_REPO_ROOT)
    get_filename_component(SPARKLE_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

file(TO_CMAKE_PATH "${SPARKLE_REPO_ROOT}" SPARKLE_REPO_ROOT)

set(SPARKLE_BOUNDARY_SOURCE_FILE_REGEX "\\.(c|cc|cpp|cxx|h|hh|hpp|hxx|inl|cmake)$|/CMakeLists\\.txt$")
set(SPARKLE_BOUNDARY_NATIVE_API_REGEX "<d3d12\\.h>|<vulkan/vulkan\\.h>|ID3D12|D3D12_|Vk[A-Z]|vk[A-Z]|Vulkan::Vulkan|\"D3D12/|\"Vulkan/")
set(SPARKLE_BOUNDARY_D3D12_IN_VULKAN_REGEX "D3D12/|<d3d12\\.h>|ID3D12|D3D12_")
set(SPARKLE_BOUNDARY_VULKAN_IN_D3D12_REGEX "Vulkan/|<vulkan/vulkan\\.h>|Vk[A-Z]|vk[A-Z]|Vulkan::Vulkan")

set_property(GLOBAL PROPERTY SPARKLE_BOUNDARY_FAILURES "")
set_property(GLOBAL PROPERTY SPARKLE_BOUNDARY_EXCEPTION_SUMMARIES "")
set_property(GLOBAL PROPERTY SPARKLE_RHI_RENDERER_PRIVATE_EXCEPTION_COUNT 0)
set_property(GLOBAL PROPERTY SPARKLE_RENDERER_CMAKE_VULKAN_EXCEPTION_COUNT 0)
set_property(GLOBAL PROPERTY SPARKLE_RENDERER_STREAMLINE_NATIVE_EXCEPTION_COUNT 0)
set_property(GLOBAL PROPERTY SPARKLE_APP_VALIDATION_NATIVE_EXCEPTION_COUNT 0)

function(sparkle_boundary_relative_path out_var absolute_path)
    file(RELATIVE_PATH _relative "${SPARKLE_REPO_ROOT}" "${absolute_path}")
    file(TO_CMAKE_PATH "${_relative}" _relative)
    set(${out_var} "${_relative}" PARENT_SCOPE)
endfunction()

function(sparkle_boundary_append_failure_text message_text)
    set_property(GLOBAL APPEND PROPERTY SPARKLE_BOUNDARY_FAILURES "${message_text}")
endfunction()

function(sparkle_boundary_append_failure rule relative_path line_number reason line_text)
    string(STRIP "${line_text}" _line_text)
    string(REPLACE ";" "," _line_text "${_line_text}")
    sparkle_boundary_append_failure_text("${relative_path}:${line_number}: [${rule}] ${reason} | ${_line_text}")
endfunction()

function(sparkle_boundary_increment_property property_name)
    get_property(_count GLOBAL PROPERTY ${property_name})
    if(NOT _count)
        set(_count 0)
    endif()

    math(EXPR _count "${_count} + 1")
    set_property(GLOBAL PROPERTY ${property_name} "${_count}")
endfunction()

function(sparkle_boundary_append_exception_summary summary_text)
    set_property(GLOBAL APPEND PROPERTY SPARKLE_BOUNDARY_EXCEPTION_SUMMARIES "${summary_text}")
endfunction()

function(sparkle_boundary_validate_counted_exception label property_name max_count removal_stage reason)
    get_property(_count GLOBAL PROPERTY ${property_name})
    if(NOT _count)
        set(_count 0)
    endif()

    if(_count GREATER max_count)
        sparkle_boundary_append_failure_text(
            "${label}: counted transitional exception grew to ${_count}/${max_count}. Removal stage: ${removal_stage}. ${reason}")
    elseif(_count LESS max_count)
        sparkle_boundary_append_exception_summary(
            "${label}: ${_count}/${max_count} transitional matches remain until ${removal_stage}. Count is below the frozen baseline. Remove or tighten this exception in the owning stage.")
    else()
        sparkle_boundary_append_exception_summary(
            "${label}: ${_count}/${max_count} transitional matches remain until ${removal_stage}. ${reason}")
    endif()
endfunction()

function(sparkle_boundary_collect_source_files out_var)
    set(_result "")
    foreach(_root IN LISTS ARGN)
        file(GLOB_RECURSE _files LIST_DIRECTORIES false "${SPARKLE_REPO_ROOT}/${_root}/*")
        foreach(_file IN LISTS _files)
            file(TO_CMAKE_PATH "${_file}" _normalized_file)
            if(_normalized_file MATCHES "${SPARKLE_BOUNDARY_SOURCE_FILE_REGEX}")
                list(APPEND _result "${_normalized_file}")
            endif()
        endforeach()
    endforeach()

    list(REMOVE_DUPLICATES _result)
    set(${out_var} ${_result} PARENT_SCOPE)
endfunction()

function(sparkle_boundary_scan_file absolute_path)
    sparkle_boundary_relative_path(_relative_path "${absolute_path}")

    file(READ "${absolute_path}" _content)
    string(REPLACE ";" "__SPARKLE_SEMICOLON__" _content "${_content}")
    string(REPLACE "\r\n" "\n" _content "${_content}")
    string(REPLACE "\r" "\n" _content "${_content}")
    string(REPLACE "\n" ";" _lines "${_content}")

    set(_line_number 0)
    foreach(_line IN LISTS _lines)
        math(EXPR _line_number "${_line_number} + 1")
        string(REPLACE "__SPARKLE_SEMICOLON__" ";" _line "${_line}")

        if(_relative_path MATCHES "^Engine/RHI/" AND _line MATCHES "Renderer/Private")
            if(_relative_path STREQUAL "Engine/RHI/Private/Shaders/DirectLightingShaders.cpp"
                AND _line MATCHES "Renderer/Private/RayTracing/RayTracedShadowUniformData\\.h")
                sparkle_boundary_increment_property(SPARKLE_RHI_RENDERER_PRIVATE_EXCEPTION_COUNT)
            else()
                sparkle_boundary_append_failure(
                    "RHI_NO_RENDERER_PRIVATE"
                    "${_relative_path}"
                    "${_line_number}"
                    "RHI code must not include Renderer-private headers."
                    "${_line}")
            endif()
        endif()

        if(_relative_path MATCHES "^Engine/Renderer/" AND _line MATCHES "${SPARKLE_BOUNDARY_NATIVE_API_REGEX}")
            if(_relative_path STREQUAL "Engine/Renderer/CMakeLists.txt" AND _line MATCHES "Vulkan::Vulkan")
                sparkle_boundary_increment_property(SPARKLE_RENDERER_CMAKE_VULKAN_EXCEPTION_COUNT)
            elseif(_relative_path STREQUAL "Engine/Renderer/Private/Upscaling/NvidiaDlss/StreamlineDlssRuntime.cpp")
                sparkle_boundary_increment_property(SPARKLE_RENDERER_STREAMLINE_NATIVE_EXCEPTION_COUNT)
            else()
                sparkle_boundary_append_failure(
                    "RENDERER_NO_BACKEND_NATIVE"
                    "${_relative_path}"
                    "${_line_number}"
                    "Renderer code must not depend on D3D12/Vulkan native APIs outside documented provider migration code."
                    "${_line}")
            endif()
        endif()

        if(_relative_path MATCHES "^Engine/RHI/Private/D3D12/" AND _line MATCHES "${SPARKLE_BOUNDARY_VULKAN_IN_D3D12_REGEX}")
            sparkle_boundary_append_failure(
                "D3D12_NO_VULKAN_BACKEND"
                "${_relative_path}"
                "${_line_number}"
                "D3D12 backend code must not include or use Vulkan backend/native identifiers."
                "${_line}")
        endif()

        if(_relative_path MATCHES "^Engine/RHI/Private/Vulkan/" AND _line MATCHES "${SPARKLE_BOUNDARY_D3D12_IN_VULKAN_REGEX}")
            sparkle_boundary_append_failure(
                "VULKAN_NO_D3D12_BACKEND"
                "${_relative_path}"
                "${_line_number}"
                "Vulkan backend code must not include or use D3D12 backend/native identifiers."
                "${_line}")
        endif()

        if(_relative_path MATCHES "^Engine/Application/Private/Validation/" AND _line MATCHES "${SPARKLE_BOUNDARY_NATIVE_API_REGEX}")
            if(_relative_path STREQUAL "Engine/Application/Private/Validation/RhiSmokeEditorValidation.cpp")
                sparkle_boundary_increment_property(SPARKLE_APP_VALIDATION_NATIVE_EXCEPTION_COUNT)
            else()
                sparkle_boundary_append_failure(
                    "APPLICATION_VALIDATION_NO_BACKEND_NATIVE"
                    "${_relative_path}"
                    "${_line_number}"
                    "Application validation must not add backend-native capture or API dependencies."
                    "${_line}")
            endif()
        endif()
    endforeach()
endfunction()

message(STATUS "Sparkle architecture boundary check")
message(STATUS "Repository root: ${SPARKLE_REPO_ROOT}")

sparkle_boundary_collect_source_files(
    SPARKLE_BOUNDARY_SOURCE_FILES
    "Engine/RHI"
    "Engine/Renderer"
    "Engine/Application/Private/Validation")

foreach(_file IN LISTS SPARKLE_BOUNDARY_SOURCE_FILES)
    sparkle_boundary_scan_file("${_file}")
endforeach()

sparkle_boundary_validate_counted_exception(
    "RHI_NO_RENDERER_PRIVATE: Engine/RHI/Private/Shaders/DirectLightingShaders.cpp"
    SPARKLE_RHI_RENDERER_PRIVATE_EXCEPTION_COUNT
    1
    "Stage 4"
    "Renderer-specific shader registration moves out of RHI.")

sparkle_boundary_validate_counted_exception(
    "RENDERER_NO_BACKEND_NATIVE: Engine/Renderer/CMakeLists.txt Vulkan::Vulkan link"
    SPARKLE_RENDERER_CMAKE_VULKAN_EXCEPTION_COUNT
    2
    "Stage 9"
    "DLSS/native interop wiring is moved behind RHI/backend-owned metadata.")

sparkle_boundary_validate_counted_exception(
    "RENDERER_NO_BACKEND_NATIVE: Streamline DLSS Vulkan bridge"
    SPARKLE_RENDERER_STREAMLINE_NATIVE_EXCEPTION_COUNT
    5
    "Stage 9"
    "Provider integration is reviewed and narrowed to documented native interop.")

sparkle_boundary_validate_counted_exception(
    "APPLICATION_VALIDATION_NO_BACKEND_NATIVE: RhiSmokeEditorValidation.cpp D3D12 capture"
    SPARKLE_APP_VALIDATION_NATIVE_EXCEPTION_COUNT
    36
    "Stage 8"
    "Backend-native capture/readback moves behind RHI/backend validation services.")

get_property(_exceptions GLOBAL PROPERTY SPARKLE_BOUNDARY_EXCEPTION_SUMMARIES)
if(_exceptions)
    message(STATUS "Transitional exceptions:")
    foreach(_exception IN LISTS _exceptions)
        message(STATUS "  - ${_exception}")
    endforeach()
endif()

get_property(_failures GLOBAL PROPERTY SPARKLE_BOUNDARY_FAILURES)
if(_failures)
    list(LENGTH _failures _failure_count)
    message(STATUS "Architecture boundary violations:")
    foreach(_failure IN LISTS _failures)
        message(STATUS "  - ${_failure}")
    endforeach()
    message(FATAL_ERROR "Architecture boundary check failed with ${_failure_count} violation(s).")
endif()

message(STATUS "Architecture boundary check passed with no new violations.")
