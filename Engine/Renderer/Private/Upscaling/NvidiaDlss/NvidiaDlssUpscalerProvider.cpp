#include "../../PCH.h"
#include "Upscaling/NvidiaDlss/NvidiaDlssUpscalerProvider.h"

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
	    .Reason = dlss.UnavailableReason};
}

bool NvidiaDlssUpscalerProvider::Initialize(const RhiCapabilities& capabilities)
{
	m_dlssCapabilities = DlssCapabilityReporter::Build(capabilities);
	m_diagnostics = QueryCapabilities(capabilities);
	if (!m_dlssCapabilities.CanCreateFeature())
	{
		return false;
	}

	m_diagnostics.Status = EUpscalerProviderStatus::Active;
	return true;
}

void NvidiaDlssUpscalerProvider::SetupFrame(const UpscalerInputContract& inputContract)
{
	m_renderExtent = inputContract.RenderExtent;
	m_outputExtent = inputContract.OutputExtent;
}

UpscalerEvaluationResult NvidiaDlssUpscalerProvider::Evaluate(const UpscalerEvaluationDesc&)
{
	return UpscalerEvaluationResult{
	    .ProducedOutput = false,
	    .UsedFallback = true,
	    .Reason = "NVIDIA DLSS provider runtime is not integrated yet."};
}

void NvidiaDlssUpscalerProvider::OnResize(RenderViewportExtent renderExtent, RenderViewportExtent outputExtent)
{
	m_renderExtent = renderExtent;
	m_outputExtent = outputExtent;
}

void NvidiaDlssUpscalerProvider::ResetHistory(std::string_view)
{
}

void NvidiaDlssUpscalerProvider::Shutdown() noexcept
{
}
