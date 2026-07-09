#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Settings/EngineRenderingRayReconstructionTypes.h"

extern ConsoleVariable<EngineRayReconstructionMode> CVarRayReconstructionMode;

const char* RayReconstructionModeToString(EngineRayReconstructionMode mode) noexcept;
