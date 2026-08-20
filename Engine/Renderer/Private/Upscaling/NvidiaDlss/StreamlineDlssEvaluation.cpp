#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssEvaluation.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
  #include "Streamline/StreamlineDlssOptions.h"
  #include "Streamline/StreamlineFrameEvaluation.h"
  #include "Streamline/StreamlineResourceInterop.h"
  #include "Upscaling/NvidiaDlss/StreamlineDlssResourceTags.h"

  #include <sl_dlss.h>

static const auto g_streamlineDlssEvaluationLogger = Logging::GetOrCreateLogger("Renderer.Streamline.DLSS");

RenderViewportExtent QueryStreamlineDlssOptimalRenderExtent(RenderViewportExtent outputExtent, EUpscalerQualityMode qualityMode) noexcept
{
	sl::DLSSOptimalSettings settings{};
	const sl::DLSSOptions options = BuildStreamlineDlssOptions(qualityMode, outputExtent);
	if (slDLSSGetOptimalSettings(options, settings) != sl::Result::eOk)
	{
		Diagnostics::Fatal(g_streamlineDlssEvaluationLogger, __FILE__, __LINE__, "Streamline DLSS could not resolve its render extent.");
	}
	return RenderViewportExtent{settings.optimalRenderWidth, settings.optimalRenderHeight};
}

bool EvaluateStreamlineDlssFrame(
    const ImageProviderFrameInput& frameInput,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation)
{
	if (!evaluation.NativeCommandList
	    || !AreStreamlineTextureViewsValid(
	        evaluation.BackendApi,
	        evaluation.NativeScalingInputColorView,
	        evaluation.NativeDepthView,
	        evaluation.NativeMotionVectorsView,
	        evaluation.NativeExposureView,
	        evaluation.NativeScalingOutputColorView))
	{
		return false;
	}

	StreamlineFrameEvaluation frameEvaluation(viewport, evaluation.NativeCommandList);
	if (!frameEvaluation.AcquireFrameToken(frameInput.FrameId))
	{
		return false;
	}

	sl::DLSSOptions options = BuildStreamlineDlssOptions(qualityMode, evaluation.OutputExtent);
	if (slDLSSSetOptions(viewport, options) != sl::Result::eOk || !frameEvaluation.SetViewConstants(frameInput))
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
