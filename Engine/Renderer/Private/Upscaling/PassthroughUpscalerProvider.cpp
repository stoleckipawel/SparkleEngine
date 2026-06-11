#include "../PCH.h"
#include "Upscaling/PassthroughUpscalerProvider.h"

UpscalerProviderCapabilities PassthroughUpscalerProvider::QueryCapabilities(const RhiCapabilities&) const
{
	return UpscalerProviderCapabilities{
	    .Kind = EUpscalerProviderKind::Passthrough,
	    .Status = EUpscalerProviderStatus::Available,
	    .CanInitialize = true,
	    .CanEvaluate = true,
	    .UsesExternalSdk = false,
	    .ProviderName = "Passthrough",
	    .ExternalRuntimeVersion = "none",
	    .RuntimeState = "Active",
	    .FeatureMatrixSummary = "external features not selected",
	    .Reason = "Deterministic passthrough fallback is always available."};
}

bool PassthroughUpscalerProvider::Initialize(const RhiCapabilities& capabilities, NativeGraphicsDeviceHandle, UpscalerPresentationBridge)
{
	m_diagnostics = QueryCapabilities(capabilities);
	m_diagnostics.Status = EUpscalerProviderStatus::Active;
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
}

UpscalerEvaluationResult PassthroughUpscalerProvider::Evaluate(const UpscalerEvaluationDesc& evaluation)
{
	const bool hasInput = static_cast<bool>(evaluation.InputColor);
	return UpscalerEvaluationResult{
	    .ProducedOutput = hasInput,
	    .UsedFallback = true,
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
