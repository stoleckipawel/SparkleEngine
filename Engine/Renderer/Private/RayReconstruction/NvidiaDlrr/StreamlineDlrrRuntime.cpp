#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlrr/StreamlineDlrrEvaluation.h"
#include "RayReconstruction/NvidiaDlrr/StreamlineDlrrRuntime.h"

#if SPARKLE_WITH_NVIDIA_STREAMLINE
#include "Streamline/StreamlineRuntimeSupport.h"

#include <sl.h>
#include <sl_dlss_d.h>
#endif

namespace
{
	constexpr const char* kStreamlineRayReconstructionNotIntegratedReason =
	    "NVIDIA Streamline is not enabled in this build.";

#if SPARKLE_WITH_NVIDIA_STREAMLINE
	constexpr std::uint32_t kDlrrViewportId = 2u;

	class StreamlineDlrrRuntime final : public IStreamlineDlrrRuntime
	{
	  public:
		bool Initialize(const StreamlineDlrrRuntimeDesc& desc) override
		{
			const StreamlineBackendContract backend = ValidateStreamlineBackend(desc.Capabilities, desc.NativeInterop);
			if (!backend.Valid)
			{
				m_diagnostics.State = EDlrrRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Backend;
				m_diagnostics.FailureReason = backend.FailureReason;
				return false;
			}

			const SharedStreamlineRuntimeSession streamlineSession = AcquireSharedStreamlineRuntime(backend, desc.NativeInterop);
			if (!streamlineSession.Succeeded)
			{
				m_diagnostics.State = EDlrrRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = streamlineSession.FailureReason;
				return false;
			}
			m_streamlineSessionAcquired = true;

			if (backend.UsesD3D12 && desc.PresentationBridge &&
			    !desc.PresentationBridge.UpgradePresentationInterface(
			        &UpgradePresentationInterfaceWithStreamline,
			        nullptr,
			        desc.PresentationBridge.UserData))
			{
				m_diagnostics.State = EDlrrRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
				m_diagnostics.FailureReason = "Streamline failed to upgrade the RHI presentation interface for manual present hooks.";
				return false;
			}

			const StreamlineAdapterInfo adapterInfo = BuildStreamlineAdapterInfo(backend, desc.Capabilities, desc.NativeInterop);
			sl::Result result = slIsFeatureSupported(sl::kFeatureDLSS_RR, adapterInfo.Info);
			if (result != sl::Result::eOk)
			{
				m_diagnostics.State = EDlrrRuntimeState::Unavailable;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Feature;
				m_diagnostics.FailureReason = FormatStreamlineFailure("slIsFeatureSupported(DLRR)", result);
				return false;
			}

			m_viewport = sl::ViewportHandle{kDlrrViewportId};
			m_diagnostics.State = EDlrrRuntimeState::Created;
			m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::None;
			m_diagnostics.FailureReason.clear();
			m_initialized = true;
			return true;
		}

		bool SetupFrame(const RayReconstructionInputContract& inputContract) override
		{
			m_lastFrameContract = inputContract;
			return m_initialized;
		}

		RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) override
		{
			if (!m_initialized)
			{
				m_diagnostics.State = EDlrrRuntimeState::Failed;
				m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
				return {
				    .ProducedOutput = false,
				    .FailureDomain = m_diagnostics.FailureDomain,
				    .Reason = "Streamline DLRR runtime is not initialized."};
			}

			RayReconstructionEvaluationResult result =
			    EvaluateStreamlineDlrrFrame(m_lastFrameContract, m_viewport, evaluation);
			if (!result.ProducedOutput)
			{
				m_diagnostics.State = EDlrrRuntimeState::Failed;
				m_diagnostics.FailureDomain = result.FailureDomain;
				m_diagnostics.FailureReason = result.Reason;
				return result;
			}

			m_diagnostics.State = EDlrrRuntimeState::Evaluating;
			m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::None;
			m_diagnostics.FailureReason.clear();
			return result;
		}

		void ResetHistory(std::string_view reason) override
		{
			(void) reason;
		}

		void Shutdown() noexcept override
		{
			if (m_streamlineSessionAcquired)
			{
				if (m_initialized)
				{
					(void) slFreeResources(sl::kFeatureDLSS_RR, m_viewport);
				}
				ReleaseSharedStreamlineRuntime();
				m_streamlineSessionAcquired = false;
			}
			m_initialized = false;
			m_diagnostics.State = EDlrrRuntimeState::NotSelected;
		}

		const StreamlineDlrrRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlrrRuntimeDiagnostics m_diagnostics = {};
		RayReconstructionInputContract m_lastFrameContract = {};
		sl::ViewportHandle m_viewport{kDlrrViewportId};
		bool m_streamlineSessionAcquired = false;
		bool m_initialized = false;
	};
#endif

	class UnavailableStreamlineDlrrRuntime final : public IStreamlineDlrrRuntime
	{
	  public:
		bool Initialize(const StreamlineDlrrRuntimeDesc& desc) override
		{
			(void) desc;
			m_diagnostics.State = EDlrrRuntimeState::Unavailable;
			m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
			m_diagnostics.FailureReason = kStreamlineRayReconstructionNotIntegratedReason;
			return false;
		}

		bool SetupFrame(const RayReconstructionInputContract& inputContract) override
		{
			(void) inputContract;
			return false;
		}

		RayReconstructionEvaluationResult Evaluate(const RayReconstructionEvaluationDesc& evaluation) override
		{
			m_diagnostics.State = EDlrrRuntimeState::Failed;
			m_diagnostics.FailureDomain =
			    HasDlrrNativeEvaluationContract(evaluation) ? ERayReconstructionProviderFailureDomain::Sdk :
			                                                               ERayReconstructionProviderFailureDomain::InputContract;
			m_diagnostics.FailureReason =
			    HasDlrrNativeEvaluationContract(evaluation)
			        ? kStreamlineRayReconstructionNotIntegratedReason
			        : "DLRR evaluation contract is missing a native command list or required native resources.";
			return RayReconstructionEvaluationResult{
			    .ProducedOutput = false,
			    .FailureDomain = m_diagnostics.FailureDomain,
			    .Reason = m_diagnostics.FailureReason};
		}

		void ResetHistory(std::string_view reason) override
		{
			(void) reason;
		}

		void Shutdown() noexcept override
		{
			m_diagnostics.State = EDlrrRuntimeState::NotSelected;
		}

		const StreamlineDlrrRuntimeDiagnostics& GetDiagnostics() const noexcept override { return m_diagnostics; }

	  private:
		StreamlineDlrrRuntimeDiagnostics m_diagnostics = {};
	};
}

std::unique_ptr<IStreamlineDlrrRuntime> CreateStreamlineDlrrRuntime()
{
#if SPARKLE_WITH_NVIDIA_STREAMLINE
	return std::make_unique<StreamlineDlrrRuntime>();
#else
	return std::make_unique<UnavailableStreamlineDlrrRuntime>();
#endif
}
