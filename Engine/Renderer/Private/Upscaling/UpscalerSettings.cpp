#include "../PCH.h"
#include "Upscaling/UpscalerSettings.h"

#include "Core/Public/Console/CVar.h"

namespace
{
	ConsoleVariable<EUpscalerProviderKind> CVarUpscalerProvider(
	    "r.Upscaler.Provider",
	    EUpscalerProviderKind::NvidiaDlss,
	    "Renderer upscaler provider: 0=Passthrough, 1=NVIDIA DLSS. Defaults to NVIDIA DLSS with deterministic passthrough fallback when unavailable.");

	ConsoleVariable<bool> CVarUpscalerDiagnosticsEnabled(
	    "r.Upscaler.Diagnostics",
	    false,
	    "Enable additional renderer upscaler diagnostics.");

	ConsoleVariable<EUpscalerQualityMode> CVarUpscalerQualityMode(
	    "r.Upscaler.QualityMode",
	    EUpscalerQualityMode::NativeAA,
	    "Renderer upscaler quality mode: 0=NativeAA, 1=Quality, 2=Balanced, 3=Performance, 4=UltraPerformance.");
}

UpscalerSettings BuildUpscalerSettingsFromCVars() noexcept
{
	return UpscalerSettings{
	    .RequestedProvider = CVarUpscalerProvider.Get(),
	    .QualityMode = CVarUpscalerQualityMode.Get(),
	    .DiagnosticsEnabled = CVarUpscalerDiagnosticsEnabled.Get()};
}

const char* UpscalerQualityModeToString(EUpscalerQualityMode mode) noexcept
{
	switch (mode)
	{
		case EUpscalerQualityMode::NativeAA:
			return "NativeAA";
		case EUpscalerQualityMode::Quality:
			return "Quality";
		case EUpscalerQualityMode::Balanced:
			return "Balanced";
		case EUpscalerQualityMode::Performance:
			return "Performance";
		case EUpscalerQualityMode::UltraPerformance:
			return "UltraPerformance";
	}

	return "Unknown";
}
