#include "../PCH.h"
#include "RayReconstruction/RayReconstructionSettings.h"

#include "Core/Public/Console/CVar.h"

namespace
{
	ConsoleVariable<EngineRayReconstructionMode> CVarRayReconstructionMode(
	    "r.RayReconstruction.Mode",
	    EngineRayReconstructionMode::Off,
	    "Renderer ray reconstruction mode: 0=Off, 1=NVIDIA DLSS Ray Reconstruction.");

	ConsoleVariable<EngineRayReconstructionQualityMode> CVarRayReconstructionQualityMode(
	    "r.RayReconstruction.QualityMode",
	    EngineRayReconstructionQualityMode::Quality,
	    "Renderer ray reconstruction quality mode: 0=Quality, 1=Balanced, 2=Performance.");

}

RayReconstructionSettings BuildRayReconstructionSettingsFromCVars() noexcept
{
	return RayReconstructionSettings{
	    .Mode = CVarRayReconstructionMode.Get(),
	    .QualityMode = CVarRayReconstructionQualityMode.Get()};
}

EngineRayReconstructionMode GetRayReconstructionModeFromCVars() noexcept
{
	return CVarRayReconstructionMode.Get();
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

const char* RayReconstructionQualityModeToString(EngineRayReconstructionQualityMode mode) noexcept
{
	switch (mode)
	{
		case EngineRayReconstructionQualityMode::Balanced:
			return "Balanced";
		case EngineRayReconstructionQualityMode::Performance:
			return "Performance";
		case EngineRayReconstructionQualityMode::Quality:
		default:
			return "Quality";
	}
}

void SetRayReconstructionModeCVar(EngineRayReconstructionMode mode) noexcept
{
	CVarRayReconstructionMode.Set(mode);
}

void SetRayReconstructionQualityModeCVar(EngineRayReconstructionQualityMode mode) noexcept
{
	CVarRayReconstructionQualityMode.Set(mode);
}
