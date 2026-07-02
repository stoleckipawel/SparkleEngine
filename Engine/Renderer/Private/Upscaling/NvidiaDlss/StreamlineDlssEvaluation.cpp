#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssEvaluation.h"

bool HasDlssNativeEvaluationContract(const UpscalerEvaluationDesc& evaluation) noexcept
{
	const bool hasNativeResources =
	    evaluation.NativeCommandList && evaluation.NativeScalingInputColor && evaluation.NativeDepth && evaluation.NativeMotionVectors &&
	    evaluation.NativeScalingOutputColor;
	if (evaluation.BackendApi != ERhiBackendApi::Vulkan)
	{
		return hasNativeResources;
	}

	return hasNativeResources && evaluation.NativeScalingInputColorView && evaluation.NativeDepthView &&
	       evaluation.NativeMotionVectorsView && evaluation.NativeScalingOutputColorView;
}

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineRuntimeSupport.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssConstants.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssResourceTags.h"

#include <sl_dlss.h>

namespace
{
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

	sl::DLSSOptions BuildDlssOptions(EUpscalerQualityMode qualityMode, RenderViewportExtent outputExtent) noexcept
	{
		sl::DLSSOptions options{};
		options.mode = ToStreamlineDlssMode(qualityMode);
		options.outputWidth = outputExtent.Width;
		options.outputHeight = outputExtent.Height;
		options.colorBuffersHDR = sl::Boolean::eTrue;
		options.useAutoExposure = sl::Boolean::eTrue;
		options.alphaUpscalingEnabled = sl::Boolean::eFalse;
		options.dlaaPreset = sl::DLSSPreset::ePresetK;
		options.qualityPreset = sl::DLSSPreset::ePresetK;
		options.balancedPreset = sl::DLSSPreset::ePresetK;
		options.performancePreset = sl::DLSSPreset::ePresetM;
		options.ultraPerformancePreset = sl::DLSSPreset::ePresetL;
		return options;
	}

	UpscalerEvaluationResult FailedDlssEvaluation(EUpscalerProviderFailureDomain failureDomain, std::string reason)
	{
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .FailureDomain = failureDomain,
		    .Reason = std::move(reason)};
	}
}

UpscalerEvaluationResult EvaluateStreamlineDlssFrame(
    const UpscalerInputContract& inputContract,
    EUpscalerQualityMode qualityMode,
    sl::ViewportHandle viewport,
    const UpscalerEvaluationDesc& evaluation)
{
	if (!HasDlssNativeEvaluationContract(evaluation))
	{
		return FailedDlssEvaluation(
		    EUpscalerProviderFailureDomain::InputContract,
		    "DLSS evaluation contract is missing a native command list or required native resources.");
	}

	const std::uint32_t frameIndex = static_cast<std::uint32_t>(inputContract.FrameIndex);
	sl::FrameToken* frameToken = nullptr;
	sl::Result result = slGetNewFrameToken(frameToken, &frameIndex);
	if (result != sl::Result::eOk || frameToken == nullptr)
	{
		return FailedDlssEvaluation(
		    EUpscalerProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slGetNewFrameToken", result));
	}

	sl::DLSSOptions options = BuildDlssOptions(qualityMode, evaluation.OutputExtent);
	result = slDLSSSetOptions(viewport, options);
	if (result != sl::Result::eOk)
	{
		return FailedDlssEvaluation(
		    EUpscalerProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slDLSSSetOptions", result));
	}

	sl::Constants constants{};
	FillStreamlineConstants(constants, inputContract);
	result = slSetConstants(constants, *frameToken, viewport);
	if (result != sl::Result::eOk)
	{
		return FailedDlssEvaluation(
		    EUpscalerProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slSetConstants", result));
	}

	auto* commandBuffer = static_cast<sl::CommandBuffer*>(evaluation.NativeCommandList.Value);
	result = TagDlssResourcesForFrame(*frameToken, viewport, evaluation);
	if (result != sl::Result::eOk)
	{
		return FailedDlssEvaluation(
		    EUpscalerProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slSetTagForFrame(DLSS)", result));
	}

	const sl::BaseStructure* inputs[] = {&viewport};
	result = slEvaluateFeature(sl::kFeatureDLSS, *frameToken, inputs, static_cast<std::uint32_t>(std::size(inputs)), commandBuffer);
	if (result != sl::Result::eOk)
	{
		return FailedDlssEvaluation(
		    EUpscalerProviderFailureDomain::Sdk,
		    FormatStreamlineFailure("slEvaluateFeature(DLSS)", result));
	}

	return UpscalerEvaluationResult{
	    .ProducedOutput = true,
	    .UsedFallback = false,
	    .FailureDomain = EUpscalerProviderFailureDomain::None,
	    .Reason = "Streamline DLSS evaluated successfully."};
}
#endif
