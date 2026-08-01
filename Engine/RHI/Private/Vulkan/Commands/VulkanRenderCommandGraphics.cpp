#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

#include "Vulkan/Descriptors/VulkanDescriptorHandles.h"
#include "Vulkan/Descriptors/VulkanDescriptorService.h"
#include "Vulkan/VulkanTypeConversions.h"
#include "Core/Public/Diagnostics/Verify.h"

#include <array>

static const auto g_vulkanRenderCommandListLogger = Logging::GetOrCreateLogger("RHI.Vulkan.CommandList");

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
	if (m_commandBuffer == VK_NULL_HANDLE || !m_hasScissorRect ||
	    VulkanDescriptorHandles::DecodeImageViewCpuHandle(depthStencil) != m_depthStencil)
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
		Diagnostics::Fatal(
		    g_vulkanRenderCommandListLogger,
		    __FILE__,
		    __LINE__,
		    "Vulkan depth-stencil view has no registered image aspect.");
	}
	return aspectMask;
}

void VulkanRenderCommandList::EndDynamicRenderingIfNeeded() noexcept
{
	if (m_commandBuffer != VK_NULL_HANDLE && m_dynamicRenderingActive)
	{
		vkCmdEndRendering(m_commandBuffer);
		m_dynamicRenderingActive = false;
	}
}
