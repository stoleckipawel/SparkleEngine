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
	const RayReconstructionProviderCapabilities providerCapabilities = QueryCapabilities(capabilities);
	if (!providerCapabilities.CanInitialize)
	{
		m_runtimeDiagnostics.State = EDlrrRuntimeState::Unavailable;
		m_runtimeDiagnostics.FailureDomain = providerCapabilities.FailureDomain;
		m_runtimeDiagnostics.FailureReason = providerCapabilities.Reason;
		return false;
	}

	m_runtime = CreateStreamlineDlrrRuntime();
	if (m_runtime == nullptr)
	{
		m_runtimeDiagnostics.State = EDlrrRuntimeState::Failed;
		m_runtimeDiagnostics.FailureDomain = ERayReconstructionProviderFailureDomain::Sdk;
		m_runtimeDiagnostics.FailureReason = "DLRR runtime factory returned no runtime instance.";
		return false;
	}

	const bool initialized = m_runtime->Initialize(
	    StreamlineDlrrRuntimeDesc{
	        .Capabilities = capabilities,
	        .NativeInterop = nativeInterop,
	        .PresentationBridge = presentationBridge});
	m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	return initialized;
}

void NvidiaDlrrProvider::SetupFrame(const RayReconstructionInputContract& inputContract)
{
	m_lastInputContract = inputContract;
	if (m_runtime != nullptr)
	{
		m_runtime->SetupFrame(inputContract);
		m_runtimeDiagnostics = m_runtime->GetDiagnostics();
	}
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
