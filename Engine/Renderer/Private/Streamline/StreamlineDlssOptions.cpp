#include "../PCH.h"
#include "Streamline/StreamlineDlssOptions.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
sl::DLSSMode ToStreamlineDlssMode(EUpscalerQualityMode mode) noexcept
{
	switch (mode)
	{
		case EUpscalerQualityMode::NativeAA:
			return sl::DLSSMode::eDLAA;
		case EUpscalerQualityMode::Quality:
			return sl::DLSSMode::eMaxQuality;
		case EUpscalerQualityMode::Balanced:
			return sl::DLSSMode::eBalanced;
		case EUpscalerQualityMode::Performance:
			return sl::DLSSMode::eMaxPerformance;
		case EUpscalerQualityMode::UltraPerformance:
			return sl::DLSSMode::eUltraPerformance;
	}

	return sl::DLSSMode::eMaxQuality;
}

sl::DLSSOptions BuildStreamlineDlssOptions(
    EUpscalerQualityMode qualityMode,
    RenderViewportExtent outputExtent) noexcept
{
	sl::DLSSOptions options{};
	options.mode = ToStreamlineDlssMode(qualityMode);
	options.outputWidth = outputExtent.Width;
	options.outputHeight = outputExtent.Height;
	options.colorBuffersHDR = sl::Boolean::eTrue;
	options.useAutoExposure = sl::Boolean::eFalse;
	options.alphaUpscalingEnabled = sl::Boolean::eFalse;
	return options;
}
#endif
