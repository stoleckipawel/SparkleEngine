#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssEvaluation.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineDlssOptions.h"
#include "Streamline/StreamlineFrameEvaluation.h"
#include "Streamline/StreamlineResourceInterop.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssResourceTags.h"

#include <sl_dlss.h>

class StreamlineDlssRequirements final
{
  public:
	static bool HasRequiredNativeResources(const UpscalerEvaluationDesc& evaluation) noexcept
	{
		return evaluation.NativeCommandList && AreStreamlineTextureViewsValid(
		                                           evaluation.BackendApi,
		                                           evaluation.NativeScalingInputColorView,
		                                           evaluation.NativeDepthView,
		                                           evaluation.NativeMotionVectorsView,
		                                           evaluation.NativeExposureView,
		                                           evaluation.NativeScalingOutputColorView);
	}
};

RenderViewportExtent QueryStreamlineDlssOptimalRenderExtent(
    RenderViewportExtent outputExtent,
    EUpscalerQualityMode qualityMode) noexcept
{
	sl::DLSSOptimalSettings settings{};
	const sl::DLSSOptions options = BuildStreamlineDlssOptions(qualityMode, outputExtent);
	if (slDLSSGetOptimalSettings(options, settings) != sl::Result::eOk)
	{
		return {};
	}
	return RenderViewportExtent{settings.optimalRenderWidth, settings.optimalRenderHeight};
}

bool EvaluateStreamlineDlssFrame(
    const ImageProviderFrameContext& frameContext,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation)
{
	if (!StreamlineDlssRequirements::HasRequiredNativeResources(evaluation))
	{
		return false;
	}

	StreamlineFrameEvaluation frameEvaluation(viewport, evaluation.NativeCommandList);
	if (!frameEvaluation.AcquireFrameToken(frameContext.FrameId))
	{
		return false;
	}

	sl::DLSSOptions options = BuildStreamlineDlssOptions(qualityMode, evaluation.OutputExtent);
	if (slDLSSSetOptions(viewport, options) != sl::Result::eOk || !frameEvaluation.SetViewConstants(frameContext))
	{
		return false;
	}

	if (TagDlssResourcesForFrame(frameEvaluation.GetFrameToken(), viewport, evaluation) != sl::Result::eOk)
	{
		return false;
	}

	return frameEvaluation.Evaluate(sl::kFeatureDLSS);
}
#endif
