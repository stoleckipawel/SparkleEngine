#include "../PCH.h"
#include "RayReconstruction/RayReconstructionSettings.h"

ConsoleVariable<EngineRayReconstructionMode> CVarRayReconstructionMode(
    "r.RayReconstruction.Mode",
    EngineRayReconstructionMode::Off,
    "Renderer ray reconstruction mode: 0=Off, 1=NVIDIA DLSS Ray Reconstruction.");

bool IsRayReconstructionEnabled() noexcept
{
	return CVarRayReconstructionMode.Get() != EngineRayReconstructionMode::Off;
}

bool ShouldUseRayReconstruction(RenderViewMode viewMode) noexcept
{
	return viewMode == RenderViewMode::Lit && IsRayReconstructionEnabled();
}

const char* RayReconstructionModeToString(EngineRayReconstructionMode mode) noexcept
{
	switch (mode)
	{
		case EngineRayReconstructionMode::NvidiaDlssRayReconstruction:
			return "NVIDIA DLSS Ray Reconstruction";
		case EngineRayReconstructionMode::Off:
		default:
			return "Off";
	}
}
