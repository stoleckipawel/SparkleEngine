cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED SPARKLE_REPO_ROOT)
    get_filename_component(SPARKLE_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)
endif()

file(TO_CMAKE_PATH "${SPARKLE_REPO_ROOT}" SPARKLE_REPO_ROOT)

set(SPARKLE_BOUNDARY_SOURCE_FILE_REGEX "\\.(c|cc|cpp|cppm|cxx|h|hh|hpp|hxx|inl|ixx|cmake|manifest)$|/CMakeLists\\.txt$")
set(SPARKLE_BOUNDARY_NATIVE_API_REGEX "<d3d12\\.h>|<vulkan/vulkan\\.h>|ID3D12|D3D12_|Vk[A-Z]|vk[A-Z]|Vulkan::Vulkan|\"D3D12/|\"Vulkan/")
set(SPARKLE_BOUNDARY_NATIVE_PTLAS_REGEX "VK_NV_partitioned_acceleration_structure|VkPartitionedAccelerationStructure|VkBuildPartitionedAccelerationStructure|VK_DESCRIPTOR_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_NV|vk(Get|Cmd)PartitionedAccelerationStructures|NvAPI_D3D12|NVAPI_D3D12|D3D12_RTAS_PARTITIONED_TLAS|ExecuteIndirectRTASOperations")
set(SPARKLE_BOUNDARY_RENDERER_SHADER_DATA_REGEX "(Frame|View|ViewCamera|ViewTemporal)UniformData|PerObjectConstantBufferData|SceneLightingUniformData|LightGpuData|MeshInstanceShaderData|MeshInstanceData|VertexSkinInfluenceData|JointMatrixData")
set(SPARKLE_BOUNDARY_D3D12_IN_VULKAN_REGEX "D3D12/|<d3d12\\.h>|ID3D12|D3D12_")
set(SPARKLE_BOUNDARY_VULKAN_IN_D3D12_REGEX "Vulkan/|<vulkan/vulkan\\.h>|Vk[A-Z]|vk[A-Z]|Vulkan::Vulkan")
set(SPARKLE_BOUNDARY_RENDERER_HIGH_LEVEL_ORCHESTRATOR_REGEX
    "^Engine/Renderer/Private/(Host/RendererSystemRoot|FramePipeline/FramePipeline|Providers/RendererImageProviderStack)\\.(cpp|h)$")
set(SPARKLE_BOUNDARY_RENDERER_PROVIDER_DETAIL_REGEX
    "Streamline/|Upscaling/Nvidia|RayReconstruction/Nvidia|NvidiaDlss")
set(SPARKLE_BOUNDARY_RENDERER_VENDOR_INTEROP_REGEX
    "^Engine/Renderer/Private/(Streamline|Upscaling/NvidiaDlss|RayReconstruction/NvidiaDlssRayReconstruction)/")
set(SPARKLE_BOUNDARY_NATIVE_DPI_POLICY_REGEX
    "Set(Process|Thread)DpiAwareness|GetDpiFor(Window|Monitor|System)|DPI_AWARENESS_CONTEXT_|VS_DPI_AWARE|<dpiAware(ness)?|ImGui_ImplWin32_(EnableDpiAwareness|GetDpiScaleFor)|DisplaySize[ \t]*=[ \t]*ImVec2")

set_property(GLOBAL PROPERTY SPARKLE_BOUNDARY_FAILURES "")

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
    if(_content MATCHES "namespace[ \t\r\n]*\\{")
        sparkle_boundary_append_failure(
            "NO_ANONYMOUS_NAMESPACE"
            "${_relative_path}"
            "1"
            "Anonymous namespaces are forbidden; assign behavior to an owning type or an established named domain namespace."
            "namespace {")
    endif()
    string(REPLACE ";" "__SPARKLE_SEMICOLON__" _content "${_content}")
    string(REPLACE "\r\n" "\n" _content "${_content}")
    string(REPLACE "\r" "\n" _content "${_content}")
    string(REPLACE "\n" ";" _lines "${_content}")

    set(_line_number 0)
    foreach(_line IN LISTS _lines)
        math(EXPR _line_number "${_line_number} + 1")
        string(REPLACE "__SPARKLE_SEMICOLON__" ";" _line "${_line}")

        if(_relative_path MATCHES "^Engine/RHI/" AND _line MATCHES "#include[^\n]*Renderer/|SparkleRenderer")
            sparkle_boundary_append_failure(
                "RHI_NO_RENDERER_DEPENDENCY"
                "${_relative_path}"
                "${_line_number}"
                "RHI must not include or link Renderer; the dependency direction is Renderer to public RHI."
                "${_line}")
        endif()

        if(_relative_path MATCHES "^Engine/RHI/Public/" AND _line MATCHES "FrameGraph")
            sparkle_boundary_append_failure(
                "RHI_PUBLIC_NO_FRAME_GRAPH_POLICY"
                "${_relative_path}"
                "${_line_number}"
                "Frame-graph tracking and lifetime policy belongs to Renderer metadata, not public RHI contracts."
                "${_line}")
        endif()

        if(_relative_path MATCHES "^Engine/RHI/" AND _line MATCHES "${SPARKLE_BOUNDARY_RENDERER_SHADER_DATA_REGEX}")
            sparkle_boundary_append_failure(
                "RHI_NO_RENDERER_SHADER_DATA"
                "${_relative_path}"
                "${_line_number}"
                "Renderer shader payload layouts belong in Renderer, not RHI resource contracts."
                "${_line}")
        endif()

        if(NOT _relative_path MATCHES "^Engine/Platform/Private/Window/(Window\\.cpp|SparkleApplication\\.manifest)$" AND
           _line MATCHES "${SPARKLE_BOUNDARY_NATIVE_DPI_POLICY_REGEX}")
            sparkle_boundary_append_failure(
                "WINDOW_DPI_POLICY_REMAINS_PLATFORM_OWNED"
                "${_relative_path}"
                "${_line_number}"
                "The shared application manifest owns process DPI mode, Platform Window owns native DPI changes, and the Win32 ImGui backend owns UI client extent."
                "${_line}")
        endif()

		if(_relative_path MATCHES "^Engine/RHI/Private/" AND
		   NOT _relative_path MATCHES "^Engine/RHI/Private/(D3D12|Vulkan)/" AND
		   _line MATCHES "${SPARKLE_BOUNDARY_NATIVE_API_REGEX}")
			sparkle_boundary_append_failure(
				"RHI_COMMON_NO_BACKEND_NATIVE"
				"${_relative_path}"
				"${_line_number}"
				"Common RHI implementation must depend on neutral contracts; native API identifiers belong in the selected backend target."
				"${_line}")
		endif()

		if(_relative_path MATCHES "^Engine/RHI/" AND _line MATCHES "Upscaler|RayReconstruction|GBuffer|RenderScene")
			sparkle_boundary_append_failure(
				"RHI_NO_RENDERER_FEATURE_POLICY"
				"${_relative_path}"
				"${_line_number}"
				"Renderer feature policy must not appear in RHI contracts or implementation; use a neutral RHI capability or external-provider contract."
				"${_line}")
		endif()

        if(_relative_path MATCHES "^Engine/Renderer/" AND _line MATCHES "${SPARKLE_BOUNDARY_NATIVE_API_REGEX}")
			sparkle_boundary_append_failure(
				"RENDERER_NO_BACKEND_NATIVE"
				"${_relative_path}"
				"${_line_number}"
				"Renderer code must not depend on D3D12/Vulkan native APIs."
				"${_line}")
        endif()

        if(_relative_path MATCHES "^Engine/Renderer/" AND _line MATCHES "RHI/Private")
            sparkle_boundary_append_failure(
                "RENDERER_NO_RHI_PRIVATE"
                "${_relative_path}"
                "${_line_number}"
                "Renderer may consume only public RHI contracts, never backend or common RHI implementation headers."
                "${_line}")
        endif()

        if((_relative_path MATCHES "^Engine/(Renderer|Editor)/" OR
            _relative_path MATCHES "^Engine/GameFramework/Public/") AND
           _line MATCHES "#include[^\n]*(World/ECS|GameFramework/Private/World)")
            sparkle_boundary_append_failure(
                "ECS_STORAGE_REMAINS_GAMEFRAMEWORK_PRIVATE"
                "${_relative_path}"
                "${_line_number}"
                "Renderer, Editor, and GameFramework public contracts may consume stable world capabilities, never private ECS storage."
                "${_line}")
        endif()

        if(_relative_path MATCHES "^Engine/Renderer/" AND
           NOT _relative_path MATCHES "${SPARKLE_BOUNDARY_RENDERER_VENDOR_INTEROP_REGEX}" AND
           _line MATCHES "ERhiBackendApi::(D3D12|Vulkan)")
            sparkle_boundary_append_failure(
                "RENDERER_NO_BACKEND_POLICY_BRANCH"
                "${_relative_path}"
                "${_line_number}"
                "Renderer policy must branch on neutral capabilities; backend identity is reserved for dedicated external-provider interop adapters."
                "${_line}")
        endif()

        if(_relative_path MATCHES "^Engine/Renderer/Public/" AND _line MATCHES "RenderHardwareInterface|RenderDeviceServices")
            sparkle_boundary_append_failure(
                "RENDERER_PUBLIC_NO_RHI_DEVICE_ESCAPE_HATCH"
                "${_relative_path}"
                "${_line_number}"
                "Renderer public APIs must expose focused renderer operations instead of the complete RHI device facade."
                "${_line}")
        endif()

        if(_relative_path MATCHES "^Engine/Renderer/" AND _line MATCHES "${SPARKLE_BOUNDARY_NATIVE_PTLAS_REGEX}")
            sparkle_boundary_append_failure(
                "RENDERER_NO_NATIVE_PTLAS"
                "${_relative_path}"
                "${_line_number}"
                "Renderer code must use backend-neutral RHI PTLAS structs; native Vulkan/D3D12/NVAPI PTLAS identifiers belong in backend-private RHI code."
                "${_line}")
        endif()

        if(_relative_path MATCHES "${SPARKLE_BOUNDARY_RENDERER_HIGH_LEVEL_ORCHESTRATOR_REGEX}" AND
           _line MATCHES "${SPARKLE_BOUNDARY_RENDERER_PROVIDER_DETAIL_REGEX}")
            sparkle_boundary_append_failure(
                "RENDERER_ORCHESTRATOR_NO_PROVIDER_DETAILS"
                "${_relative_path}"
                "${_line_number}"
                "High-level renderer orchestrators must depend on provider interfaces/factories, not vendor implementations."
                "${_line}")
        endif()

        if(_relative_path STREQUAL "Engine/Renderer/Private/Frame/Core/Frame.cpp" AND _line MATCHES "Passes/")
            sparkle_boundary_append_failure(
                "FRAME_ORCHESTRATOR_NO_PASS_IMPLEMENTATIONS"
                "${_relative_path}"
                "${_line_number}"
                "The frame orchestrator must call subsystem-level frame APIs rather than concrete pass implementations."
                "${_line}")
        endif()

        if(_relative_path MATCHES "^Tools/Shaders/ShaderCompiler/Private/(Cooking|Verification)/" AND
           NOT _relative_path STREQUAL "Tools/Shaders/ShaderCompiler/Private/Cooking/ShaderPackageCooker.cpp" AND
           _line MATCHES "Cooking/ShaderPackageCooker\\.h")
            sparkle_boundary_append_failure(
                "SHADER_COOKING_NO_FACADE_DEPENDENCY"
                "${_relative_path}"
                "${_line_number}"
                "Shader cooking internals must depend on settings/result contracts, not upward on the cooker facade."
                "${_line}")
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

    endforeach()
endfunction()

message(STATUS "Sparkle architecture boundary check")
message(STATUS "Repository root: ${SPARKLE_REPO_ROOT}")

sparkle_boundary_collect_source_files(
    SPARKLE_BOUNDARY_SOURCE_FILES
    "CMake"
    "Engine"
    "Tools"
    "Projects")
list(APPEND SPARKLE_BOUNDARY_SOURCE_FILES "${SPARKLE_REPO_ROOT}/CMakeLists.txt")
list(FILTER SPARKLE_BOUNDARY_SOURCE_FILES EXCLUDE REGEX "/CMake/ArchitectureBoundaryCheck\\.cmake$")

foreach(_file IN LISTS SPARKLE_BOUNDARY_SOURCE_FILES)
    sparkle_boundary_scan_file("${_file}")
endforeach()

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
