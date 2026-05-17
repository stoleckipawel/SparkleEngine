if(NOT DEFINED RHI_BACKEND_BOUNDARY_SOURCE_DIR)
    set(RHI_BACKEND_BOUNDARY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    RHI_BACKEND_BOUNDARY_SOURCE_DIR
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH RHI_BACKEND_BOUNDARY_SOURCE_DIR)

set(RHI_BACKEND_BOUNDARY_VIOLATIONS "")

function(append_rhi_backend_violation message_text)
    set(RHI_BACKEND_BOUNDARY_VIOLATIONS
        "${RHI_BACKEND_BOUNDARY_VIOLATIONS}${message_text}\n"
        PARENT_SCOPE)
endfunction()

function(read_required_file file_path out_text)
    if(NOT EXISTS "${file_path}")
        append_rhi_backend_violation("missing required file: ${file_path}")
        set(${out_text} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${file_path}" file_text)
    set(${out_text} "${file_text}" PARENT_SCOPE)
endfunction()

function(require_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_rhi_backend_violation("${relative_path}: missing '${token}': ${description}")
    endif()
endfunction()

function(forbid_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(NOT match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_rhi_backend_violation("${relative_path}: found forbidden '${token}': ${description}")
    endif()
endfunction()

function(is_rhi_backend_private_path relative_path out_var)
    if(relative_path MATCHES "^Engine/RHI/Private/(D3D12|Vulkan)/")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(is_third_party_or_generated_path relative_path out_var)
    if(relative_path MATCHES "(^|/)Private/ThirdParty/" OR relative_path MATCHES "(^|/)third_party/" OR relative_path MATCHES "(^|/)build/")
        set(${out_var} TRUE PARENT_SCOPE)
    else()
        set(${out_var} FALSE PARENT_SCOPE)
    endif()
endfunction()

function(check_file_for_backend_tokens file_path)
    set(options)
    set(one_value_args DESCRIPTION)
    set(multi_value_args TOKENS)
    cmake_parse_arguments(CHECK "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
    is_third_party_or_generated_path("${relative_path}" should_skip)
    if(should_skip)
        return()
    endif()

    file(READ "${file_path}" file_text)
    foreach(token IN LISTS CHECK_TOKENS)
        string(FIND "${file_text}" "${token}" match_index)
        if(NOT match_index EQUAL -1)
            append_rhi_backend_violation("${relative_path}: found forbidden '${token}': ${CHECK_DESCRIPTION}")
        endif()
    endforeach()
endfunction()

set(rhi_public_root "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Public")
if(EXISTS "${rhi_public_root}")
    file(GLOB_RECURSE rhi_public_files
        "${rhi_public_root}/*.h"
        "${rhi_public_root}/*.hpp"
    )

    foreach(rhi_public_file IN LISTS rhi_public_files)
        check_file_for_backend_tokens(
            "${rhi_public_file}"
            TOKENS
                "RhiOwnedHeapHandle"
                "CreateOwnedHeap"
                "ReleaseOwnedHeap"
                "CreatePlacedTextureResource"
                "CreatePlacedBufferResource"
                "PlacedResource"
                "SetShaderVisibleDescriptorHeaps"
                "SetDescriptorHeaps"
                "GetShaderResourceHeapHandle"
                "NativeDescriptorHeapHandle"
                "ERhiDescriptorHeapType"
                "DescriptorHeap"
                "heapOffset"
                "ownedHeap"
                "CreateRenderTargetView"
                "CreateDepthStencilView"
                "CreateTextureShaderResourceView"
                "CreateTextureUnorderedAccessView"
                "CreateBufferShaderResourceView"
                "CreateBufferUnorderedAccessView"
                "CreateRayTracingAccelerationStructureShaderResourceView"
                "RootParameterIndex"
                "RootConstantBufferView"
                "RootShaderResourceView"
                "RootUnorderedAccessView"
                "DescriptorTableShaderResourceView"
                "DescriptorTableUnorderedAccessView"
                "DescriptorTableSampler"
                "InlineUniformDataAsRootConstants"
                "SetGraphicsRootConstants"
                "SetComputeRootConstants"
                "#include <vulkan/"
                "#include \"vulkan/"
                "#include \"Vulkan/"
                "vk_mem_alloc"
                "VkInstance"
                "VkPhysicalDevice"
                "VkDevice"
                "VkQueue"
                "VkCommandBuffer"
                "VkImage"
                "VkBuffer"
                "VmaAllocator"
                "VmaAllocation"
            DESCRIPTION "public RHI vocabulary must use neutral memory block, aliasing resource, and global binding state names"
        )
    endforeach()

    set(rhi_resource_view_path "${rhi_public_root}/Resources/RhiResourceView.h")
    read_required_file("${rhi_resource_view_path}" rhi_resource_view_text)
    if(rhi_resource_view_text)
        require_text("${rhi_resource_view_path}" "${rhi_resource_view_text}" "RhiResourceViewDesc" "public RHI must describe logical resource views independently of native descriptors")
        require_text("${rhi_resource_view_path}" "${rhi_resource_view_text}" "RhiResourceViewHandle" "public RHI must identify views independently of D3D12 CPU/GPU descriptor handles")
        require_text("${rhi_resource_view_path}" "${rhi_resource_view_text}" "AccelerationStructureShaderResource" "view model must include acceleration-structure SRV intent")
    endif()

    set(rhi_interface_path "${rhi_public_root}/Device/RenderHardwareInterface.h")
    read_required_file("${rhi_interface_path}" rhi_interface_text)
    if(rhi_interface_text)
        require_text("${rhi_interface_path}" "${rhi_interface_text}" "CreateTextureResource" "public RHI must expose neutral texture resource creation from RhiTextureResourceDesc")
        require_text("${rhi_interface_path}" "${rhi_interface_text}" "CreateBufferResource" "public RHI must expose neutral buffer resource creation from RhiBufferResourceDesc")
        require_text("${rhi_interface_path}" "${rhi_interface_text}" "RhiMemoryResidencyClass" "neutral resource creation must describe residency without exposing backend heap types")
    endif()

    set(rhi_pipeline_state_path "${rhi_public_root}/Pipeline/RhiPipelineStateDesc.h")
    read_required_file("${rhi_pipeline_state_path}" rhi_pipeline_state_text)
    if(rhi_pipeline_state_text)
        require_text("${rhi_pipeline_state_path}" "${rhi_pipeline_state_text}" "RhiBindingPoint" "binding layouts must expose neutral set/binding coordinates")
        require_text("${rhi_pipeline_state_path}" "${rhi_pipeline_state_text}" "RhiBindlessBindingMetadata" "binding layouts must reserve future bindless metadata without enabling bindless behavior")
        require_text("${rhi_pipeline_state_path}" "${rhi_pipeline_state_text}" "PushConstants" "push constants/root constants must share one public RHI description")
    endif()
endif()

set(rhi_cmake_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/CMakeLists.txt")
read_required_file("${rhi_cmake_path}" rhi_cmake_text)
if(rhi_cmake_text)
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "option(SPARKLE_RHI_WITH_D3D12" "backend enablement must be explicit")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "option(SPARKLE_RHI_WITH_VULKAN" "backend enablement must be explicit")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "set(SPARKLE_RHI_DEFAULT_BACKEND" "build default backend must be explicit")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "find_package(Vulkan QUIET)" "Vulkan SDK discovery must be explicit and backend-scoped")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "add_library(SparkleRHICommon OBJECT" "common RHI implementation must be separated from backend implementation")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "add_library(SparkleRHI_D3D12 STATIC" "D3D12 backend must have its own target")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "add_library(SparkleVMA INTERFACE" "VMA must be represented as a backend-private dependency target")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "add_library(SparkleRHI_Vulkan STATIC" "Vulkan backend must have its own implementation target")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_COMMON_PRIVATE_SOURCES" "common source glob must be named and separated")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_D3D12_SOURCES" "D3D12 source glob must be named and separated")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_VULKAN_SOURCES" "Vulkan source glob must be named and separated")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "list(FILTER SPARKLE_RHI_COMMON_PRIVATE_SOURCES EXCLUDE REGEX \"/Private/D3D12/\")" "D3D12 sources must be excluded from common RHI sources")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "list(FILTER SPARKLE_RHI_COMMON_PRIVATE_SOURCES EXCLUDE REGEX \"/Private/Vulkan/\")" "Vulkan sources must be excluded from common RHI sources")
    require_text("${rhi_cmake_path}" "${rhi_cmake_text}" "target_link_libraries(SparkleRHI_Vulkan" "Vulkan SDK and VMA must be linked only through the Vulkan backend target")
    forbid_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_PRIVATE_SOURCES" "do not restore one undifferentiated private source glob")
    forbid_text("${rhi_cmake_path}" "${rhi_cmake_text}" "SPARKLE_RHI_THIRD_PARTY_SOURCES" "backend third-party sources must be named per backend")
endif()

set(rhi_device_services_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Device/RenderDeviceServices.cpp")
read_required_file("${rhi_device_services_path}" rhi_device_services_text)
if(rhi_device_services_text)
    require_text("${rhi_device_services_path}" "${rhi_device_services_text}" "CreateVulkanRenderDeviceServices" "Vulkan backend selection must route through a backend-private service factory")
endif()

set(vulkan_rhi_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Device/VulkanRhi.cpp")
read_required_file("${vulkan_rhi_path}" vulkan_rhi_text)
if(vulkan_rhi_text)
    require_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "vkCreateInstance" "Vulkan backend must own instance creation privately")
    require_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "vkEnumeratePhysicalDevices" "Vulkan backend must enumerate adapters privately")
    require_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "vkCreateDevice" "Vulkan backend must own logical device creation privately")
    require_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "VK_DEBUG_UTILS_MESSAGE_SEVERITY" "Vulkan diagnostics must wire debug utils messages privately")
    require_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "synchronization2" "Vulkan backend must explicitly query or enable synchronization2")
    require_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "dynamicRendering" "Vulkan backend must explicitly query or enable dynamic rendering")
    require_text("${vulkan_rhi_path}" "${vulkan_rhi_text}" "VK_EXT_MEMORY_BUDGET_EXTENSION_NAME" "Vulkan backend should enable memory budget reporting when the adapter exposes it")
endif()

set(vulkan_interface_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp")
read_required_file("${vulkan_interface_path}" vulkan_interface_text)
if(vulkan_interface_text)
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "ERhiBackendApi::Vulkan" "Vulkan RenderHardwareInterface must identify its backend API")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "CookedShaderBinaryFormat::SpirV" "Vulkan RenderHardwareInterface must require SPIR-V shader binaries")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "GetBackBufferResource" "Vulkan RenderHardwareInterface must expose swapchain images through the normal back buffer API")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "RhiResourceViewHandle" "Vulkan back buffer image views must be tracked through the neutral resource view model")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "vkCmdBeginRendering" "Vulkan present rendering must use the backend command buffer path")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "vkCmdPipelineBarrier2" "Vulkan present resources must transition through backend-private image barriers")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "CreateTextureResource" "Vulkan must create textures from RhiTextureResourceDesc through the neutral RHI path")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "CreateBufferResource" "Vulkan must create buffers from RhiBufferResourceDesc through the neutral RHI path")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT" "Vulkan static mesh vertex buffers must be real Vulkan vertex buffers")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "VK_BUFFER_USAGE_INDEX_BUFFER_BIT" "Vulkan static mesh index buffers must be real Vulkan index buffers")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "RhiMemoryResidencyClass::HostUpload" "Vulkan upload/staging parity must use host-visible VMA allocations without exposing Vulkan layouts to Renderer")
    require_text("${vulkan_interface_path}" "${vulkan_interface_text}" "ToResourceStateMapping" "Vulkan present transitions must route through the shared ResourceState mapping")
endif()

set(vulkan_type_conversions_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/VulkanTypeConversions.cpp")
read_required_file("${vulkan_type_conversions_path}" vulkan_type_conversions_text)
if(vulkan_type_conversions_text)
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "BuildBufferCreateInfo" "Vulkan buffer resource creation must be driven by RhiBufferResourceDesc")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "BuildTextureCreateInfo" "Vulkan texture resource creation must be driven by RhiTextureResourceDesc")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "ToResourceStateMapping" "Vulkan must keep ResourceState-to-layout/access/stage translation centralized")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "IsBufferResourceStateSupported" "Vulkan must reject ResourceState combinations that are invalid for buffers")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "IsImageResourceStateSupported" "Vulkan must reject ResourceState combinations that are invalid for images")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "VkPipelineStageFlags2" "Vulkan ResourceState mapping must use synchronization2 stage masks")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "VkAccessFlags2" "Vulkan ResourceState mapping must use synchronization2 access masks")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "VkImageLayout" "Vulkan ResourceState mapping must define image layouts explicitly")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT" "Vulkan render-target states must map to color attachment output stages")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT" "Vulkan render-target states must map explicit access masks")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL" "Vulkan render-target states must map explicit image layouts")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR" "Vulkan present states must map explicit present layouts")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "ToVkCompareOp" "Vulkan PSO creation must map neutral depth/stencil compare state centrally")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "ToVkStencilOp" "Vulkan PSO creation must map neutral stencil state centrally")
    require_text("${vulkan_type_conversions_path}" "${vulkan_type_conversions_text}" "ToVkCullModeFlags" "Vulkan PSO creation must map neutral raster state centrally")
endif()

set(vulkan_shader_module_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Pipeline/VulkanShaderModule.cpp")
read_required_file("${vulkan_shader_module_path}" vulkan_shader_module_text)
if(vulkan_shader_module_text)
    require_text("${vulkan_shader_module_path}" "${vulkan_shader_module_text}" "CookedShaderBinaryFormat::SpirV" "Vulkan shader modules must resolve SPIR-V cooked binaries privately")
    require_text("${vulkan_shader_module_path}" "${vulkan_shader_module_text}" "vkCreateShaderModule" "Vulkan shader modules must own native shader-module creation")
    require_text("${vulkan_shader_module_path}" "${vulkan_shader_module_text}" "vkDestroyShaderModule" "Vulkan shader modules must release native shader modules")
    require_text("${vulkan_shader_module_path}" "${vulkan_shader_module_text}" "DebugArtifact" "Vulkan pipeline errors must surface cooked shader debug artifacts where available")
endif()

set(vulkan_binding_layout_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Pipeline/VulkanBindingLayout.cpp")
read_required_file("${vulkan_binding_layout_path}" vulkan_binding_layout_text)
if(vulkan_binding_layout_text)
    require_text("${vulkan_binding_layout_path}" "${vulkan_binding_layout_text}" "vkCreateDescriptorSetLayout" "Vulkan binding layouts must compile reflection into descriptor set layouts for pipeline layout compatibility")
    require_text("${vulkan_binding_layout_path}" "${vulkan_binding_layout_text}" "vkDestroyDescriptorSetLayout" "Vulkan binding layouts must own descriptor set layout lifetime")
    require_text("${vulkan_binding_layout_path}" "${vulkan_binding_layout_text}" "CookedShaderBinaryFormat::SpirV" "Vulkan binding layout reflection must use SPIR-V records")
    require_text("${vulkan_binding_layout_path}" "${vulkan_binding_layout_text}" "VkPushConstantRange" "Vulkan binding layouts must carry push constant ranges into pipeline layouts")
endif()

set(vulkan_pipeline_state_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Pipeline/VulkanPipelineState.cpp")
read_required_file("${vulkan_pipeline_state_path}" vulkan_pipeline_state_text)
if(vulkan_pipeline_state_text)
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "VulkanPipelineCacheKey" "Vulkan PSO cache keys must account for backend-relevant native pipeline state privately")
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "vkCreatePipelineLayout" "Vulkan PSOs must create native pipeline layouts privately")
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "vkDestroyPipelineLayout" "Vulkan PSOs must release pipeline layouts privately")
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "vkCreateGraphicsPipelines" "Vulkan graphics PSOs must create native graphics pipelines")
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "vkCreateComputePipelines" "Vulkan compute PSOs must create native compute pipelines")
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "vkDestroyPipeline" "Vulkan PSOs must release native pipelines")
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "VkPipelineRenderingCreateInfo" "Vulkan graphics PSOs must target dynamic rendering")
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "VK_DYNAMIC_STATE_VIEWPORT" "Vulkan graphics PSOs must leave viewport state dynamic")
    require_text("${vulkan_pipeline_state_path}" "${vulkan_pipeline_state_text}" "VK_DYNAMIC_STATE_SCISSOR" "Vulkan graphics PSOs must leave scissor state dynamic")
endif()

set(d3d12_type_conversions_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/D3D12/D3D12TypeConversions.cpp")
read_required_file("${d3d12_type_conversions_path}" d3d12_type_conversions_text)
if(d3d12_type_conversions_text)
    require_text("${d3d12_type_conversions_path}" "${d3d12_type_conversions_text}" "ToResourceStates" "D3D12 ResourceState translation must stay explicit and testable")
    require_text("${d3d12_type_conversions_path}" "${d3d12_type_conversions_text}" "D3D12_RESOURCE_STATE_RENDER_TARGET" "D3D12 render-target state mapping must be explicit")
    require_text("${d3d12_type_conversions_path}" "${d3d12_type_conversions_text}" "D3D12_RESOURCE_STATE_DEPTH_WRITE" "D3D12 depth-write state mapping must be explicit")
    require_text("${d3d12_type_conversions_path}" "${d3d12_type_conversions_text}" "D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE" "D3D12 shader-resource state mapping must include shader stages explicitly")
    require_text("${d3d12_type_conversions_path}" "${d3d12_type_conversions_text}" "D3D12_RESOURCE_STATE_UNORDERED_ACCESS" "D3D12 unordered-access state mapping must be explicit")
    require_text("${d3d12_type_conversions_path}" "${d3d12_type_conversions_text}" "D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE" "D3D12 ray tracing acceleration structure state mapping must be explicit")
    require_text("${d3d12_type_conversions_path}" "${d3d12_type_conversions_text}" "D3D12_RESOURCE_STATE_COPY_SOURCE" "D3D12 copy-source state mapping must be explicit")
    require_text("${d3d12_type_conversions_path}" "${d3d12_type_conversions_text}" "D3D12_RESOURCE_STATE_PRESENT" "D3D12 present state mapping must be explicit")
endif()

set(vulkan_swapchain_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/SwapChain/VulkanSwapChain.cpp")
read_required_file("${vulkan_swapchain_path}" vulkan_swapchain_text)
if(vulkan_swapchain_text)
    require_text("${vulkan_swapchain_path}" "${vulkan_swapchain_text}" "vkCreateWin32SurfaceKHR" "Vulkan swapchain must create a backend-private Win32 surface")
    require_text("${vulkan_swapchain_path}" "${vulkan_swapchain_text}" "vkCreateSwapchainKHR" "Vulkan swapchain must own native swapchain creation")
    require_text("${vulkan_swapchain_path}" "${vulkan_swapchain_text}" "vkGetSwapchainImagesKHR" "Vulkan swapchain images must be wrapped as backend records")
    require_text("${vulkan_swapchain_path}" "${vulkan_swapchain_text}" "vkCreateImageView" "Vulkan back buffers must receive image views")
    require_text("${vulkan_swapchain_path}" "${vulkan_swapchain_text}" "vkAcquireNextImageKHR" "Vulkan acquire flow must be backend-private")
    require_text("${vulkan_swapchain_path}" "${vulkan_swapchain_text}" "vkQueuePresentKHR" "Vulkan present flow must be backend-private")
    require_text("${vulkan_swapchain_path}" "${vulkan_swapchain_text}" "RenderConfig::BackBufferFormat" "Vulkan present format selection must prefer the shared back buffer format")
endif()

set(vulkan_command_context_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Commands/VulkanCommandContext.cpp")
read_required_file("${vulkan_command_context_path}" vulkan_command_context_text)
if(vulkan_command_context_text)
    require_text("${vulkan_command_context_path}" "${vulkan_command_context_text}" "vkCreateCommandPool" "Vulkan command buffers must be allocated from backend-private command pools")
    require_text("${vulkan_command_context_path}" "${vulkan_command_context_text}" "vkBeginCommandBuffer" "Vulkan command context must begin command recording")
    require_text("${vulkan_command_context_path}" "${vulkan_command_context_text}" "vkEndCommandBuffer" "Vulkan command context must end command recording")
    require_text("${vulkan_command_context_path}" "${vulkan_command_context_text}" "vkQueueSubmit" "Vulkan command context must own queue submission")
    require_text("${vulkan_command_context_path}" "${vulkan_command_context_text}" "vkCreateSemaphore" "Vulkan present synchronization must stay backend-private")
    require_text("${vulkan_command_context_path}" "${vulkan_command_context_text}" "vkCreateFence" "Vulkan frame retirement must use backend-private fences")
    require_text("${vulkan_command_context_path}" "${vulkan_command_context_text}" "RetireFenceValue" "Vulkan frame retirement must track delayed-destruction-compatible fence tickets")
endif()

set(vulkan_command_list_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Commands/VulkanRenderCommandList.cpp")
read_required_file("${vulkan_command_list_path}" vulkan_command_list_text)
if(vulkan_command_list_text)
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdSetViewport" "Vulkan command list must translate viewport commands")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdSetScissor" "Vulkan command list must translate scissor commands")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdClearAttachments" "Vulkan command list must translate render-target clears")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "m_beginDebugUtilsLabel" "Vulkan command list must support debug markers when debug utils are available")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdBindVertexBuffers" "Vulkan command list must bind vertex buffers created through the neutral mesh path")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdBindIndexBuffer" "Vulkan command list must bind index buffers created through the neutral mesh path")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdCopyBuffer" "Vulkan command list must translate neutral buffer copies privately")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdCopyImage" "Vulkan command list must translate neutral image copies privately")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "FindAllocationRecord" "Vulkan command list must resolve opaque resources through backend-private metadata")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdPipelineBarrier2" "Vulkan barriers must use synchronization2")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "VkBufferMemoryBarrier2" "Vulkan buffer transitions must be typed and explicit")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "VkImageMemoryBarrier2" "Vulkan image transitions must be typed and explicit")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "ToResourceStateMapping" "Vulkan command-list barriers must consume the centralized ResourceState mapping")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdBeginRendering" "Vulkan FrameGraph attachment execution must use dynamic rendering")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdEndRendering" "Vulkan FrameGraph attachment execution must close dynamic rendering before non-render work")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdBindPipeline" "Vulkan command list must bind backend-private pipeline state")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdDrawIndexed" "Vulkan command list must execute indexed draws")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdDraw" "Vulkan command list must execute non-indexed draws")
    require_text("${vulkan_command_list_path}" "${vulkan_command_list_text}" "vkCmdDispatch" "Vulkan command list must execute compute dispatches")
endif()

set(vulkan_memory_allocator_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Memory/VulkanGpuMemoryAllocator.cpp")
read_required_file("${vulkan_memory_allocator_path}" vulkan_memory_allocator_text)
if(vulkan_memory_allocator_text)
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "VMA_IMPLEMENTATION" "Vulkan backend must compile the VMA implementation in one backend-private translation unit")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vk_mem_alloc.h" "Vulkan backend allocator must include VMA privately")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaCreateAllocator" "Vulkan backend must create a VMA allocator")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaCreateBuffer" "Vulkan buffers must be created through VMA")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaCreateImage" "Vulkan images must be created through VMA")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaGetHeapBudgets" "Vulkan memory diagnostics must expose VMA budget snapshots")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaBuildStatsString" "Vulkan memory diagnostics must expose VMA JSON/stat dumps")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaDestroyBuffer" "Vulkan delayed destruction must release buffers through VMA")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaDestroyImage" "Vulkan delayed destruction must release images through VMA")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "CreateMemoryUsageSnapshot" "Vulkan memory diagnostics must use the shared public snapshot structures")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "WriteAllocatorJsonDump" "Vulkan memory diagnostics must mirror D3D12 allocator dump behavior")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "QueueDestroyResource" "Vulkan owned resources must support delayed destruction")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaMapMemory" "Vulkan upload/staging must use VMA host-visible mappings")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "vmaFlushAllocation" "Vulkan upload writes must flush host-visible allocations through VMA")
    require_text("${vulkan_memory_allocator_path}" "${vulkan_memory_allocator_text}" "FindAllocationRecord" "Vulkan barriers and copies must have backend-private resource metadata lookup")
endif()

set(vulkan_memory_record_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Memory/VulkanGpuAllocation.h")
read_required_file("${vulkan_memory_record_path}" vulkan_memory_record_text)
if(vulkan_memory_record_text)
    require_text("${vulkan_memory_record_path}" "${vulkan_memory_record_text}" "VulkanGpuAllocationRecord" "Vulkan memory records must track live backend allocations")
    require_text("${vulkan_memory_record_path}" "${vulkan_memory_record_text}" "RhiMemoryCategory" "Vulkan allocation records must track shared memory categories")
    require_text("${vulkan_memory_record_path}" "${vulkan_memory_record_text}" "RhiMemoryResidencyClass" "Vulkan allocation records must track shared residency classes")
    require_text("${vulkan_memory_record_path}" "${vulkan_memory_record_text}" "DebugName" "Vulkan allocation records must retain debug names for diagnostics")
    require_text("${vulkan_memory_record_path}" "${vulkan_memory_record_text}" "ResourceSizeInBytes" "Vulkan allocation records must retain copy/barrier size metadata")
    require_text("${vulkan_memory_record_path}" "${vulkan_memory_record_text}" "AspectMask" "Vulkan image records must retain subresource aspect metadata for barriers")
endif()

set(vulkan_backend_root "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private/Vulkan")
if(EXISTS "${vulkan_backend_root}")
    file(GLOB_RECURSE vulkan_backend_files
        "${vulkan_backend_root}/*.h"
        "${vulkan_backend_root}/*.hpp"
        "${vulkan_backend_root}/*.cpp"
        "${vulkan_backend_root}/*.cxx"
    )

    foreach(vulkan_backend_file IN LISTS vulkan_backend_files)
        cmake_path(RELATIVE_PATH vulkan_backend_file BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE vulkan_relative_path)
        if(vulkan_relative_path MATCHES "^Engine/RHI/Private/Vulkan/Memory/")
            continue()
        endif()

        file(READ "${vulkan_backend_file}" vulkan_backend_file_text)
        foreach(token IN ITEMS "vkCreateBuffer(" "vkCreateImage(" "vkAllocateMemory(")
            string(FIND "${vulkan_backend_file_text}" "${token}" vulkan_direct_allocation_index)
            if(NOT vulkan_direct_allocation_index EQUAL -1)
                append_rhi_backend_violation("${vulkan_relative_path}: found forbidden '${token}': normal Vulkan resources must be allocated through VulkanGpuMemoryAllocator/VMA")
            endif()
        endforeach()
    endforeach()
endif()

set(renderer_cmake_path "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/Renderer/CMakeLists.txt")
read_required_file("${renderer_cmake_path}" renderer_cmake_text)
if(renderer_cmake_text)
    foreach(token IN ITEMS d3d12 dxgi d3dcompiler dxguid vulkan SparkleD3D12MA SparkleVMA VMA)
        forbid_text("${renderer_cmake_path}" "${renderer_cmake_text}" "${token}" "Renderer must link through RHI targets instead of raw backend APIs or allocator libraries")
    endforeach()
endif()

set(common_rhi_private_root "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/RHI/Private")
if(EXISTS "${common_rhi_private_root}")
    file(GLOB_RECURSE rhi_private_files
        "${common_rhi_private_root}/*.h"
        "${common_rhi_private_root}/*.hpp"
        "${common_rhi_private_root}/*.cpp"
        "${common_rhi_private_root}/*.cxx"
    )

    foreach(rhi_private_file IN LISTS rhi_private_files)
        cmake_path(RELATIVE_PATH rhi_private_file BASE_DIRECTORY "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        is_rhi_backend_private_path("${relative_path}" is_backend_path)
        if(is_backend_path)
            continue()
        endif()

        check_file_for_backend_tokens(
            "${rhi_private_file}"
            TOKENS
                "#include \"D3D12/"
                "#include <d3d12"
                "#include <dxgi"
                "#include \"Vulkan/"
                "#include <vulkan/"
                "D3D12MemAlloc"
                "VulkanMemoryAllocator"
                "vk_mem_alloc"
                "VkInstance"
                "VkPhysicalDevice"
                "VkDevice"
                "VkQueue"
                "VkCommandBuffer"
                "VkImage"
                "VkBuffer"
                "VmaAllocator"
                "VmaAllocation"
            DESCRIPTION "common RHI private code must not include backend-private native headers or allocator libraries"
        )
    endforeach()
endif()

set(high_level_roots
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/Application"
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/Editor"
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/GameFramework"
    "${RHI_BACKEND_BOUNDARY_SOURCE_DIR}/Engine/Renderer"
)

foreach(high_level_root IN LISTS high_level_roots)
    if(NOT EXISTS "${high_level_root}")
        continue()
    endif()

    file(GLOB_RECURSE high_level_files
        "${high_level_root}/*.h"
        "${high_level_root}/*.hpp"
        "${high_level_root}/*.cpp"
        "${high_level_root}/*.cxx"
        "${high_level_root}/CMakeLists.txt"
    )

    foreach(high_level_file IN LISTS high_level_files)
        check_file_for_backend_tokens(
            "${high_level_file}"
            TOKENS
                "#include \"D3D12/"
                "#include <d3d12"
                "#include <dxgi"
                "#include \"Vulkan/"
                "#include <vulkan/"
                "D3D12MemAlloc"
                "VulkanMemoryAllocator"
                "vk_mem_alloc"
                "VkInstance"
                "VkPhysicalDevice"
                "VkDevice"
                "VkQueue"
                "VkCommandBuffer"
                "VkImage"
                "VkBuffer"
                "VmaAllocator"
                "VmaAllocation"
                "SetShaderVisibleDescriptorHeaps"
                "SetDescriptorHeaps"
                "NativeDescriptorHeapHandle"
                "RootParameterIndex"
                "SetRoot32BitConstants"
                "BindRootShaderResourceView"
                "BindRootUnorderedAccessView"
                "RootConstants"
                "RootSignature"
            DESCRIPTION "Application, Editor, GameFramework, and Renderer must use backend-neutral RHI surfaces"
        )
    endforeach()
endforeach()

if(RHI_BACKEND_BOUNDARY_VIOLATIONS)
    string(PREPEND RHI_BACKEND_BOUNDARY_VIOLATIONS
        "RHI backend boundary validation failed. Keep common RHI and Renderer backend-neutral, keep backend API libraries on backend targets, and keep public RHI vocabulary backend-agnostic.\n")
    message(FATAL_ERROR "${RHI_BACKEND_BOUNDARY_VIOLATIONS}")
endif()

message(STATUS "RHI backend boundary check passed for CMake target split, Renderer link hygiene, common-source backend leakage, and public RHI vocabulary.")