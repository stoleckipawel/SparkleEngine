#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

#include "Vulkan/Memory/VulkanGpuAllocation.h"
#include "Vulkan/Memory/VulkanGpuMemoryAllocator.h"
#include "Vulkan/Pipeline/VulkanPipeline.h"
#include "Vulkan/Resources/VulkanRecordingUploadPage.h"
#include "Core/Public/Diagnostics/Verify.h"
#include "Interop/RhiInteropService.h"

#include <atomic>

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
