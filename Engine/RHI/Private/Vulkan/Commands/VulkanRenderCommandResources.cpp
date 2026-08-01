#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

#include "Vulkan/Descriptors/VulkanDescriptorService.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Resources/VulkanRecordingUploadPage.h"
#include "Vulkan/VulkanTypeConversions.h"
#include "Core/Public/Diagnostics/Verify.h"

static const auto g_vulkanRenderCommandListLogger = Logging::GetOrCreateLogger("RHI.Vulkan.CommandList");

void VulkanRenderCommandList::CopyResource(RhiResourceHandle destinationResource, RhiResourceHandle sourceResource) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || m_memoryAllocator == nullptr || !destinationResource || !sourceResource)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan CopyResource requires an active command buffer and two valid resources.");
	}
	TrackResource(destinationResource);
	TrackResource(sourceResource);
	EndDynamicRenderingIfNeeded();

	VulkanRecordingResource destination;
	VulkanRecordingResource source;
	if (!ResolveResource(destinationResource, destination) || !ResolveResource(sourceResource, source) ||
	    destination.ResourceKind != source.ResourceKind)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan CopyResource requires two resolved resources of the same kind.");
	}

	if (destination.ResourceKind == VulkanGpuAllocationResourceKind::Buffer && destination.Buffer != VK_NULL_HANDLE &&
	    source.Buffer != VK_NULL_HANDLE)
	{
		if (destination.ResourceSizeInBytes == 0 || destination.ResourceSizeInBytes != source.ResourceSizeInBytes)
		{
			Diagnostics::Fatal(
			    g_vulkanRenderCommandListLogger,
			    __FILE__,
			    __LINE__,
			    "Vulkan CopyResource requires equal non-empty buffer sizes.");
		}
		const VkBufferCopy copyRegion{.srcOffset = 0, .dstOffset = 0, .size = destination.ResourceSizeInBytes};
		vkCmdCopyBuffer(m_commandBuffer, source.Buffer, destination.Buffer, 1, &copyRegion);
		return;
	}

	if (destination.ResourceKind == VulkanGpuAllocationResourceKind::Image && destination.Image != VK_NULL_HANDLE &&
	    source.Image != VK_NULL_HANDLE)
	{
		if (destination.Extent.width == 0 || destination.Extent.height == 0 || destination.Extent.depth == 0 ||
		    destination.Extent.width != source.Extent.width || destination.Extent.height != source.Extent.height ||
		    destination.Extent.depth != source.Extent.depth || destination.AspectMask != source.AspectMask)
		{
			Diagnostics::Fatal(
			    g_vulkanRenderCommandListLogger,
			    __FILE__,
			    __LINE__,
			    "Vulkan CopyResource requires equal non-empty image extents and aspect masks.");
		}

		const VkImageSubresourceLayers sourceLayers{.aspectMask = source.AspectMask, .mipLevel = 0, .baseArrayLayer = 0, .layerCount = 1};
		const VkImageSubresourceLayers destinationLayers{
		    .aspectMask = destination.AspectMask,
		    .mipLevel = 0,
		    .baseArrayLayer = 0,
		    .layerCount = 1};
		const VkImageCopy copyRegion{
		    .srcSubresource = sourceLayers,
		    .srcOffset = VkOffset3D{},
		    .dstSubresource = destinationLayers,
		    .dstOffset = VkOffset3D{},
		    .extent = destination.Extent};
		vkCmdCopyImage(
		    m_commandBuffer,
		    source.Image,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    destination.Image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    1,
		    &copyRegion);
		return;
	}

	Diagnostics::Fatal(g_vulkanRenderCommandListLogger, __FILE__, __LINE__, "Vulkan CopyResource received an incomplete native resource.");
}

void VulkanRenderCommandList::AliasResource(RhiResourceHandle beforeResource, RhiResourceHandle afterResource) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || !beforeResource || !afterResource)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan aliasing barriers require an active command buffer and two valid resources.");
	}
	TrackResource(beforeResource);
	TrackResource(afterResource);
	EndDynamicRenderingIfNeeded();

	const VkMemoryBarrier2 memoryBarrier{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
	    .srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT,
	    .dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT,
	    .dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT | VK_ACCESS_2_MEMORY_WRITE_BIT};
	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 1,
	    .pMemoryBarriers = &memoryBarrier,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 0,
	    .pImageMemoryBarriers = nullptr};
	vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

void VulkanRenderCommandList::TransitionResource(RhiResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || !resource)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan resource transitions require an active command buffer and a valid resource.");
	}
	if (before == after)
	{
		return;
	}
	TrackResource(resource);
	EndDynamicRenderingIfNeeded();

	VulkanResourceStateMapping sourceState = ResolveResourceState(before);
	const VulkanResourceStateMapping destinationState = ResolveResourceState(after);

	VulkanRecordingResource recordingResource;
	if (!ResolveResource(resource, recordingResource))
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan resource transition references a resource that is not registered for command recording.");
	}

	switch (recordingResource.ResourceKind)
	{
		case VulkanGpuAllocationResourceKind::Buffer:
			RecordBufferTransition(recordingResource, before, after, sourceState, destinationState);
			return;
		case VulkanGpuAllocationResourceKind::Image:
			RecordImageTransition(recordingResource, before, after, sourceState, destinationState);
			return;
	}

	Diagnostics::Fatal(
	    g_vulkanRenderCommandListLogger,
	    __FILE__,
	    __LINE__,
	    "Vulkan resource transition references recording metadata with an unknown resource kind.");
}

void VulkanRenderCommandList::RecordBufferTransition(
    const VulkanRecordingResource& resource,
    ResourceState before,
    ResourceState after,
    const VulkanResourceStateMapping& sourceState,
    const VulkanResourceStateMapping& destinationState) noexcept
{
	if (resource.Buffer == VK_NULL_HANDLE || resource.ResourceSizeInBytes == 0)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan buffer transition references incomplete recording metadata.");
	}
	if (!VulkanTypeConversions::IsBufferResourceStateSupported(before) || !VulkanTypeConversions::IsBufferResourceStateSupported(after))
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan buffer transition uses an image-only resource state.");
	}

	const VkBufferMemoryBarrier2 bufferBarrier{
	    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = sourceState.StageMask,
	    .srcAccessMask = sourceState.AccessMask,
	    .dstStageMask = destinationState.StageMask,
	    .dstAccessMask = destinationState.AccessMask,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .buffer = resource.Buffer,
	    .offset = 0,
	    .size = resource.ResourceSizeInBytes};
	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,

	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 0,
	    .pMemoryBarriers = nullptr,
	    .bufferMemoryBarrierCount = 1,
	    .pBufferMemoryBarriers = &bufferBarrier,
	    .imageMemoryBarrierCount = 0,
	    .pImageMemoryBarriers = nullptr};
	vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

void VulkanRenderCommandList::RecordImageTransition(
    const VulkanRecordingResource& resource,
    ResourceState before,
    ResourceState after,
    const VulkanResourceStateMapping& sourceState,
    const VulkanResourceStateMapping& destinationState) noexcept
{
	if (resource.Image == VK_NULL_HANDLE || resource.AspectMask == 0)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan image transition references incomplete recording metadata.");
	}
	if (!VulkanTypeConversions::IsImageResourceStateSupported(before) || !VulkanTypeConversions::IsImageResourceStateSupported(after))
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan image transition uses a buffer-only resource state.");
	}

	const VkImageMemoryBarrier2 imageBarrier{
	    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = sourceState.StageMask,
	    .srcAccessMask = sourceState.AccessMask,
	    .dstStageMask = destinationState.StageMask,
	    .dstAccessMask = destinationState.AccessMask,
	    .oldLayout = sourceState.ImageLayout,
	    .newLayout = destinationState.ImageLayout,
	    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
	    .image = resource.Image,
	    .subresourceRange = VkImageSubresourceRange{
	        .aspectMask = resource.AspectMask,
	        .baseMipLevel = 0,
	        .levelCount = VK_REMAINING_MIP_LEVELS,
	        .baseArrayLayer = 0,
	        .layerCount = VK_REMAINING_ARRAY_LAYERS}};
	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 0,
	    .pMemoryBarriers = nullptr,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 1,
	    .pImageMemoryBarriers = &imageBarrier};
	vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

void VulkanRenderCommandList::UnorderedAccessBarrier(RhiResourceHandle resource) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || !resource)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan unordered-access barriers require an active command buffer and a valid resource.");
	}
	TrackResource(resource);
	EndDynamicRenderingIfNeeded();

	VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	if (m_queueType == ERhiQueueType::Graphics)
	{
		srcStageMask |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	}
	VkAccessFlags2 srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	VkPipelineStageFlags2 dstStageMask = srcStageMask;
	VkAccessFlags2 dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	VulkanRecordingResource recordingResource;
	if (!ResolveResource(resource, recordingResource))
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan unordered-access barrier references a resource that is not registered for command recording.");
	}
	if (recordingResource.AccelerationStructure != VK_NULL_HANDLE)
	{
		srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
		srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
		if (m_queueType == ERhiQueueType::Graphics)
		{
			dstStageMask |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
		}
		dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR;
	}

	const VkMemoryBarrier2 memoryBarrier{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = srcStageMask,
	    .srcAccessMask = srcAccessMask,
	    .dstStageMask = dstStageMask,
	    .dstAccessMask = dstAccessMask};
	const VkDependencyInfo dependencyInfo{
	    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
	    .pNext = nullptr,
	    .dependencyFlags = 0,
	    .memoryBarrierCount = 1,
	    .pMemoryBarriers = &memoryBarrier,
	    .bufferMemoryBarrierCount = 0,
	    .pBufferMemoryBarriers = nullptr,
	    .imageMemoryBarrierCount = 0,
	    .pImageMemoryBarriers = nullptr};
	vkCmdPipelineBarrier2(m_commandBuffer, &dependencyInfo);
}

VulkanResourceStateMapping VulkanRenderCommandList::ResolveResourceState(ResourceState state) const noexcept
{
	VulkanResourceStateMapping mapping = VulkanTypeConversions::ToResourceStateMapping(state);
	if (m_queueType != ERhiQueueType::Compute)
	{
		return mapping;
	}

	if (state == ResourceState::ShaderResource || state == ResourceState::UnorderedAccess)
	{
		mapping.StageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	}
	else if (state == ResourceState::RayTracingAccelerationStructure)
	{
		mapping.StageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
	}
	return mapping;
}

VkBuffer VulkanRenderCommandList::ResolveBuffer(RhiGpuVirtualAddress gpuAddress) const noexcept
{
	return ResolveBufferBinding(gpuAddress).Buffer;
}

bool VulkanRenderCommandList::ResolveResource(RhiResourceHandle resource, VulkanRecordingResource& outResource) const noexcept
{
	if (m_memoryAllocator != nullptr &&
	    (m_memoryAllocator->ResolveRecordingResource(resource, outResource) ||
	     (IsCoordinatorRecording() && m_memoryAllocator->ResolveCoordinatorRecordingResource(resource, outResource))))
	{
		return true;
	}
	return m_descriptorService != nullptr && m_descriptorService->ResolveRegisteredImageResource(resource, outResource);
}

bool VulkanRenderCommandList::ResolveAddress(RhiGpuVirtualAddress address, VulkanRecordingResource& outResource) const noexcept
{
	if (m_memoryAllocator == nullptr)
	{
		return false;
	}

	return m_memoryAllocator->ResolveRecordingAddress(address, outResource) ||
	       (IsCoordinatorRecording() && m_memoryAllocator->ResolveCoordinatorRecordingAddress(address, outResource));
}

VulkanRenderCommandList::BufferBinding VulkanRenderCommandList::ResolveBufferBinding(RhiGpuVirtualAddress gpuAddress) const noexcept
{
	if (gpuAddress == 0)
	{
		Diagnostics::Fatal(g_vulkanRenderCommandListLogger, __FILE__, __LINE__, "Vulkan buffer binding requires a non-zero GPU address.");
	}

	BufferBinding binding;
	if (m_recordingUploadPage != nullptr && m_recordingUploadPage->Resolve(gpuAddress, binding.Buffer, binding.Offset, binding.Range))
	{
		return binding;
	}

	VulkanRecordingResource resource;
	if (ResolveAddress(gpuAddress, resource) && resource.Buffer != VK_NULL_HANDLE)
	{
		if (resource.BufferDeviceAddress == 0 || gpuAddress < resource.BufferDeviceAddress)
		{
			Diagnostics::Fatal(
			    g_vulkanRenderCommandListLogger,
			    __FILE__,
			    __LINE__,
			    "Vulkan buffer binding resolved incomplete address metadata.");
		}
		const VkDeviceSize offset = gpuAddress - resource.BufferDeviceAddress;
		if (offset >= resource.ResourceSizeInBytes)
		{
			Diagnostics::Fatal(
			    g_vulkanRenderCommandListLogger,
			    __FILE__,
			    __LINE__,
			    "Vulkan buffer binding address lies outside the resolved resource.");
		}
		binding.Buffer = resource.Buffer;
		binding.Offset = offset;
		binding.Range = resource.ResourceSizeInBytes - offset;
		return binding;
	}

	Diagnostics::Fatal(
	    g_vulkanRenderCommandListLogger,
	    __FILE__,
	    __LINE__,
	    "Vulkan buffer binding references a GPU address that is not registered for command recording.");
}
