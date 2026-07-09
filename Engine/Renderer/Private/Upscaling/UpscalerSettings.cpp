#include "../PCH.h"
#include "Upscaling/UpscalerSettings.h"

#include <algorithm>

ConsoleVariable<EUpscalerProviderKind> CVarUpscalerProvider(
    "r.Upscaler.Provider",
    EUpscalerProviderKind::NvidiaDlss,
    "Renderer upscaler provider: 0=Linear, 1=NVIDIA DLSS.");
ConsoleVariable<EUpscalerQualityMode> CVarUpscalerQualityMode(
    "r.Upscaler.QualityMode",
    EUpscalerQualityMode::NativeAA,
    "Renderer upscaler quality mode: 0=NativeAA, 1=Quality, 2=Balanced, 3=Performance, 4=UltraPerformance.");

namespace
{
	struct UpscalerRenderScale final
	{
		std::uint32_t Numerator = 1u;
		std::uint32_t Denominator = 1u;
	};

	UpscalerRenderScale GetDlssRenderScale(EUpscalerQualityMode qualityMode) noexcept
	{
		switch (qualityMode)
		{
			case EUpscalerQualityMode::Quality:
				return {.Numerator = 2u, .Denominator = 3u}; // 66.7%, 1.5x upscale
			case EUpscalerQualityMode::Balanced:
				return {.Numerator = 58u, .Denominator = 100u}; // 58%, about 1.72x upscale
			case EUpscalerQualityMode::Performance:
				return {.Numerator = 1u, .Denominator = 2u}; // 50%, 2x upscale
			case EUpscalerQualityMode::UltraPerformance:
				return {.Numerator = 1u, .Denominator = 3u}; // 33.3%, 3x upscale
			case EUpscalerQualityMode::NativeAA:
			default:
				return {.Numerator = 1u, .Denominator = 1u}; // 100%, native/DLAA
		}
	}

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

RenderViewportExtent ResolveUpscalerRenderExtent(RenderViewportExtent outputExtent, EUpscalerQualityMode qualityMode) noexcept
{
	const UpscalerRenderScale renderScale = GetDlssRenderScale(qualityMode);
	return ScaleExtent(outputExtent, renderScale.Numerator, renderScale.Denominator);
}
