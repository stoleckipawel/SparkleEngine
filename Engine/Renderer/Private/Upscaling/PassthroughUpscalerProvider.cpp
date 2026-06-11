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
	    .Reason = "Deterministic passthrough fallback is always available."};
}

bool PassthroughUpscalerProvider::Initialize(const RhiCapabilities& capabilities)
{
	m_diagnostics = QueryCapabilities(capabilities);
	m_diagnostics.Status = EUpscalerProviderStatus::Active;
	return true;
}

void PassthroughUpscalerProvider::SetupFrame(const UpscalerFrameSetupDesc& frameSetup)
{
	m_renderExtent = frameSetup.RenderExtent;
	m_outputExtent = frameSetup.OutputExtent;
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
}

void PassthroughUpscalerProvider::ResetHistory(std::string_view)
{
}

void PassthroughUpscalerProvider::Shutdown() noexcept
{
}
