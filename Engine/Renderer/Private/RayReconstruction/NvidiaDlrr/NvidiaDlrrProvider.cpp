#include "../../PCH.h"
#include "RayReconstruction/NvidiaDlrr/NvidiaDlrrProvider.h"

namespace
{
	ERendererProviderCapabilityState MapRuntimeState(const StreamlineDlrrRuntimeDiagnostics& diagnostics) noexcept
	{
		switch (diagnostics.State)
		{
			case EDlrrRuntimeState::Created:
			case EDlrrRuntimeState::Evaluating:
				return ERendererProviderCapabilityState::Enabled;
			case EDlrrRuntimeState::Failed:
				return ERendererProviderCapabilityState::RuntimeFailed;
			case EDlrrRuntimeState::Unavailable:
				return diagnostics.FailureDomain == ERayReconstructionProviderFailureDomain::Feature
				           ? ERendererProviderCapabilityState::UnsupportedHardware
				           : ERendererProviderCapabilityState::Unavailable;
			case EDlrrRuntimeState::NotSelected:
			default:
				return ERendererProviderCapabilityState::Available;
		}
	}
}

RayReconstructionProviderCapabilities NvidiaDlrrProvider::QueryCapabilities(const RhiCapabilities& capabilities) const
{
	const bool backendSupported = capabilities.BackendApi == ERhiBackendApi::D3D12 || capabilities.BackendApi == ERhiBackendApi::Vulkan;
	const bool interopSupported = capabilities.ExternalFeatureInterop.ExposesNativeDevice &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeGraphicsQueue &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeGraphicsCommandList &&
	                              capabilities.ExternalFeatureInterop.ExposesNativeResources;
	const RendererProviderRayReconstructionResourceContract resourceContract =
	    BuildRayReconstructionProviderResourceContract(RayReconstructionInputContract{});
	return RayReconstructionProviderCapabilities{
	    .Kind = ERayReconstructionProviderKind::NvidiaDlrr,
	    .CapabilityState = backendSupported && interopSupported ? ERendererProviderCapabilityState::Available :
	                                                             ERendererProviderCapabilityState::Unavailable,
	    .FailureDomain = backendSupported && interopSupported ? ERayReconstructionProviderFailureDomain::None :
	                                                            ERayReconstructionProviderFailureDomain::Backend,
	    .CanInitialize = backendSupported && interopSupported,
	    .CanEvaluate = backendSupported && interopSupported,
	    .UsesExternalSdk = true,
	    .ProviderName = "NVIDIA DLRR",
	    .ResourceContract = resourceContract,
	    .Reason = backendSupported && interopSupported ? "" : "Native Streamline interop requirements are not satisfied."};
}

bool NvidiaDlrrProvider::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop nativeInterop,
    RayReconstructionPresentationBridge presentationBridge)
{
	m_diagnostics = QueryCapabilities(capabilities);
	if (!m_diagnostics.CanInitialize)
	{
		return false;
	}

	m_runtime = CreateStreamlineDlrrRuntime();
	if (m_runtime == nullptr)
	{
		m_diagnostics.CapabilityState = ERendererProviderCapabilityState::MissingDependency;
		m_diagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
		m_diagnostics.CanEvaluate = false;
		m_diagnostics.Reason = "DLRR runtime factory returned no runtime instance.";
		return false;
	}

	const bool initialized = m_runtime->Initialize(
	    StreamlineDlrrRuntimeDesc{
	        .Capabilities = capabilities,
	        .NativeInterop = nativeInterop,
	        .PresentationBridge = presentationBridge});
	m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	m_diagnostics = GetDiagnostics();
	return initialized;
}

void NvidiaDlrrProvider::SetupFrame(const RayReconstructionInputContract& inputContract)
{
	m_lastInputContract = inputContract;
	m_diagnostics.ResourceContract = BuildRayReconstructionProviderResourceContract(inputContract);
	if (m_runtime != nullptr)
	{
		m_runtime->SetupFrame(inputContract);
		m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	}
	m_diagnostics = GetDiagnostics();
}

RayReconstructionEvaluationResult NvidiaDlrrProvider::Evaluate(const RayReconstructionEvaluationDesc& evaluation)
{
	if (m_runtime == nullptr)
	{
		return RayReconstructionEvaluationResult{
		    .ProducedOutput = false,
		    .FailureDomain = ERayReconstructionProviderFailureDomain::Sdk,
		    .Reason = "NVIDIA DLRR runtime was not created."};
	}

	RayReconstructionEvaluationResult result = m_runtime->Evaluate(evaluation);
	m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	m_diagnostics = GetDiagnostics();
	return result;
}

void NvidiaDlrrProvider::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	(void) renderExtent;
	(void) outputExtent;
}

void NvidiaDlrrProvider::ResetHistory(std::string_view reason)
{
	if (m_runtime != nullptr)
	{
		m_runtime->ResetHistory(reason);
		m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	}
}

void NvidiaDlrrProvider::Shutdown() noexcept
{
	if (m_runtime != nullptr)
	{
		m_runtime->Shutdown();
		m_runtime.reset();
	}
}

RayReconstructionProviderCapabilities NvidiaDlrrProvider::GetDiagnostics() const
{
	const RendererProviderRayReconstructionResourceContract resourceContract =
	    BuildRayReconstructionProviderResourceContract(m_lastInputContract);
	return RayReconstructionProviderCapabilities{
	    .Kind = ERayReconstructionProviderKind::NvidiaDlrr,
	    .CapabilityState = MapRuntimeState(m_runtimeDiagnostics),
	    .FailureDomain = m_runtimeDiagnostics.FailureDomain,
	    .CanInitialize = m_runtimeDiagnostics.State != EDlrrRuntimeState::Unavailable,
	    .CanEvaluate = m_runtimeDiagnostics.State == EDlrrRuntimeState::Created ||
	                   m_runtimeDiagnostics.State == EDlrrRuntimeState::Evaluating,
	    .UsesExternalSdk = true,
	    .ProviderName = "NVIDIA DLRR",
	    .ResourceContract = resourceContract,
	    .Reason = m_runtimeDiagnostics.FailureReason};
}
