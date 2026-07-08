#pragma once

#include "RayReconstruction/RayReconstructionInputContract.h"
#include "RayReconstruction/RayReconstructionProvider.h"
#include "RayReconstruction/RayReconstructionSettings.h"

bool HasDlrrNativeEvaluationContract(const RayReconstructionEvaluationDesc& evaluation) noexcept;

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include <sl.h>

RayReconstructionEvaluationResult EvaluateStreamlineDlrrFrame(
    const RayReconstructionInputContract& inputContract,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation);
#endif
