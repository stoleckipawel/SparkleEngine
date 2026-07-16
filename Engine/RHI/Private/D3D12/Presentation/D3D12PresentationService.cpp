#include "D3D12/Presentation/D3D12PresentationService.h"

#include "D3D12/D3D12RenderHardwareInterface.h"

D3D12PresentationService::D3D12PresentationService(D3D12RenderHardwareInterface& owner) noexcept : m_owner(&owner) {}

RhiViewport D3D12PresentationService::GetBackBufferViewport() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferViewport() : RhiViewport{};
}

RhiRect D3D12PresentationService::GetBackBufferScissorRect() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferScissorRect() : RhiRect{};
}

RhiCpuDescriptorHandle D3D12PresentationService::GetBackBufferRenderTargetView() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferRenderTargetView() : RhiCpuDescriptorHandle{};
}

RhiResourceHandle D3D12PresentationService::GetBackBufferResource() const noexcept
{
	return m_owner != nullptr ? m_owner->GetBackBufferResource() : RhiResourceHandle{};
}

std::uint64_t D3D12PresentationService::ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept
{
	return m_owner != nullptr ? m_owner->ResolveImGuiTextureId(shaderResourceView) : 0;
}

void D3D12PresentationService::BeginPresentRenderPass(const float clearColor[4]) noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->BeginPresentRenderPass(clearColor);
	}
}

void D3D12PresentationService::BeginPresentOverlayPass() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->BeginPresentOverlayPass();
	}
}

void D3D12PresentationService::EndPresentRenderPass() noexcept
{
	if (m_owner != nullptr)
	{
		m_owner->EndPresentRenderPass();
	}
}

PixelFormat D3D12PresentationService::GetPresentColorFormat() const noexcept
{
	return m_owner != nullptr ? m_owner->GetPresentColorFormat() : PixelFormat::Unknown;
}
