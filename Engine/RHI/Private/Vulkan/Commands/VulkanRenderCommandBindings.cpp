#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Descriptors/VulkanRecordingDescriptorPool.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Resources/VulkanRecordingUploadPage.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <array>

static const auto g_vulkanRenderCommandListLogger = Logging::GetOrCreateLogger("RHI.Vulkan.CommandList");

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

void VulkanRenderCommandList::ResetBoundState() noexcept
{
	EndDynamicRenderingIfNeeded();
	m_graphicsBindingLayout = nullptr;
	m_computeBindingLayout = nullptr;
	m_graphicsPipelineLayout = VK_NULL_HANDLE;
	m_computePipelineLayout = VK_NULL_HANDLE;
	m_graphicsDescriptorSets.clear();
	m_computeDescriptorSets.clear();
	m_graphicsDirtyDescriptorSets.clear();

	m_computeDirtyDescriptorSets.clear();
	m_graphicsBoundDescriptorSets.clear();
	m_computeBoundDescriptorSets.clear();
	m_retainedDescriptorTables.clear();
	m_retainedDescriptorHandles.clear();
	m_retainedDescriptorBuffers.clear();
}

void VulkanRenderCommandList::BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(
	    m_graphicsBindingLayout,
	    binding->BindingPoint.Set,
	    m_graphicsDescriptorSets,
	    m_graphicsBoundDescriptorSets);
	const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
	m_retainedDescriptorBuffers.push_back(buffer.Buffer);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_graphicsDirtyDescriptorSets);
}

void VulkanRenderCommandList::BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(
	    m_graphicsBindingLayout,
	    binding->BindingPoint.Set,
	    m_graphicsDescriptorSets,
	    m_graphicsBoundDescriptorSets);
	const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
	m_retainedDescriptorBuffers.push_back(buffer.Buffer);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_graphicsDirtyDescriptorSets);
}

void VulkanRenderCommandList::BindGraphicsUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindGraphicsShaderResource(bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindGraphicsAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_graphicsBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(
	    m_graphicsBindingLayout,
	    binding->BindingPoint.Set,
	    m_graphicsDescriptorSets,
	    m_graphicsBoundDescriptorSets);
	WriteAccelerationStructureBinding(descriptorSet, *binding, resource);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_graphicsDirtyDescriptorSets);
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
	const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
	m_retainedDescriptorBuffers.push_back(buffer.Buffer);
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
	const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
	m_retainedDescriptorBuffers.push_back(buffer.Buffer);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_computeDirtyDescriptorSets);
}

void VulkanRenderCommandList::BindComputeUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindComputeShaderResource(bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindComputeAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(m_computeBindingLayout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet = EnsureDescriptorSet(
	    m_computeBindingLayout,
	    binding->BindingPoint.Set,
	    m_computeDescriptorSets,
	    m_computeBoundDescriptorSets);
	WriteAccelerationStructureBinding(descriptorSet, *binding, resource);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, m_computeDirtyDescriptorSets);
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

void VulkanRenderCommandList::WriteAccelerationStructureBinding(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    RhiResourceHandle resourceHandle) noexcept
{
	VulkanGpuAllocationRecord* const allocation =
	    m_memoryAllocator != nullptr ? m_memoryAllocator->FindAllocationRecord(resourceHandle) : nullptr;
	if (allocation == nullptr || (allocation->AccelerationStructure == VK_NULL_HANDLE && !allocation->IsPartitionedAccelerationStructure))
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan acceleration-structure binding references an unresolved resource.");
	}

	if (allocation->IsPartitionedAccelerationStructure)
	{
		m_descriptorAllocator->WritePartitionedAccelerationStructureDescriptor(descriptorSet, binding, allocation->DeviceAddress);
		return;
	}

	m_descriptorAllocator->WriteAccelerationStructureDescriptor(descriptorSet, binding, allocation->AccelerationStructure);
}

VkDescriptorSet VulkanRenderCommandList::EnsureDescriptorSet(
    const VulkanBindingLayout* layout,
    std::uint32_t setIndex,
    std::vector<VkDescriptorSet>& descriptorSets,
    std::vector<bool>& boundSets) noexcept
{
	if (layout == nullptr || m_descriptorAllocator == nullptr || m_recordingDescriptorPool == nullptr ||
	    setIndex >= layout->GetDescriptorSetLayouts().size())
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
		descriptorSets[setIndex] = m_recordingDescriptorPool->AllocateSet(layout->GetDescriptorSetLayouts()[setIndex]);
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

	std::array<VkCopyDescriptorSet, 32> copies{};
	std::uint32_t copyCount = 0;

	const CompiledBinding* const bindings = layout->GetBindings();
	for (std::size_t bindingIndex = 0; bindingIndex < layout->GetBindingCount(); ++bindingIndex)
	{
		const CompiledBinding& binding = bindings[bindingIndex];
		if (binding.BindingPoint.Set != setIndex || binding.Type == CompiledBindingType::PushConstants ||
		    binding.Type == CompiledBindingType::SamplerTable)
		{
			continue;
		}

		copies[copyCount++] = VkCopyDescriptorSet{
		    .sType = VK_STRUCTURE_TYPE_COPY_DESCRIPTOR_SET,
		    .pNext = nullptr,
		    .srcSet = sourceSet,
		    .srcBinding = binding.BindingPoint.Binding,
		    .srcArrayElement = 0,
		    .dstSet = destinationSet,
		    .dstBinding = binding.BindingPoint.Binding,
		    .dstArrayElement = 0,
		    .descriptorCount = binding.DescriptorCount};

		if (copyCount == copies.size())
		{
			vkUpdateDescriptorSets(m_rhi->GetDevice(), 0, nullptr, copyCount, copies.data());
			copyCount = 0;
		}
	}

	if (copyCount != 0)
	{
		vkUpdateDescriptorSets(m_rhi->GetDevice(), 0, nullptr, copyCount, copies.data());
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
	for (std::uint32_t setIndex = 0; setIndex < m_graphicsDescriptorSets.size() && setIndex < m_graphicsDirtyDescriptorSets.size();
	     ++setIndex)
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
	for (std::uint32_t setIndex = 0; setIndex < m_computeDescriptorSets.size() && setIndex < m_computeDirtyDescriptorSets.size();
	     ++setIndex)
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
