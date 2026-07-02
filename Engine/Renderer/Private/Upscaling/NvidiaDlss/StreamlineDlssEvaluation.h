#pragma once

#include "Upscaling/UpscalerInputContract.h"
#include "Upscaling/UpscalerProvider.h"
#include "Upscaling/UpscalerSettings.h"

bool HasDlssNativeEvaluationContract(const UpscalerEvaluationDesc& evaluation) noexcept;

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

UpscalerEvaluationResult EvaluateStreamlineDlssFrame(
    const UpscalerInputContract& inputContract,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation);
#endif
