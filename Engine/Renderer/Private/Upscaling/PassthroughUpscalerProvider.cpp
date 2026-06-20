#include "../PCH.h"
#include "Upscaling/PassthroughUpscalerProvider.h"

UpscalerProviderCapabilities PassthroughUpscalerProvider::QueryCapabilities(const RhiCapabilities&) const
{
	const RendererProviderResourceContract resourceContract{
	    .Color = {.Requirement = ERendererProviderResourceRequirement::Required, .Available = true},
	    .Depth = {.Requirement = ERendererProviderResourceRequirement::Optional, .Available = true},
	    .MotionVectors = {.Requirement = ERendererProviderResourceRequirement::Optional, .Available = true},
	    .Exposure = {.Requirement = ERendererProviderResourceRequirement::Unused, .Available = false},
	    .Normals = {.Requirement = ERendererProviderResourceRequirement::Unused, .Available = false},
	    .History = {.Requirement = ERendererProviderResourceRequirement::Unused, .Available = false},
	    .Jitter = {.Requirement = ERendererProviderResourceRequirement::Unused, .Available = false},
	    .CameraMatrices = {.Requirement = ERendererProviderResourceRequirement::Unused, .Available = false},
	    .FrameIndex = {.Requirement = ERendererProviderResourceRequirement::Unused, .Available = false},
	};
	return UpscalerProviderCapabilities{
	    .Kind = EUpscalerProviderKind::Passthrough,
	    .Category = ERendererProviderCategory::Upscaler,
	    .CapabilityState = ERendererProviderCapabilityState::Available,
	    .FailureDomain = EUpscalerProviderFailureDomain::None,
	    .CanInitialize = true,
	    .CanEvaluate = true,
	    .UsesExternalSdk = false,
	    .ProviderName = "Passthrough",
	    .ResourceContract = resourceContract,
	    .ResourceContractSummary = BuildProviderResourceContractSummary(resourceContract),
	    .ExternalRuntimeVersion = "none",
	    .RuntimeState = "Enabled",
	    .FeatureMatrixSummary = "external features not selected",
	    .Reason = "Deterministic passthrough fallback is always available."};
}

bool PassthroughUpscalerProvider::Initialize(const RhiCapabilities& capabilities, RhiNativeDeviceQueueInterop, UpscalerPresentationBridge)
{
	m_diagnostics = QueryCapabilities(capabilities);
	m_diagnostics.CapabilityState = ERendererProviderCapabilityState::Enabled;
	return true;
}

void PassthroughUpscalerProvider::SetupFrame(const UpscalerInputContract& inputContract)
{
	m_renderExtent = inputContract.RenderExtent;
	m_outputExtent = inputContract.OutputExtent;
	m_diagnostics.RenderExtent = m_renderExtent;
	m_diagnostics.OutputExtent = m_outputExtent;
	m_diagnostics.ResetRequested = inputContract.ResetRequested;
	m_diagnostics.ResetReason = inputContract.ResetReason;
	m_diagnostics.ResourceContract = BuildUpscalerProviderResourceContract(inputContract);
	m_diagnostics.ResourceContractSummary = BuildProviderResourceContractSummary(m_diagnostics.ResourceContract);
}

UpscalerEvaluationResult PassthroughUpscalerProvider::Evaluate(const UpscalerEvaluationDesc& evaluation)
{
	const bool hasInput = static_cast<bool>(evaluation.InputColor);
	return UpscalerEvaluationResult{
	    .ProducedOutput = hasInput,
	    .UsedFallback = true,
	    .FailureDomain = hasInput ? EUpscalerProviderFailureDomain::None : EUpscalerProviderFailureDomain::InputContract,
	    .Reason = hasInput ? "Passthrough selected the native scene color product." : "Passthrough has no input scene color product."};
}

void PassthroughUpscalerProvider::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	m_renderExtent = renderExtent;
	m_outputExtent = outputExtent;
	m_diagnostics.RenderExtent = renderExtent;
	m_diagnostics.OutputExtent = outputExtent;
}

void PassthroughUpscalerProvider::ResetHistory(std::string_view)
{
}

void PassthroughUpscalerProvider::Shutdown() noexcept
{
}
