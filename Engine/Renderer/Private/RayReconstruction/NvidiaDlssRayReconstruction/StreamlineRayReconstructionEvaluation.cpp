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

namespace
{
	bool HasRequiredNativeResources(const RayReconstructionEvaluationDesc& evaluation) noexcept
	{
		return evaluation.NativeCommandList && AreStreamlineTextureViewsValid(
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
		                                           evaluation.NativeExposureView);
	}

	sl::DLSSDOptions BuildRayReconstructionOptions(
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

}

RenderViewportExtent QueryStreamlineRayReconstructionOptimalRenderExtent(
    RenderViewportExtent outputExtent,
    EUpscalerQualityMode qualityMode) noexcept
{
	sl::DLSSDOptimalSettings settings{};
	const sl::DLSSDOptions options = BuildRayReconstructionOptions(qualityMode, outputExtent);
	if (slDLSSDGetOptimalSettings(options, settings) != sl::Result::eOk)
	{
		return {};
	}
	return RenderViewportExtent{settings.optimalRenderWidth, settings.optimalRenderHeight};
}

bool EvaluateStreamlineRayReconstructionFrame(
    const ImageProviderFrameContext& frameContext,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation)
{
	if (!HasRequiredNativeResources(evaluation))
	{
		return false;
	}

	StreamlineFrameEvaluation frameEvaluation(viewport, evaluation.NativeCommandList);
	if (!frameEvaluation.AcquireFrameToken(frameContext.FrameId))
	{
		return false;
	}

	const sl::DLSSOptions dlssOptions = BuildStreamlineDlssOptions(qualityMode, evaluation.OutputExtent);
	if (slDLSSSetOptions(viewport, dlssOptions) != sl::Result::eOk)
	{
		return false;
	}

	sl::DLSSDOptions options = BuildRayReconstructionOptions(qualityMode, evaluation.OutputExtent);
	options.worldToCameraView = ToStreamlineMatrix(frameContext.Camera.ViewMTX);
	options.cameraViewToWorld = ToStreamlineMatrix(frameContext.Camera.InvViewMTX);
	if (slDLSSDSetOptions(viewport, options) != sl::Result::eOk || !frameEvaluation.SetViewConstants(frameContext))
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
