#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlss/NvidiaDlssRayReconstructionProvider.h"

namespace
{
	ERendererProviderCapabilityState MapRuntimeState(const StreamlineDlssRayReconstructionRuntimeDiagnostics& diagnostics) noexcept
	{
		switch (diagnostics.State)
		{
			case EDlssRayReconstructionRuntimeState::Created:
			case EDlssRayReconstructionRuntimeState::Evaluating:
				return ERendererProviderCapabilityState::Enabled;
			case EDlssRayReconstructionRuntimeState::FailedWithFallback:
				return ERendererProviderCapabilityState::RuntimeFailed;
			case EDlssRayReconstructionRuntimeState::Unavailable:
				return diagnostics.FailureDomain == ERayReconstructionProviderFailureDomain::Feature
				           ? ERendererProviderCapabilityState::UnsupportedHardware
				           : ERendererProviderCapabilityState::Unavailable;
			case EDlssRayReconstructionRuntimeState::NotSelected:
			default:
				return ERendererProviderCapabilityState::Available;
		}
	}
}

RayReconstructionProviderCapabilities NvidiaDlssRayReconstructionProvider::QueryCapabilities(const RhiCapabilities& capabilities) const
{
	const bool backendSupported = capabilities.BackendApi == ERhiBackendApi::D3D12 || capabilities.BackendApi == ERhiBackendApi::Vulkan;
	const bool interopSupported = capabilities.ExternalFeatureInterop.ExposesNativeDevice &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeGraphicsCommandList &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeResources;
	const RendererProviderRayReconstructionResourceContract resourceContract =
	    BuildRayReconstructionProviderResourceContract(RayReconstructionInputContract{});
	return RayReconstructionProviderCapabilities{
	    .Kind = ERayReconstructionProviderKind::NvidiaDlssRayReconstruction,
	    .CapabilityState = backendSupported && interopSupported ? ERendererProviderCapabilityState::Available :
	                                                             ERendererProviderCapabilityState::Unavailable,
	    .FailureDomain = backendSupported && interopSupported ? ERayReconstructionProviderFailureDomain::None :
	                                                            ERayReconstructionProviderFailureDomain::Backend,
	    .CanInitialize = backendSupported && interopSupported,
	    .CanEvaluate = backendSupported && interopSupported,
	    .UsesExternalSdk = true,
	    .ProviderName = "NVIDIA DLSS Ray Reconstruction",
	    .ResourceContract = resourceContract,
	    .ResourceContractSummary = BuildProviderResourceContractSummary(resourceContract),
	    .Reason = backendSupported && interopSupported ? "" : "Native Streamline interop requirements are not satisfied."};
}

bool NvidiaDlssRayReconstructionProvider::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop,
    RayReconstructionPresentationBridge presentationBridge)
{
	const RayReconstructionSettings settings = BuildRayReconstructionSettingsFromCVars();
	m_qualityMode = settings.QualityMode;
	m_diagnostics = QueryCapabilities(capabilities);
	if (!m_diagnostics.CanInitialize)
	{
		return false;
	}

	m_runtime = CreateStreamlineDlssRayReconstructionRuntime();
	if (m_runtime == nullptr)
	{
		m_diagnostics.CapabilityState = ERendererProviderCapabilityState::MissingDependency;
		m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
		m_diagnostics.CanEvaluate = false;
		m_diagnostics.Reason = "DLRR runtime factory returned no runtime instance.";
		return false;
	}

	const bool initialized = m_runtime->Initialize(
	    StreamlineDlssRayReconstructionRuntimeDesc{
	        .Capabilities = capabilities,
	        .NativeInterop = nativeInterop,
	        .PresentationBridge = presentationBridge,
	        .QualityMode = m_qualityMode});
	m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	m_diagnostics = GetDiagnostics();
	return initialized;
}

void NvidiaDlssRayReconstructionProvider::SetupFrame(const RayReconstructionInputContract& inputContract)
{
	m_lastInputContract = inputContract;
	m_diagnostics.ResourceContract = BuildRayReconstructionProviderResourceContract(inputContract);
	m_diagnostics.ResourceContractSummary = BuildProviderResourceContractSummary(m_diagnostics.ResourceContract);
	if (m_runtime != nullptr)
	{
		m_runtime->SetupFrame(inputContract);
		m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	}
	m_diagnostics = GetDiagnostics();
}

RayReconstructionEvaluationResult NvidiaDlssRayReconstructionProvider::Evaluate(const RayReconstructionEvaluationDesc& evaluation)
{
	if (m_runtime == nullptr)
	{
		return RayReconstructionEvaluationResult{
		    .ProducedOutput = false,
		    .UsedFallback = true,
		    .FailureDomain = ERayReconstructionProviderFailureDomain::Sdk,
		    .Reason = "NVIDIA DLRR runtime was not created."};
	}

	RayReconstructionEvaluationResult result = m_runtime->Evaluate(evaluation);
	m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	m_diagnostics = GetDiagnostics();
	return result;
}

void NvidiaDlssRayReconstructionProvider::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	m_runtimeDiagnostics.RenderExtent = renderExtent;
	m_runtimeDiagnostics.OutputExtent = outputExtent;
}

void NvidiaDlssRayReconstructionProvider::ResetHistory(std::string_view reason)
{
	m_runtimeDiagnostics.ResetRequested = true;
	m_runtimeDiagnostics.ResetReason = std::string(reason);
	if (m_runtime != nullptr)
	{
		m_runtime->ResetHistory(reason);
		m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	}
}

void NvidiaDlssRayReconstructionProvider::Shutdown() noexcept
{
	if (m_runtime != nullptr)
	{
		m_runtime->Shutdown();
		m_runtime.reset();
	}
}

RayReconstructionProviderCapabilities NvidiaDlssRayReconstructionProvider::GetDiagnostics() const
{
	const RendererProviderRayReconstructionResourceContract resourceContract =
	    BuildRayReconstructionProviderResourceContract(m_lastInputContract);
	return RayReconstructionProviderCapabilities{
	    .Kind = ERayReconstructionProviderKind::NvidiaDlssRayReconstruction,
	    .CapabilityState = MapRuntimeState(m_runtimeDiagnostics),
	    .FailureDomain = m_runtimeDiagnostics.FailureDomain,
	    .CanInitialize = m_runtimeDiagnostics.State != EDlssRayReconstructionRuntimeState::Unavailable,
	    .CanEvaluate = m_runtimeDiagnostics.State == EDlssRayReconstructionRuntimeState::Created ||
	                   m_runtimeDiagnostics.State == EDlssRayReconstructionRuntimeState::Evaluating,
	    .UsesExternalSdk = true,
	    .ProviderName = "NVIDIA DLSS Ray Reconstruction",
	    .ResourceContract = resourceContract,
	    .ResourceContractSummary = BuildProviderResourceContractSummary(resourceContract),
	    .ExternalRuntimeVersion = m_runtimeDiagnostics.SdkVersion,
	    .RuntimeState = DlssRayReconstructionRuntimeStateToString(m_runtimeDiagnostics.State),
	    .SelectedQualityMode = RayReconstructionQualityModeToString(m_qualityMode),
	    .RenderExtent = m_runtimeDiagnostics.RenderExtent,
	    .OutputExtent = m_runtimeDiagnostics.OutputExtent,
	    .ResetRequested = m_runtimeDiagnostics.ResetRequested,
	    .ResetReason = m_runtimeDiagnostics.ResetReason,
	    .Reason = m_runtimeDiagnostics.FailureReason};
}
