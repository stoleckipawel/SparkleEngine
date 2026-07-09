#pragma once

#include "Core/Public/Console/CVar.h"
#include "Renderer/Public/Settings/EngineRenderingUpscalingTypes.h"
#include "Renderer/Public/Viewport/ViewportContracts.h"

extern ConsoleVariable<EUpscalerProviderKind> CVarUpscalerProvider;
extern ConsoleVariable<EUpscalerQualityMode> CVarUpscalerQualityMode;

const char* UpscalerQualityModeToString(EUpscalerQualityMode mode) noexcept;
RenderViewportExtent ResolveUpscalerRenderExtent(RenderViewportExtent outputExtent, EUpscalerQualityMode qualityMode) noexcept;
