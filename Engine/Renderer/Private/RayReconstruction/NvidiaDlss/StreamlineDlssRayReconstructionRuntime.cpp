#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlss/StreamlineDlssRayReconstructionEvaluation.h"
#include "RayReconstruction/NvidiaDlss/StreamlineDlssRayReconstructionRuntime.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineRuntimeSupport.h"

#include <sl.h>
#include <sl_dlss_d.h>
#endif

namespace
{
	constexpr const char* kStreamlineRayReconstructionSdkVersion = "Streamline DLSS Ray Reconstruction SDK";
	constexpr const char* kStreamlineRayReconstructionNotIntegratedReason =
	    "NVIDIA Streamline is not enabled in this build.";

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	class StreamlineDlssRayReconstructionRuntime final : public IStreamlineDlssRayReconstructionRuntime
	{
	  public:
		bool Initialize(const StreamlineDlssRayReconstructionRuntimeDesc& desc) override
		{
			m_qualityMode = desc.QualityMode;
			m_diagnostics.SdkVersion = kStreamlineRayReconstructionSdkVersion;
			m_diagnostics.SelectedQualityMode = RayReconstructionQualityModeToString(desc.QualityMode);

			const StreamlineBackendContract backend = ValidateStreamlineBackend(desc.Capabilities, desc.NativeInterop);
			if (!backend.Valid)
			{
				m_diagnostics.State = EDlssRayReconstructionRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Backend;
				m_diagnostics.FailureReason = backend.FailureReason;
				return false;
			}

			const sl::Feature features[] = {sl::kFeatureDLSS_RR};
			sl::Preferences preferences{};
			FillStreamlinePreferences(
			    preferences,
			    features,
			    static_cast<std::uint32_t>(std::size(features)),
			    backend.UsesVulkan ? sl::RenderAPI::eVulkan : sl::RenderAPI::eD3D12);

			sl::Result result = slInit(preferences, sl::kSDKVersion);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssRayReconstructionRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slInit(DLSS_RR)", result);
				return false;
			}
			m_initialized = true;

			if (backend.UsesD3D12 && desc.PresentationBridge &&
			    !desc.PresentationBridge.UpgradePresentationInterface(
			        &UpgradePresentationInterfaceWithStreamline,
			        nullptr,
			        desc.PresentationBridge.UserData))
			{
				m_diagnostics.State = EDlssRayReconstructionRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = "Streamline failed to upgrade the RHI presentation interface for manual present hooks.";
				return false;
			}

			result = SetStreamlineNativeDevice(backend, desc.NativeInterop);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssRayReconstructionRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = FormatStreamlineFailure("Streamline native device setup", result);
				return false;
			}

			const StreamlineAdapterInfo adapterInfo = BuildStreamlineAdapterInfo(backend, desc.Capabilities, desc.NativeInterop);
			result = slIsFeatureSupported(sl::kFeatureDLSS_RR, adapterInfo.Info);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssRayReconstructionRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Feature;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slIsFeatureSupported(DLSS_RR)", result);
				return false;
			}

			m_viewport = sl::ViewportHandle{0u};
			m_diagnostics.State = EDlssRayReconstructionRuntimeState::Created;
			m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::None;
			m_diagnostics.FailureReason.clear();
			return true;
		}

		bool SetupFrame(const RayReconstructionInputContract& inputContract) override
		{
			m_lastFrameContract = inputContract;
			m_diagnostics.RenderExtent = inputContract.RenderExtent;
			m_diagnostics.OutputExtent = inputContract.OutputExtent;
			m_diagnostics.ResetRequested = inputContract.ResetRequested;
			m_diagnostics.ResetReason = inputContract.ResetReason;
			return m_initialized;
		}

		RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) override
		{
			if (!m_initialized)
			{
				m_diagnostics.State = EDlssRayReconstructionRuntimeState::FailedWithFallback;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
				return {
				    .ProducedOutput = false,
				    .UsedFallback = true,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = "Streamline DLRR runtime is not initialized."};
			}
			RayReconstructionEvaluationResult result =
			    EvaluateStreamlineDlssRayReconstructionFrame(m_lastFrameContract, m_qualityMode, m_viewport, evaluation);
			if (!result.ProducedOutput)
			{
				m_diagnostics.State = EDlssRayReconstructionRuntimeState::FailedWithFallback;
				m_diagnostics.FailureDomain = result.FailureDomain;
				m_diagnostics.FailureReason = result.Reason;
				return result;
			}

			m_diagnostics.State = EDlssRayReconstructionRuntimeState::Evaluating;
			m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::None;
			m_diagnostics.FailureReason.clear();
			return result;
		}

		void ResetHistory(std::string_view reason) override
		{
			m_diagnostics.ResetRequested = true;
			m_diagnostics.ResetReason = std::string(reason);
		}

		void Shutdown() noexcept override
		{
			if (m_initialized)
			{
				(void) slShutdown();
				m_initialized = false;
			}
			m_diagnostics.State = EDlssRayReconstructionRuntimeState::NotSelected;
		}

		const StreamlineDlssRayReconstructionRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlssRayReconstructionRuntimeDiagnostics m_diagnostics = {};
		RayReconstructionInputContract m_lastFrameContract = {};
		sl::ViewportHandle m_viewport{0u};
		EngineRayReconstructionQualityMode m_qualityMode = EngineRayReconstructionQualityMode::Quality;
		bool m_initialized = false;
	};
#endif

	class UnavailableStreamlineDlssRayReconstructionRuntime final : public IStreamlineDlssRayReconstructionRuntime
	{
	  public:
		bool Initialize(const StreamlineDlssRayReconstructionRuntimeDesc& desc) override
		{
			m_diagnostics.State = EDlssRayReconstructionRuntimeState::Unavailable;
			m_diagnostics.SdkVersion = "not-integrated";
			m_diagnostics.SelectedQualityMode = RayReconstructionQualityModeToString(desc.QualityMode);
			m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
			m_diagnostics.FailureReason = kStreamlineRayReconstructionNotIntegratedReason;
			return false;
		}

		bool SetupFrame(const RayReconstructionInputContract& inputContract) override
		{
			m_diagnostics.RenderExtent = inputContract.RenderExtent;
			m_diagnostics.OutputExtent = inputContract.OutputExtent;
			m_diagnostics.ResetRequested = inputContract.ResetRequested;
			m_diagnostics.ResetReason = inputContract.ResetReason;
			return false;
		}

		RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) override
		{
			m_diagnostics.State = EDlssRayReconstructionRuntimeState::FailedWithFallback;
			m_diagnostics.RenderExtent = evaluation.RenderExtent;
			m_diagnostics.OutputExtent = evaluation.OutputExtent;
			m_diagnostics.FailureDomain =
			    HasDlssRayReconstructionNativeEvaluationContract(evaluation) ? ERayReconstructionProviderFailureDomain::Sdk :
			                                                               ERayReconstructionProviderFailureDomain::InputContract;
			m_diagnostics.FailureReason =
			    HasDlssRayReconstructionNativeEvaluationContract(evaluation)
			        ? kStreamlineRayReconstructionNotIntegratedReason
			        : "DLRR evaluation contract is missing a native command list or required native resources.";
			return RayReconstructionEvaluationResult{
			    .ProducedOutput = false,
			    .UsedFallback = true,
			    .FailureDomain = m_diagnostics.FailureDomain,
			    .Reason = m_diagnostics.FailureReason};
		}

		void ResetHistory(std::string_view reason) override
		{
			m_diagnostics.ResetRequested = true;
			m_diagnostics.ResetReason = std::string(reason);
		}

		void Shutdown() noexcept override
		{
			m_diagnostics.State = EDlssRayReconstructionRuntimeState::NotSelected;
		}

		const StreamlineDlssRayReconstructionRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlssRayReconstructionRuntimeDiagnostics m_diagnostics = {};
	};
}

const char* DlssRayReconstructionRuntimeStateToString(EDlssRayReconstructionRuntimeState state) noexcept
{
	switch (state)
	{
		case EDlssRayReconstructionRuntimeState::Unavailable:
			return "unavailable";
		case EDlssRayReconstructionRuntimeState::Created:
			return "created";
		case EDlssRayReconstructionRuntimeState::Evaluating:
			return "evaluating";
		case EDlssRayReconstructionRuntimeState::FailedWithFallback:
			return "failed with fallback";
		case EDlssRayReconstructionRuntimeState::NotSelected:
		default:
			return "not selected";
	}
}

std::unique_ptr<IStreamlineDlssRayReconstructionRuntime> CreateStreamlineDlssRayReconstructionRuntime()
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	return std::make_unique<StreamlineDlssRayReconstructionRuntime>();
#else
	return std::make_unique<UnavailableStreamlineDlssRayReconstructionRuntime>();
#endif
}
