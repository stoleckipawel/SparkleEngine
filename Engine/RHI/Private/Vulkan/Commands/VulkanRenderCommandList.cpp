#include "Vulkan/VulkanPCH.h"

#include "Vulkan/Commands/VulkanRenderCommandList.h"

#include <algorithm>

void VulkanRenderCommandList::SetNativeCommandBuffer(
	VkCommandBuffer commandBuffer,
	PFN_vkCmdBeginDebugUtilsLabelEXT beginLabel,
	PFN_vkCmdEndDebugUtilsLabelEXT endLabel,
	PFN_vkCmdInsertDebugUtilsLabelEXT insertLabel) noexcept
{
	m_commandBuffer = commandBuffer;
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

void VulkanRenderCommandList::SetPipelineState(const RenderPipelineState&) noexcept {}

void VulkanRenderCommandList::SetGraphicsBindingLayout(const RenderBindingLayout&) noexcept {}

void VulkanRenderCommandList::SetComputeBindingLayout(const RenderBindingLayout&) noexcept {}

void VulkanRenderCommandList::BindGraphicsConstantBuffer(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindGraphicsShaderResource(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindGraphicsUnorderedAccess(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t, RhiDescriptorTableBinding) noexcept {}

void VulkanRenderCommandList::BindGraphicsDescriptorTable(std::uint32_t, RhiGpuDescriptorHandle) noexcept {}

void VulkanRenderCommandList::SetGraphicsPushConstants(std::uint32_t, std::uint32_t, const void*, std::uint32_t) noexcept {}

void VulkanRenderCommandList::BindComputeConstantBuffer(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindComputeShaderResource(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindComputeUnorderedAccess(std::uint32_t, RhiGpuVirtualAddress) noexcept {}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t, RhiDescriptorTableBinding) noexcept {}

void VulkanRenderCommandList::BindComputeDescriptorTable(std::uint32_t, RhiGpuDescriptorHandle) noexcept {}

void VulkanRenderCommandList::SetComputePushConstants(std::uint32_t, std::uint32_t, const void*, std::uint32_t) noexcept {}

void VulkanRenderCommandList::SetPrimitiveTopology(RhiPrimitiveTopology) noexcept {}

void VulkanRenderCommandList::BindVertexBuffer(const RhiVertexBufferView&) noexcept {}

void VulkanRenderCommandList::BindIndexBuffer(const RhiIndexBufferView&) noexcept {}

void VulkanRenderCommandList::SetRenderTarget(RhiCpuDescriptorHandle rtv, const RhiCpuDescriptorHandle*) noexcept
{
	m_renderTargets = {};
	m_renderTargets[0] = DecodeImageViewHandle(rtv);
	m_renderTargetCount = m_renderTargets[0] != VK_NULL_HANDLE ? 1u : 0u;
}

void VulkanRenderCommandList::SetRenderTargets(std::uint32_t numRTVs, const RhiCpuDescriptorHandle* rtvs, const RhiCpuDescriptorHandle*) noexcept
{
	m_renderTargets = {};
	m_renderTargetCount = 0;
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

void VulkanRenderCommandList::ClearDepthStencil(RhiCpuDescriptorHandle, float, std::uint8_t) noexcept {}

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

void VulkanRenderCommandList::DrawIndexedInstanced(std::uint32_t, std::uint32_t, std::uint32_t, std::int32_t, std::uint32_t) noexcept {}

void VulkanRenderCommandList::DrawInstanced(std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t) noexcept {}

void VulkanRenderCommandList::Dispatch(std::uint32_t, std::uint32_t, std::uint32_t) noexcept {}

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

void VulkanRenderCommandList::CopyResource(NativeResourceHandle, NativeResourceHandle) noexcept {}

void VulkanRenderCommandList::AliasResource(NativeResourceHandle, NativeResourceHandle) noexcept {}

void VulkanRenderCommandList::TransitionResource(NativeResourceHandle, ResourceState, ResourceState) noexcept {}

void VulkanRenderCommandList::UnorderedAccessBarrier(NativeResourceHandle) noexcept {}

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