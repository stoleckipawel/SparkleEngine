#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipelineState.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <algorithm>

void VulkanRenderCommandList::CloseOpenRendering() noexcept
{
	EndDynamicRenderingIfNeeded();
}

void VulkanRenderCommandList::SetNativeCommandBuffer(
    VkCommandBuffer commandBuffer,
    PFN_vkCmdBeginDebugUtilsLabelEXT beginLabel,
    PFN_vkCmdEndDebugUtilsLabelEXT endLabel,
    PFN_vkCmdInsertDebugUtilsLabelEXT insertLabel) noexcept
{
	m_commandBuffer = commandBuffer;
	m_dynamicRenderingActive = false;
	m_renderTargets = {};
	m_renderTargetCount = 0;
	m_depthStencil = VK_NULL_HANDLE;
	m_hasScissorRect = false;
	m_graphicsDescriptorSets.clear();
	m_computeDescriptorSets.clear();
	m_retainedDescriptorTables.clear();
	m_retainedDescriptorHandles.clear();
	m_retainedDescriptorBuffers.clear();
	m_beginDebugUtilsLabel = beginLabel;
	m_endDebugUtilsLabel = endLabel;
	m_insertDebugUtilsLabel = insertLabel;
}

ERhiBackendApi VulkanRenderCommandList::GetBackendApi() const noexcept
{
	return ERhiBackendApi::Vulkan;
}

NativeGraphicsCommandListHandle VulkanRenderCommandList::GetNativeHandle() const noexcept
{
	return NativeGraphicsCommandListHandle{m_commandBuffer};
}

bool VulkanRenderCommandList::SupportsDiagnosticScopes() const noexcept
{
	return m_commandBuffer != VK_NULL_HANDLE && m_beginDebugUtilsLabel != nullptr && m_endDebugUtilsLabel != nullptr;
}

void VulkanRenderCommandList::BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	if (SupportsDiagnosticScopes())
	{
		const std::string ownedLabel(label);
		const VkDebugUtilsLabelEXT nativeLabel = BuildLabel(ownedLabel.c_str(), color);
		m_beginDebugUtilsLabel(m_commandBuffer, &nativeLabel);
	}
}

void VulkanRenderCommandList::EndDiagnosticScope() noexcept
{
	if (m_commandBuffer != VK_NULL_HANDLE && m_endDebugUtilsLabel != nullptr)
	{
		m_endDebugUtilsLabel(m_commandBuffer);
	}
}

void VulkanRenderCommandList::InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	if (m_commandBuffer != VK_NULL_HANDLE && m_insertDebugUtilsLabel != nullptr)
	{
		const std::string ownedLabel(label);
		const VkDebugUtilsLabelEXT nativeLabel = BuildLabel(ownedLabel.c_str(), color);
		m_insertDebugUtilsLabel(m_commandBuffer, &nativeLabel);
	}
}

void VulkanRenderCommandList::SetPipelineState(const RenderPipelineState& pipelineState) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const auto& vulkanPipelineState = static_cast<const VulkanPipelineState&>(pipelineState);
	if (vulkanPipelineState.GetBindPoint() == VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		EndDynamicRenderingIfNeeded();
		m_computePipelineLayout = vulkanPipelineState.GetPipelineLayout();
	}
	else
	{
		m_graphicsPipelineLayout = vulkanPipelineState.GetPipelineLayout();
	}
	vkCmdBindPipeline(m_commandBuffer, vulkanPipelineState.GetBindPoint(), vulkanPipelineState.GetPipeline());
}

void VulkanRenderCommandList::SetGraphicsBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_graphicsBindingLayout = static_cast<const VulkanBindingLayout*>(&bindingLayout);
	m_graphicsDescriptorSets.assign(m_graphicsBindingLayout->GetDescriptorSetLayouts().size(), VK_NULL_HANDLE);
}

void VulkanRenderCommandList::SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_computeBindingLayout = static_cast<const VulkanBindingLayout*>(&bindingLayout);
	m_computeDescriptorSets.assign(m_computeBindingLayout->GetDescriptorSetLayouts().size(), VK_NULL_HANDLE);
}

void VulkanRenderCommandList::BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(m_graphicsBindingLayout, binding->BindingPoint.Set, m_graphicsDescriptorSets);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, reinterpret_cast<VkBuffer>(gpuAddress), 0, VK_WHOLE_SIZE);
	m_retainedDescriptorBuffers.push_back(reinterpret_cast<VkBuffer>(gpuAddress));
	BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout, binding->BindingPoint.Set, descriptorSet);
}

void VulkanRenderCommandList::BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(m_graphicsBindingLayout, binding->BindingPoint.Set, m_graphicsDescriptorSets);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, reinterpret_cast<VkBuffer>(gpuAddress), 0, VK_WHOLE_SIZE);
	m_retainedDescriptorBuffers.push_back(reinterpret_cast<VkBuffer>(gpuAddress));
	BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout, binding->BindingPoint.Set, descriptorSet);
}

void VulkanRenderCommandList::BindGraphicsUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindGraphicsShaderResource(bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(m_graphicsBindingLayout, binding->BindingPoint.Set, m_graphicsDescriptorSets);
	m_descriptorAllocator->WriteDescriptorTable(descriptorSet, *binding, tableBinding);
	m_retainedDescriptorTables.push_back(tableBinding);
	BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout, binding->BindingPoint.Set, descriptorSet);
}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(m_graphicsBindingLayout, binding->BindingPoint.Set, m_graphicsDescriptorSets);
	m_descriptorAllocator->WriteDescriptorHandle(descriptorSet, *binding, baseDescriptor);
	m_retainedDescriptorHandles.push_back(baseDescriptor);
	BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout, binding->BindingPoint.Set, descriptorSet);
}

void VulkanRenderCommandList::SetGraphicsPushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (m_commandBuffer == VK_NULL_HANDLE || m_graphicsPipelineLayout == VK_NULL_HANDLE || binding == nullptr || data == nullptr ||
	    num32BitValues == 0)
	{
		return;
	}
	vkCmdPushConstants(
	    m_commandBuffer,
	    m_graphicsPipelineLayout,
	    ToVkShaderStages(binding->VisibilityMask),
	    destOffsetIn32BitValues * sizeof(std::uint32_t),
	    num32BitValues * sizeof(std::uint32_t),
	    data);
}

void VulkanRenderCommandList::BindComputeConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_computeBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, reinterpret_cast<VkBuffer>(gpuAddress), 0, VK_WHOLE_SIZE);
	m_retainedDescriptorBuffers.push_back(reinterpret_cast<VkBuffer>(gpuAddress));
	BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, binding->BindingPoint.Set, descriptorSet);
}

void VulkanRenderCommandList::BindComputeShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_computeBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, reinterpret_cast<VkBuffer>(gpuAddress), 0, VK_WHOLE_SIZE);
	m_retainedDescriptorBuffers.push_back(reinterpret_cast<VkBuffer>(gpuAddress));
	BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, binding->BindingPoint.Set, descriptorSet);
}

void VulkanRenderCommandList::BindComputeUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindComputeShaderResource(bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_computeBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets);
	m_descriptorAllocator->WriteDescriptorTable(descriptorSet, *binding, tableBinding);
	m_retainedDescriptorTables.push_back(tableBinding);
	BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, binding->BindingPoint.Set, descriptorSet);
}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_computeBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets);
	m_descriptorAllocator->WriteDescriptorHandle(descriptorSet, *binding, baseDescriptor);
	m_retainedDescriptorHandles.push_back(baseDescriptor);
	BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, binding->BindingPoint.Set, descriptorSet);
}

void VulkanRenderCommandList::SetComputePushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_computeBindingLayout, bindingIndex);
	if (m_commandBuffer == VK_NULL_HANDLE || m_computePipelineLayout == VK_NULL_HANDLE || binding == nullptr || data == nullptr ||
	    num32BitValues == 0)
	{
		return;
	}
	vkCmdPushConstants(
	    m_commandBuffer,
	    m_computePipelineLayout,
	    ToVkShaderStages(binding->VisibilityMask),
	    destOffsetIn32BitValues * sizeof(std::uint32_t),
	    num32BitValues * sizeof(std::uint32_t),
	    data);
}

void VulkanRenderCommandList::SetPrimitiveTopology(RhiPrimitiveTopology) noexcept {}

void VulkanRenderCommandList::BindVertexBuffer(const RhiVertexBufferView& view) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || view.BufferLocation == 0)
	{
		return;
	}

	const VkBuffer buffer = reinterpret_cast<VkBuffer>(view.BufferLocation);
	constexpr VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &buffer, &offset);
}

void VulkanRenderCommandList::BindIndexBuffer(const RhiIndexBufferView& view) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || view.BufferLocation == 0)
	{
		return;
	}

	const VkBuffer buffer = reinterpret_cast<VkBuffer>(view.BufferLocation);
	vkCmdBindIndexBuffer(m_commandBuffer, buffer, 0, VulkanTypeConversions::ToVkIndexType(view.Format));
}

void VulkanRenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept
{
	EndDynamicRenderingIfNeeded();
	m_renderTargets = {};
	m_renderTargets[0] = DecodeImageViewHandle(rtv);
	m_renderTargetCount = m_renderTargets[0] != VK_NULL_HANDLE ? 1u : 0u;
	m_depthStencil = dsv != nullptr ? DecodeImageViewHandle(*dsv) : VK_NULL_HANDLE;
}

void VulkanRenderCommandList::SetRenderTargets(
    std::uint32_t numRTVs,
    const RhiCpuDescriptorHandle* rtvs,
    const RhiCpuDescriptorHandle* dsv) noexcept
{
	EndDynamicRenderingIfNeeded();
	m_renderTargets = {};
	m_renderTargetCount = 0;
	m_depthStencil = dsv != nullptr ? DecodeImageViewHandle(*dsv) : VK_NULL_HANDLE;
	if (rtvs == nullptr)
	{
		return;
	}

	const std::uint32_t count = std::min(numRTVs, MaxRenderTargets);
	for (std::uint32_t index = 0; index < count; ++index)
	{
		m_renderTargets[index] = DecodeImageViewHandle(rtvs[index]);
		if (m_renderTargets[index] != VK_NULL_HANDLE)
		{
			m_renderTargetCount = index + 1u;
		}
	}
}

void VulkanRenderCommandList::ClearRenderTarget(RhiCpuDescriptorHandle rtv, const float color[4]) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || color == nullptr || !m_hasScissorRect)
	{
		return;
	}

	const VkImageView imageView = DecodeImageViewHandle(rtv);
	std::uint32_t colorAttachment = MaxRenderTargets;
	for (std::uint32_t index = 0; index < m_renderTargetCount; ++index)
	{
		if (m_renderTargets[index] == imageView)
		{
			colorAttachment = index;
			break;
		}
	}
	if (colorAttachment == MaxRenderTargets)
	{
		return;
	}
	BeginDynamicRenderingIfNeeded();
	if (!m_dynamicRenderingActive)
	{
		return;
	}

	VkClearValue clearValue = {};
	clearValue.color.float32[0] = color[0];
	clearValue.color.float32[1] = color[1];
	clearValue.color.float32[2] = color[2];
	clearValue.color.float32[3] = color[3];
	const VkClearAttachment clearAttachment{
	    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
	    .colorAttachment = colorAttachment,
	    .clearValue = clearValue};
	const VkClearRect clearRect{.rect = m_scissorRect, .baseArrayLayer = 0, .layerCount = 1};
	vkCmdClearAttachments(m_commandBuffer, 1, &clearAttachment, 1, &clearRect);
}

void VulkanRenderCommandList::ClearDepthStencil(RhiCpuDescriptorHandle dsv, float depth, std::uint8_t stencil) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || !m_hasScissorRect || DecodeImageViewHandle(dsv) != m_depthStencil)
	{
		return;
	}

	BeginDynamicRenderingIfNeeded();
	if (!m_dynamicRenderingActive)
	{
		return;
	}
	VkClearValue clearValue = {};
	clearValue.depthStencil.depth = depth;
	clearValue.depthStencil.stencil = stencil;
	const VkClearAttachment clearAttachment{
	    .aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT,
	    .colorAttachment = 0,
	    .clearValue = clearValue};
	const VkClearRect clearRect{.rect = m_scissorRect, .baseArrayLayer = 0, .layerCount = 1};
	vkCmdClearAttachments(m_commandBuffer, 1, &clearAttachment, 1, &clearRect);
}

void VulkanRenderCommandList::SetViewport(const RhiViewport& viewport) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const VkViewport nativeViewport{
	    .x = viewport.X,
	    .y = viewport.Y,
	    .width = viewport.Width,
	    .height = viewport.Height,
	    .minDepth = viewport.MinDepth,
	    .maxDepth = viewport.MaxDepth};
	vkCmdSetViewport(m_commandBuffer, 0, 1, &nativeViewport);
}

void VulkanRenderCommandList::SetScissorRect(const RhiRect& rect) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const VkRect2D nativeRect{
	    .offset = VkOffset2D{.x = rect.Left, .y = rect.Top},
	    .extent = VkExtent2D{
	        .width = static_cast<std::uint32_t>(rect.Right - rect.Left),
	        .height = static_cast<std::uint32_t>(rect.Bottom - rect.Top)}};
	m_scissorRect = nativeRect;
	m_hasScissorRect = nativeRect.extent.width > 0 && nativeRect.extent.height > 0;
	vkCmdSetScissor(m_commandBuffer, 0, 1, &nativeRect);
}

void VulkanRenderCommandList::DrawIndexedInstanced(
    std::uint32_t indexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startIndexLocation,
    std::int32_t baseVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	BeginDynamicRenderingIfNeeded();
	if (!m_dynamicRenderingActive)
	{
		return;
	}
	vkCmdDrawIndexed(m_commandBuffer, indexCountPerInstance, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
}

void VulkanRenderCommandList::DrawInstanced(
    std::uint32_t vertexCountPerInstance,
    std::uint32_t instanceCount,
    std::uint32_t startVertexLocation,
    std::uint32_t startInstanceLocation) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	BeginDynamicRenderingIfNeeded();
	if (!m_dynamicRenderingActive)
	{
		return;
	}
	vkCmdDraw(m_commandBuffer, vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void VulkanRenderCommandList::Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	EndDynamicRenderingIfNeeded();
	vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanRenderCommandList::BuildBottomLevelAccelerationStructure(
    const RhiRayTracingGeometryDesc&,
    RhiGpuVirtualAddress,
    RhiGpuVirtualAddress) noexcept
{
}

void VulkanRenderCommandList::BuildTopLevelAccelerationStructure(
    RhiGpuVirtualAddress,
    std::uint32_t,
    RhiGpuVirtualAddress,
    RhiGpuVirtualAddress) noexcept
{
}

void VulkanRenderCommandList::CopyResource(NativeResourceHandle destinationResource, NativeResourceHandle sourceResource) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || m_memoryAllocator == nullptr || !destinationResource || !sourceResource)
	{
		return;
	}
	EndDynamicRenderingIfNeeded();

	VulkanGpuAllocationRecord* const destinationRecord = m_memoryAllocator->FindAllocationRecord(destinationResource);
	VulkanGpuAllocationRecord* const sourceRecord = m_memoryAllocator->FindAllocationRecord(sourceResource);
	if (destinationRecord == nullptr || sourceRecord == nullptr || destinationRecord->ResourceKind != sourceRecord->ResourceKind)
	{
		return;
	}

	if (destinationRecord->ResourceKind == VulkanGpuAllocationResourceKind::Buffer && destinationRecord->Buffer != VK_NULL_HANDLE &&
	    sourceRecord->Buffer != VK_NULL_HANDLE)
	{
		const VkBufferCopy copyRegion{
		    .srcOffset = 0,
		    .dstOffset = 0,
		    .size = std::min(destinationRecord->ResourceSizeInBytes, sourceRecord->ResourceSizeInBytes)};
		if (copyRegion.size > 0)
		{
			vkCmdCopyBuffer(m_commandBuffer, sourceRecord->Buffer, destinationRecord->Buffer, 1, &copyRegion);
		}
		return;
	}

	if (destinationRecord->ResourceKind == VulkanGpuAllocationResourceKind::Image && destinationRecord->Image != VK_NULL_HANDLE &&
	    sourceRecord->Image != VK_NULL_HANDLE)
	{
		const VkExtent3D copyExtent{
		    .width = std::min(destinationRecord->Extent.width, sourceRecord->Extent.width),
		    .height = std::min(destinationRecord->Extent.height, sourceRecord->Extent.height),
		    .depth = std::min(destinationRecord->Extent.depth, sourceRecord->Extent.depth)};
		if (copyExtent.width == 0 || copyExtent.height == 0 || copyExtent.depth == 0)
		{
			return;
		}

		const VkImageSubresourceLayers sourceLayers{
		    .aspectMask = sourceRecord->AspectMask,
		    .mipLevel = 0,
		    .baseArrayLayer = 0,
		    .layerCount = 1};
		const VkImageSubresourceLayers destinationLayers{
		    .aspectMask = destinationRecord->AspectMask,
		    .mipLevel = 0,
		    .baseArrayLayer = 0,
		    .layerCount = 1};
		const VkImageCopy copyRegion{
		    .srcSubresource = sourceLayers,
		    .srcOffset = VkOffset3D{},
		    .dstSubresource = destinationLayers,
		    .dstOffset = VkOffset3D{},
		    .extent = copyExtent};
		vkCmdCopyImage(
		    m_commandBuffer,
		    sourceRecord->Image,
		    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		    destinationRecord->Image,
		    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		    1,
		    &copyRegion);
	}
}

void VulkanRenderCommandList::AliasResource(NativeResourceHandle, NativeResourceHandle) noexcept {}

void VulkanRenderCommandList::TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || !resource || before == after)
	{
		return;
	}
	EndDynamicRenderingIfNeeded();

	const VulkanResourceStateMapping sourceState = VulkanTypeConversions::ToResourceStateMapping(before);
	const VulkanResourceStateMapping destinationState = VulkanTypeConversions::ToResourceStateMapping(after);
	VulkanGpuAllocationRecord* const record = m_memoryAllocator != nullptr ? m_memoryAllocator->FindAllocationRecord(resource) : nullptr;

	if (record != nullptr && record->ResourceKind == VulkanGpuAllocationResourceKind::Buffer && record->Buffer != VK_NULL_HANDLE)
	{
		if (!VulkanTypeConversions::IsBufferResourceStateSupported(before) || !VulkanTypeConversions::IsBufferResourceStateSupported(after))
		{
			return;
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
		    .buffer = record->Buffer,
		    .offset = 0,
		    .size = record->ResourceSizeInBytes > 0 ? record->ResourceSizeInBytes : VK_WHOLE_SIZE};
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
		return;
	}

	const VkImage image = record != nullptr && record->Image != VK_NULL_HANDLE ? record->Image : static_cast<VkImage>(resource.Value);
	if (image == VK_NULL_HANDLE)
	{
		return;
	}
	if (!VulkanTypeConversions::IsImageResourceStateSupported(before) || !VulkanTypeConversions::IsImageResourceStateSupported(after))
	{
		return;
	}

	const VkImageAspectFlags aspectMask = record != nullptr && record->AspectMask != 0 ? record->AspectMask : VK_IMAGE_ASPECT_COLOR_BIT;
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
	    .image = image,
	    .subresourceRange = VkImageSubresourceRange{
	        .aspectMask = aspectMask,
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

void VulkanRenderCommandList::UnorderedAccessBarrier(NativeResourceHandle) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	EndDynamicRenderingIfNeeded();

	const VkMemoryBarrier2 memoryBarrier{
	    .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2,
	    .pNext = nullptr,
	    .srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
	                    VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
	    .srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT,
	    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
	                    VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR,
	    .dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT};
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

VkImageView VulkanRenderCommandList::DecodeImageViewHandle(RhiCpuDescriptorHandle handle) noexcept
{
	return reinterpret_cast<VkImageView>(handle.Value);
}

VkDebugUtilsLabelEXT VulkanRenderCommandList::BuildLabel(const char* label, RhiDiagnosticLabelColor color) noexcept
{
	return VkDebugUtilsLabelEXT{
	    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
	    .pNext = nullptr,
	    .pLabelName = label,
	    .color = {
	        static_cast<float>(color.Red) / 255.0f,
	        static_cast<float>(color.Green) / 255.0f,
	        static_cast<float>(color.Blue) / 255.0f,
	        static_cast<float>(color.Alpha) / 255.0f}};
}

const CompiledBinding* VulkanRenderCommandList::FindBindingByIndex(const VulkanBindingLayout* layout, std::uint32_t bindingIndex) noexcept
{
	if (layout == nullptr)
	{
		return nullptr;
	}

	const CompiledBinding* const bindings = layout->GetBindings();
	for (std::size_t index = 0; index < layout->GetBindingCount(); ++index)
	{
		if (bindings[index].BindingIndex == bindingIndex)
		{
			return &bindings[index];
		}
	}
	return nullptr;
}

VkShaderStageFlags VulkanRenderCommandList::ToVkShaderStages(ShaderStageMask visibilityMask) noexcept
{
	VkShaderStageFlags result = 0;
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Vertex))
	{
		result |= VK_SHADER_STAGE_VERTEX_BIT;
	}
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Pixel))
	{
		result |= VK_SHADER_STAGE_FRAGMENT_BIT;
	}
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Compute))
	{
		result |= VK_SHADER_STAGE_COMPUTE_BIT;
	}
	return result != 0 ? result : VK_SHADER_STAGE_ALL;
}

void VulkanRenderCommandList::BeginDynamicRenderingIfNeeded() noexcept
{
	if (m_dynamicRenderingActive || m_commandBuffer == VK_NULL_HANDLE || !m_hasScissorRect ||
	    (m_renderTargetCount == 0 && m_depthStencil == VK_NULL_HANDLE))
	{
		return;
	}

	std::array<VkRenderingAttachmentInfo, MaxRenderTargets> colorAttachments = {};
	for (std::uint32_t index = 0; index < m_renderTargetCount; ++index)
	{
		colorAttachments[index] = VkRenderingAttachmentInfo{
		    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
		    .pNext = nullptr,
		    .imageView = m_renderTargets[index],
		    .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		    .resolveMode = VK_RESOLVE_MODE_NONE,
		    .resolveImageView = VK_NULL_HANDLE,
		    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		    .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		    .clearValue = {}};
	}

	VkRenderingAttachmentInfo depthStencilAttachment{
	    .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
	    .pNext = nullptr,
	    .imageView = m_depthStencil,
	    .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
	    .resolveMode = VK_RESOLVE_MODE_NONE,
	    .resolveImageView = VK_NULL_HANDLE,
	    .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	    .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
	    .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
	    .clearValue = {}};

	const VkRenderingInfo renderingInfo{
	    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .renderArea = m_scissorRect,
	    .layerCount = 1,
	    .viewMask = 0,
	    .colorAttachmentCount = m_renderTargetCount,
	    .pColorAttachments = m_renderTargetCount > 0 ? colorAttachments.data() : nullptr,
	    .pDepthAttachment = m_depthStencil != VK_NULL_HANDLE ? &depthStencilAttachment : nullptr,
	    .pStencilAttachment = m_depthStencil != VK_NULL_HANDLE ? &depthStencilAttachment : nullptr};
	vkCmdBeginRendering(m_commandBuffer, &renderingInfo);
	m_dynamicRenderingActive = true;
}

VkDescriptorSet VulkanRenderCommandList::EnsureDescriptorSet(
    const VulkanBindingLayout* layout,
    std::uint32_t setIndex,
    std::vector<VkDescriptorSet>& descriptorSets) noexcept
{
	if (layout == nullptr || m_descriptorAllocator == nullptr || setIndex >= layout->GetDescriptorSetLayouts().size())
	{
		return VK_NULL_HANDLE;
	}

	if (descriptorSets.size() <= setIndex)
	{
		descriptorSets.resize(setIndex + 1u, VK_NULL_HANDLE);
	}
	if (descriptorSets[setIndex] == VK_NULL_HANDLE)
	{
		descriptorSets[setIndex] = m_descriptorAllocator->AllocateTransientSet(layout->GetDescriptorSetLayouts()[setIndex]);
	}
	return descriptorSets[setIndex];
}

void VulkanRenderCommandList::BindDescriptorSet(
    VkPipelineBindPoint bindPoint,
    VkPipelineLayout pipelineLayout,
    std::uint32_t setIndex,
    VkDescriptorSet descriptorSet) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || pipelineLayout == VK_NULL_HANDLE || descriptorSet == VK_NULL_HANDLE)
	{
		return;
	}
	vkCmdBindDescriptorSets(m_commandBuffer, bindPoint, pipelineLayout, setIndex, 1, &descriptorSet, 0, nullptr);
}

void VulkanRenderCommandList::EndDynamicRenderingIfNeeded() noexcept
{
	if (m_commandBuffer != VK_NULL_HANDLE && m_dynamicRenderingActive)
	{
		vkCmdEndRendering(m_commandBuffer);
		m_dynamicRenderingActive = false;
	}
}