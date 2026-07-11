#include "../PCH.h"
#include "Streamline/StreamlineDlssFrameState.h"

StreamlineDlssFrameState::StreamlineDlssFrameState() noexcept : m_qualityMode(GetRequestedQualityMode()) {}

EUpscalerQualityMode StreamlineDlssFrameState::GetRequestedQualityMode() const noexcept
{
	return CVarUpscalerQualityMode.Get();
}

RenderViewportExtent StreamlineDlssFrameState::StoreResolution(
    RenderViewportExtent outputExtent,
    RenderViewportExtent providerRenderExtent) noexcept
{
	m_resolvedOutputExtent = outputExtent;
	m_resolvedRenderExtent = providerRenderExtent;
	m_frameValid = false;
	return providerRenderExtent.IsValid() ? providerRenderExtent : outputExtent;
}

void StreamlineDlssFrameState::SetupFrame(const ImageProviderFrameContext& frameContext) noexcept
{
	m_frameContext = frameContext;
	const EUpscalerQualityMode qualityMode = GetRequestedQualityMode();
	if (qualityMode != m_qualityMode)
	{
		m_qualityMode = qualityMode;
		m_frameContext.ResetHistory = true;
	}

	m_frameValid = m_resolvedRenderExtent.IsValid() && frameContext.OutputExtent == m_resolvedOutputExtent &&
	               frameContext.RenderExtent == m_resolvedRenderExtent;
}

void StreamlineDlssFrameState::Reset() noexcept
{
	m_frameContext = {};
	m_resolvedOutputExtent = {};
	m_resolvedRenderExtent = {};
	m_frameValid = false;
}
