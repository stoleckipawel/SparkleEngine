#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Settings/EngineRenderingRayReconstructionTypes.h"

#include <cstdint>

extern ConsoleVariable<EngineRayReconstructionMode> CVarRayReconstructionMode;

bool IsRayReconstructionEnabled() noexcept;
std::uint32_t GetRayReconstructionModeKey() noexcept;
const char* RayReconstructionModeToString(EngineRayReconstructionMode mode) noexcept;
