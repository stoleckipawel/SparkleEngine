#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Frame/RhiFrameConstants.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Memory/RhiMemoryTypes.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

void FramePipeline::CreateExposureHistoryResources() noexcept
{
	RenderHardwareInterface& renderHardwareInterface = m_systems->GetRenderHardwareInterface();
	RhiResourceService& resourceService = renderHardwareInterface.GetResourceService();
	const RhiTextureResourceDesc historyDesc{
	    .Width = 1u,
	    .Height = 1u,
	    .Format = PixelFormat::R32G32B32A32_Float,
	    .MipLevels = 1u,
	    .AllowRenderTarget = false,
	    .AllowDepthStencil = false,
	    .AllowUnorderedAccess = true};

	for (RhiOwnedResourceHandle& historyResource : m_exposureHistoryResources)
	{
		if (historyResource)
		{
			continue;
		}

		historyResource = resourceService.CreateTextureResource(
		    historyDesc,
		    ResourceState::Undefined,
		    RhiMemoryCategory::Texture,
		    RhiMemoryResidencyClass::DeviceLocal,
		    L"ExposureHistory");
	}

	m_exposureHistoryValid = false;
}

void FramePipeline::ReleaseExposureHistoryResources() noexcept
{
	if (m_systems == nullptr)
	{
		return;
	}

	RhiResourceService& resourceService = m_systems->GetRenderHardwareInterface().GetResourceService();
	for (RhiOwnedResourceHandle& historyResource : m_exposureHistoryResources)
	{
		if (historyResource)
		{
			resourceService.ReleaseOwnedResource(historyResource);
			historyResource = {};
		}
	}
	m_exposureHistoryValid = false;
}

void FramePipeline::BindExposureHistoryFrameGraphResources() noexcept
{
	if (m_frameGraph == nullptr || !m_frameResources.History.HasExposureHistory())
	{
		return;
	}

	const std::uint32_t currentFrameIndex = m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex();
	const std::uint32_t previousFrameIndex =
	    (currentFrameIndex + RhiFrameConstants::FramesInFlight - 1u) % RhiFrameConstants::FramesInFlight;
	const RhiOwnedResourceHandle previousExposure = m_exposureHistoryResources[previousFrameIndex];
	const RhiOwnedResourceHandle currentExposure = m_exposureHistoryResources[currentFrameIndex];
	if (!previousExposure || !currentExposure)
	{
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.PreviousExposure);
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.CurrentExposure);
		m_exposureHistoryValid = false;
		return;
	}

	const ResourceState currentState = m_exposureHistoryValid ? ResourceState::ShaderResource : ResourceState::Undefined;
	m_frameGraph->BindPersistentTexture(m_frameResources.History.PreviousExposure, previousExposure, currentState);
	m_frameGraph->BindPersistentTexture(m_frameResources.History.CurrentExposure, currentExposure, currentState);
}

void FramePipeline::ResetExposureHistory() noexcept
{
	m_exposureHistoryValid = false;
}

bool FramePipeline::HasExposureHistoryResources() const noexcept
{
	for (const RhiOwnedResourceHandle historyResource : m_exposureHistoryResources)
	{
		if (!historyResource)
		{
			return false;
		}
	}
	return true;
}
