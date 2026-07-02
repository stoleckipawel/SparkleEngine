#pragma once

#include "Renderer/Public/Settings/EngineRenderingRayReconstructionTypes.h"

struct RayReconstructionSettings final
{
	EngineRayReconstructionMode Mode = EngineRayReconstructionMode::Off;
	EngineRayReconstructionQualityMode QualityMode = EngineRayReconstructionQualityMode::Quality;
	bool DiagnosticsEnabled = false;
};

RayReconstructionSettings BuildRayReconstructionSettingsFromCVars() noexcept;
EngineRayReconstructionMode GetRayReconstructionModeFromCVars() noexcept;
const char* RayReconstructionModeToString(EngineRayReconstructionMode mode) noexcept;
const char* RayReconstructionQualityModeToString(EngineRayReconstructionQualityMode mode) noexcept;

void SetRayReconstructionModeCVar(EngineRayReconstructionMode mode) noexcept;
void SetRayReconstructionQualityModeCVar(EngineRayReconstructionQualityMode mode) noexcept;
