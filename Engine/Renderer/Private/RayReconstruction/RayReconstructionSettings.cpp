#include "../PCH.h"
#include "RayReconstruction/RayReconstructionSettings.h"

ConsoleVariable<EngineRayReconstructionMode> CVarRayReconstructionMode(
    "r.RayReconstruction.Mode",
    EngineRayReconstructionMode::Off,
    "Renderer ray reconstruction mode: 0=Off, 1=NVIDIA DLRR.");

const char* RayReconstructionModeToString(EngineRayReconstructionMode mode) noexcept
{
	switch (mode)
	{
		case EngineRayReconstructionMode::NvidiaDlrr:
			return "NVIDIA DLRR";
		case EngineRayReconstructionMode::Off:
		default:
			return "Off";
	}
}
