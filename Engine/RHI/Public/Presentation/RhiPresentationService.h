#pragma once

#include "../Descriptors/RhiDescriptorHandles.h"
#include "../Formats/PixelFormat.h"
#include "../Resources/RhiResourceDesc.h"
#include "../Resources/RhiResourceHandles.h"
#include "../RHIAPI.h"

class SPARKLE_RHI_API RhiPresentationService
{
public:
	virtual ~RhiPresentationService() noexcept = default;
	RhiPresentationService(const RhiPresentationService&) = delete;
	RhiPresentationService& operator=(const RhiPresentationService&) = delete;
	RhiPresentationService(RhiPresentationService&&) = delete;
	RhiPresentationService& operator=(RhiPresentationService&&) = delete;

	virtual RhiViewport GetBackBufferViewport() const noexcept = 0;
	virtual RhiRect GetBackBufferScissorRect() const noexcept = 0;
	virtual RhiCpuDescriptorHandle GetBackBufferRenderTargetView() const noexcept = 0;
	virtual RhiResourceHandle GetBackBufferResource() const noexcept = 0;
	virtual void BeginPresentRenderPass(RhiClearColorView clearColor) noexcept = 0;
	virtual void BeginPresentOverlayPass() noexcept = 0;
	virtual void EndPresentRenderPass() noexcept = 0;
	virtual PixelFormat GetPresentColorFormat() const noexcept = 0;

protected:
	RhiPresentationService() noexcept = default;
};
