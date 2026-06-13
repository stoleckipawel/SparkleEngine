#pragma once

#include "Presentation/RhiPresentationService.h"

class D3D12RenderHardwareInterface;

class D3D12PresentationService final : public RhiPresentationService
{
  public:
	explicit D3D12PresentationService(D3D12RenderHardwareInterface& owner) noexcept;

	RhiViewport GetBackBufferViewport() const noexcept override;
	RhiRect GetBackBufferScissorRect() const noexcept override;
	RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept override;
	NativeResourceHandle GetBackBufferResource() const noexcept override;
	std::uint64_t ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept override;
	void BeginPresentRenderPass(const float clearColor[4]) noexcept override;
	void BeginPresentOverlayPass() noexcept override;
	void EndPresentRenderPass() noexcept override;
	PixelFormat GetPresentColorFormat() const noexcept override;

  private:
	D3D12RenderHardwareInterface* m_owner = nullptr;
};
