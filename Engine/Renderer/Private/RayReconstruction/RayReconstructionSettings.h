#pragma once

#include "Renderer/Public/Settings/EngineRenderingRayReconstructionTypes.h"

struct RayReconstructionSettings final
{
	EngineRayReconstructionMode Mode = EngineRayReconstructionMode::Off;
};

RayReconstructionSettings BuildRayReconstructionSettingsFromCVars() noexcept;
EngineRayReconstructionMode GetRayReconstructionModeFromCVars() noexcept;
const char* RayReconstructionModeToString(EngineRayReconstructionMode mode) noexcept;

void SetRayReconstructionModeCVar(EngineRayReconstructionMode mode) noexcept;
