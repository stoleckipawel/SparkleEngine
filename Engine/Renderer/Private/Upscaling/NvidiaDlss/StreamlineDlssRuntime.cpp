#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"

namespace
{
	constexpr const char* kRuntimeNotIntegratedReason =
	    "NVIDIA Streamline SDK is not integrated in this build; DLSS Super Resolution remains unavailable.";

	bool HasNativeEvaluationContract(const UpscalerEvaluationDesc& evaluation) noexcept
	{
		return evaluation.NativeCommandList && evaluation.NativeInputColor && evaluation.NativeDepth && evaluation.NativeMotionVectors &&
		       evaluation.NativeOutputColor;
	}

	DlssFeatureMatrix BuildUnavailableFeatureMatrix(std::string_view reason)
	{
		DlssFeatureMatrix matrix;
		matrix.Entries = {
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::SuperResolution,
		        .State = EDlssFeatureState::Unavailable,
		        .QualityModes = "NativeAA, Quality, Balanced, Performance, UltraPerformance",
		        .ModelPresetRecommendation = "SDK queried preset recommendation required.",
		        .RequiredResources = "HUD-less color, depth, motion vectors, exposure or auto-exposure, final output",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::NativeAA,
		        .State = EDlssFeatureState::Unavailable,
		        .QualityModes = "NativeAA",
		        .ModelPresetRecommendation = "Reuse Super Resolution preset recommendation at render extent equal to output extent.",
		        .RequiredResources = "Same as Super Resolution; render extent must equal output extent",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::RayReconstruction,
		        .State = EDlssFeatureState::Unavailable,
		        .QualityModes = "SDK queried modes required.",
		        .ModelPresetRecommendation = "SDK queried preset recommendation required.",
		        .RequiredResources = "Noisy indirect lighting signals plus guide buffers; not direct shadow visibility",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::FrameGeneration,
		        .State = EDlssFeatureState::Unavailable,
		        .RequiresLatencyHook = true,
		        .QualityModes = "SDK queried generated-frame modes required.",
		        .ModelPresetRecommendation = "SDK queried frame-generation model recommendation required.",
		        .RequiredResources = "Present contract, frame IDs, optical-flow inputs, UI separation, latency hooks",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::MultiFrameGeneration,
		        .State = EDlssFeatureState::Unavailable,
		        .RequiresLatencyHook = true,
		        .QualityModes = "SDK queried generated-frame multiplier modes required.",
		        .ModelPresetRecommendation = "SDK queried MFG model recommendation required.",
		        .RequiredResources = "Frame Generation contract plus SDK-reported MFG limits",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::DynamicMultiFrameGeneration,
		        .State = EDlssFeatureState::Unavailable,
		        .RequiresLatencyHook = true,
		        .QualityModes = "SDK queried dynamic generated-frame modes required.",
		        .ModelPresetRecommendation = "SDK queried dynamic MFG model recommendation required.",
		        .RequiredResources = "MFG contract plus runtime frame-pacing and scheduling policy",
		        .Reason = std::string(reason)},
		    DlssFeatureMatrixEntry{
		        .Feature = EDlssFeatureKind::LatencyHook,
		        .State = EDlssFeatureState::Unavailable,
		        .QualityModes = "Required only by selected generated-frame paths.",
		        .ModelPresetRecommendation = "Not applicable.",
		        .RequiredResources = "Frame markers and latency hook points required by selected SDK feature",
		        .Reason = std::string(reason)}};
		return matrix;
	}

	class UnavailableStreamlineDlssRuntime final : public IStreamlineDlssRuntime
	{
	  public:
		bool Initialize(const StreamlineDlssRuntimeDesc& desc) override
		{
			m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
			m_diagnostics.SdkVersion = "not-integrated";
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(desc.QualityMode);
			m_diagnostics.FeatureMatrix = BuildUnavailableFeatureMatrix(kRuntimeNotIntegratedReason);
			m_diagnostics.FailureReason = kRuntimeNotIntegratedReason;
			return false;
		}

		bool SetupFrame(const UpscalerInputContract& inputContract) override
		{
			m_diagnostics.RenderExtent = inputContract.RenderExtent;
			m_diagnostics.OutputExtent = inputContract.OutputExtent;
			m_diagnostics.ResetRequested = inputContract.ResetRequested;
			m_diagnostics.ResetReason = inputContract.ResetReason;
			m_lastFrameContract = inputContract;
			return false;
		}

		UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) override
		{
			m_diagnostics.State = EDlssProviderRuntimeState::FailedWithFallback;
			m_diagnostics.RenderExtent = evaluation.RenderExtent;
			m_diagnostics.OutputExtent = evaluation.OutputExtent;
			m_diagnostics.FailureReason =
			    HasNativeEvaluationContract(evaluation)
			        ? kRuntimeNotIntegratedReason
			        : "DLSS evaluation contract is missing a native command list or required native resources.";
			return UpscalerEvaluationResult{
			    .ProducedOutput = false,
			    .UsedFallback = true,
			    .Reason = m_diagnostics.FailureReason};
		}

		void ResetHistory(std::string_view reason) override
		{
			m_diagnostics.ResetRequested = true;
			m_diagnostics.ResetReason = std::string(reason);
		}

		void Shutdown() noexcept override
		{
			m_diagnostics.State = EDlssProviderRuntimeState::NotSelected;
		}

		const StreamlineDlssRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlssRuntimeDiagnostics m_diagnostics = {};
		UpscalerInputContract m_lastFrameContract = {};
	};
}

const char* DlssProviderRuntimeStateToString(EDlssProviderRuntimeState state) noexcept
{
	switch (state)
	{
		case EDlssProviderRuntimeState::NotSelected:
			return "NotSelected";
		case EDlssProviderRuntimeState::Unavailable:
			return "Unavailable";
		case EDlssProviderRuntimeState::AvailableNotCreated:
			return "AvailableNotCreated";
		case EDlssProviderRuntimeState::Created:
			return "Created";
		case EDlssProviderRuntimeState::Evaluating:
			return "Evaluating";
		case EDlssProviderRuntimeState::FailedWithFallback:
			return "FailedWithFallback";
	}

	return "Unknown";
}

const char* DlssFeatureKindToString(EDlssFeatureKind feature) noexcept
{
	switch (feature)
	{
		case EDlssFeatureKind::SuperResolution:
			return "SuperResolution";
		case EDlssFeatureKind::NativeAA:
			return "NativeAA";
		case EDlssFeatureKind::RayReconstruction:
			return "RayReconstruction";
		case EDlssFeatureKind::FrameGeneration:
			return "FrameGeneration";
		case EDlssFeatureKind::MultiFrameGeneration:
			return "MultiFrameGeneration";
		case EDlssFeatureKind::DynamicMultiFrameGeneration:
			return "DynamicMultiFrameGeneration";
		case EDlssFeatureKind::LatencyHook:
			return "LatencyHook";
	}

	return "Unknown";
}

const char* DlssFeatureStateToString(EDlssFeatureState state) noexcept
{
	switch (state)
	{
		case EDlssFeatureState::NotSelected:
			return "NotSelected";
		case EDlssFeatureState::Unavailable:
			return "Unavailable";
		case EDlssFeatureState::Available:
			return "Available";
		case EDlssFeatureState::Enabled:
			return "Enabled";
		case EDlssFeatureState::Active:
			return "Active";
		case EDlssFeatureState::FailedWithFallback:
			return "FailedWithFallback";
	}

	return "Unknown";
}

StreamlineDlssRuntimeCapabilities QueryStreamlineDlssRuntimeCapabilities(const RhiCapabilities&) noexcept
{
	return StreamlineDlssRuntimeCapabilities{
	    .RuntimeIntegrated = false,
	    .RuntimeAvailable = false,
	    .FeatureQuerySucceeded = false,
	    .FeatureSupported = false,
	    .FeatureMatrix = BuildUnavailableFeatureMatrix(kRuntimeNotIntegratedReason),
	    .SdkVersion = "not-integrated",
	    .Reason = kRuntimeNotIntegratedReason};
}

std::unique_ptr<IStreamlineDlssRuntime> CreateStreamlineDlssRuntime()
{
	return std::make_unique<UnavailableStreamlineDlssRuntime>();
}
