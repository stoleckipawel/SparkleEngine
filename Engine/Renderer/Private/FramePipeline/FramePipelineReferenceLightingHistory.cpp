#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Frame/RhiFrameConstants.h"
#include "Debug/RendererCVars.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Memory/RhiMemoryTypes.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

void FramePipeline::CreateReferenceLightingHistoryResources() noexcept
{
	if (GetLightingMode() != LightingMode::ReferencePathTraced)
	{
		ReleaseReferenceLightingHistoryResources();
		return;
	}
	const RenderViewportExtent extent = m_frameGraphRenderExtent;
	if (!extent.IsValid())
	{
		return;
	}
	if (extent.Width != m_referenceLightingHistoryExtent.Width || extent.Height != m_referenceLightingHistoryExtent.Height)
	{
		ReleaseReferenceLightingHistoryResources();
	}

	RhiResourceService& resourceService = m_systems->GetRenderHardwareInterface().GetResourceService();
	const RhiTextureResourceDesc desc{
	    .Width = extent.Width,
	    .Height = extent.Height,
	    .Format = PixelFormat::R32G32B32A32_Float,
	    .MipLevels = 1u,
	    .AllowRenderTarget = false,
	    .AllowDepthStencil = false,
	    .AllowUnorderedAccess = true};
	for (RhiOwnedResourceHandle& resource : m_referenceLightingHistoryResources)
	{
		if (!resource)
		{
			resource = resourceService.CreateTextureResource(
			    desc,
			    ResourceState::Undefined,
			    RhiMemoryCategory::Texture,
			    RhiMemoryResidencyClass::DeviceLocal,
			    L"ReferenceLightingHistory");
		}
	}
	m_referenceLightingHistoryExtent = extent;
	m_referenceLightingHistoryValid = false;
}

void FramePipeline::ReleaseReferenceLightingHistoryResources() noexcept
{
	if (m_systems == nullptr)
	{
		return;
	}
	RhiResourceService& resourceService = m_systems->GetRenderHardwareInterface().GetResourceService();
	for (RhiOwnedResourceHandle& resource : m_referenceLightingHistoryResources)
	{
		if (resource)
		{
			resourceService.ReleaseOwnedResource(resource);
			resource = {};
		}
	}
	m_referenceLightingHistoryExtent = {};
	m_referenceLightingHistoryValid = false;
	m_referenceLightingStateKey = 0u;
}

void FramePipeline::BindReferenceLightingHistoryFrameGraphResources() noexcept
{
	if (m_frameGraph == nullptr || !m_frameResources.History.HasReferenceLightingHistory())
	{
		return;
	}
	const std::uint32_t currentFrameIndex = m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex();
	const std::uint32_t previousFrameIndex =
	    (currentFrameIndex + RhiFrameConstants::FramesInFlight - 1u) % RhiFrameConstants::FramesInFlight;
	const RhiOwnedResourceHandle previous = m_referenceLightingHistoryResources[previousFrameIndex];
	const RhiOwnedResourceHandle current = m_referenceLightingHistoryResources[currentFrameIndex];
	if (!previous || !current)
	{
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.PreviousReferenceLighting);
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.CurrentReferenceLighting);
		m_referenceLightingHistoryValid = false;
		return;
	}
	const ResourceState currentState = m_referenceLightingHistoryValid ? ResourceState::ShaderResource : ResourceState::Undefined;
	m_frameGraph->BindPersistentTexture(m_frameResources.History.PreviousReferenceLighting, previous, currentState);
	m_frameGraph->BindPersistentTexture(m_frameResources.History.CurrentReferenceLighting, current, currentState);
}

void FramePipeline::ResetReferenceLightingHistory() noexcept
{
	m_referenceLightingHistoryValid = false;
}

bool FramePipeline::HasReferenceLightingHistoryResources() const noexcept
{
	for (const RhiOwnedResourceHandle resource : m_referenceLightingHistoryResources)
	{
		if (!resource)
		{
			return false;
		}
	}
	return true;
}
