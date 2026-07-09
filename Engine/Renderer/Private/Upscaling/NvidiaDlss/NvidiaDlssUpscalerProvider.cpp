#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h"

#include "Upscaling/NvidiaDlss/StreamlineDlssFeatureMatrix.h"
#include "Upscaling/UpscalerSettings.h"

#include <string>

namespace
{
	constexpr std::uint32_t kStableExtentFramesBeforeDlssEvaluation = 2;

	ERendererProviderCapabilityState MapDlssCapabilityState(const DlssCapabilityReport& dlss) noexcept
	{
		switch (dlss.RuntimeState)
		{
			case EDlssProviderRuntimeState::AvailableNotCreated:
				return ERendererProviderCapabilityState::Available;
			case EDlssProviderRuntimeState::Created:
			case EDlssProviderRuntimeState::Evaluating:
				return ERendererProviderCapabilityState::Enabled;
			case EDlssProviderRuntimeState::Failed:
				return ERendererProviderCapabilityState::RuntimeFailed;
			case EDlssProviderRuntimeState::Unavailable:
				if (!dlss.SdkRuntimeIntegrated || !dlss.SdkRuntimeAvailable)
				{
					return ERendererProviderCapabilityState::MissingDependency;
				}
				if (dlss.FeatureQuerySucceeded && !dlss.FeatureSupported)
				{
					return ERendererProviderCapabilityState::UnsupportedHardware;
				}
				return ERendererProviderCapabilityState::Unavailable;
			case EDlssProviderRuntimeState::NotSelected:
			default:
				return dlss.CanCreateFeature() ? ERendererProviderCapabilityState::Available : ERendererProviderCapabilityState::Unavailable;
		}
	}

	bool ExtentsEqual(RenderViewportExtent lhs, RenderViewportExtent rhs) noexcept
	{
		return lhs.Width == rhs.Width && lhs.Height == rhs.Height;
	}

	bool NativeAAExtentContractValid(const UpscalerInputContract& inputContract) noexcept
	{
		return inputContract.RenderExtent.Width == inputContract.OutputExtent.Width &&
		       inputContract.RenderExtent.Height == inputContract.OutputExtent.Height;
	}
}

UpscalerProviderCapabilities NvidiaDlssUpscalerProvider::QueryCapabilities(const RhiCapabilities& capabilities) const
{
	const DlssCapabilityReport dlss = DlssCapabilityReporter::Build(capabilities);
	const bool canCreate = dlss.CanCreateFeature();
	const RendererProviderUpscalerResourceContract resourceContract = BuildUpscalerProviderResourceContract(UpscalerInputContract{});
	return UpscalerProviderCapabilities{
	    .Kind = EUpscalerProviderKind::NvidiaDlss,
	    .Category = ERendererProviderCategory::Upscaler,
	    .CapabilityState = canCreate ? ERendererProviderCapabilityState::Available : MapDlssCapabilityState(dlss),
	    .FailureDomain = canCreate ? EUpscalerProviderFailureDomain::None : dlss.FailureDomain,
	    .CanInitialize = canCreate,
	    .CanEvaluate = canCreate,
	    .UsesExternalSdk = true,
	    .ProviderName = "NVIDIA DLSS",
	    .ResourceContract = resourceContract,
	    .Reason = dlss.UnavailableReason};
}

bool NvidiaDlssUpscalerProvider::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop,
    UpscalerPresentationBridge presentationBridge)
{
	m_qualityMode = CVarUpscalerQualityMode.Get();
	m_dlssCapabilities = DlssCapabilityReporter::Build(capabilities);
	MarkSelectedDlssFeature(m_dlssCapabilities.FeatureMatrix, GetDlssFeatureForQualityMode(m_qualityMode));
	if (!m_dlssCapabilities.CanCreateFeature())
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::Unavailable;
		return false;
	}

	m_runtime = CreateStreamlineDlssRuntime();
	if (m_runtime == nullptr)
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::Failed;
		m_dlssCapabilities.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
		m_dlssCapabilities.UnavailableReason = "DLSS runtime factory returned no runtime instance.";
		return false;
	}

	const bool initialized = m_runtime->Initialize(
	    StreamlineDlssRuntimeDesc{
	        .Capabilities = capabilities,
	        .NativeInterop = nativeInterop,
	        .PresentationBridge = presentationBridge,
	        .QualityMode = m_qualityMode});
	DlssCapabilityReporter::ApplyRuntimeDiagnostics(m_dlssCapabilities, m_runtime->GetDiagnostics());
	return initialized;
}

void NvidiaDlssUpscalerProvider::SetupFrame(const UpscalerInputContract& inputContract)
{
	UpscalerInputContract frameContract = inputContract;
	const EUpscalerQualityMode qualityMode = CVarUpscalerQualityMode.Get();
	if (qualityMode != m_qualityMode)
	{
		m_qualityMode = qualityMode;
		MarkSelectedDlssFeature(m_dlssCapabilities.FeatureMatrix, GetDlssFeatureForQualityMode(m_qualityMode));
		frameContract.ResetRequested = true;
		frameContract.HistoryInvalid = true;
		frameContract.ResetReason = "DLSS quality mode changed";
		if (m_runtime != nullptr)
		{
			m_runtime->SetQualityMode(m_qualityMode);
		}
	}

	m_lastInputContract = frameContract;
	if (ExtentsEqual(m_lastObservedRenderExtent, frameContract.RenderExtent) &&
	    ExtentsEqual(m_lastObservedOutputExtent, frameContract.OutputExtent))
	{
		++m_stableExtentFrameCount;
	}
	else
	{
		m_lastObservedRenderExtent = frameContract.RenderExtent;
		m_lastObservedOutputExtent = frameContract.OutputExtent;
		m_stableExtentFrameCount = 1;
	}
	m_extentReadyForEvaluation = m_stableExtentFrameCount >= kStableExtentFramesBeforeDlssEvaluation;

	if (m_qualityMode == EUpscalerQualityMode::NativeAA && !NativeAAExtentContractValid(frameContract))
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::Failed;
		m_dlssCapabilities.FailureDomain = EUpscalerProviderFailureDomain::InputContract;
		m_dlssCapabilities.UnavailableReason =
		    "DLSS NativeAA requires render extent to equal output extent; renderer copy will preserve final color.";
		MarkDlssFeatureFailed(
		    m_dlssCapabilities.FeatureMatrix,
		    EDlssFeatureKind::NativeAA,
		    m_dlssCapabilities.UnavailableReason);
		return;
	}

	if (m_runtime != nullptr)
	{
		m_runtime->SetupFrame(frameContract);
		DlssCapabilityReporter::ApplyRuntimeDiagnostics(m_dlssCapabilities, m_runtime->GetDiagnostics());
	}
}

UpscalerEvaluationResult NvidiaDlssUpscalerProvider::Evaluate(const UpscalerEvaluationDesc& evaluation)
{
	if (m_runtime == nullptr)
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::Failed;
		m_dlssCapabilities.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
		m_dlssCapabilities.UnavailableReason = "NVIDIA DLSS runtime was not created.";
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .FailureDomain = m_dlssCapabilities.FailureDomain,
		    .Reason = m_dlssCapabilities.UnavailableReason};
	}

	if (!m_extentReadyForEvaluation)
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::Created;
		m_dlssCapabilities.FailureDomain = EUpscalerProviderFailureDomain::None;
		m_dlssCapabilities.UnavailableReason =
		    "Waiting for stable render/output extent before first DLSS evaluation; renderer copy will preserve final color.";
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .FailureDomain = EUpscalerProviderFailureDomain::None,
		    .Reason = m_dlssCapabilities.UnavailableReason};
	}

	UpscalerEvaluationResult result = m_runtime->Evaluate(evaluation);
	DlssCapabilityReporter::ApplyRuntimeDiagnostics(m_dlssCapabilities, m_runtime->GetDiagnostics());
	return result;
}

void NvidiaDlssUpscalerProvider::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	m_lastObservedRenderExtent = renderExtent;
	m_lastObservedOutputExtent = outputExtent;
	m_stableExtentFrameCount = 0;
	m_extentReadyForEvaluation = false;
}

void NvidiaDlssUpscalerProvider::ResetHistory(std::string_view reason)
{
	if (m_runtime != nullptr)
	{
		m_runtime->ResetHistory(reason);
		DlssCapabilityReporter::ApplyRuntimeDiagnostics(m_dlssCapabilities, m_runtime->GetDiagnostics());
	}
}

void NvidiaDlssUpscalerProvider::Shutdown() noexcept
{
	if (m_runtime != nullptr)
	{
		m_runtime->Shutdown();
		m_runtime.reset();
	}
}

UpscalerProviderCapabilities NvidiaDlssUpscalerProvider::GetDiagnostics() const
{
	const bool canEvaluate =
	    m_dlssCapabilities.RuntimeState == EDlssProviderRuntimeState::Created ||
	    m_dlssCapabilities.RuntimeState == EDlssProviderRuntimeState::Evaluating;
	const RendererProviderUpscalerResourceContract resourceContract = BuildUpscalerProviderResourceContract(m_lastInputContract);
	return UpscalerProviderCapabilities{
	    .Kind = EUpscalerProviderKind::NvidiaDlss,
	    .Category = ERendererProviderCategory::Upscaler,
	    .CapabilityState = MapDlssCapabilityState(m_dlssCapabilities),
	    .FailureDomain = m_dlssCapabilities.FailureDomain,
	    .CanInitialize = m_dlssCapabilities.CanCreateFeature(),
	    .CanEvaluate = canEvaluate,
	    .UsesExternalSdk = true,
	    .ProviderName = "NVIDIA DLSS",
	    .ResourceContract = resourceContract,
	    .Reason = m_dlssCapabilities.UnavailableReason};
}
