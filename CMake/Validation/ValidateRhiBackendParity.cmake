if(NOT DEFINED RHI_BACKEND_PARITY_SOURCE_DIR)
    set(RHI_BACKEND_PARITY_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../..")
endif()

get_filename_component(
    RHI_BACKEND_PARITY_SOURCE_DIR
    "${RHI_BACKEND_PARITY_SOURCE_DIR}"
    ABSOLUTE
    BASE_DIR "${CMAKE_CURRENT_LIST_DIR}/../.."
)

cmake_path(NORMAL_PATH RHI_BACKEND_PARITY_SOURCE_DIR)

set(RHI_BACKEND_PARITY_VIOLATIONS "")

function(append_rhi_backend_parity_violation message_text)
    set(RHI_BACKEND_PARITY_VIOLATIONS
        "${RHI_BACKEND_PARITY_VIOLATIONS}${message_text}\n"
        PARENT_SCOPE)
endfunction()

function(read_required_rhi_backend_parity_file file_path out_text)
    if(NOT EXISTS "${file_path}")
        append_rhi_backend_parity_violation("missing required file: ${file_path}")
        set(${out_text} "" PARENT_SCOPE)
        return()
    endif()

    file(READ "${file_path}" file_text)
    set(${out_text} "${file_text}" PARENT_SCOPE)
endfunction()

function(require_rhi_backend_parity_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_BACKEND_PARITY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_rhi_backend_parity_violation("${relative_path}: missing '${token}': ${description}")
    endif()
endfunction()

function(forbid_rhi_backend_parity_text file_path text token description)
    string(FIND "${text}" "${token}" match_index)
    if(NOT match_index EQUAL -1)
        cmake_path(RELATIVE_PATH file_path BASE_DIRECTORY "${RHI_BACKEND_PARITY_SOURCE_DIR}" OUTPUT_VARIABLE relative_path)
        append_rhi_backend_parity_violation("${relative_path}: found forbidden '${token}': ${description}")
    endif()
endfunction()

function(require_backend_method_parity interface_path interface_text header_path header_text source_path source_text class_name method_list_name)
    foreach(method_name IN LISTS ${method_list_name})
        require_rhi_backend_parity_text(
            "${interface_path}"
            "${interface_text}"
            "${method_name}("
            "public interface must expose ${method_name} for backend parity checks"
        )
        require_rhi_backend_parity_text(
            "${header_path}"
            "${header_text}"
            "${method_name}("
            "${class_name} must declare the public ${method_name} override"
        )
        require_rhi_backend_parity_text(
            "${source_path}"
            "${source_text}"
            "${class_name}::${method_name}"
            "${class_name} must define the public ${method_name} override or explicit unsupported behavior"
        )
    endforeach()
endfunction()

function(check_rhi_backend_parity_tree_for_tokens root_path description)
    if(NOT EXISTS "${root_path}")
        append_rhi_backend_parity_violation("missing required source root: ${root_path}")
        return()
    endif()

    file(GLOB_RECURSE checked_files
        "${root_path}/*.h"
        "${root_path}/*.hpp"
        "${root_path}/*.cpp"
        "${root_path}/*.cxx"
    )

    foreach(checked_file IN LISTS checked_files)
        file(READ "${checked_file}" checked_text)
        foreach(token IN LISTS ARGN)
            forbid_rhi_backend_parity_text("${checked_file}" "${checked_text}" "${token}" "${description}")
        endforeach()
    endforeach()
endfunction()

set(rhi_interface_methods
    GetBackendApi
    GetRequiredShaderBinaryFormat
    GetCurrentFrameIndex
    WaitForIdle
    GetDeviceHandle
    GetGraphicsQueueHandle
    GetGraphicsCommandList
    GetRayTracingCapabilities
    GetDiagnostics
    InitializeImGuiBackend
    BeginImGuiFrame
    RenderImGuiDrawData
    ShutdownImGuiBackend
    CreateBindingLayout
    CreateGraphicsPipelineState
    CreateComputePipelineState
    BindGlobalDescriptorState
    AllocateDescriptor
    ReleaseDescriptor
    AllocateDescriptorTable
    GetDescriptorTableCpuHandle
    ReleaseDescriptorTable
    AllocateShaderResourceDescriptor
    ReleaseShaderResourceDescriptor
    GetPerFrameConstantData
    GetPerFrameConstantGpuAddress
    AllocateUniformConstantBuffer
    AllocatePerViewConstantBuffer
    AllocatePerObjectVertexConstants
    AllocatePerObjectPixelConstants
    GetSharedSamplerBinding
    GetBackBufferViewport
    GetBackBufferScissorRect
    GetBackBufferRenderTargetView
    GetBackBufferResource
    CreateTextureFromPath
    CreateTextureResource
    CreateBufferResource
    CreateVertexBuffer
    CreateIndexBuffer
    ReleaseOwnedResource
    GetNativeResource
    GetResourceGpuVirtualAddress
    GetBottomLevelAccelerationStructurePrebuildInfo
    GetTopLevelAccelerationStructurePrebuildInfo
    CreateRayTracingScratchBuffer
    CreateRayTracingAccelerationStructureBuffer
    CreateRayTracingInstanceBuffer
    GetTextureAllocationInfo
    GetBufferAllocationInfo
    CreateTransientMemoryBlock
    ReleaseTransientMemoryBlock
    CreateAliasingTextureResource
    CreateAliasingBufferResource
    CreateResourceView
    ReleaseResourceView
    GetResourceViewCpuHandle
    GetResourceViewGpuHandle
    SupportsUnorderedAccess
    BeginPresentRenderPass
    BeginPresentOverlayPass
    EndPresentRenderPass
    GetPresentColorFormat
)

set(command_list_methods
    GetBackendApi
    GetNativeHandle
    SupportsDiagnosticScopes
    BeginDiagnosticScope
    EndDiagnosticScope
    InsertDiagnosticMarker
    SetPipelineState
    SetGraphicsBindingLayout
    SetComputeBindingLayout
    BindGraphicsConstantBuffer
    BindGraphicsShaderResource
    BindGraphicsUnorderedAccess
    BindGraphicsDescriptorTable
    SetGraphicsPushConstants
    BindComputeConstantBuffer
    BindComputeShaderResource
    BindComputeUnorderedAccess
    BindComputeDescriptorTable
    SetComputePushConstants
    SetPrimitiveTopology
    BindVertexBuffer
    BindIndexBuffer
    SetRenderTarget
    SetRenderTargets
    ClearRenderTarget
    ClearDepthStencil
    SetViewport
    SetScissorRect
    DrawIndexedInstanced
    DrawInstanced
    Dispatch
    BuildBottomLevelAccelerationStructure
    BuildTopLevelAccelerationStructure
    CopyResource
    AliasResource
    TransitionResource
    UnorderedAccessBarrier
)

set(rhi_interface_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Public/Device/RenderHardwareInterface.h")
set(d3d12_rhi_header_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.h")
set(d3d12_rhi_source_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/D3D12/D3D12RenderHardwareInterface.cpp")
set(vulkan_rhi_header_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.h")
set(vulkan_rhi_source_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/VulkanRenderHardwareInterface.cpp")

read_required_rhi_backend_parity_file("${rhi_interface_path}" rhi_interface_text)
read_required_rhi_backend_parity_file("${d3d12_rhi_header_path}" d3d12_rhi_header_text)
read_required_rhi_backend_parity_file("${d3d12_rhi_source_path}" d3d12_rhi_source_text)
read_required_rhi_backend_parity_file("${vulkan_rhi_header_path}" vulkan_rhi_header_text)
read_required_rhi_backend_parity_file("${vulkan_rhi_source_path}" vulkan_rhi_source_text)

if(rhi_interface_text AND d3d12_rhi_header_text AND d3d12_rhi_source_text)
    require_backend_method_parity(
        "${rhi_interface_path}"
        "${rhi_interface_text}"
        "${d3d12_rhi_header_path}"
        "${d3d12_rhi_header_text}"
        "${d3d12_rhi_source_path}"
        "${d3d12_rhi_source_text}"
        "D3D12RenderHardwareInterface"
        rhi_interface_methods
    )
endif()

if(rhi_interface_text AND vulkan_rhi_header_text AND vulkan_rhi_source_text)
    require_backend_method_parity(
        "${rhi_interface_path}"
        "${rhi_interface_text}"
        "${vulkan_rhi_header_path}"
        "${vulkan_rhi_header_text}"
        "${vulkan_rhi_source_path}"
        "${vulkan_rhi_source_text}"
        "VulkanRenderHardwareInterface"
        rhi_interface_methods
    )
endif()

set(command_interface_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Public/Commands/RenderCommandList.h")
set(d3d12_command_header_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/D3D12/D3D12RenderCommandList.h")
set(d3d12_command_source_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/D3D12/D3D12RenderCommandList.cpp")
set(vulkan_command_header_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Commands/VulkanRenderCommandList.h")
set(vulkan_command_source_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Commands/VulkanRenderCommandList.cpp")

read_required_rhi_backend_parity_file("${command_interface_path}" command_interface_text)
read_required_rhi_backend_parity_file("${d3d12_command_header_path}" d3d12_command_header_text)
read_required_rhi_backend_parity_file("${d3d12_command_source_path}" d3d12_command_source_text)
read_required_rhi_backend_parity_file("${vulkan_command_header_path}" vulkan_command_header_text)
read_required_rhi_backend_parity_file("${vulkan_command_source_path}" vulkan_command_source_text)

if(command_interface_text AND d3d12_command_header_text AND d3d12_command_source_text)
    require_backend_method_parity(
        "${command_interface_path}"
        "${command_interface_text}"
        "${d3d12_command_header_path}"
        "${d3d12_command_header_text}"
        "${d3d12_command_source_path}"
        "${d3d12_command_source_text}"
        "D3D12RenderCommandList"
        command_list_methods
    )
endif()

if(command_interface_text AND vulkan_command_header_text AND vulkan_command_source_text)
    require_backend_method_parity(
        "${command_interface_path}"
        "${command_interface_text}"
        "${vulkan_command_header_path}"
        "${vulkan_command_header_text}"
        "${vulkan_command_source_path}"
        "${vulkan_command_source_text}"
        "VulkanRenderCommandList"
        command_list_methods
    )
endif()

if(d3d12_rhi_source_text)
    require_rhi_backend_parity_text("${d3d12_rhi_source_path}" "${d3d12_rhi_source_text}" "return ERhiBackendApi::D3D12" "D3D12 backend must identify itself through the neutral backend enum")
    require_rhi_backend_parity_text("${d3d12_rhi_source_path}" "${d3d12_rhi_source_text}" "return CookedShaderBinaryFormat::Dxil" "D3D12 backend must request DXIL shader variants")
    require_rhi_backend_parity_text("${d3d12_rhi_source_path}" "${d3d12_rhi_source_text}" "CreateRenderDiagnostics" "D3D12 backend must expose RenderDiagnostics through the shared facade")
endif()

if(vulkan_rhi_source_text)
    require_rhi_backend_parity_text("${vulkan_rhi_source_path}" "${vulkan_rhi_source_text}" "return ERhiBackendApi::Vulkan" "Vulkan backend must identify itself through the neutral backend enum")
    require_rhi_backend_parity_text("${vulkan_rhi_source_path}" "${vulkan_rhi_source_text}" "return CookedShaderBinaryFormat::SpirV" "Vulkan backend must request SPIR-V shader variants")
    require_rhi_backend_parity_text("${vulkan_rhi_source_path}" "${vulkan_rhi_source_text}" "CreateRenderDiagnostics" "Vulkan backend must expose RenderDiagnostics through the shared facade")
    require_rhi_backend_parity_text("${vulkan_rhi_source_path}" "${vulkan_rhi_source_text}" "FailRenderingNotImplemented(\"CreateRayTracingScratchBuffer\")" "deferred Vulkan RT scratch support must fail explicitly")
    require_rhi_backend_parity_text("${vulkan_rhi_source_path}" "${vulkan_rhi_source_text}" "FailRenderingNotImplemented(\"CreateRayTracingAccelerationStructureBuffer\")" "deferred Vulkan RT acceleration-structure support must fail explicitly")
    require_rhi_backend_parity_text("${vulkan_rhi_source_path}" "${vulkan_rhi_source_text}" "FailRenderingNotImplemented(\"CreateRayTracingInstanceBuffer\")" "deferred Vulkan RT instance buffer support must fail explicitly")
endif()

if(vulkan_command_source_text)
    require_rhi_backend_parity_text("${vulkan_command_source_path}" "${vulkan_command_source_text}" "FailVulkanRayTracingCommandUnsupported" "deferred Vulkan AS build commands must fail explicitly instead of becoming no-ops")
endif()

set(memory_diagnostics_tokens
    SupportsBudgetQueries
    SupportsJsonDump
    GetLatestMemorySnapshot
    WriteAllocatorJsonDump
    SupportsMemoryBudgetQueries
    SupportsMemoryJsonDump
)

foreach(backend_name IN ITEMS D3D12 Vulkan)
    if(backend_name STREQUAL "D3D12")
        set(memory_diagnostics_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/D3D12/Diagnostics/D3D12RenderDiagnostics.cpp")
        set(memory_allocator_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/D3D12/Memory/D3D12GpuMemoryAllocator.cpp")
    else()
        set(memory_diagnostics_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Diagnostics/VulkanRenderDiagnostics.cpp")
        set(memory_allocator_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Memory/VulkanGpuMemoryAllocator.cpp")
    endif()

    read_required_rhi_backend_parity_file("${memory_diagnostics_path}" memory_diagnostics_text)
    read_required_rhi_backend_parity_file("${memory_allocator_path}" memory_allocator_text)
    if(memory_diagnostics_text)
        foreach(token IN LISTS memory_diagnostics_tokens)
            require_rhi_backend_parity_text("${memory_diagnostics_path}" "${memory_diagnostics_text}" "${token}" "${backend_name} diagnostics must expose shared memory capability and snapshot parity")
        endforeach()
    endif()
    if(memory_allocator_text)
        require_rhi_backend_parity_text("${memory_allocator_path}" "${memory_allocator_text}" "CreateMemoryUsageSnapshot" "${backend_name} allocator must provide shared memory usage snapshots")
        require_rhi_backend_parity_text("${memory_allocator_path}" "${memory_allocator_text}" "SupportsBudgetQueries" "${backend_name} allocator must report budget query support")
        require_rhi_backend_parity_text("${memory_allocator_path}" "${memory_allocator_text}" "SupportsJsonDump" "${backend_name} allocator must report JSON dump support")
        require_rhi_backend_parity_text("${memory_allocator_path}" "${memory_allocator_text}" "RhiMemoryCategory::FrameGraphTransient" "${backend_name} allocator must classify transient aliasing memory through shared categories")
    endif()
endforeach()

check_rhi_backend_parity_tree_for_tokens(
    "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Public"
    "public RHI contracts must not expose native backend implementation tokens"
    "#include <d3d12"
    "#include <dxgi"
    "#include <vulkan/"
    "ID3D12"
    "IDXGI"
    "D3D12_"
    "VkInstance"
    "VkDevice"
    "VkQueue"
    "VkCommandBuffer"
    "VkImage"
    "VkBuffer"
    "VmaAllocator"
    "VmaAllocation"
    "D3D12MA"
)

check_rhi_backend_parity_tree_for_tokens(
    "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/Renderer"
    "Renderer must stay backend-neutral and route graphics API work through public RHI contracts"
    "#include <d3d12"
    "#include <dxgi"
    "#include <vulkan/"
    "ID3D12"
    "IDXGI"
    "D3D12_"
    "VkInstance"
    "VkDevice"
    "VkQueue"
    "VkCommandBuffer"
    "VkImage"
    "VkBuffer"
    "VmaAllocator"
    "VmaAllocation"
    "D3D12MA"
)

file(GLOB_RECURSE vulkan_placeholder_files
    "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/*.gitkeep"
)
foreach(vulkan_placeholder_file IN LISTS vulkan_placeholder_files)
    cmake_path(RELATIVE_PATH vulkan_placeholder_file BASE_DIRECTORY "${RHI_BACKEND_PARITY_SOURCE_DIR}" OUTPUT_VARIABLE relative_placeholder_path)
    append_rhi_backend_parity_violation("${relative_placeholder_path}: stale Vulkan placeholder file remains after backend implementation")
endforeach()

set(backend_selection_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Core/RhiBackendSelection.cpp")
set(render_device_services_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Device/RenderDeviceServices.cpp")
set(smoke_validation_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/Application/Private/Validation/RhiSmokeValidation.cpp")
set(vulkan_device_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/Vulkan/Device/VulkanRhi.cpp")
set(d3d12_device_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/Engine/RHI/Private/D3D12/D3D12Rhi.cpp")
set(smoke_doc_path "${RHI_BACKEND_PARITY_SOURCE_DIR}/docs/plans/backend-runtime-smoke-tests.md")

read_required_rhi_backend_parity_file("${backend_selection_path}" backend_selection_text)
read_required_rhi_backend_parity_file("${render_device_services_path}" render_device_services_text)
read_required_rhi_backend_parity_file("${smoke_validation_path}" smoke_validation_text)
read_required_rhi_backend_parity_file("${vulkan_device_path}" vulkan_device_text)
read_required_rhi_backend_parity_file("${d3d12_device_path}" d3d12_device_text)
read_required_rhi_backend_parity_file("${smoke_doc_path}" smoke_doc_text)

if(backend_selection_text)
    require_rhi_backend_parity_text("${backend_selection_path}" "${backend_selection_text}" "SPARKLE_RHI_BACKEND" "runtime smoke tests must be able to select backend through environment")
    require_rhi_backend_parity_text("${backend_selection_path}" "${backend_selection_text}" "--rhi=" "runtime smoke tests must be able to select backend through command line")
    require_rhi_backend_parity_text("${backend_selection_path}" "${backend_selection_text}" "--graphics-api=" "runtime smoke tests must be able to select graphics API through command line")
endif()

if(render_device_services_text)
    require_rhi_backend_parity_text("${render_device_services_path}" "${render_device_services_text}" "Creating RHI backend" "runtime logs must name the selected backend")
    require_rhi_backend_parity_text("${render_device_services_path}" "${render_device_services_text}" "CreateD3D12RenderDeviceServices" "runtime backend factory must create D3D12 through backend-private services")
    require_rhi_backend_parity_text("${render_device_services_path}" "${render_device_services_text}" "CreateVulkanRenderDeviceServices" "runtime backend factory must create Vulkan through backend-private services")
endif()

if(smoke_validation_text)
    require_rhi_backend_parity_text("${smoke_validation_path}" "${smoke_validation_text}" "SPARKLE_SMOKE_VALIDATE_RHI" "runtime smoke validation must be opt-in through environment")
    require_rhi_backend_parity_text("${smoke_validation_path}" "${smoke_validation_text}" "RHI smoke diagnostics capabilities" "runtime smoke logs must show diagnostics capability support")
    require_rhi_backend_parity_text("${smoke_validation_path}" "${smoke_validation_text}" "SPARKLE_SMOKE_SHADER_RELOAD_FRAME" "runtime smoke validation must be able to exercise backend shader package reload")
endif()

if(vulkan_device_text)
    require_rhi_backend_parity_text("${vulkan_device_path}" "${vulkan_device_text}" "Selected Vulkan adapter" "Vulkan runtime logs must name selected adapter/device evidence")
    require_rhi_backend_parity_text("${vulkan_device_path}" "${vulkan_device_text}" "Vulkan features:" "Vulkan runtime logs must name feature support and enablement")
endif()

if(d3d12_device_text)
    require_rhi_backend_parity_text("${d3d12_device_path}" "${d3d12_device_text}" "DXR capability:" "D3D12 runtime logs must name ray tracing feature support")
endif()

if(smoke_doc_text)
    require_rhi_backend_parity_text("${smoke_doc_path}" "${smoke_doc_text}" "SPARKLE_SMOKE_VALIDATE_RHI" "smoke test docs must describe the opt-in validation hook")
    require_rhi_backend_parity_text("${smoke_doc_path}" "${smoke_doc_text}" "SPARKLE_RHI_BACKEND" "smoke test docs must describe environment backend selection")
    require_rhi_backend_parity_text("${smoke_doc_path}" "${smoke_doc_text}" "--rhi=" "smoke test docs must describe command-line backend selection")
    require_rhi_backend_parity_text("${smoke_doc_path}" "${smoke_doc_text}" "ShowcaseRuntime" "smoke test docs must cover runtime host launch evidence")
    require_rhi_backend_parity_text("${smoke_doc_path}" "${smoke_doc_text}" "ShowcaseEditor" "smoke test docs must cover editor host launch evidence")
    require_rhi_backend_parity_text("${smoke_doc_path}" "${smoke_doc_text}" "Creating RHI backend" "smoke test docs must name the selected-backend log evidence")
    require_rhi_backend_parity_text("${smoke_doc_path}" "${smoke_doc_text}" "RHI smoke diagnostics capabilities" "smoke test docs must name diagnostics capability log evidence")
endif()

if(RHI_BACKEND_PARITY_VIOLATIONS)
    string(PREPEND RHI_BACKEND_PARITY_VIOLATIONS
        "RHI backend parity validation failed. D3D12/Vulkan implementations must stay contract-complete, public and Renderer layers must stay backend-neutral, diagnostics/shader parity must be explicit, and runtime smoke evidence must be documented.\n")
    message(FATAL_ERROR "${RHI_BACKEND_PARITY_VIOLATIONS}")
endif()

message(STATUS "RHI backend parity check passed for backend contracts, public/Renderer forbidden tokens, memory diagnostics, shader variants, and runtime smoke evidence.")