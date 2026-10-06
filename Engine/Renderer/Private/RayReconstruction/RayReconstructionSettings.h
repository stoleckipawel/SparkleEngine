#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Settings/EngineRenderingRayReconstructionTypes.h"
#include "Renderer/Public/Viewport/RenderViewMode.h"

extern ConsoleVariable<EngineRayReconstructionMode> CVarRayReconstructionMode;

bool IsRayReconstructionEnabled() noexcept;
bool ShouldUseRayReconstruction(RenderViewMode viewMode) noexcept;
const char* RayReconstructionModeToString(EngineRayReconstructionMode mode) noexcept;
