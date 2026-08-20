#include "../PCH.h"
#include "Streamline/StreamlineDlssFrameState.h"

StreamlineDlssFrameState::StreamlineDlssFrameState() noexcept :
    m_qualityMode(GetRequestedQualityMode())
{
}

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
	return providerRenderExtent;
}

void StreamlineDlssFrameState::SetupFrame(const ImageProviderFrameInput& frameInput) noexcept
{
	m_frameInput = frameInput;
	const EUpscalerQualityMode qualityMode = GetRequestedQualityMode();
	if (qualityMode != m_qualityMode)
	{
		m_qualityMode = qualityMode;
		m_frameInput.ResetHistory = true;
	}

	m_frameValid = m_resolvedRenderExtent.IsValid() && frameInput.OutputExtent == m_resolvedOutputExtent
	    && frameInput.RenderExtent == m_resolvedRenderExtent;
}

void StreamlineDlssFrameState::Reset() noexcept
{
	m_frameInput = {};
	m_resolvedOutputExtent = {};
	m_resolvedRenderExtent = {};
	m_frameValid = false;
}
