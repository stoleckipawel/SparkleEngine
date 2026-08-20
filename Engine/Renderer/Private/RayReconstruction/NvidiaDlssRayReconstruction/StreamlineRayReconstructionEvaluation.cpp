#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlssRayReconstruction/StreamlineRayReconstructionEvaluation.h"
#include "Upscaling/UpscalerSettings.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
  #include "RayReconstruction/NvidiaDlssRayReconstruction/StreamlineRayReconstructionResourceTags.h"
  #include "Streamline/StreamlineDlssOptions.h"
  #include "Streamline/StreamlineFrameEvaluation.h"
  #include "Streamline/StreamlineResourceInterop.h"
  #include "Streamline/StreamlineViewConstants.h"

  #include <sl_dlss.h>
  #include <sl_dlss_d.h>

static const auto g_streamlineRayReconstructionEvaluationLogger = Logging::GetOrCreateLogger("Renderer.Streamline.RayReconstruction");

static sl::DLSSDOptions BuildStreamlineRayReconstructionOptions(
    EUpscalerQualityMode qualityMode,
    RenderViewportExtent outputExtent) noexcept
{
	sl::DLSSDOptions options{};
	options.mode = ToStreamlineDlssMode(qualityMode);
	options.outputWidth = outputExtent.Width;
	options.outputHeight = outputExtent.Height;
	options.colorBuffersHDR = sl::Boolean::eTrue;
	options.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::eUnpacked;
	options.alphaUpscalingEnabled = sl::Boolean::eFalse;
	options.dlaaPreset = sl::DLSSDPreset::ePresetD;
	options.qualityPreset = sl::DLSSDPreset::ePresetD;
	options.balancedPreset = sl::DLSSDPreset::ePresetD;
	options.performancePreset = sl::DLSSDPreset::ePresetD;
	options.ultraPerformancePreset = sl::DLSSDPreset::ePresetD;
	options.ultraQualityPreset = sl::DLSSDPreset::ePresetD;
	return options;
}

RenderViewportExtent QueryStreamlineRayReconstructionOptimalRenderExtent(
    RenderViewportExtent outputExtent,
    EUpscalerQualityMode qualityMode) noexcept
{
	sl::DLSSDOptimalSettings settings{};
	const sl::DLSSDOptions options = BuildStreamlineRayReconstructionOptions(qualityMode, outputExtent);
	if (slDLSSDGetOptimalSettings(options, settings) != sl::Result::eOk)
	{
		Diagnostics::Fatal(
		    g_streamlineRayReconstructionEvaluationLogger,
		    __FILE__,
		    __LINE__,
		    "Streamline DLSS Ray Reconstruction could not resolve its render extent.");
	}
	return RenderViewportExtent{settings.optimalRenderWidth, settings.optimalRenderHeight};
}

bool EvaluateStreamlineRayReconstructionFrame(
    const ImageProviderFrameInput& frameInput,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation)
{
	if (!evaluation.NativeCommandList
	    || !AreStreamlineTextureViewsValid(
	        evaluation.BackendApi,
	        evaluation.NativeNoisyInputColorView,
	        evaluation.NativeOutputColorView,
	        evaluation.NativeDepthView,
	        evaluation.NativeMotionVectorsView,
	        evaluation.NativeNormalsView,
	        evaluation.NativeRoughnessView,
	        evaluation.NativeDiffuseAlbedoView,
	        evaluation.NativeSpecularAlbedoView,
	        evaluation.NativeSpecularHitDistanceView,
	        evaluation.NativeExposureView))
	{
		return false;
	}

	StreamlineFrameEvaluation frameEvaluation(viewport, evaluation.NativeCommandList);
	if (!frameEvaluation.AcquireFrameToken(frameInput.FrameId))
	{
		return false;
	}

	const sl::DLSSOptions dlssOptions = BuildStreamlineDlssOptions(qualityMode, evaluation.OutputExtent);
	if (slDLSSSetOptions(viewport, dlssOptions) != sl::Result::eOk)
	{
		return false;
	}

	sl::DLSSDOptions options = BuildStreamlineRayReconstructionOptions(qualityMode, evaluation.OutputExtent);
	options.worldToCameraView = ToStreamlineMatrix(frameInput.Camera.ViewMTX);
	options.cameraViewToWorld = ToStreamlineMatrix(frameInput.Camera.InvViewMTX);
	if (slDLSSDSetOptions(viewport, options) != sl::Result::eOk || !frameEvaluation.SetViewConstants(frameInput))
	{
		return false;
	}

	if (TagRayReconstructionResourcesForFrame(frameEvaluation.GetFrameToken(), viewport, evaluation) != sl::Result::eOk)
	{
		return false;
	}

	return frameEvaluation.Evaluate(sl::kFeatureDLSS_RR);
}
#endif
