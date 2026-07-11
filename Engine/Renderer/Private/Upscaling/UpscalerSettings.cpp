#include "../PCH.h"
#include "Upscaling/UpscalerSettings.h"

ConsoleVariable<EUpscalerProviderKind> CVarUpscalerProvider(
    "r.Upscaler.Provider",
    EUpscalerProviderKind::NvidiaDlss,
    "Renderer upscaler provider: 0=Linear, 1=NVIDIA DLSS.");
ConsoleVariable<EUpscalerQualityMode> CVarUpscalerQualityMode(
    "r.Upscaler.QualityMode",
    EUpscalerQualityMode::NativeAA,
    "Renderer upscaler quality mode: 0=NativeAA, 1=Quality, 2=Balanced, 3=Performance, 4=UltraPerformance.");

bool IsExternalUpscalerEnabled() noexcept
{
	return CVarUpscalerProvider.Get() != EUpscalerProviderKind::Linear;
}

std::uint32_t GetUpscalerProviderSelectionKey() noexcept
{
	return static_cast<std::uint32_t>(CVarUpscalerProvider.Get());
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
