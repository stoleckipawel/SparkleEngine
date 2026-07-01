#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h"

#include "Upscaling/UpscalerSettings.h"

#include <string>

namespace
{
	constexpr std::uint32_t kStableExtentFramesBeforeDlssEvaluation = 2;

	RendererProviderUpscalerResourceContract BuildDlssResourceContract(const UpscalerInputContract* inputContract = nullptr) noexcept
	{
		const bool hasInputContract = inputContract != nullptr;
		return RendererProviderUpscalerResourceContract{
		    .ScalingInputColor = {.Requirement = ERendererProviderResourceRequirement::Required,
		                          .Available = hasInputContract ? static_cast<bool>(inputContract->ScalingInputColor) : false},
		    .ScalingOutputColor = {.Requirement = ERendererProviderResourceRequirement::Required,
		                           .Available = hasInputContract ? static_cast<bool>(inputContract->ScalingOutputColor) : false},
		    .Depth = {.Requirement = ERendererProviderResourceRequirement::Required,
		              .Available = hasInputContract ? static_cast<bool>(inputContract->Depth) : false},
		    .MotionVectors = {.Requirement = ERendererProviderResourceRequirement::Required,
		                      .Available = hasInputContract ? static_cast<bool>(inputContract->MotionVectors) : false},
		    .Exposure = {.Requirement = ERendererProviderResourceRequirement::Optional,
		                 .Available = hasInputContract ? static_cast<bool>(inputContract->Exposure) : false},
		    .History = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = hasInputContract},
		    .Jitter = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = hasInputContract},
		    .CameraMatrices = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = hasInputContract},
		    .FrameIndex = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = hasInputContract},
		};
	}

	ERendererProviderCapabilityState MapDlssCapabilityState(const DlssCapabilityReport& dlss) noexcept
	{
		switch (dlss.RuntimeState)
		{
			case EDlssProviderRuntimeState::AvailableNotCreated:
				return ERendererProviderCapabilityState::Available;
			case EDlssProviderRuntimeState::Created:
			case EDlssProviderRuntimeState::Evaluating:
				return ERendererProviderCapabilityState::Enabled;
			case EDlssProviderRuntimeState::FailedWithFallback:
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

	EDlssFeatureKind GetSelectedDlssFeature(EUpscalerQualityMode qualityMode) noexcept
	{
		return qualityMode == EUpscalerQualityMode::NativeAA ? EDlssFeatureKind::NativeAA : EDlssFeatureKind::SuperResolution;
	}

	bool ExtentsEqual(RenderViewportExtent lhs, RenderViewportExtent rhs) noexcept
	{
		return lhs.Width == rhs.Width && lhs.Height == rhs.Height;
	}

	std::string BuildFeatureMatrixSummary(const DlssFeatureMatrix& matrix)
	{
		std::string summary;
		for (const DlssFeatureMatrixEntry& entry : matrix.Entries)
		{
			if (!summary.empty())
			{
				summary += ", ";
			}
			summary += DlssFeatureKindToString(entry.Feature);
			summary += "=";
			summary += DlssFeatureStateToString(entry.State);
		}
		return summary;
	}

	void MarkSelectedFeature(DlssFeatureMatrix& matrix, EDlssFeatureKind selectedFeature)
	{
		for (DlssFeatureMatrixEntry& entry : matrix.Entries)
		{
			if (entry.Feature == selectedFeature)
			{
				if (entry.State == EDlssFeatureState::Available)
				{
					entry.State = EDlssFeatureState::Enabled;
				}
				continue;
			}

			if (entry.State == EDlssFeatureState::Available || entry.State == EDlssFeatureState::Enabled)
			{
				entry.State = EDlssFeatureState::NotSelected;
			}
		}
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
	const RendererProviderUpscalerResourceContract resourceContract = BuildDlssResourceContract();
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
	    .ResourceContractSummary = BuildProviderResourceContractSummary(resourceContract),
	    .ExternalRuntimeVersion = dlss.SdkVersion,
	    .RuntimeState = DlssProviderRuntimeStateToString(dlss.RuntimeState),
	    .FeatureMatrixSummary = BuildFeatureMatrixSummary(dlss.FeatureMatrix),
	    .Reason = dlss.UnavailableReason};
}

bool NvidiaDlssUpscalerProvider::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop,
    UpscalerPresentationBridge presentationBridge)
{
	const UpscalerSettings settings = BuildUpscalerSettingsFromCVars();
	m_qualityMode = settings.QualityMode;
	m_dlssCapabilities = DlssCapabilityReporter::Build(capabilities);
	m_dlssCapabilities.SelectedQualityMode = UpscalerQualityModeToString(m_qualityMode);
	MarkSelectedFeature(m_dlssCapabilities.FeatureMatrix, GetSelectedDlssFeature(m_qualityMode));
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
		m_diagnostics.CapabilityState = ERendererProviderCapabilityState::MissingDependency;
		m_diagnostics.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
		m_diagnostics.CanEvaluate = false;
		m_diagnostics.Reason = m_dlssCapabilities.UnavailableReason;
		DlssCapabilityReporter::LogOnce(m_dlssCapabilities);
		return false;
	}

	const bool initialized = m_runtime->Initialize(
	    StreamlineDlssRuntimeDesc{
	        .Capabilities = capabilities,
	        .NativeInterop = nativeInterop,
	        .PresentationBridge = presentationBridge,
	        .QualityMode = m_qualityMode,
	        .DiagnosticsEnabled = settings.DiagnosticsEnabled,
	        .ApplicationName = "SparkleEngine",
	        .ApplicationId = 0});
	DlssCapabilityReporter::ApplyRuntimeDiagnostics(m_dlssCapabilities, m_runtime->GetDiagnostics());
	m_diagnostics = GetDiagnostics();
	if (!initialized)
	{
		const std::shared_ptr<spdlog::logger> logger = Logging::GetOrCreateLogger("Renderer.DLSS");
		SPDLOG_LOGGER_WARN(
		    logger,
		    "DLSS provider initialization failed: runtimeState={} reason='{}'",
		    DlssProviderRuntimeStateToString(m_dlssCapabilities.RuntimeState),
		    m_dlssCapabilities.UnavailableReason);
	}
	DlssCapabilityReporter::LogOnce(m_dlssCapabilities);
	return initialized;
}

void NvidiaDlssUpscalerProvider::SetupFrame(const UpscalerInputContract& inputContract)
{
	m_lastInputContract = inputContract;
	m_renderExtent = inputContract.RenderExtent;
	m_outputExtent = inputContract.OutputExtent;
	if (ExtentsEqual(m_lastObservedRenderExtent, inputContract.RenderExtent) &&
	    ExtentsEqual(m_lastObservedOutputExtent, inputContract.OutputExtent))
	{
		++m_stableExtentFrameCount;
	}
	else
	{
		m_lastObservedRenderExtent = inputContract.RenderExtent;
		m_lastObservedOutputExtent = inputContract.OutputExtent;
		m_stableExtentFrameCount = 1;
	}
	m_extentReadyForEvaluation = m_stableExtentFrameCount >= kStableExtentFramesBeforeDlssEvaluation;

	m_dlssCapabilities.RenderExtent = m_renderExtent;
	m_dlssCapabilities.OutputExtent = m_outputExtent;
	m_dlssCapabilities.ResetRequested = inputContract.ResetRequested;
	m_dlssCapabilities.ResetReason = inputContract.ResetReason;
	m_diagnostics.ResourceContract = BuildDlssResourceContract(&inputContract);
	m_diagnostics.ResourceContractSummary = BuildProviderResourceContractSummary(m_diagnostics.ResourceContract);
	if (m_qualityMode == EUpscalerQualityMode::NativeAA && !NativeAAExtentContractValid(inputContract))
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::FailedWithFallback;
		m_dlssCapabilities.FailureDomain = EUpscalerProviderFailureDomain::InputContract;
		m_dlssCapabilities.UnavailableReason =
		    "DLSS NativeAA requires render extent to equal output extent; using deterministic passthrough fallback.";
		for (DlssFeatureMatrixEntry& entry : m_dlssCapabilities.FeatureMatrix.Entries)
		{
			if (entry.Feature == EDlssFeatureKind::NativeAA)
			{
				entry.State = EDlssFeatureState::FailedWithFallback;
				entry.Reason = m_dlssCapabilities.UnavailableReason;
			}
		}
		m_diagnostics = GetDiagnostics();
		return;
	}

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
		m_dlssCapabilities.FailureDomain = EUpscalerProviderFailureDomain::Sdk;
		m_dlssCapabilities.UnavailableReason = "NVIDIA DLSS runtime was not created.";
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .FailureDomain = m_dlssCapabilities.FailureDomain,
		    .Reason = m_dlssCapabilities.UnavailableReason};
	}

	if (!m_extentReadyForEvaluation)
	{
		m_dlssCapabilities.RuntimeState = EDlssProviderRuntimeState::Created;
		m_dlssCapabilities.FailureDomain = EUpscalerProviderFailureDomain::None;
		m_dlssCapabilities.UnavailableReason =
		    "Waiting for stable render/output extent before first DLSS evaluation; using deterministic passthrough fallback.";
		m_diagnostics = GetDiagnostics();
		return UpscalerEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .FailureDomain = EUpscalerProviderFailureDomain::None,
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
	m_lastObservedRenderExtent = renderExtent;
	m_lastObservedOutputExtent = outputExtent;
	m_stableExtentFrameCount = 0;
	m_extentReadyForEvaluation = false;
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
	const bool canEvaluate =
	    m_dlssCapabilities.RuntimeState == EDlssProviderRuntimeState::Created ||
	    m_dlssCapabilities.RuntimeState == EDlssProviderRuntimeState::Evaluating;
	const RendererProviderUpscalerResourceContract resourceContract = BuildDlssResourceContract(&m_lastInputContract);
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
	    .ResourceContractSummary = BuildProviderResourceContractSummary(resourceContract),
	    .ExternalRuntimeVersion = m_dlssCapabilities.SdkVersion,
	    .RuntimeState = DlssProviderRuntimeStateToString(m_dlssCapabilities.RuntimeState),
	    .SelectedQualityMode = m_dlssCapabilities.SelectedQualityMode,
	    .FeatureMatrixSummary = BuildFeatureMatrixSummary(m_dlssCapabilities.FeatureMatrix),
	    .RenderExtent = m_dlssCapabilities.RenderExtent,
	    .OutputExtent = m_dlssCapabilities.OutputExtent,
	    .ResetRequested = m_dlssCapabilities.ResetRequested,
	    .ResetReason = m_dlssCapabilities.ResetReason,
	    .Reason = m_dlssCapabilities.UnavailableReason};
}
