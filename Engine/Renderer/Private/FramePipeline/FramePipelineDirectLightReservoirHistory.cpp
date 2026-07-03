#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Frame/RhiFrameConstants.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Interop/ResourceState.h"
#include "RHI/Public/Memory/RhiMemoryTypes.h"
#include "RHI/Public/Resources/RhiResourceDesc.h"

namespace
{
	RhiTextureResourceDesc BuildDirectLightReservoirTextureDesc(RenderViewportExtent extent, PixelFormat format) noexcept
	{
		return RhiTextureResourceDesc{
		    .Width = extent.Width,
		    .Height = extent.Height,
		    .Format = format,
		    .MipLevels = 1u,
		    .AllowRenderTarget = false,
		    .AllowDepthStencil = false,
		    .AllowUnorderedAccess = true};
	}
}

void FramePipeline::CreateDirectLightReservoirHistoryResources() noexcept
{
	const RenderViewportExtent sceneExtent = m_frameGraphSceneExtent.IsValid() ? m_frameGraphSceneExtent : ResolveSceneExtent();
	if (!sceneExtent.IsValid())
	{
		return;
	}

	if (m_directLightReservoirHistoryExtent.Width != sceneExtent.Width ||
	    m_directLightReservoirHistoryExtent.Height != sceneExtent.Height)
	{
		ReleaseDirectLightReservoirHistoryResources();
		m_directLightReservoirHistoryExtent = sceneExtent;
	}

	RenderHardwareInterface& renderHardwareInterface = m_systems->GetRenderHardwareInterface();
	RhiResourceService& resourceService = renderHardwareInterface.GetResourceService();
	const RhiTextureResourceDesc sampleDesc =
	    BuildDirectLightReservoirTextureDesc(sceneExtent, PixelFormat::R32G32B32A32_Float);
	const RhiTextureResourceDesc weightDesc =
	    BuildDirectLightReservoirTextureDesc(sceneExtent, PixelFormat::R32G32B32A32_Float);
	const RhiTextureResourceDesc surfaceDesc =
	    BuildDirectLightReservoirTextureDesc(sceneExtent, PixelFormat::R16G16B16A16_Float);

	for (DirectLightReservoirHistoryFrameResources& frameResources : m_directLightReservoirHistoryResources)
	{
		if (!frameResources.Sample)
		{
			frameResources.Sample = resourceService.CreateTextureResource(
			    sampleDesc,
			    ResourceState::ShaderResource,
			    RhiMemoryCategory::Texture,
			    RhiMemoryResidencyClass::DeviceLocal,
			    L"DirectLightReservoirSampleHistory");
		}

		if (!frameResources.Weight)
		{
			frameResources.Weight = resourceService.CreateTextureResource(
			    weightDesc,
			    ResourceState::ShaderResource,
			    RhiMemoryCategory::Texture,
			    RhiMemoryResidencyClass::DeviceLocal,
			    L"DirectLightReservoirWeightHistory");
		}

		if (!frameResources.Surface)
		{
			frameResources.Surface = resourceService.CreateTextureResource(
			    surfaceDesc,
			    ResourceState::ShaderResource,
			    RhiMemoryCategory::Texture,
			    RhiMemoryResidencyClass::DeviceLocal,
			    L"DirectLightReservoirSurfaceHistory");
		}
	}

	m_directLightReservoirHistoryValid = false;
}

void FramePipeline::ReleaseDirectLightReservoirHistoryResources() noexcept
{
	if (m_systems == nullptr)
	{
		return;
	}

	RhiResourceService& resourceService = m_systems->GetRenderHardwareInterface().GetResourceService();
	for (DirectLightReservoirHistoryFrameResources& frameResources : m_directLightReservoirHistoryResources)
	{
		if (frameResources.Sample)
		{
			resourceService.ReleaseOwnedResource(frameResources.Sample);
			frameResources.Sample = {};
		}

		if (frameResources.Weight)
		{
			resourceService.ReleaseOwnedResource(frameResources.Weight);
			frameResources.Weight = {};
		}

		if (frameResources.Surface)
		{
			resourceService.ReleaseOwnedResource(frameResources.Surface);
			frameResources.Surface = {};
		}
	}

	m_directLightReservoirHistoryValid = false;
}

void FramePipeline::BindDirectLightReservoirHistoryFrameGraphResources() noexcept
{
	if (m_frameGraph == nullptr || !m_frameResources.History.HasDirectLightReservoirHistory())
	{
		return;
	}

	const std::uint32_t currentFrameIndex = m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex();
	const std::uint32_t previousFrameIndex =
	    (currentFrameIndex + RhiFrameConstants::FramesInFlight - 1u) % RhiFrameConstants::FramesInFlight;
	const DirectLightReservoirHistoryFrameResources& previousResources =
	    m_directLightReservoirHistoryResources[previousFrameIndex];
	const DirectLightReservoirHistoryFrameResources& currentResources =
	    m_directLightReservoirHistoryResources[currentFrameIndex];

	if (!previousResources.Sample ||
	    !previousResources.Weight ||
	    !previousResources.Surface ||
	    !currentResources.Sample ||
	    !currentResources.Weight ||
	    !currentResources.Surface)
	{
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.PreviousDirectLightReservoirSample);
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.PreviousDirectLightReservoirWeight);
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.PreviousDirectLightReservoirSurface);
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.CurrentDirectLightReservoirSample);
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.CurrentDirectLightReservoirWeight);
		m_frameGraph->ClearPersistentTextureBinding(m_frameResources.History.CurrentDirectLightReservoirSurface);
		m_directLightReservoirHistoryValid = false;
		return;
	}

	m_frameGraph->BindPersistentTexture(
	    m_frameResources.History.PreviousDirectLightReservoirSample,
	    previousResources.Sample,
	    ResourceState::ShaderResource);
	m_frameGraph->BindPersistentTexture(
	    m_frameResources.History.PreviousDirectLightReservoirWeight,
	    previousResources.Weight,
	    ResourceState::ShaderResource);
	m_frameGraph->BindPersistentTexture(
	    m_frameResources.History.PreviousDirectLightReservoirSurface,
	    previousResources.Surface,
	    ResourceState::ShaderResource);
	m_frameGraph->BindPersistentTexture(
	    m_frameResources.History.CurrentDirectLightReservoirSample,
	    currentResources.Sample,
	    ResourceState::ShaderResource);
	m_frameGraph->BindPersistentTexture(
	    m_frameResources.History.CurrentDirectLightReservoirWeight,
	    currentResources.Weight,
	    ResourceState::ShaderResource);
	m_frameGraph->BindPersistentTexture(
	    m_frameResources.History.CurrentDirectLightReservoirSurface,
	    currentResources.Surface,
	    ResourceState::ShaderResource);
}

void FramePipeline::ResetDirectLightReservoirHistory() noexcept
{
	m_directLightReservoirHistoryValid = false;
}

bool FramePipeline::HasDirectLightReservoirHistoryResources() const noexcept
{
	for (const DirectLightReservoirHistoryFrameResources& frameResources : m_directLightReservoirHistoryResources)
	{
		if (!frameResources.Sample || !frameResources.Weight || !frameResources.Surface)
		{
			return false;
		}
	}
	return true;
}
