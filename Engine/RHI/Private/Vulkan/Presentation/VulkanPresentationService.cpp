#include "Vulkan/Presentation/VulkanPresentationService.h"

#include "Vulkan/VulkanRenderHardwareInterface.h"

VulkanPresentationService::VulkanPresentationService(VulkanRenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiViewport VulkanPresentationService::GetBackBufferViewport() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferViewport() : RhiViewport{};
}

RhiRect VulkanPresentationService::GetBackBufferScissorRect() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle VulkanPresentationService::GetBackBufferRenderTargetView() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferRenderTargetView() : RhiCpuDescriptorHandle{};
}

RhiResourceHandle VulkanPresentationService::GetBackBufferResource() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferResource() : RhiResourceHandle{};
}

void VulkanPresentationService::BeginPresentRenderPass(const float clearColor[4]) noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->BeginPresentRenderPass(clearColor);
	}
}

void VulkanPresentationService::BeginPresentOverlayPass() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->BeginPresentOverlayPass();
	}
}

void VulkanPresentationService::EndPresentRenderPass() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->EndPresentRenderPass();
	}
}

PixelFormat VulkanPresentationService::GetPresentColorFormat() const noexcept
{
	return m_owner != nullptr ? m_owner->GetPresentColorFormat() : PixelFormat::Unknown;
}
