#pragma once

#include "Providers/ImageProviderFrameContext.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

RenderViewportExtent QueryStreamlineDlssOptimalRenderExtent(
    RenderViewportExtent outputExtent,
    EUpscalerQualityMode qualityMode) noexcept;
bool EvaluateStreamlineDlssFrame(
    const ImageProviderFrameContext& frameContext,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation);
#endif
