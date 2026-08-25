#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"
#include "Vulkan/Pipeline/VulkanRayTracingPipeline.h"
#include "Vulkan/RayTracing/VulkanRayTracingShaderTable.h"

#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/VulkanTypeConversions.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Validation/RhiContract.h"

static const auto g_vulkanRenderCommandListLogger = Logging::GetOrCreateLogger("RHI.Vulkan.CommandList");

void VulkanRenderCommandList::SetRayTracingPipeline(const RayTracingPipeline& pipeline) noexcept
{
	const auto* nativePipeline = dynamic_cast<const VulkanRayTracingPipeline*>(&pipeline);
	if (m_commandBuffer == VK_NULL_HANDLE || nativePipeline == nullptr)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan ray-tracing pipeline binding received no command buffer or a foreign pipeline.");
	}
	m_rayTracingBindings.PipelineLayout = nativePipeline->GetPipelineLayout();
	vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, nativePipeline->GetPipeline());
	m_boundRayTracingPipeline = &pipeline;
}

void VulkanRenderCommandList::ConfigurePartitionedTlasInput(
    const RhiPartitionedTlasDesc& desc,
    VkPartitionedAccelerationStructureInstancesInputNV& input,
    VkPartitionedAccelerationStructureFlagsNV& flags) noexcept
{
	flags = VkPartitionedAccelerationStructureFlagsNV{
	    .sType = VK_STRUCTURE_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_FLAGS_NV,
	    .pNext = nullptr,
	    .enablePartitionTranslation = desc.AllowPartitionTranslation ? VK_TRUE : VK_FALSE};
	input = VkPartitionedAccelerationStructureInstancesInputNV{
	    .sType = VK_STRUCTURE_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCES_INPUT_NV,
	    .pNext = &flags,
	    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	    .instanceCount = desc.InstanceCapacity,
	    .maxInstancePerPartitionCount = desc.MaxInstancesPerPartition,
	    .partitionCount = desc.PartitionCount,
	    .maxInstanceInGlobalPartitionCount = desc.MaxInstancesInGlobalPartition};
}

void VulkanRenderCommandList::BuildBottomLevelAccelerationStructure(
    const RhiRayTracingGeometryDesc& geometry,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress) noexcept
{
	const VkDeviceAddress vertexBufferAddress = ResolveRayTracingBufferAddress(geometry.VertexBuffer);
	const VkDeviceAddress indexBufferAddress = ResolveRayTracingBufferAddress(geometry.IndexBuffer);
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_memoryAllocator == nullptr
	    || m_rhi->GetCmdBuildAccelerationStructures() == nullptr || !RhiContract::IsRayTracingGeometryDescUsable(geometry)
	    || vertexBufferAddress == 0 || indexBufferAddress == 0 || scratchGpuAddress == 0 || resultGpuAddress == 0)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan BLAS build received incomplete geometry, device, command-buffer, or GPU-address inputs.");
	}
	EndDynamicRenderingIfNeeded();

	TrackResource(geometry.VertexBuffer.Resource);
	TrackResource(geometry.IndexBuffer.Resource);

	VulkanRecordingResource resultResource;
	if (!ResolveAddress(resultGpuAddress, resultResource) || resultResource.AccelerationStructure == VK_NULL_HANDLE
	    || resultResource.AccelerationStructureType != VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan BLAS destination does not resolve to a bottom-level acceleration structure.");
	}

	const VkAccelerationStructureGeometryTrianglesDataKHR triangles{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
	    .pNext = nullptr,
	    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
	    .vertexData = VkDeviceOrHostAddressConstKHR{.deviceAddress = vertexBufferAddress},
	    .vertexStride = geometry.VertexStrideInBytes,
	    .maxVertex = geometry.VertexCount > 0 ? geometry.VertexCount - 1u : 0u,
	    .indexType = VulkanTypeConversions::ToVkIndexType(geometry.IndexFormat),
	    .indexData = VkDeviceOrHostAddressConstKHR{.deviceAddress = indexBufferAddress},
	    .transformData = VkDeviceOrHostAddressConstKHR{.deviceAddress = 0}};
	const VkAccelerationStructureGeometryKHR nativeGeometry{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = nullptr,
	    .geometryType = VK_GEOMETRY_TYPE_TRIANGLES_KHR,
	    .geometry = VkAccelerationStructureGeometryDataKHR{.triangles = triangles},
	    .flags = geometry.Opaque ? VK_GEOMETRY_OPAQUE_BIT_KHR : 0u};
	const VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	    .pNext = nullptr,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR,
	    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	    .srcAccelerationStructure = VK_NULL_HANDLE,
	    .dstAccelerationStructure = resultResource.AccelerationStructure,
	    .geometryCount = 1,
	    .pGeometries = &nativeGeometry,
	    .ppGeometries = nullptr,
	    .scratchData = VkDeviceOrHostAddressKHR{.deviceAddress = scratchGpuAddress}};
	const std::uint32_t primitiveCount = geometry.IndexCount / 3u;
	const VkAccelerationStructureBuildRangeInfoKHR rangeInfo{
	    .primitiveCount = primitiveCount,
	    .primitiveOffset = 0,
	    .firstVertex = 0,
	    .transformOffset = 0};
	const VkAccelerationStructureBuildRangeInfoKHR* rangeInfos[] = {&rangeInfo};
	m_rhi->GetCmdBuildAccelerationStructures()(m_commandBuffer, 1, &buildInfo, rangeInfos);

	const VkMemoryBarrier2 buildBarrier{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
	    .srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
	    .dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
	        | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
	    .dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR};
	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 1,
	    .pMemoryBarriers = &buildBarrier,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 0,
	    .pImageMemoryBarriers = nullptr};
	vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

void VulkanRenderCommandList::BuildTopLevelAccelerationStructure(
    RhiGpuVirtualAddress instanceDescsGpuAddress,
    std::uint32_t instanceCount,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress,
    ERhiClassicTlasBuildMode buildMode) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_memoryAllocator == nullptr
	    || m_rhi->GetCmdBuildAccelerationStructures() == nullptr || instanceDescsGpuAddress == 0 || scratchGpuAddress == 0
	    || resultGpuAddress == 0)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan classic TLAS build received no device, command buffer, build entry point, or GPU address.");
	}
	EndDynamicRenderingIfNeeded();

	VulkanRecordingResource resultResource;
	if (!ResolveAddress(resultGpuAddress, resultResource) || resultResource.AccelerationStructure == VK_NULL_HANDLE
	    || resultResource.AccelerationStructureType != VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan classic TLAS destination does not resolve to a top-level acceleration structure.");
	}

	const VkAccelerationStructureGeometryInstancesDataKHR instances{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
	    .pNext = nullptr,
	    .arrayOfPointers = VK_FALSE,
	    .data = VkDeviceOrHostAddressConstKHR{.deviceAddress = instanceDescsGpuAddress}};
	const VkAccelerationStructureGeometryKHR geometry{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
	    .pNext = nullptr,
	    .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
	    .geometry = VkAccelerationStructureGeometryDataKHR{.instances = instances},
	    .flags = VK_GEOMETRY_OPAQUE_BIT_KHR};
	const VkBuildAccelerationStructureFlagsKHR nativeBuildFlags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR
	    | (buildMode != ERhiClassicTlasBuildMode::Build ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR
	                                                    : static_cast<VkBuildAccelerationStructureFlagsKHR>(0));
	const VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	    .pNext = nullptr,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	    .flags = nativeBuildFlags,
	    .mode = buildMode == ERhiClassicTlasBuildMode::Update ? VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR
	                                                          : VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	    .srcAccelerationStructure = buildMode == ERhiClassicTlasBuildMode::Update ? resultResource.AccelerationStructure : VK_NULL_HANDLE,
	    .dstAccelerationStructure = resultResource.AccelerationStructure,
	    .geometryCount = 1,
	    .pGeometries = &geometry,
	    .ppGeometries = nullptr,

	    .scratchData = VkDeviceOrHostAddressKHR{.deviceAddress = scratchGpuAddress}};
	const VkAccelerationStructureBuildRangeInfoKHR rangeInfo{
	    .primitiveCount = instanceCount,
	    .primitiveOffset = 0,
	    .firstVertex = 0,
	    .transformOffset = 0};
	const VkAccelerationStructureBuildRangeInfoKHR* rangeInfos[] = {&rangeInfo};
	m_rhi->GetCmdBuildAccelerationStructures()(m_commandBuffer, 1, &buildInfo, rangeInfos);

	const VkMemoryBarrier2 buildBarrier{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
	    .srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
	    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
	    .dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR};
	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 1,
	    .pMemoryBarriers = &buildBarrier,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 0,
	    .pImageMemoryBarriers = nullptr};
	vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

void VulkanRenderCommandList::BuildPartitionedTopLevelAccelerationStructure(const RhiPartitionedTlasBuildCommandDesc& desc) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_rhi->GetCmdBuildPartitionedAccelerationStructures() == nullptr
	    || !desc.DestinationResource || desc.DestinationAccelerationStructure == 0 || desc.Scratch == 0 || desc.OperationHeaders == 0
	    || desc.OperationCount == 0 || desc.Layout.InstanceCapacity == 0 || desc.Layout.PartitionCount == 0)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan partitioned TLAS build received incomplete device, command-buffer, layout, or GPU-address inputs.");
	}
	EndDynamicRenderingIfNeeded();

	const VkMemoryBarrier2 operationDataBarrier{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_HOST_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
	    .srcAccessMask = VK_ACCESS_2_HOST_WRITE_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
	    .dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
	    .dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR};
	const VkDependencyInfo operationDataDependency{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 1,
	    .pMemoryBarriers = &operationDataBarrier,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 0,
	    .pImageMemoryBarriers = nullptr};
	vkCmdPipelineBarrier2(m_commandBuffer, &operationDataDependency);

	VkPartitionedAccelerationStructureFlagsNV partitionedTlasFlags{};
	VkPartitionedAccelerationStructureInstancesInputNV input{};
	ConfigurePartitionedTlasInput(desc.Layout, input, partitionedTlasFlags);
	const VkBuildPartitionedAccelerationStructureInfoNV buildInfo{
	    .sType = VK_STRUCTURE_TYPE_BUILD_PARTITIONED_ACCELERATION_STRUCTURE_INFO_NV,
	    .pNext = nullptr,
	    .input = input,
	    .srcAccelerationStructureData = desc.SourceAccelerationStructure,
	    .dstAccelerationStructureData = desc.DestinationAccelerationStructure,
	    .scratchData = desc.Scratch,
	    .srcInfos = desc.OperationHeaders,
	    .srcInfosCount = desc.OperationCount};
	m_rhi->GetCmdBuildPartitionedAccelerationStructures()(m_commandBuffer, &buildInfo);

	const VkMemoryBarrier2 buildBarrier{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
	    .srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR,
	    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT
	        | VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
	    .dstAccessMask =
	        VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_READ_BIT};
	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 1,
	    .pMemoryBarriers = &buildBarrier,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 0,
	    .pImageMemoryBarriers = nullptr};
	vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

void VulkanRenderCommandList::TraceRays(const TraceRaysDesc& desc) noexcept
{
	try
	{
		RhiContract::ValidateTraceRaysDesc(desc, m_queueType);
	}
	catch (const std::exception& error)
	{
		Diagnostics::Fatal(g_vulkanRenderCommandListLogger, __FILE__, __LINE__, error.what());
	}
	const auto* pipeline = dynamic_cast<const VulkanRayTracingPipeline*>(desc.Pipeline);
	const auto* table = dynamic_cast<const VulkanRayTracingShaderTable*>(desc.ShaderTable);
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_rhi->GetCmdTraceRays() == nullptr || pipeline == nullptr
	    || table == nullptr)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan TraceRays requires a command buffer, loaded function, and matching native objects.");
	}
	if (m_boundRayTracingPipeline != desc.Pipeline)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan TraceRays requires its exact pipeline to be bound first.");
	}
	TrackResource(table->GetResource());
	const VkDeviceAddress baseAddress = table->GetDeviceAddress();
	const auto nativeRegion = [baseAddress](const RhiRayTracingShaderTableRegion& region)
	{
		return VkStridedDeviceAddressRegionKHR{
		    .deviceAddress = region.SizeInBytes != 0 ? baseAddress + region.OffsetInBytes : 0,
		    .stride = region.StrideInBytes,
		    .size = region.SizeInBytes};
	};
	const VkStridedDeviceAddressRegionKHR rayGeneration = nativeRegion(desc.RayGeneration);
	const VkStridedDeviceAddressRegionKHR miss = nativeRegion(desc.Miss);
	const VkStridedDeviceAddressRegionKHR hitGroup = nativeRegion(desc.HitGroup);
	const VkStridedDeviceAddressRegionKHR callable = nativeRegion(desc.Callable);
	FlushRayTracingDescriptorSets();
	m_rhi->GetCmdTraceRays()(m_commandBuffer, &rayGeneration, &miss, &hitGroup, &callable, desc.Width, desc.Height, desc.Depth);
}

VkDeviceAddress VulkanRenderCommandList::ResolveRayTracingBufferAddress(const RhiRayTracingBufferBinding& binding) const noexcept
{
	VulkanRecordingResource resource;
	if (!ResolveResource(binding.Resource, resource) || resource.Buffer == VK_NULL_HANDLE || resource.BufferDeviceAddress == 0)
	{
		return 0;
	}

	return resource.BufferDeviceAddress + binding.OffsetInBytes;
}
