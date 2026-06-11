#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h"

#include "Upscaling/UpscalerSettings.h"

UpscalerProviderCapabilities NvidiaDlssUpscalerProvider::QueryCapabilities(const RhiCapabilities& capabilities) const
{
	const DlssCapabilityReport dlss = DlssCapabilityReporter::Build(capabilities);
	const bool canCreate = dlss.CanCreateFeature();
	return UpscalerProviderCapabilities{
	    .Kind = EUpscalerProviderKind::NvidiaDlss,
	    .Status = canCreate ? EUpscalerProviderStatus::Available : EUpscalerProviderStatus::Unavailable,
	    .CanInitialize = canCreate,
	    .CanEvaluate = canCreate,
	    .UsesExternalSdk = true,
	    .ProviderName = "NVIDIA DLSS",
	    .ExternalRuntimeVersion = dlss.SdkVersion,
	    .RuntimeState = DlssProviderRuntimeStateToString(dlss.RuntimeState),
	    .Reason = dlss.UnavailableReason};
}

bool NvidiaDlssUpscalerProvider::Initialize(const RhiCapabilities& capabilities)
{
	m_qualityMode = BuildUpscalerSettingsFromCVars().QualityMode;
	m_dlssCapabilities = DlssCapabilityReporter::Build(capabilities);
	m_dlssCapabilities.SelectedQualityMode = UpscalerQualityModeToString(m_qualityMode);
	m_diagnostics = QueryCapabilities(capabilities);
	if (!m_dlssCapabilities.CanCreateFeature())
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::Unavailable;
		m_diagnostics = GetDiagnostics();
		DlssCapabilityReporter::LogOnce(m_dlssCapabilities);
		return false;
	}

	m_runtime = CreateStreamlineDlssRuntime();
	if (m_runtime == nullptr)
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::FailedWithFallback;
		m_dlssCapabilities.UnavailableReason = "DLSS runtime factory returned no runtime instance.";
		m_diagnostics.Status = EUpscalerProviderStatus::Unavailable;
		m_diagnostics.CanEvaluate = false;
		m_diagnostics.Reason = m_dlssCapabilities.UnavailableReason;
		DlssCapabilityReporter::LogOnce(m_dlssCapabilities);
		return false;
	}

	const bool initialized = m_runtime->Initialize(
	    StreamlineDlssRuntimeDesc{
	        .Capabilities = capabilities,
	        .QualityMode = m_qualityMode,
	        .ApplicationName = "SparkleEngine",
	        .ApplicationId = 0});
	DlssCapabilityReporter::ApplyRuntimeDiagnostics(m_dlssCapabilities, m_runtime->GetDiagnostics());
	m_diagnostics = GetDiagnostics();
	DlssCapabilityReporter::LogOnce(m_dlssCapabilities);
	return initialized;
}

void NvidiaDlssUpscalerProvider::SetupFrame(const UpscalerInputContract& inputContract)
{
	m_lastInputContract = inputContract;
	m_renderExtent = inputContract.RenderExtent;
	m_outputExtent = inputContract.OutputExtent;
	m_dlssCapabilities.RenderExtent = m_renderExtent;
	m_dlssCapabilities.OutputExtent = m_outputExtent;
	m_dlssCapabilities.ResetRequested = inputContract.ResetRequested;
	m_dlssCapabilities.ResetReason = inputContract.ResetReason;

	if (m_runtime != nullptr)
	{
		m_runtime->SetupFrame(inputContract);
		DlssCapabilityReporter::ApplyRuntimeDiagnostics(m_dlssCapabilities, m_runtime->GetDiagnostics());
	}
}

UpscalerEvaluationResult NvidiaDlssUpscalerProvider::Evaluate(const UpscalerEvaluationDesc& evaluation)
{
	if (m_runtime == nullptr)
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::FailedWithFallback;
		m_dlssCapabilities.UnavailableReason = "NVIDIA DLSS runtime was not created.";
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .Reason = m_dlssCapabilities.UnavailableReason};
	}

	UpscalerEvaluationResult result = m_runtime->Evaluate(evaluation);
	DlssCapabilityReporter::ApplyRuntimeDiagnostics(m_dlssCapabilities, m_runtime->GetDiagnostics());
	m_diagnostics = GetDiagnostics();
	return result;
}

void NvidiaDlssUpscalerProvider::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	m_renderExtent = renderExtent;
	m_outputExtent = outputExtent;
	m_dlssCapabilities.RenderExtent = renderExtent;
	m_dlssCapabilities.OutputExtent = outputExtent;
}

void NvidiaDlssUpscalerProvider::ResetHistory(std::string_view reason)
{
	m_dlssCapabilities.ResetRequested = true;
	m_dlssCapabilities.ResetReason = std::string(reason);
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
	EUpscalerProviderStatus status = EUpscalerProviderStatus::Unavailable;
	switch (m_dlssCapabilities.RuntimeState)
	{
		case EDlssProviderRuntimeState::AvailableNotCreated:
			status = EUpscalerProviderStatus::Available;
			break;
		case EDlssProviderRuntimeState::Created:
		case EDlssProviderRuntimeState::Evaluating:
			status = EUpscalerProviderStatus::Active;
			break;
		case EDlssProviderRuntimeState::FailedWithFallback:
			status = EUpscalerProviderStatus::FailedWithFallback;
			break;
		case EDlssProviderRuntimeState::NotSelected:
		case EDlssProviderRuntimeState::Unavailable:
		default:
			status = EUpscalerProviderStatus::Unavailable;
			break;
	}

	const bool canEvaluate =
	    m_dlssCapabilities.RuntimeState == EDlssProviderRuntimeState::Created ||
	    m_dlssCapabilities.RuntimeState == EDlssProviderRuntimeState::Evaluating;
	return UpscalerProviderCapabilities{
	    .Kind = EUpscalerProviderKind::NvidiaDlss,
	    .Status = status,
	    .CanInitialize = m_dlssCapabilities.CanCreateFeature(),
	    .CanEvaluate = canEvaluate,
	    .UsesExternalSdk = true,
	    .ProviderName = "NVIDIA DLSS",
	    .ExternalRuntimeVersion = m_dlssCapabilities.SdkVersion,
	    .RuntimeState = DlssProviderRuntimeStateToString(m_dlssCapabilities.RuntimeState),
	    .SelectedQualityMode = m_dlssCapabilities.SelectedQualityMode,
	    .RenderExtent = m_dlssCapabilities.RenderExtent,
	    .OutputExtent = m_dlssCapabilities.OutputExtent,
	    .ResetRequested = m_dlssCapabilities.ResetRequested,
	    .ResetReason = m_dlssCapabilities.ResetReason,
	    .Reason = m_dlssCapabilities.UnavailableReason};
}
