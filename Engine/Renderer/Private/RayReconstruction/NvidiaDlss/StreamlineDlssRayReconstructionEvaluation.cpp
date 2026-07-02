#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlss/StreamlineDlssRayReconstructionEvaluation.h"

bool HasDlssRayReconstructionNativeEvaluationContract(const RayReconstructionEvaluationDesc& evaluation) noexcept
{
	const bool hasNativeResources =
	    evaluation.NativeCommandList && evaluation.NativeNoisyInputColor && evaluation.NativeOutputColor && evaluation.NativeDepth &&
	    evaluation.NativeMotionVectors && evaluation.NativeNormals && evaluation.NativeRoughness && evaluation.NativeDiffuseAlbedo &&
	    evaluation.NativeSpecularAlbedo && evaluation.NativeSpecularHitDistance && evaluation.NativeExposure;
	if (evaluation.BackendApi != ERhiBackendApi::Vulkan)
	{
		return hasNativeResources;
	}

	return hasNativeResources && evaluation.NativeNoisyInputColorView && evaluation.NativeOutputColorView && evaluation.NativeDepthView &&
	       evaluation.NativeMotionVectorsView && evaluation.NativeNormalsView && evaluation.NativeRoughnessView &&
	       evaluation.NativeDiffuseAlbedoView && evaluation.NativeSpecularAlbedoView && evaluation.NativeSpecularHitDistanceView &&
	       evaluation.NativeExposureView;
}

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "RayReconstruction/NvidiaDlss/StreamlineDlssRayReconstructionResourceTags.h"
#include "Streamline/StreamlineRuntimeSupport.h"
#include "Streamline/StreamlineViewConstants.h"

#include <sl_dlss_d.h>

namespace
{
	sl::DLSSMode ToStreamlineDlssMode(EngineRayReconstructionQualityMode mode) noexcept
	{
		switch (mode)
		{
			case EngineRayReconstructionQualityMode::Balanced:
				return sl::DLSSMode::eBalanced;
			case EngineRayReconstructionQualityMode::Performance:
				return sl::DLSSMode::eMaxPerformance;
			case EngineRayReconstructionQualityMode::Quality:
			default:
				return sl::DLSSMode::eMaxQuality;
		}
	}

	sl::DLSSDOptions BuildRayReconstructionOptions(
	    const RayReconstructionInputContract& inputContract,
	    EngineRayReconstructionQualityMode qualityMode,
	    RenderViewportExtent outputExtent) noexcept
	{
		sl::DLSSDOptions options{};
		options.mode = ToStreamlineDlssMode(qualityMode);
		options.outputWidth = outputExtent.Width;
		options.outputHeight = outputExtent.Height;
		options.colorBuffersHDR = sl::Boolean::eTrue;
		options.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::eUnpacked;
		options.alphaUpscalingEnabled = sl::Boolean::eFalse;
		options.worldToCameraView = ToStreamlineMatrix(inputContract.Camera.ViewMTX);
		options.cameraViewToWorld = ToStreamlineMatrix(inputContract.Camera.InvViewMTX);
		options.qualityPreset = sl::DLSSDPreset::ePresetD;
		options.balancedPreset = sl::DLSSDPreset::ePresetD;
		options.performancePreset = sl::DLSSDPreset::ePresetD;
		return options;
	}

	RayReconstructionEvaluationResult FailedDlrrEvaluation(
	    ERayReconstructionProviderFailureDomain failureDomain,
	    std::string reason)
	{
		return RayReconstructionEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .FailureDomain = failureDomain,
		    .Reason = std::move(reason)};
	}
}

RayReconstructionEvaluationResult EvaluateStreamlineDlssRayReconstructionFrame(
    const RayReconstructionInputContract& inputContract,
    EngineRayReconstructionQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation)
{
	if (!HasDlssRayReconstructionNativeEvaluationContract(evaluation))
	{
		return FailedDlrrEvaluation(
		    ERayReconstructionProviderFailureDomain::InputContract,
		    "DLRR evaluation contract is missing a native command list or required native resources.");
	}

	const std::uint32_t frameIndex = static_cast<std::uint32_t>(inputContract.FrameIndex);
	sl::FrameToken* frameToken = nullptr;
	sl::Result result = slGetNewFrameToken(frameToken, &frameIndex);
	if (result != sl::Result::eOk || frameToken == nullptr)
	{
		return FailedDlrrEvaluation(
		    ERayReconstructionProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slGetNewFrameToken", result));
	}

	sl::DLSSDOptions options = BuildRayReconstructionOptions(inputContract, qualityMode, evaluation.OutputExtent);
	result = slDLSSDSetOptions(viewport, options);
	if (result != sl::Result::eOk)
	{
		return FailedDlrrEvaluation(
		    ERayReconstructionProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slDLSSDSetOptions", result));
	}

	sl::Constants constants{};
	FillStreamlineViewConstants(
	    constants,
	    StreamlineViewConstantsInput{
	        .Camera = inputContract.Camera,
	        .TemporalData = inputContract.TemporalData,
	        .TemporalState = inputContract.TemporalState,
	        .RenderExtent = inputContract.RenderExtent,
	        .MotionVectorsCurrentMinusPrevious =
	            inputContract.MotionVectorConvention.Direction == ERayReconstructionMotionVectorDirection::CurrentMinusPrevious,
	        .ReversedDeviceDepth = inputContract.DepthConvention == ERayReconstructionDepthConvention::ReversedDeviceDepth,
	        .ResetRequested = inputContract.ResetRequested});
	result = slSetConstants(constants, *frameToken, viewport);
	if (result != sl::Result::eOk)
	{
		return FailedDlrrEvaluation(
		    ERayReconstructionProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slSetConstants(DLSS_RR)", result));
	}

	auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
	result = TagDlssRayReconstructionResourcesForFrame(*frameToken, viewport, evaluation);
	if (result != sl::Result::eOk)
	{
		return FailedDlrrEvaluation(
		    ERayReconstructionProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slSetTagForFrame(DLSS_RR)", result));
	}

	const sl::BaseStructure* inputs[] = {&viewport};
	result = slEvaluateFeature(sl::kFeatureDLSS_RR, *frameToken, inputs, static_cast<std::uint32_t>(std::size(inputs)), commandBuffer);
	if (evaluation.ResetCommandState != nullptr)
	{
		evaluation.ResetCommandState(evaluation.ResetCommandStateUserData);
	}
	if (result != sl::Result::eOk)
	{
		return FailedDlrrEvaluation(
		    ERayReconstructionProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slEvaluateFeature(DLSS_RR)", result));
	}

	return RayReconstructionEvaluationResult{
	    .ProducedOutput = true,
	    .UsedFallback = false,
	    .FailureDomain = ERayReconstructionProviderFailureDomain::None,
	    .Reason = "Streamline DLRR evaluated successfully."};
}
#endif
