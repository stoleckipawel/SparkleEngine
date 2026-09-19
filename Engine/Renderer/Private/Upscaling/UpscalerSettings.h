#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Settings/EngineRenderingUpscalingTypes.h"

#include <cstdint>

extern ConsoleVariable<EUpscalerProviderKind> CVarUpscalerProvider;
extern ConsoleVariable<EUpscalerQualityMode> CVarUpscalerQualityMode;

std::uint32_t GetUpscalerProviderSelectionKey() noexcept;
const char* UpscalerQualityModeToString(EUpscalerQualityMode mode) noexcept;
