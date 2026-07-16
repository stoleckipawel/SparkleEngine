#pragma once

#include "Presentation/RhiPresentationService.h"

class VulkanRenderHardwareInterface;

class VulkanPresentationService final : public RhiPresentationService
{
  public:
	explicit VulkanPresentationService(VulkanRenderHardwareInterface& owner) noexcept;

	RhiViewport GetBackBufferViewport() const noexcept override;
	RhiRect GetBackBufferScissorRect() const noexcept override;
	RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept override;
	RhiResourceHandle GetBackBufferResource() const noexcept override;
	std::uint64_t ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept override;
	void BeginPresentRenderPass(const float clearColor[4]) noexcept override;
	void BeginPresentOverlayPass() noexcept override;
	void EndPresentRenderPass() noexcept override;
	PixelFormat GetPresentColorFormat() const noexcept override;

  private:
	VulkanRenderHardwareInterface* m_owner = nullptr;
};
