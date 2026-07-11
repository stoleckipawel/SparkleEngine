#pragma once

#include "Providers/ImageProviderFrameContext.h"
#include "Upscaling/UpscalerSettings.h"

// Shared temporal/resolution state for DLSS-family image providers. Feature
// implementations remain responsible only for capability queries and evaluation.
class StreamlineDlssFrameState final
{
  public:
	StreamlineDlssFrameState() noexcept;
	EUpscalerQualityMode GetRequestedQualityMode() const noexcept;
	RenderViewportExtent StoreResolution(
	    RenderViewportExtent outputExtent,
	    RenderViewportExtent providerRenderExtent) noexcept;
	void SetupFrame(const ImageProviderFrameContext& frameContext) noexcept;
	void Reset() noexcept;

	bool IsValid() const noexcept { return m_frameValid; }
	EUpscalerQualityMode GetQualityMode() const noexcept { return m_qualityMode; }
	const ImageProviderFrameContext& GetFrameContext() const noexcept { return m_frameContext; }

  private:
	ImageProviderFrameContext m_frameContext = {};
	RenderViewportExtent m_resolvedOutputExtent = {};
	RenderViewportExtent m_resolvedRenderExtent = {};
	EUpscalerQualityMode m_qualityMode = EUpscalerQualityMode::Quality;
	bool m_frameValid = false;
};
