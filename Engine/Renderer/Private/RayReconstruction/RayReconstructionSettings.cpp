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

std::uint32_t GetRayReconstructionModeKey() noexcept
{
	return static_cast<std::uint32_t>(CVarRayReconstructionMode.Get());
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
