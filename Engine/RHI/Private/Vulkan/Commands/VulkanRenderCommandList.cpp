#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipelineState.h"
#include "Vulkan/VulkanTypeConversions.h"

#include <algorithm>
#include <format>
#include <string_view>

static const auto g_vulkanRenderCommandListLogger = Logging::GetOrCreateLogger("RHI.Vulkan.CommandList");

VkPartitionedAccelerationStructureInstancesInputNV VulkanRenderCommandList::BuildPartitionedTlasInput(
    const RhiPartitionedTlasDesc& desc) noexcept
{
	return VkPartitionedAccelerationStructureInstancesInputNV{
	    .sType = VK_STRUCTURE_TYPE_PARTITIONED_ACCELERATION_STRUCTURE_INSTANCES_INPUT_NV,
	    .pNext = nullptr,
	    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	    .instanceCount = desc.InstanceCapacity,
	    .maxInstancePerPartitionCount = desc.MaxInstancesPerPartition,
	    .partitionCount = desc.PartitionCount,
	    .maxInstanceInGlobalPartitionCount = desc.MaxInstancesInGlobalPartition};
}

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
	m_graphicsDirtyDescriptorSets.clear();
	m_computeDirtyDescriptorSets.clear();
	m_graphicsBoundDescriptorSets.clear();
	m_computeBoundDescriptorSets.clear();
	m_retainedDescriptorTables.clear();
	m_retainedDescriptorHandles.clear();
	m_retainedDescriptorBuffers.clear();
	m_debugEvents = VulkanDebugEventFunctions{.BeginLabel = beginLabel, .EndLabel = endLabel, .InsertLabel = insertLabel};
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
	return VulkanDebugEvents::SupportsScopes(m_commandBuffer, m_debugEvents);
}

void VulkanRenderCommandList::BeginDiagnosticScope(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	VulkanDebugEvents::BeginScope(m_commandBuffer, m_debugEvents, label, color);
}

void VulkanRenderCommandList::EndDiagnosticScope() noexcept
{
	VulkanDebugEvents::EndScope(m_commandBuffer, m_debugEvents);
}

void VulkanRenderCommandList::InsertDiagnosticMarker(std::string_view label, RhiDiagnosticLabelColor color) noexcept
{
	VulkanDebugEvents::InsertMarker(m_commandBuffer, m_debugEvents, label, color);
}

void VulkanRenderCommandList::SetPipelineState(const RenderPipelineState& pipelineState) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const auto& vulkanPipelineState = static_cast<const VulkanPipelineState&>(pipelineState);
	if (vulkanPipelineState.GetPipeline() == VK_NULL_HANDLE || vulkanPipelineState.GetPipelineLayout() == VK_NULL_HANDLE)
	{
		SPDLOG_LOGGER_ERROR(g_vulkanRenderCommandListLogger, "VulkanRenderCommandList: refused to bind an incomplete pipeline state.");
		return;
	}
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
	m_graphicsDirtyDescriptorSets.assign(m_graphicsDescriptorSets.size(), false);
	m_graphicsBoundDescriptorSets.assign(m_graphicsDescriptorSets.size(), false);
}

void VulkanRenderCommandList::SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	m_computeBindingLayout = static_cast<const VulkanBindingLayout*>(&bindingLayout);
	m_computeDescriptorSets.assign(m_computeBindingLayout->GetDescriptorSetLayouts().size(), VK_NULL_HANDLE);
	m_computeDirtyDescriptorSets.assign(m_computeDescriptorSets.size(), false);
	m_computeBoundDescriptorSets.assign(m_computeDescriptorSets.size(), false);
}

void VulkanRenderCommandList::BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(m_graphicsBindingLayout, binding->BindingPoint.Set, m_graphicsDescriptorSets, m_graphicsBoundDescriptorSets);
	if (binding->SemanticKind == ShaderParameterSemanticKind::AccelerationStructure)
	{
		VulkanGpuAllocationRecord* const record =
		    m_memoryAllocator != nullptr ? m_memoryAllocator->FindAllocationRecordByDeviceAddress(gpuAddress) : nullptr;
		if (record == nullptr || (record->AccelerationStructure == VK_NULL_HANDLE && !record->IsPartitionedAccelerationStructure))
		{
			return;
		}
		if (record->IsPartitionedAccelerationStructure)
		{
			m_descriptorAllocator->WritePartitionedAccelerationStructureDescriptor(descriptorSet, *binding, record->DeviceAddress);
		}
		else
		{
			m_descriptorAllocator->WriteAccelerationStructureDescriptor(descriptorSet, *binding, record->AccelerationStructure);
		}
	}
	else
	{
		m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, reinterpret_cast<VkBuffer>(gpuAddress), 0, VK_WHOLE_SIZE);
		m_retainedDescriptorBuffers.push_back(reinterpret_cast<VkBuffer>(gpuAddress));
	}
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_graphicsDirtyDescriptorSets);
}

void VulkanRenderCommandList::BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(m_graphicsBindingLayout, binding->BindingPoint.Set, m_graphicsDescriptorSets, m_graphicsBoundDescriptorSets);
	if (binding->SemanticKind == ShaderParameterSemanticKind::AccelerationStructure)
	{
		VulkanGpuAllocationRecord* const record =
		    m_memoryAllocator != nullptr ? m_memoryAllocator->FindAllocationRecordByDeviceAddress(gpuAddress) : nullptr;
		if (record == nullptr || (record->AccelerationStructure == VK_NULL_HANDLE && !record->IsPartitionedAccelerationStructure))
		{
			return;
		}
		if (record->IsPartitionedAccelerationStructure)
		{
			m_descriptorAllocator->WritePartitionedAccelerationStructureDescriptor(descriptorSet, *binding, record->DeviceAddress);
		}
		else
		{
			m_descriptorAllocator->WriteAccelerationStructureDescriptor(descriptorSet, *binding, record->AccelerationStructure);
		}
	}
	else
	{
		m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, reinterpret_cast<VkBuffer>(gpuAddress), 0, VK_WHOLE_SIZE);
		m_retainedDescriptorBuffers.push_back(reinterpret_cast<VkBuffer>(gpuAddress));
	}
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_graphicsDirtyDescriptorSets);
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
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(m_graphicsBindingLayout, binding->BindingPoint.Set, m_graphicsDescriptorSets, m_graphicsBoundDescriptorSets);
	if (binding->Type != CompiledBindingType::SamplerTable)
	{
		m_descriptorAllocator->WriteDescriptorTable(descriptorSet, *binding, tableBinding);
	}
	m_retainedDescriptorTables.push_back(tableBinding);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_graphicsDirtyDescriptorSets);
}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(m_graphicsBindingLayout, binding->BindingPoint.Set, m_graphicsDescriptorSets, m_graphicsBoundDescriptorSets);
	m_descriptorAllocator->WriteDescriptorHandle(descriptorSet, *binding, baseDescriptor);
	m_retainedDescriptorHandles.push_back(baseDescriptor);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_graphicsDirtyDescriptorSets);
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
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets, m_computeBoundDescriptorSets);
	if (binding->SemanticKind == ShaderParameterSemanticKind::AccelerationStructure)
	{
		VulkanGpuAllocationRecord* const record =
		    m_memoryAllocator != nullptr ? m_memoryAllocator->FindAllocationRecordByDeviceAddress(gpuAddress) : nullptr;
		if (record == nullptr || (record->AccelerationStructure == VK_NULL_HANDLE && !record->IsPartitionedAccelerationStructure))
		{
			return;
		}
		if (record->IsPartitionedAccelerationStructure)
		{
			m_descriptorAllocator->WritePartitionedAccelerationStructureDescriptor(descriptorSet, *binding, record->DeviceAddress);
		}
		else
		{
			m_descriptorAllocator->WriteAccelerationStructureDescriptor(descriptorSet, *binding, record->AccelerationStructure);
		}
	}
	else
	{
		m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, reinterpret_cast<VkBuffer>(gpuAddress), 0, VK_WHOLE_SIZE);
		m_retainedDescriptorBuffers.push_back(reinterpret_cast<VkBuffer>(gpuAddress));
	}
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_computeDirtyDescriptorSets);
}

void VulkanRenderCommandList::BindComputeShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_computeBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets, m_computeBoundDescriptorSets);
	if (binding->SemanticKind == ShaderParameterSemanticKind::AccelerationStructure)
	{
		VulkanGpuAllocationRecord* const record =
		    m_memoryAllocator != nullptr ? m_memoryAllocator->FindAllocationRecordByDeviceAddress(gpuAddress) : nullptr;
		if (record == nullptr || (record->AccelerationStructure == VK_NULL_HANDLE && !record->IsPartitionedAccelerationStructure))
		{
			return;
		}
		if (record->IsPartitionedAccelerationStructure)
		{
			m_descriptorAllocator->WritePartitionedAccelerationStructureDescriptor(descriptorSet, *binding, record->DeviceAddress);
		}
		else
		{
			m_descriptorAllocator->WriteAccelerationStructureDescriptor(descriptorSet, *binding, record->AccelerationStructure);
		}
	}
	else
	{
		m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, reinterpret_cast<VkBuffer>(gpuAddress), 0, VK_WHOLE_SIZE);
		m_retainedDescriptorBuffers.push_back(reinterpret_cast<VkBuffer>(gpuAddress));
	}
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_computeDirtyDescriptorSets);
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
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets, m_computeBoundDescriptorSets);
	if (binding->Type != CompiledBindingType::SamplerTable)
	{
		m_descriptorAllocator->WriteDescriptorTable(descriptorSet, *binding, tableBinding);
	}
	m_retainedDescriptorTables.push_back(tableBinding);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_computeDirtyDescriptorSets);
}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_computeBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets, m_computeBoundDescriptorSets);
	m_descriptorAllocator->WriteDescriptorHandle(descriptorSet, *binding, baseDescriptor);
	m_retainedDescriptorHandles.push_back(baseDescriptor);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_computeDirtyDescriptorSets);
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

	const VkBuffer buffer = ResolveBuffer(view.BufferLocation);
	if (buffer == VK_NULL_HANDLE)
	{
		return;
	}
	constexpr VkDeviceSize offset = 0;
	vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &buffer, &offset);
}

void VulkanRenderCommandList::BindIndexBuffer(const RhiIndexBufferView& view) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || view.BufferLocation == 0)
	{
		return;
	}

	const VkBuffer buffer = ResolveBuffer(view.BufferLocation);
	if (buffer == VK_NULL_HANDLE)
	{
		return;
	}
	vkCmdBindIndexBuffer(m_commandBuffer, buffer, 0, VulkanTypeConversions::ToVkIndexType(view.Format));
}

void VulkanRenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle* dsv) noexcept
{
	EndDynamicRenderingIfNeeded();
	m_renderTargets = {};
	m_renderTargets[0] = VulkanDescriptorHandles::DecodeImageViewCpuHandle(rtv);
	m_renderTargetCount = m_renderTargets[0] != VK_NULL_HANDLE ? 1u : 0u;
	m_depthStencil = dsv != nullptr ? VulkanDescriptorHandles::DecodeImageViewCpuHandle(*dsv) : VK_NULL_HANDLE;
}

void VulkanRenderCommandList::SetRenderTargets(
    std::uint32_t numRTVs,
    const RhiCpuDescriptorHandle* rtvs,
    const RhiCpuDescriptorHandle* dsv) noexcept
{
	EndDynamicRenderingIfNeeded();
	m_renderTargets = {};
	m_renderTargetCount = 0;
	m_depthStencil = dsv != nullptr ? VulkanDescriptorHandles::DecodeImageViewCpuHandle(*dsv) : VK_NULL_HANDLE;
	if (rtvs == nullptr)
	{
		return;
	}

	const std::uint32_t count = std::min(numRTVs, MaxRenderTargets);
	for (std::uint32_t index = 0; index < count; ++index)
	{
		m_renderTargets[index] = VulkanDescriptorHandles::DecodeImageViewCpuHandle(rtvs[index]);
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

	const VkImageView imageView = VulkanDescriptorHandles::DecodeImageViewCpuHandle(rtv);
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
	if (m_commandBuffer == VK_NULL_HANDLE || !m_hasScissorRect || VulkanDescriptorHandles::DecodeImageViewCpuHandle(dsv) != m_depthStencil)
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
	    .y = viewport.Y + viewport.Height,
	    .width = viewport.Width,
	    .height = -viewport.Height,
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
	FlushGraphicsDescriptorSets();
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
	FlushGraphicsDescriptorSets();
	vkCmdDraw(m_commandBuffer, vertexCountPerInstance, instanceCount, startVertexLocation, startInstanceLocation);
}

void VulkanRenderCommandList::Dispatch(std::uint32_t groupCountX, std::uint32_t groupCountY, std::uint32_t groupCountZ) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	EndDynamicRenderingIfNeeded();
	FlushComputeDescriptorSets();
	vkCmdDispatch(m_commandBuffer, groupCountX, groupCountY, groupCountZ);
}

void VulkanRenderCommandList::BuildBottomLevelAccelerationStructure(
    const RhiRayTracingGeometryDesc& geometry,
    RhiGpuVirtualAddress scratchGpuAddress,
    RhiGpuVirtualAddress resultGpuAddress) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_memoryAllocator == nullptr ||
	    m_rhi->GetCmdBuildAccelerationStructures() == nullptr || scratchGpuAddress == 0 || resultGpuAddress == 0)
	{
		return;
	}
	EndDynamicRenderingIfNeeded();

	VulkanGpuAllocationRecord* const resultRecord = m_memoryAllocator->FindAllocationRecordByDeviceAddress(resultGpuAddress);
	if (resultRecord == nullptr || resultRecord->AccelerationStructure == VK_NULL_HANDLE ||
	    resultRecord->AccelerationStructureType != VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
	{
		return;
	}

	const VkAccelerationStructureGeometryTrianglesDataKHR triangles{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR,
	    .pNext = nullptr,
	    .vertexFormat = VK_FORMAT_R32G32B32_SFLOAT,
	    .vertexData = VkDeviceOrHostAddressConstKHR{.deviceAddress = geometry.VertexBuffer},
	    .vertexStride = geometry.VertexStrideInBytes,
	    .maxVertex = geometry.VertexCount > 0 ? geometry.VertexCount - 1u : 0u,
	    .indexType = VulkanTypeConversions::ToVkIndexType(geometry.IndexFormat),
	    .indexData = VkDeviceOrHostAddressConstKHR{.deviceAddress = geometry.IndexBuffer},
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
	    .dstAccelerationStructure = resultRecord->AccelerationStructure,
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
	    .dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
	                    VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
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
    RhiGpuVirtualAddress resultGpuAddress) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_memoryAllocator == nullptr ||
	    m_rhi->GetCmdBuildAccelerationStructures() == nullptr || instanceDescsGpuAddress == 0 || instanceCount == 0 ||
	    scratchGpuAddress == 0 || resultGpuAddress == 0)
	{
		return;
	}
	EndDynamicRenderingIfNeeded();

	VulkanGpuAllocationRecord* const resultRecord = m_memoryAllocator->FindAllocationRecordByDeviceAddress(resultGpuAddress);
	if (resultRecord == nullptr || resultRecord->AccelerationStructure == VK_NULL_HANDLE ||
	    resultRecord->AccelerationStructureType != VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
	{
		return;
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
	const VkAccelerationStructureBuildGeometryInfoKHR buildInfo{
	    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
	    .pNext = nullptr,
	    .type = VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR,
	    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
	    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
	    .srcAccelerationStructure = VK_NULL_HANDLE,
	    .dstAccelerationStructure = resultRecord->AccelerationStructure,
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
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_rhi->GetCmdBuildPartitionedAccelerationStructures() == nullptr ||
	    desc.DestinationAccelerationStructure == 0 || desc.Scratch == 0 || desc.OperationHeaders == 0 || desc.OperationCount == 0 ||
	    desc.Layout.InstanceCapacity == 0 || desc.Layout.PartitionCount == 0)
	{
		return;
	}
	EndDynamicRenderingIfNeeded();

	const VkPartitionedAccelerationStructureInstancesInputNV input = BuildPartitionedTlasInput(desc.Layout);
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
	    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
	                    VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
	    .dstAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_SHADER_SAMPLED_READ_BIT |
	                     VK_ACCESS_2_SHADER_STORAGE_READ_BIT};
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

void VulkanRenderCommandList::AliasResource(NativeResourceHandle beforeResource, NativeResourceHandle afterResource) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || (!beforeResource && !afterResource))
	{
		return;
	}
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

void VulkanRenderCommandList::TransitionResource(NativeResourceHandle resource, ResourceState before, ResourceState after) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || !resource || before == after)
	{
		return;
	}
	EndDynamicRenderingIfNeeded();

	VulkanResourceStateMapping sourceState = VulkanTypeConversions::ToResourceStateMapping(before);
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
	if (record == nullptr && before == ResourceState::Present && destinationState.ImageLayout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
	{
		sourceState = VulkanResourceStateMapping{};
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

void VulkanRenderCommandList::UnorderedAccessBarrier(NativeResourceHandle resource) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}
	EndDynamicRenderingIfNeeded();

	VkPipelineStageFlags2 srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
	VkAccessFlags2 srcAccessMask = VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	VkPipelineStageFlags2 dstStageMask = srcStageMask;
	VkAccessFlags2 dstAccessMask = VK_ACCESS_2_SHADER_STORAGE_READ_BIT | VK_ACCESS_2_SHADER_STORAGE_WRITE_BIT;
	VulkanGpuAllocationRecord* const record =
	    m_memoryAllocator != nullptr && resource ? m_memoryAllocator->FindAllocationRecord(resource) : nullptr;
	if (record != nullptr && record->AccelerationStructure != VK_NULL_HANDLE)
	{
		srcStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
		srcAccessMask = VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
		dstStageMask = VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT |
		               VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
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

VkBuffer VulkanRenderCommandList::ResolveBuffer(RhiGpuVirtualAddress gpuAddress) const noexcept
{
	if (gpuAddress == 0)
	{
		return VK_NULL_HANDLE;
	}
	if (m_memoryAllocator != nullptr)
	{
		VulkanGpuAllocationRecord* const record = m_memoryAllocator->FindAllocationRecordByDeviceAddress(gpuAddress);
		if (record != nullptr && record->Buffer != VK_NULL_HANDLE)
		{
			return record->Buffer;
		}
	}
	return reinterpret_cast<VkBuffer>(gpuAddress);
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
    std::vector<VkDescriptorSet>& descriptorSets,
    std::vector<bool>& boundSets) noexcept
{
	if (layout == nullptr || m_descriptorAllocator == nullptr || setIndex >= layout->GetDescriptorSetLayouts().size())
	{
		return VK_NULL_HANDLE;
	}

	if (descriptorSets.size() <= setIndex)
	{
		descriptorSets.resize(setIndex + 1u, VK_NULL_HANDLE);
	}
	if (boundSets.size() <= setIndex)
	{
		boundSets.resize(setIndex + 1u, false);
	}
	if (descriptorSets[setIndex] == VK_NULL_HANDLE || boundSets[setIndex])
	{
		const VkDescriptorSet previousSet = descriptorSets[setIndex];
		descriptorSets[setIndex] = m_descriptorAllocator->AllocateTransientSet(layout->GetDescriptorSetLayouts()[setIndex]);
		if (descriptorSets[setIndex] != VK_NULL_HANDLE)
		{
			m_descriptorAllocator->WriteFallbackDescriptors(
			    descriptorSets[setIndex],
			    layout->GetBindings(),
			    layout->GetBindingCount(),
			    setIndex);
		}
		if (previousSet != VK_NULL_HANDLE && boundSets[setIndex] && descriptorSets[setIndex] != VK_NULL_HANDLE)
		{
			CopyDescriptorSet(layout, setIndex, previousSet, descriptorSets[setIndex]);
		}
		boundSets[setIndex] = false;
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

void VulkanRenderCommandList::CopyDescriptorSet(
    const VulkanBindingLayout* layout,
    std::uint32_t setIndex,
    VkDescriptorSet sourceSet,
    VkDescriptorSet destinationSet) noexcept
{
	if (m_rhi == nullptr || layout == nullptr || sourceSet == VK_NULL_HANDLE || destinationSet == VK_NULL_HANDLE)
	{
		return;
	}

	std::vector<VkCopyDescriptorSet> copies;
	copies.reserve(layout->GetBindingCount());
	const CompiledBinding* const bindings = layout->GetBindings();
	for (std::size_t bindingIndex = 0; bindingIndex < layout->GetBindingCount(); ++bindingIndex)
	{
		const CompiledBinding& binding = bindings[bindingIndex];
		if (binding.BindingPoint.Set != setIndex || binding.Type == CompiledBindingType::PushConstants ||
		    binding.Type == CompiledBindingType::SamplerTable)
		{
			continue;
		}

		copies.push_back(
		    VkCopyDescriptorSet{
		        .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
		        .pNext = nullptr,
		        .srcSet = sourceSet,
		        .srcBinding = binding.BindingPoint.Binding,
		        .srcArrayElement = 0,
		        .dstSet = destinationSet,
		        .dstBinding = binding.BindingPoint.Binding,
		        .dstArrayElement = 0,
		        .descriptorCount = binding.DescriptorCount});
	}

	if (!copies.empty())
	{
		vkUpdateDescriptorSets(m_rhi->GetDevice(), 0, nullptr, static_cast<std::uint32_t>(copies.size()), copies.data());
	}
}

void VulkanRenderCommandList::MarkDescriptorSetDirty(std::uint32_t setIndex, std::vector<bool>& dirtySets) noexcept
{
	if (dirtySets.size() <= setIndex)
	{
		dirtySets.resize(static_cast<std::size_t>(setIndex) + 1u, false);
	}
	dirtySets[setIndex] = true;
}

void VulkanRenderCommandList::FlushGraphicsDescriptorSets() noexcept
{
	for (std::uint32_t setIndex = 0; setIndex < m_graphicsDescriptorSets.size() && setIndex < m_graphicsDirtyDescriptorSets.size(); ++setIndex)
	{
		if (!m_graphicsDirtyDescriptorSets[setIndex])
		{
			continue;
		}
		BindDescriptorSet(VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipelineLayout, setIndex, m_graphicsDescriptorSets[setIndex]);
		m_graphicsDirtyDescriptorSets[setIndex] = false;
		if (m_graphicsBoundDescriptorSets.size() <= setIndex)
		{
			m_graphicsBoundDescriptorSets.resize(static_cast<std::size_t>(setIndex) + 1u, false);
		}
		m_graphicsBoundDescriptorSets[setIndex] = true;
	}
}

void VulkanRenderCommandList::FlushComputeDescriptorSets() noexcept
{
	for (std::uint32_t setIndex = 0; setIndex < m_computeDescriptorSets.size() && setIndex < m_computeDirtyDescriptorSets.size(); ++setIndex)
	{
		if (!m_computeDirtyDescriptorSets[setIndex])
		{
			continue;
		}
		BindDescriptorSet(VK_PIPELINE_BIND_POINT_COMPUTE, m_computePipelineLayout, setIndex, m_computeDescriptorSets[setIndex]);
		m_computeDirtyDescriptorSets[setIndex] = false;
		if (m_computeBoundDescriptorSets.size() <= setIndex)
		{
			m_computeBoundDescriptorSets.resize(static_cast<std::size_t>(setIndex) + 1u, false);
		}
		m_computeBoundDescriptorSets[setIndex] = true;
	}
}

void VulkanRenderCommandList::EndDynamicRenderingIfNeeded() noexcept
{
	if (m_commandBuffer != VK_NULL_HANDLE && m_dynamicRenderingActive)
	{
		vkCmdEndRendering(m_commandBuffer);
		m_dynamicRenderingActive = false;
	}
}
