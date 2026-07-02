#include "../PCH.h"
#include "RayReconstruction/NoopRayReconstructionProvider.h"

RayReconstructionProviderCapabilities NoopRayReconstructionProvider::QueryCapabilities(const RhiCapabilities&) const
{
	return RayReconstructionProviderCapabilities{
	    .Kind = ERayReconstructionProviderKind::None,
	    .CapabilityState = ERendererProviderCapabilityState::Enabled,
	    .CanInitialize = true,
	    .CanEvaluate = false,
	    .ProviderName = "No ray reconstruction",
	    .Reason = "Ray reconstruction is disabled."};
}

bool NoopRayReconstructionProvider::Initialize(
    const RhiCapabilities& capabilities,
    RhiNativeDeviceQueueInterop,
    RayReconstructionPresentationBridge)
{
	m_diagnostics = QueryCapabilities(capabilities);
	return true;
}

void NoopRayReconstructionProvider::SetupFrame(const RayReconstructionInputContract& inputContract)
{
	m_diagnostics.RenderExtent = inputContract.RenderExtent;
	m_diagnostics.OutputExtent = inputContract.OutputExtent;
	m_diagnostics.ResetRequested = inputContract.ResetRequested;
	m_diagnostics.ResetReason = inputContract.ResetReason;
}

RayReconstructionEvaluationResult NoopRayReconstructionProvider::Evaluate(const RayReconstructionEvaluationDesc&)
{
	return RayReconstructionEvaluationResult{
	    .ProducedOutput = false,
	    .UsedFallback = true,
	    .FailureDomain = ERayReconstructionProviderFailureDomain::None,
	    .Reason = "Ray reconstruction is disabled."};
}

void NoopRayReconstructionProvider::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	m_diagnostics.RenderExtent = renderExtent;
	m_diagnostics.OutputExtent = outputExtent;
}

void NoopRayReconstructionProvider::ResetHistory(std::string_view reason)
{
	m_diagnostics.ResetRequested = true;
	m_diagnostics.ResetReason = std::string(reason);
}
