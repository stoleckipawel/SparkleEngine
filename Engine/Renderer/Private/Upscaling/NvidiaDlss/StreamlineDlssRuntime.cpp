#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssEvaluation.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssRuntime.h"
#include "Upscaling/NvidiaDlss/StreamlineDlssFeatureMatrix.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineRuntimeSupport.h"

#include <sl.h>
#include <sl_dlss.h>
#endif

namespace
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	constexpr std::uint32_t kDlssViewportId = 1u;

	bool ExtentsMatch(RenderViewportExtent lhs, RenderViewportExtent rhs) noexcept
	{
		return lhs.Width == rhs.Width && lhs.Height == rhs.Height;
	}

	EUpscalerQualityMode ResolveFrameQualityMode(
	    EUpscalerQualityMode requestedMode,
	    RenderViewportExtent renderExtent,
	    RenderViewportExtent outputExtent) noexcept
	{
		if (requestedMode != EUpscalerQualityMode::NativeAA && ExtentsMatch(renderExtent, outputExtent))
		{
			return EUpscalerQualityMode::NativeAA;
		}
		return requestedMode;
	}

	class StreamlineDlssRuntime final : public IStreamlineDlssRuntime
	{
	  public:
		bool Initialize(const StreamlineDlssRuntimeDesc& desc) override
		{
			m_qualityMode = desc.QualityMode;
			m_diagnostics.SdkVersion = kStreamlineDlssSdkVersion;
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(desc.QualityMode);

			const StreamlineBackendContract backend = ValidateStreamlineBackend(desc.Capabilities, desc.NativeInterop);
			if (!backend.Valid)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Backend;
				m_diagnostics.FailureReason = backend.FailureReason;
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			const SharedStreamlineRuntimeSession streamlineSession = AcquireSharedStreamlineRuntime(backend, desc.NativeInterop);
			if (!streamlineSession.Succeeded)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = streamlineSession.FailureReason;
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}
			m_streamlineSessionAcquired = true;

			if (backend.UsesD3D12 && desc.PresentationBridge &&
			    !desc.PresentationBridge.UpgradePresentationInterface(
			        &UpgradePresentationInterfaceWithStreamline,
			        nullptr,
			        desc.PresentationBridge.UserData))
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = "Streamline failed to upgrade the RHI presentation interface for manual present hooks.";
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			const StreamlineAdapterInfo adapterInfo = BuildStreamlineAdapterInfo(backend, desc.Capabilities, desc.NativeInterop);
			sl::Result result = slIsFeatureSupported(sl::kFeatureDLSS, adapterInfo.Info);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Feature;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slIsFeatureSupported(DLSS)", result);
				m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(false, m_diagnostics.FailureReason);
				return false;
			}

			m_viewport = sl::ViewportHandle{kDlssViewportId};
			m_diagnostics.State = EDlssProviderRuntimeState::Created;
			m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::None;
			m_diagnostics.FeatureMatrix = CreateStreamlineDlssFeatureMatrix(true, "DLSS Super Resolution is supported by Streamline.");
			m_initialized = true;
			return true;
		}

		void SetQualityMode(EUpscalerQualityMode qualityMode) override
		{
			if (m_qualityMode == qualityMode)
			{
				return;
			}

			m_qualityMode = qualityMode;
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(qualityMode);
			ResetHistory("DLSS quality mode changed");
		}

		bool SetupFrame(const UpscalerInputContract& inputContract) override
		{
			m_lastFrameContract = inputContract;
			m_diagnostics.RenderExtent = inputContract.RenderExtent;
			m_diagnostics.OutputExtent = inputContract.OutputExtent;
			m_diagnostics.ResetRequested = inputContract.ResetRequested;
			m_diagnostics.ResetReason = inputContract.ResetReason;
			return m_initialized;
		}

		UpscalerEvaluationResult Evaluate(const UpscalerEvaluationDesc& evaluation) override
		{
			if (!m_initialized)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Failed;
				m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
				return {
				    .ProducedOutput = false,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = "Streamline DLSS runtime is not initialized."};
			}

			const EUpscalerQualityMode frameQualityMode =
			    ResolveFrameQualityMode(m_qualityMode, evaluation.RenderExtent, evaluation.OutputExtent);
			m_diagnostics.SelectedQualityMode = frameQualityMode == m_qualityMode ?
			                                        UpscalerQualityModeToString(m_qualityMode) :
			                                        "NativeAA (render extent equals output extent)";

			UpscalerEvaluationResult result = EvaluateStreamlineDlssFrame(m_lastFrameContract, frameQualityMode, m_viewport, evaluation);
			if (!result.ProducedOutput)
			{
				m_diagnostics.State = EDlssProviderRuntimeState::Failed;
				m_diagnostics.FailureDomain = result.FailureDomain;
				m_diagnostics.FailureReason = result.Reason;
				return result;
			}

			m_diagnostics.State = EDlssProviderRuntimeState::Evaluating;
			m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::None;
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
			if (m_streamlineSessionAcquired)
			{
				if (m_initialized)
				{
					(void) slFreeResources(sl::kFeatureDLSS, m_viewport);
				}
				ReleaseSharedStreamlineRuntime();
				m_streamlineSessionAcquired = false;
			}
			m_initialized = false;
			m_diagnostics.State = EDlssProviderRuntimeState::NotSelected;
		}

		const StreamlineDlssRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlssRuntimeDiagnostics m_diagnostics = {};
		UpscalerInputContract m_lastFrameContract = {};
		sl::ViewportHandle m_viewport{kDlssViewportId};
		EUpscalerQualityMode m_qualityMode = EUpscalerQualityMode::Quality;
		bool m_streamlineSessionAcquired = false;
		bool m_initialized = false;
	};
#endif

	class UnavailableStreamlineDlssRuntime final : public IStreamlineDlssRuntime
	{
	  public:
		bool Initialize(const StreamlineDlssRuntimeDesc& desc) override
		{
			m_diagnostics.State = EDlssProviderRuntimeState::Unavailable;
			m_diagnostics.SdkVersion = "not-integrated";
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(desc.QualityMode);
			m_diagnostics.FeatureMatrix = CreateUnavailableStreamlineDlssFeatureMatrix(kStreamlineDlssNotIntegratedReason);
			m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
			m_diagnostics.FailureReason = kStreamlineDlssNotIntegratedReason;
			return false;
		}

		void SetQualityMode(EUpscalerQualityMode qualityMode) override
		{
			m_diagnostics.SelectedQualityMode = UpscalerQualityModeToString(qualityMode);
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
			m_diagnostics.State = EDlssProviderRuntimeState::Failed;
			m_diagnostics.RenderExtent = evaluation.RenderExtent;
			m_diagnostics.OutputExtent = evaluation.OutputExtent;
			m_diagnostics.FailureDomain =
			    HasDlssNativeEvaluationContract(evaluation) ? EUpscalerProviderFailureDomain::Sdk :
			                                             EUpscalerProviderFailureDomain::InputContract;
			m_diagnostics.FailureReason =
			    HasDlssNativeEvaluationContract(evaluation)
			        ? kStreamlineDlssNotIntegratedReason
			        : "DLSS evaluation contract is missing a native command list or required native resources.";
			return UpscalerEvaluationResult{
			    .ProducedOutput = false,
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
			m_diagnostics.State = EDlssProviderRuntimeState::NotSelected;
		}

		const StreamlineDlssRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlssRuntimeDiagnostics m_diagnostics = {};
		UpscalerInputContract m_lastFrameContract = {};
	};
}

std::unique_ptr<IStreamlineDlssRuntime> CreateStreamlineDlssRuntime()
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	return std::make_unique<StreamlineDlssRuntime>();
#else
	return std::make_unique<UnavailableStreamlineDlssRuntime>();
#endif
}
