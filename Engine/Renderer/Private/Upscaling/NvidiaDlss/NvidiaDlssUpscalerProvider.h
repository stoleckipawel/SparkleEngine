#pragma once

#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"
#include "Streamline/StreamlineDlssFrameState.h"

class NvidiaDlssUpscalerProvider final : public IUpscalerProvider
{
public:
	bool Initialize(const RhiCapabilities& capabilities, RhiNativeDeviceQueueInterop nativeInterop) override;
	RenderViewportExtent ResolveRenderExtent(RenderViewportExtent outputExtent) noexcept override;
	void SetupFrame(const ImageProviderFrameInput& frameInput) override;
	bool Evaluate(const UpscalerEvaluationDesc& evaluation) override;
	void Shutdown() noexcept override;

private:
	StreamlineDlssFrameState m_frameState;
	bool m_initialized = false;
};
