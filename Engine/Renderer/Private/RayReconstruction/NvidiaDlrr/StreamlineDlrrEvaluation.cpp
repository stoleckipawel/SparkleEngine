#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlrr/StreamlineDlrrEvaluation.h"

bool HasDlrrNativeEvaluationContract(const RayReconstructionEvaluationDesc& evaluation) noexcept
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
#include "RayReconstruction/NvidiaDlrr/StreamlineDlrrResourceTags.h"
#include "Streamline/StreamlineRuntimeSupport.h"
#include "Streamline/StreamlineViewConstants.h"

#include <sl_dlss_d.h>

namespace
{
	sl::DLSSDOptions BuildRayReconstructionOptions(
	    const RayReconstructionInputContract& inputContract,
	    RenderViewportExtent outputExtent) noexcept
	{
		sl::DLSSDOptions options{};
		options.mode = sl::DLSSMode::eDLAA;
		options.outputWidth = outputExtent.Width;
		options.outputHeight = outputExtent.Height;
		options.colorBuffersHDR = sl::Boolean::eTrue;
		options.normalRoughnessMode = sl::DLSSDNormalRoughnessMode::eUnpacked;
		options.alphaUpscalingEnabled = sl::Boolean::eFalse;
		options.worldToCameraView = ToStreamlineMatrix(inputContract.Camera.ViewMTX);
		options.cameraViewToWorld = ToStreamlineMatrix(inputContract.Camera.InvViewMTX);
		options.dlaaPreset = sl::DLSSDPreset::ePresetD;
		options.qualityPreset = sl::DLSSDPreset::ePresetD;
		options.balancedPreset = sl::DLSSDPreset::ePresetD;
		options.performancePreset = sl::DLSSDPreset::ePresetD;
		options.ultraPerformancePreset = sl::DLSSDPreset::ePresetD;
		options.ultraQualityPreset = sl::DLSSDPreset::ePresetD;
		return options;
	}

	RayReconstructionEvaluationResult FailedDlrrEvaluation(
	    ERayReconstructionProviderFailureDomain failureDomain,
	    std::string reason)
	{
		return RayReconstructionEvaluationResult{
		    .ProducedOutput = false,
		    .FailureDomain = failureDomain,
		    .Reason = std::move(reason)};
	}
}

RayReconstructionEvaluationResult EvaluateStreamlineDlrrFrame(
    const RayReconstructionInputContract& inputContract,
    sl::ViewportHandle viewport,
    const RayReconstructionEvaluationDesc& evaluation)
{
	if (!HasDlrrNativeEvaluationContract(evaluation))
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

	sl::DLSSDOptions options = BuildRayReconstructionOptions(inputContract, evaluation.OutputExtent);
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
		    FormatStreamlineFailure("slSetConstants(DLRR)", result));
	}

	auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
	result = TagDlrrResourcesForFrame(*frameToken, viewport, evaluation);
	if (result != sl::Result::eOk)
	{
		return FailedDlrrEvaluation(
		    ERayReconstructionProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slSetTagForFrame(DLRR)", result));
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
		    FormatStreamlineFailure("slEvaluateFeature(DLRR)", result));
	}

	return RayReconstructionEvaluationResult{
	    .ProducedOutput = true,
	    .FailureDomain = ERayReconstructionProviderFailureDomain::None,
	    .Reason = "Streamline DLRR evaluated successfully."};
}
#endif
