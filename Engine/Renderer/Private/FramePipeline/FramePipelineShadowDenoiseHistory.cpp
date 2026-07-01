#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Denoising/ShadowDenoiseContract.h"
#include "Frame/RhiFrameConstants.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RayTracing/Effects/Shadows/RayTracedShadowSettings.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Memory/RhiMemoryTypes.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

void FramePipeline::CreateShadowDenoiseHistoryResources(RenderViewportExtent sceneExtent) noexcept
{
	if (!sceneExtent.IsValid())
	{
		return;
	}

	RenderHardwareInterface& renderHardwareInterface = m_systems->GetRenderHardwareInterface();
	RhiResourceService& resourceService = renderHardwareInterface.GetResourceService();
	const RhiTextureResourceDesc historyDesc{
	    .Width = sceneExtent.Width,
	    .Height = sceneExtent.Height,
	    .Format = ShadowDenoiseContract::DenoisedVisibilityFormat,
	    .MipLevels = 1u,
	    .AllowRenderTarget = false,
	    .AllowDepthStencil = false,
	    .AllowUnorderedAccess = true};

	for (RhiOwnedResourceHandle& historyResource : m_shadowDenoiseHistoryResources)
	{
		if (historyResource)
		{
			continue;
		}

		historyResource = resourceService.CreateTextureResource(
		    historyDesc,
		    ResourceState::ShaderResource,
		    RhiMemoryCategory::Texture,
		    RhiMemoryResidencyClass::DeviceLocal,
		    L"ShadowDenoiseVisibilityHistory");
	}
}

void FramePipeline::ReleaseShadowDenoiseHistoryResources() noexcept
{
	if (m_systems == nullptr)
	{
		return;
	}

	RhiResourceService& resourceService = m_systems->GetRenderHardwareInterface().GetResourceService();
	for (RhiOwnedResourceHandle& historyResource : m_shadowDenoiseHistoryResources)
	{
		if (historyResource)
		{
			resourceService.ReleaseOwnedResource(historyResource);
			historyResource = {};
		}
	}
}

void FramePipeline::RefreshShadowDenoiseHistoryResources() noexcept
{
	if (m_renderPath != FrameRenderPath::RealtimeDeferred)
	{
		ReleaseShadowDenoiseHistoryResources();
		return;
	}

	const RayTracedShadowDenoiserMode mode = BuildRayTracedShadowSettingsFromCVars().DenoiserMode;
	if (mode != m_lastShadowDenoiserMode)
	{
		ResetShadowDenoiseHistory();
		m_lastShadowDenoiserMode = mode;
	}

	if (mode == RayTracedShadowDenoiserMode::NrdSigma)
	{
		CreateShadowDenoiseHistoryResources(m_frameGraphSceneExtent);
	}
	else
	{
		ReleaseShadowDenoiseHistoryResources();
	}
}

void FramePipeline::BindShadowDenoiseHistoryFrameGraphResources() noexcept
{
	if (m_frameGraph == nullptr || !m_frameResources.History.HasShadowDenoiseHistory())
	{
		return;
	}

	const std::uint32_t currentFrameIndex = m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex();
	const std::uint32_t previousFrameIndex =
	    (currentFrameIndex + RhiFrameConstants::FramesInFlight - 1u) % RhiFrameConstants::FramesInFlight;
	const RhiOwnedResourceHandle previousVisibility = m_shadowDenoiseHistoryResources[previousFrameIndex];
	const RhiOwnedResourceHandle currentVisibility = m_shadowDenoiseHistoryResources[currentFrameIndex];
	if (!previousVisibility || !currentVisibility)
	{
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.PreviousDenoisedShadowVisibility);
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.CurrentDenoisedShadowVisibility);
		return;
	}

	m_frameGraph->BindPersistentTexture(
	    m_frameResources.History.PreviousDenoisedShadowVisibility,
	    previousVisibility,
	    ResourceState::ShaderResource);
	m_frameGraph->BindPersistentTexture(
	    m_frameResources.History.CurrentDenoisedShadowVisibility,
	    currentVisibility,
	    ResourceState::ShaderResource);
}

void FramePipeline::ResetShadowDenoiseHistory() noexcept
{
	ReleaseShadowDenoiseHistoryResources();
}
