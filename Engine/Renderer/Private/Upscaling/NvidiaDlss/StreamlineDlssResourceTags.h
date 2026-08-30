#pragma once

#include "Upscaling/UpscalerProvider.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
  #include <sl.h>

sl::Result TagDlssResourcesForFrame(
    const sl::FrameToken& frameToken,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation) noexcept;
#endif
