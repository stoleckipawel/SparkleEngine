#pragma once

#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "Upscaling/UpscalerSettings.h"
#include "Streamline/StreamlineDlssFrameState.h"

class NvidiaDlssRayReconstructionProvider final : public IRayReconstructionProvider
{
  public:
	bool Initialize(const RhiCapabilities& capabilities, RhiNativeDeviceQueueInterop nativeInterop) override;
	RenderViewportExtent ResolveRenderExtent(RenderViewportExtent outputExtent) noexcept override;
	void SetupFrame(const ImageProviderFrameContext& frameContext) override;
	bool Evaluate(const RayReconstructionEvaluationDesc& evaluation) override;
	void Shutdown() noexcept override;

  private:
	StreamlineDlssFrameState m_frameState;
	bool m_initialized = false;
};
