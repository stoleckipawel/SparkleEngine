#pragma once

#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "Upscaling/UpscalerSettings.h"
#include "Streamline/StreamlineDlssFrameState.h"

class NvidiaDlssRayReconstructionProvider final : public IRayReconstructionProvider
{
public:
	bool Initialize(const RhiCapabilities& capabilities, RhiNativeDeviceQueueInterop nativeInterop) override;
	void SetDenoisingExtent(RenderViewportExtent extent) noexcept override;
	void SetupFrame(const ImageProviderFrameInput& frameInput) override;
	bool Evaluate(const RayReconstructionEvaluationDesc& evaluation) override;
	void Shutdown() noexcept override;

private:
	StreamlineDlssFrameState m_frameState;
	bool m_initialized = false;
};
