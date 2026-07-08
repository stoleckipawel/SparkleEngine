#pragma once

#include "RayReconstruction/RayReconstructionProvider.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

sl::Result TagDlrrResourcesForFrame(
    const sl::FrameToken& frameToken,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation) noexcept;
#endif
