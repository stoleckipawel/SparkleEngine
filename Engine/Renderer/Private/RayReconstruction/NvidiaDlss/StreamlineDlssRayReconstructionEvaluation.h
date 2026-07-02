#pragma once

#include "RayReconstruction/RayReconstructionInputContract.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"

bool HasDlssRayReconstructionNativeEvaluationContract(const RayReconstructionEvaluationDesc& evaluation) noexcept;

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

RayReconstructionEvaluationResult EvaluateStreamlineDlssRayReconstructionFrame(
    const RayReconstructionInputContract& inputContract,
    EngineRayReconstructionQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation);
#endif
