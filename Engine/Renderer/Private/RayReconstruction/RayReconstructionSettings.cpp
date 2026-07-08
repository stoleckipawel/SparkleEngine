#include "../PCH.h"
#include "RayReconstruction/RayReconstructionSettings.h"

#include "Core/Public/Console/CVar.h"

namespace
{
	ConsoleVariable<EngineRayReconstructionMode> CVarRayReconstructionMode(
	    "r.RayReconstruction.Mode",
	    EngineRayReconstructionMode::Off,
	    "Renderer ray reconstruction mode: 0=Off, 1=NVIDIA DLRR.");
}

RayReconstructionSettings BuildRayReconstructionSettingsFromCVars() noexcept
{
	return RayReconstructionSettings{
	    .Mode = CVarRayReconstructionMode.Get()};
}

EngineRayReconstructionMode GetRayReconstructionModeFromCVars() noexcept
{
	return CVarRayReconstructionMode.Get();
}

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

void SetRayReconstructionModeCVar(EngineRayReconstructionMode mode) noexcept
{
	CVarRayReconstructionMode.Set(mode);
}
