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
	InitializeShaderBindingState(static_cast<const VulkanBindingLayout&>(bindingLayout), m_graphicsBindings);
}

void VulkanRenderCommandList::SetComputeBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	InitializeShaderBindingState(static_cast<const VulkanBindingLayout&>(bindingLayout), m_computeBindings);
}

void VulkanRenderCommandList::SetRayTracingBindingLayout(const RenderBindingLayout& bindingLayout) noexcept
{
	InitializeShaderBindingState(static_cast<const VulkanBindingLayout&>(bindingLayout), m_rayTracingBindings);
}

void VulkanRenderCommandList::ResetBoundState() noexcept
{
	EndDynamicRenderingIfNeeded();
	ResetShaderBindingState(m_graphicsBindings);
	ResetShaderBindingState(m_computeBindings);
	ResetShaderBindingState(m_rayTracingBindings);
	m_boundRayTracingPipeline = nullptr;
	m_retainedDescriptorTables.clear();
	m_retainedDescriptorHandles.clear();
	m_retainedDescriptorBuffers.clear();
}

void VulkanRenderCommandList::BindGraphicsConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_graphicsBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindGraphicsShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_graphicsBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindGraphicsUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_graphicsBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindGraphicsAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	BindShaderAccelerationStructure(m_graphicsBindings, bindingIndex, resource);
}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	BindShaderDescriptorTable(m_graphicsBindings, bindingIndex, tableBinding);
}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	BindShaderDescriptorTable(m_graphicsBindings, bindingIndex, baseDescriptor);
}

void VulkanRenderCommandList::SetGraphicsPushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	SetShaderPushConstants(m_graphicsBindings, bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void VulkanRenderCommandList::BindComputeConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_computeBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindComputeShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_computeBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindComputeUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_computeBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindComputeAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	BindShaderAccelerationStructure(m_computeBindings, bindingIndex, resource);
}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	BindShaderDescriptorTable(m_computeBindings, bindingIndex, tableBinding);
}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	BindShaderDescriptorTable(m_computeBindings, bindingIndex, baseDescriptor);
}

void VulkanRenderCommandList::SetComputePushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	SetShaderPushConstants(m_computeBindings, bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
}

void VulkanRenderCommandList::BindRayTracingConstantBuffer(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_rayTracingBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindRayTracingShaderResource(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_rayTracingBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindRayTracingUnorderedAccess(std::uint32_t bindingIndex, RhiGpuVirtualAddress gpuAddress) noexcept
{
	BindShaderBuffer(m_rayTracingBindings, bindingIndex, gpuAddress);
}

void VulkanRenderCommandList::BindRayTracingAccelerationStructure(std::uint32_t bindingIndex, RhiResourceHandle resource) noexcept
{
	BindShaderAccelerationStructure(m_rayTracingBindings, bindingIndex, resource);
}

void VulkanRenderCommandList::BindRayTracingDescriptorTable(std::uint32_t bindingIndex, RhiDescriptorTableBinding tableBinding) noexcept
{
	BindShaderDescriptorTable(m_rayTracingBindings, bindingIndex, tableBinding);
}

void VulkanRenderCommandList::BindRayTracingDescriptorTable(std::uint32_t bindingIndex, RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	BindShaderDescriptorTable(m_rayTracingBindings, bindingIndex, baseDescriptor);
}

void VulkanRenderCommandList::SetRayTracingPushConstants(
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	SetShaderPushConstants(m_rayTracingBindings, bindingIndex, num32BitValues, data, destOffsetIn32BitValues);
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
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::RayGeneration))
	{
		result |= VK_SHADER_STAGE_RAYGEN_BIT_KHR;
	}
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Miss))
	{
		result |= VK_SHADER_STAGE_MISS_BIT_KHR;
	}
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::ClosestHit))
	{
		result |= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
	}
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::AnyHit))
	{
		result |= VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
	}
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Intersection))
	{
		result |= VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
	}
	if (HasAnyShaderStageMask(visibilityMask, ShaderStageMask::Callable))
	{
		result |= VK_SHADER_STAGE_CALLABLE_BIT_KHR;
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
	if (layout == nullptr || m_descriptorAllocator == nullptr || m_recordingDescriptorPool == nullptr
	    || setIndex >= layout->GetDescriptorSetLayouts().size())
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
		if (binding.BindingPoint.Set != setIndex || binding.Type == CompiledBindingType::PushConstants
		    || binding.Type == CompiledBindingType::SamplerTable)
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

void VulkanRenderCommandList::InitializeShaderBindingState(const VulkanBindingLayout& layout, ShaderBindingState& state)
{
	state.Layout = &layout;
	state.DescriptorSets.assign(layout.GetDescriptorSetLayouts().size(), VK_NULL_HANDLE);
	state.DirtyDescriptorSets.assign(state.DescriptorSets.size(), false);
	state.BoundDescriptorSets.assign(state.DescriptorSets.size(), false);
}

void VulkanRenderCommandList::ReserveShaderBindingState(ShaderBindingState& state, std::size_t descriptorSetCount)
{
	state.DescriptorSets.reserve(descriptorSetCount);
	state.DirtyDescriptorSets.reserve(descriptorSetCount);
	state.BoundDescriptorSets.reserve(descriptorSetCount);
}

void VulkanRenderCommandList::ClearShaderBindingDescriptors(ShaderBindingState& state) noexcept
{
	state.DescriptorSets.clear();
	state.DirtyDescriptorSets.clear();
	state.BoundDescriptorSets.clear();
}

void VulkanRenderCommandList::ResetShaderBindingState(ShaderBindingState& state) noexcept
{
	state.Layout = nullptr;
	state.PipelineLayout = VK_NULL_HANDLE;
	ClearShaderBindingDescriptors(state);
}

void VulkanRenderCommandList::BindShaderBuffer(
    ShaderBindingState& state,
    std::uint32_t bindingIndex,
    RhiGpuVirtualAddress gpuAddress) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(state.Layout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(state.Layout, binding->BindingPoint.Set, state.DescriptorSets, state.BoundDescriptorSets);
	const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
	m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
	m_retainedDescriptorBuffers.push_back(buffer.Buffer);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, state.DirtyDescriptorSets);
}

void VulkanRenderCommandList::BindShaderAccelerationStructure(
    ShaderBindingState& state,
    std::uint32_t bindingIndex,
    RhiResourceHandle resource) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(state.Layout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(state.Layout, binding->BindingPoint.Set, state.DescriptorSets, state.BoundDescriptorSets);
	WriteAccelerationStructureBinding(descriptorSet, *binding, resource);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, state.DirtyDescriptorSets);
}

void VulkanRenderCommandList::BindShaderDescriptorTable(
    ShaderBindingState& state,
    std::uint32_t bindingIndex,
    RhiDescriptorTableBinding tableBinding) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(state.Layout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(state.Layout, binding->BindingPoint.Set, state.DescriptorSets, state.BoundDescriptorSets);
	if (binding->Type != CompiledBindingType::SamplerTable)
	{
		m_descriptorAllocator->WriteDescriptorTable(descriptorSet, *binding, tableBinding);
	}
	m_retainedDescriptorTables.push_back(tableBinding);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, state.DirtyDescriptorSets);
}

void VulkanRenderCommandList::BindShaderDescriptorTable(
    ShaderBindingState& state,
    std::uint32_t bindingIndex,
    RhiGpuDescriptorHandle baseDescriptor) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(state.Layout, bindingIndex);
	if (binding == nullptr || m_descriptorAllocator == nullptr)
	{
		return;
	}
	VkDescriptorSet descriptorSet =
	    EnsureDescriptorSet(state.Layout, binding->BindingPoint.Set, state.DescriptorSets, state.BoundDescriptorSets);
	m_descriptorAllocator->WriteDescriptorHandle(descriptorSet, *binding, baseDescriptor);
	m_retainedDescriptorHandles.push_back(baseDescriptor);
	MarkDescriptorSetDirty(binding->BindingPoint.Set, state.DirtyDescriptorSets);
}

void VulkanRenderCommandList::SetShaderPushConstants(
    const ShaderBindingState& state,
    std::uint32_t bindingIndex,
    std::uint32_t num32BitValues,
    const void* data,
    std::uint32_t destOffsetIn32BitValues) noexcept
{
	const CompiledBinding* const binding = FindBindingByIndex(state.Layout, bindingIndex);
	if (m_commandBuffer == VK_NULL_HANDLE || state.PipelineLayout == VK_NULL_HANDLE || binding == nullptr || data == nullptr
	    || num32BitValues == 0)
	{
		return;
	}
	vkCmdPushConstants(
	    m_commandBuffer,
	    state.PipelineLayout,
	    ToVkShaderStages(binding->VisibilityMask),
	    destOffsetIn32BitValues * sizeof(std::uint32_t),
	    num32BitValues * sizeof(std::uint32_t),
	    data);
}

void VulkanRenderCommandList::FlushShaderDescriptorSets(VkPipelineBindPoint bindPoint, ShaderBindingState& state) noexcept
{
	for (std::uint32_t setIndex = 0; setIndex < state.DescriptorSets.size() && setIndex < state.DirtyDescriptorSets.size(); ++setIndex)
	{
		if (!state.DirtyDescriptorSets[setIndex])
		{
			continue;
		}
		BindDescriptorSet(bindPoint, state.PipelineLayout, setIndex, state.DescriptorSets[setIndex]);
		state.DirtyDescriptorSets[setIndex] = false;
		if (state.BoundDescriptorSets.size() <= setIndex)
		{
			state.BoundDescriptorSets.resize(static_cast<std::size_t>(setIndex) + 1u, false);
		}
		state.BoundDescriptorSets[setIndex] = true;
	}
}

void VulkanRenderCommandList::FlushGraphicsDescriptorSets() noexcept
{
	FlushShaderDescriptorSets(VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsBindings);
}

void VulkanRenderCommandList::FlushComputeDescriptorSets() noexcept
{
	FlushShaderDescriptorSets(VK_PIPELINE_BIND_POINT_COMPUTE, m_computeBindings);
}

void VulkanRenderCommandList::FlushRayTracingDescriptorSets() noexcept
{
	FlushShaderDescriptorSets(VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rayTracingBindings);
}
