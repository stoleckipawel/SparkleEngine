#include "../PCH.h"
#include "Upscaling/UpscalerSettings.h"

#include "Core/Public/Console/CVar.h"

#include <algorithm>

namespace
{
	ConsoleVariable<EUpscalerProviderKind> CVarUpscalerProvider(
	    "r.Upscaler.Provider",
	    EUpscalerProviderKind::NvidiaDlss,
	    "Renderer upscaler provider: 0=Linear, 1=NVIDIA DLSS.");

	ConsoleVariable<EUpscalerQualityMode> CVarUpscalerQualityMode(
	    "r.Upscaler.QualityMode",
	    EUpscalerQualityMode::NativeAA,
	    "Renderer upscaler quality mode: 0=NativeAA, 1=Quality, 2=Balanced, 3=Performance, 4=UltraPerformance.");

	RenderViewportExtent ScaleExtent(RenderViewportExtent outputExtent, std::uint32_t numerator, std::uint32_t denominator) noexcept
	{
		if (!outputExtent.IsValid() || denominator == 0u)
		{
			return outputExtent;
		}

		return RenderViewportExtent{
		    (std::max)(1u, (outputExtent.Width * numerator + denominator - 1u) / denominator),
		    (std::max)(1u, (outputExtent.Height * numerator + denominator - 1u) / denominator)};
	}
}

UpscalerSettings BuildUpscalerSettingsFromCVars() noexcept
{
	return UpscalerSettings{
	    .RequestedProvider = CVarUpscalerProvider.Get(),
	    .QualityMode = CVarUpscalerQualityMode.Get()};
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

void SetUpscalerProviderCVar(EUpscalerProviderKind provider) noexcept
{
	CVarUpscalerProvider.Set(provider);
}

void SetUpscalerQualityModeCVar(EUpscalerQualityMode mode) noexcept
{
	CVarUpscalerQualityMode.Set(mode);
}

RenderViewportExtent ResolveUpscalerRenderExtent(RenderViewportExtent outputExtent, EUpscalerQualityMode qualityMode) noexcept
{
	switch (qualityMode)
	{
		case EUpscalerQualityMode::Quality:
			return ScaleExtent(outputExtent, 2u, 3u);
		case EUpscalerQualityMode::Balanced:
			return ScaleExtent(outputExtent, 58u, 100u);
		case EUpscalerQualityMode::Performance:
			return ScaleExtent(outputExtent, 1u, 2u);
		case EUpscalerQualityMode::UltraPerformance:
			return ScaleExtent(outputExtent, 1u, 3u);
		case EUpscalerQualityMode::NativeAA:
		default:
			return outputExtent;
	}
}
