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

	class UnavailableStreamlineDlssRuntime final : public IStreamlineDlssRuntime
	{
	  public:
		bool Initialize(const StreamlineDlssRuntimeDesc& desc) override
		{
			m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
			m_diagnostics.SdkVersion = "not-integrated";
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(desc.QualityMode);
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

StreamlineDlssRuntimeCapabilities QueryStreamlineDlssRuntimeCapabilities(const RhiCapabilities&) noexcept
{
	return StreamlineDlssRuntimeCapabilities{
	    .RuntimeIntegrated = false,
	    .RuntimeAvailable = false,
	    .FeatureQuerySucceeded = false,
	    .FeatureSupported = false,
	    .SdkVersion = "not-integrated",
	    .Reason = kRuntimeNotIntegratedReason};
}

std::unique_ptr<IStreamlineDlssRuntime> CreateStreamlineDlssRuntime()
{
	return std::make_unique<UnavailableStreamlineDlssRuntime>();
}
