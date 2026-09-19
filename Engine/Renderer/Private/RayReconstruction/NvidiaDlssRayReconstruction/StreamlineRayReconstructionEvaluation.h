#pragma once

#include "Providers/ImageProviderFrameInput.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "Upscaling/UpscalerSettings.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
  #include <sl.h>

bool EvaluateStreamlineRayReconstructionFrame(
    const ImageProviderFrameInput& frameInput,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation);
#endif
