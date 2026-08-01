#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

#include "Vulkan/Descriptors/VulkanDescriptorAllocator.h"
#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"
#include "Vulkan/Descriptors/VulkanDescriptorService.h"
#include "Vulkan/Descriptors/VulkanRecordingDescriptorPool.h"
#include "Vulkan/Device/VulkanRhi.h"
#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanBindingLayout.h"
#include "Vulkan/Pipeline/VulkanPipeline.h"
#include "Vulkan/Resources/VulkanRecordingUploadPage.h"
#include "Vulkan/VulkanTypeConversions.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Interop/RhiInteropService.h"
#include "Validation/RhiContract.h"

#include <algorithm>
#include <format>
#include <string_view>

static const auto g_vulkanRenderCommandListLogger = Logging::GetOrCreateLogger("RHI.Vulkan.CommandList");

VulkanRenderCommandList::VulkanRenderCommandList()
{
	m_graphicsDescriptorSets.reserve(8);
	m_computeDescriptorSets.reserve(8);
	m_graphicsDirtyDescriptorSets.reserve(8);
	m_computeDirtyDescriptorSets.reserve(8);
	m_graphicsBoundDescriptorSets.reserve(8);
	m_computeBoundDescriptorSets.reserve(8);

	m_retainedDescriptorTables.reserve(32);
	m_retainedDescriptorHandles.reserve(32);
	m_retainedDescriptorBuffers.reserve(32);
	m_recordingResourceUses.reserve(32);
	m_transientAllocationUses.reserve(32);
}

VulkanRenderCommandList::~VulkanRenderCommandList() noexcept
{
	ResetTrackedResources();
	AbandonTransientAllocationUses();
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

void VulkanRenderCommandList::CloseOpenRendering() noexcept
{
	EndDynamicRenderingIfNeeded();
}

RhiGpuVirtualAddress VulkanRenderCommandList::AllocateUniformConstantBuffer(const void* data, std::uint32_t sizeInBytes) noexcept
{
	return m_isRecording && m_recordingUploadPage != nullptr ? m_recordingUploadPage->AllocateAndCopy(data, sizeInBytes)
	                                                         : RhiGpuVirtualAddress{};
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
	m_depthStencilAspectMask = 0;
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

bool VulkanRenderCommandList::IsCoordinatorRecording() const noexcept
{
	return m_recordingOwner.IsCoordinator();
}

void VulkanRenderCommandList::TrackTransientAllocation(VulkanGpuAllocationRecord& allocation) noexcept
{
	allocation.RecordingReferenceCount.fetch_add(1, std::memory_order_relaxed);
	if (allocation.ParentMemoryBlock != nullptr)
	{
		allocation.ParentMemoryBlock->RecordingReferenceCount.fetch_add(1, std::memory_order_relaxed);
	}

	m_transientAllocationUses.push_back(&allocation);
}

void VulkanRenderCommandList::ResolveTransientAllocationUses(RhiSubmissionToken submissionToken) noexcept
{
	ReleaseTransientAllocationUses(submissionToken);
}

void VulkanRenderCommandList::AbandonTransientAllocationUses() noexcept
{
	ReleaseTransientAllocationUses({});
}

void VulkanRenderCommandList::ReleaseTransientAllocationUses(RhiSubmissionToken submissionToken) noexcept
{
	for (VulkanGpuAllocationRecord* allocation : m_transientAllocationUses)
	{
		if (allocation == nullptr)
		{
			continue;
		}

		allocation->LastUse.MarkUsed(submissionToken);
		if (allocation->ParentMemoryBlock != nullptr)
		{
			allocation->ParentMemoryBlock->LastUse.MarkUsed(submissionToken);
		}

		const std::uint32_t previousReferences = allocation->RecordingReferenceCount.fetch_sub(1, std::memory_order_relaxed);
		if (previousReferences == 0)
		{
			Diagnostics::Fatal(
			    g_vulkanRenderCommandListLogger,
			    __FILE__,
			    __LINE__,
			    "Vulkan transient allocation recording reference underflowed.");
		}

		if (allocation->ParentMemoryBlock != nullptr)
		{
			VulkanGpuMemoryBlockRecord& memoryBlock = *allocation->ParentMemoryBlock;
			const std::uint32_t previousBlockReferences = memoryBlock.RecordingReferenceCount.fetch_sub(1, std::memory_order_relaxed);
			if (previousBlockReferences == 0)
			{
				Diagnostics::Fatal(
				    g_vulkanRenderCommandListLogger,
				    __FILE__,
				    __LINE__,
				    "Vulkan transient memory-block recording reference underflowed.");
			}
		}
	}
	m_transientAllocationUses.clear();
}

void VulkanRenderCommandList::OnResourceTrackingStarted(RhiResourceHandle resource) noexcept
{
	if (m_memoryAllocator == nullptr)
	{
		return;
	}

	VulkanRecordingResourceUseToken use = m_memoryAllocator->RetainRecordingResource(resource);
	if (!use && IsCoordinatorRecording())
	{
		use = m_memoryAllocator->RetainCoordinatorRecordingResource(resource);
	}

	m_recordingResourceUses.push_back(RecordingResourceUse{.Resource = resource, .Token = use});
}

void VulkanRenderCommandList::OnResourceTrackingFinished(RhiResourceHandle resource, RhiSubmissionToken submissionToken) noexcept
{
	if (m_memoryAllocator == nullptr)
	{
		return;
	}

	if (m_recordingResourceReleaseIndex >= m_recordingResourceUses.size())
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan command-list resource tracking finished without a matching retained resource.");
	}
	const RecordingResourceUse& use = m_recordingResourceUses[m_recordingResourceReleaseIndex++];
	if (use.Resource.Value != resource.Value)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan command-list resource tracking release order does not match its recording order.");
	}
	m_memoryAllocator->ReleaseRecordingResource(use.Token, submissionToken);

	if (m_recordingResourceReleaseIndex == m_recordingResourceUses.size())
	{
		m_recordingResourceUses.clear();
		m_recordingResourceReleaseIndex = 0;
	}
}

NativeGraphicsCommandListHandle VulkanRenderCommandList::GetNativeHandle(const RhiNativeInteropRequest& request) const noexcept
{
	return IsRhiNativeInteropRequestValid(request) ? NativeGraphicsCommandListHandle{m_commandBuffer} : NativeGraphicsCommandListHandle{};
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

void VulkanRenderCommandList::SetPipeline(const RenderPipeline& pipeline) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	const auto& vulkanPipeline = static_cast<const VulkanPipeline&>(pipeline);
	if (vulkanPipeline.GetPipeline() == VK_NULL_HANDLE || vulkanPipeline.GetPipelineLayout() == VK_NULL_HANDLE)
	{
		SPDLOG_LOGGER_ERROR(g_vulkanRenderCommandListLogger, "VulkanRenderCommandList: refused to bind an incomplete pipeline.");
		return;
	}
	if (vulkanPipeline.GetBindPoint() == VK_PIPELINE_BIND_POINT_COMPUTE)
	{
		EndDynamicRenderingIfNeeded();
		m_computePipelineLayout = vulkanPipeline.GetPipelineLayout();
	}
	else
	{
		m_graphicsPipelineLayout = vulkanPipeline.GetPipelineLayout();
	}
	vkCmdBindPipeline(m_commandBuffer, vulkanPipeline.GetBindPoint(), vulkanPipeline.GetPipeline());
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
	if (binding->SemanticKind == ShaderParameterSemanticKind::AccelerationStructure)
	{
		VkDescriptorSet descriptorSet = EnsureDescriptorSet(
		    m_graphicsBindingLayout,
		    binding->BindingPoint.Set,
		    m_graphicsDescriptorSets,
		    m_graphicsBoundDescriptorSets);
		WriteAccelerationStructureBinding(descriptorSet, *binding, gpuAddress);
	}
	else
	{
		VkDescriptorSet descriptorSet = EnsureDescriptorSet(
		    m_graphicsBindingLayout,
		    binding->BindingPoint.Set,
		    m_graphicsDescriptorSets,
		    m_graphicsBoundDescriptorSets);
		const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
		m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
		m_retainedDescriptorBuffers.push_back(buffer.Buffer);
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
	if (binding->SemanticKind == ShaderParameterSemanticKind::AccelerationStructure)
	{
		VkDescriptorSet descriptorSet = EnsureDescriptorSet(
		    m_graphicsBindingLayout,
		    binding->BindingPoint.Set,
		    m_graphicsDescriptorSets,
		    m_graphicsBoundDescriptorSets);
		WriteAccelerationStructureBinding(descriptorSet, *binding, gpuAddress);
	}
	else
	{
		VkDescriptorSet descriptorSet = EnsureDescriptorSet(
		    m_graphicsBindingLayout,
		    binding->BindingPoint.Set,
		    m_graphicsDescriptorSets,
		    m_graphicsBoundDescriptorSets);
		const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
		m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
		m_retainedDescriptorBuffers.push_back(buffer.Buffer);
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
	if (binding->SemanticKind == ShaderParameterSemanticKind::AccelerationStructure)
	{
		VkDescriptorSet descriptorSet =
		    EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets, m_computeBoundDescriptorSets);
		WriteAccelerationStructureBinding(descriptorSet, *binding, gpuAddress);
	}
	else
	{
		VkDescriptorSet descriptorSet =
		    EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets, m_computeBoundDescriptorSets);
		const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
		m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
		m_retainedDescriptorBuffers.push_back(buffer.Buffer);
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
	if (binding->SemanticKind == ShaderParameterSemanticKind::AccelerationStructure)
	{
		VkDescriptorSet descriptorSet =
		    EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets, m_computeBoundDescriptorSets);
		WriteAccelerationStructureBinding(descriptorSet, *binding, gpuAddress);
	}
	else
	{
		VkDescriptorSet descriptorSet =
		    EnsureDescriptorSet(m_computeBindingLayout, binding->BindingPoint.Set, m_computeDescriptorSets, m_computeBoundDescriptorSets);
		const BufferBinding buffer = ResolveBufferBinding(gpuAddress);
		m_descriptorAllocator->WriteBufferDescriptor(descriptorSet, *binding, buffer.Buffer, buffer.Offset, buffer.Range);
		m_retainedDescriptorBuffers.push_back(buffer.Buffer);
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

void VulkanRenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle renderTarget, const RhiCpuDescriptorHandle* depthStencil) noexcept
{
	EndDynamicRenderingIfNeeded();
	m_renderTargets = {};
	m_renderTargets[0] = VulkanDescriptorHandles::DecodeImageViewCpuHandle(renderTarget);
	m_renderTargetCount = m_renderTargets[0] != VK_NULL_HANDLE ? 1u : 0u;
	m_depthStencil = depthStencil != nullptr ? VulkanDescriptorHandles::DecodeImageViewCpuHandle(*depthStencil) : VK_NULL_HANDLE;
	m_depthStencilAspectMask = ResolveDepthStencilAspectMask(m_depthStencil);
}

void VulkanRenderCommandList::SetRenderTargets(
    std::uint32_t renderTargetCount,
    const RhiCpuDescriptorHandle* renderTargets,
    const RhiCpuDescriptorHandle* depthStencil) noexcept
{
	EndDynamicRenderingIfNeeded();
	m_renderTargets = {};
	m_renderTargetCount = 0;
	m_depthStencil = depthStencil != nullptr ? VulkanDescriptorHandles::DecodeImageViewCpuHandle(*depthStencil) : VK_NULL_HANDLE;
	m_depthStencilAspectMask = ResolveDepthStencilAspectMask(m_depthStencil);
	if (renderTargetCount > MaxRenderTargets || (renderTargetCount != 0 && renderTargets == nullptr))
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan SetRenderTargets received an invalid render-target count or array.");
	}

	for (std::uint32_t index = 0; index < renderTargetCount; ++index)
	{
		m_renderTargets[index] = VulkanDescriptorHandles::DecodeImageViewCpuHandle(renderTargets[index]);
		if (m_renderTargets[index] != VK_NULL_HANDLE)
		{
			m_renderTargetCount = index + 1u;
		}
	}
}

void VulkanRenderCommandList::ClearRenderTarget(RhiCpuDescriptorHandle renderTarget, const float color[4]) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || color == nullptr || !m_hasScissorRect)
	{
		return;
	}

	const VkImageView imageView = VulkanDescriptorHandles::DecodeImageViewCpuHandle(renderTarget);
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

void VulkanRenderCommandList::ClearDepthStencil(RhiCpuDescriptorHandle depthStencil, float depth, std::uint8_t stencil) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || !m_hasScissorRect || VulkanDescriptorHandles::DecodeImageViewCpuHandle(depthStencil) != m_depthStencil)
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
	    .aspectMask = m_depthStencilAspectMask != 0 ? m_depthStencilAspectMask : VK_IMAGE_ASPECT_DEPTH_BIT,
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
	const VkDeviceAddress vertexBufferAddress = ResolveRayTracingBufferAddress(geometry.VertexBuffer);
	const VkDeviceAddress indexBufferAddress = ResolveRayTracingBufferAddress(geometry.IndexBuffer);
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_memoryAllocator == nullptr ||
	    m_rhi->GetCmdBuildAccelerationStructures() == nullptr || !RhiContract::IsRayTracingGeometryDescUsable(geometry) ||
	    vertexBufferAddress == 0 || indexBufferAddress == 0 || scratchGpuAddress == 0 || resultGpuAddress == 0)
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
	if (!ResolveAddress(resultGpuAddress, resultResource) || resultResource.AccelerationStructure == VK_NULL_HANDLE ||
	    resultResource.AccelerationStructureType != VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR)
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
    RhiGpuVirtualAddress resultGpuAddress,
    ERhiClassicTlasBuildMode buildMode) noexcept
{
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_memoryAllocator == nullptr ||
	    m_rhi->GetCmdBuildAccelerationStructures() == nullptr || instanceDescsGpuAddress == 0 || scratchGpuAddress == 0 ||
	    resultGpuAddress == 0)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan classic TLAS build received no device, command buffer, build entry point, or GPU address.");
	}
	EndDynamicRenderingIfNeeded();

	VulkanRecordingResource resultResource;
	if (!ResolveAddress(resultGpuAddress, resultResource) || resultResource.AccelerationStructure == VK_NULL_HANDLE ||
	    resultResource.AccelerationStructureType != VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR)
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
	const VkBuildAccelerationStructureFlagsKHR nativeBuildFlags =
	    VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR |
	    (buildMode != ERhiClassicTlasBuildMode::Build ? VK_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT_KHR
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
	if (m_commandBuffer == VK_NULL_HANDLE || m_rhi == nullptr || m_rhi->GetCmdBuildPartitionedAccelerationStructures() == nullptr ||
	    !desc.DestinationResource || desc.DestinationAccelerationStructure == 0 || desc.Scratch == 0 || desc.OperationHeaders == 0 ||
	    desc.OperationCount == 0 ||
	    desc.Layout.InstanceCapacity == 0 || desc.Layout.PartitionCount == 0)
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
	    .dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT |
	                    VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR,
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

VkDeviceAddress VulkanRenderCommandList::ResolveRayTracingBufferAddress(const RhiRayTracingBufferBinding& binding) const noexcept
{
	VulkanRecordingResource resource;
	if (!ResolveResource(binding.Resource, resource) || resource.Buffer == VK_NULL_HANDLE || resource.BufferDeviceAddress == 0)
	{
		return 0;
	}

	return resource.BufferDeviceAddress + binding.OffsetInBytes;
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

void VulkanRenderCommandList::WriteAccelerationStructureBinding(
    VkDescriptorSet descriptorSet,
    const CompiledBinding& binding,
    RhiGpuVirtualAddress address) noexcept
{
	VulkanRecordingResource resource;
	if (!ResolveAddress(address, resource) ||
	    (resource.AccelerationStructure == VK_NULL_HANDLE && !resource.IsPartitionedAccelerationStructure))
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan acceleration-structure binding references an unresolved GPU address.");
	}

	if (resource.IsPartitionedAccelerationStructure)
	{
		m_descriptorAllocator->WritePartitionedAccelerationStructureDescriptor(descriptorSet, binding, resource.DeviceAddress);
		return;
	}

	m_descriptorAllocator->WriteAccelerationStructureDescriptor(descriptorSet, binding, resource.AccelerationStructure);
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

	const bool hasDepthAttachment = (m_depthStencilAspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != 0;
	const bool hasStencilAttachment = (m_depthStencilAspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != 0;
	const VkRenderingInfo renderingInfo{
	    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
	    .pNext = nullptr,
	    .flags = 0,
	    .renderArea = m_scissorRect,
	    .layerCount = 1,
	    .viewMask = 0,
	    .colorAttachmentCount = m_renderTargetCount,
	    .pColorAttachments = m_renderTargetCount > 0 ? colorAttachments.data() : nullptr,
	    .pDepthAttachment = hasDepthAttachment ? &depthStencilAttachment : nullptr,
	    .pStencilAttachment = hasStencilAttachment ? &depthStencilAttachment : nullptr};
	vkCmdBeginRendering(m_commandBuffer, &renderingInfo);
	m_dynamicRenderingActive = true;
}

VkImageAspectFlags VulkanRenderCommandList::ResolveDepthStencilAspectMask(VkImageView imageView) const noexcept
{
	if (imageView == VK_NULL_HANDLE)
	{
		return 0;
	}

	if (m_descriptorService == nullptr)
	{
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan depth-stencil aspect resolution requires an active descriptor manager.");
	}
	const VkImageAspectFlags aspectMask = m_descriptorService->ResolveImageViewAspectMask(imageView);
	if (aspectMask == 0)
	{
		Diagnostics::Fatal(g_vulkanRenderCommandListLogger, __FILE__, __LINE__, "Vulkan depth-stencil view has no registered image aspect.");
	}
	return aspectMask;
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

void VulkanRenderCommandList::EndDynamicRenderingIfNeeded() noexcept
{
	if (m_commandBuffer != VK_NULL_HANDLE && m_dynamicRenderingActive)
	{
		vkCmdEndRendering(m_commandBuffer);
		m_dynamicRenderingActive = false;
	}
}
