#pragma once

#include "Providers/ImageProviderFrameContext.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"
#include "Upscaling/UpscalerSettings.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

RenderViewportExtent QueryStreamlineRayReconstructionOptimalRenderExtent(
    RenderViewportExtent outputExtent,
    EUpscalerQualityMode qualityMode) noexcept;
bool EvaluateStreamlineRayReconstructionFrame(
    const ImageProviderFrameContext& frameContext,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation);
#endif
