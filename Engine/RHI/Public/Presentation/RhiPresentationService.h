#pragma once

#include "../Commands/RenderCommandList.h"
#include "../Descriptors/RhiDescriptorHandles.h"
#include "../Formats/PixelFormat.h"
#include "../Interop/RhiNativeHandles.h"
#include "../RHIAPI.h"

#include <cstdint>

class SPARKLE_RHI_API RhiPresentationService
{
  public:
	virtual ~RhiPresentationService() noexcept = default;

	virtual RhiViewport GetBackBufferViewport() const noexcept = 0;
	virtual RhiRect GetBackBufferScissorRect() const noexcept = 0;
	virtual RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept = 0;
	virtual NativeResourceHandle GetBackBufferResource() const noexcept = 0;
	virtual std::uint64_t ResolveImGuiTextureId(RhiGpuDescriptorHandle shaderResourceView) noexcept = 0;
	virtual void BeginPresentRenderPass(const float clearColor[4]) noexcept = 0;
	virtual void BeginPresentOverlayPass() noexcept = 0;
	virtual void EndPresentRenderPass() noexcept = 0;
	virtual PixelFormat GetPresentColorFormat() const noexcept = 0;
};
