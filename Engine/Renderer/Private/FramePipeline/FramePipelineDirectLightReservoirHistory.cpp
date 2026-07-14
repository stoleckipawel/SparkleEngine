#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Debug/RendererCVars.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

namespace
{
	constexpr ReservoirHistoryDebugNames DirectLightReservoirNames{
	    .Sample = L"DirectLightReservoirSampleHistory",
	    .Weight = L"DirectLightReservoirWeightHistory",
	    .Surface = L"DirectLightReservoirSurfaceHistory"};
}

void FramePipeline::CreateDirectLightReservoirHistoryResources() noexcept
{
	RhiResourceService& resourceService = m_systems->GetRenderHardwareInterface().GetResourceService();
	if (GetLightingMode() != LightingMode::RestirPathTraced)
	{
		ReleaseReservoirHistoryResources(resourceService, m_directLightReservoirHistoryResources);
		m_directLightReservoirHistoryValid = false;
		m_restirLightingSceneStateKey = 0u;
		return;
	}

	const RenderViewportExtent renderExtent =
	    m_frameGraphRenderExtent.IsValid() ? m_frameGraphRenderExtent : ResolveFrameResolution().Render;
	if (!renderExtent.IsValid())
	{
		return;
	}

	EnsureReservoirHistoryResources(resourceService, renderExtent, DirectLightReservoirNames, m_directLightReservoirHistoryResources);
	m_directLightReservoirHistoryValid = false;
}

void FramePipeline::ReleaseDirectLightReservoirHistoryResources() noexcept
{
	if (m_systems != nullptr)
	{
		ReleaseReservoirHistoryResources(
		    m_systems->GetRenderHardwareInterface().GetResourceService(),
		    m_directLightReservoirHistoryResources);
	}
	m_directLightReservoirHistoryValid = false;
	m_restirLightingSceneStateKey = 0u;
}

void FramePipeline::BindDirectLightReservoirHistoryFrameGraphResources() noexcept
{
	if (m_frameGraph == nullptr || !m_frameResources.History.HasDirectLightReservoirHistory())
	{
		return;
	}

	const ReservoirHistoryFrameGraphHandles handles{
	    .PreviousSample = m_frameResources.History.PreviousDirectLightReservoirSample,
	    .PreviousWeight = m_frameResources.History.PreviousDirectLightReservoirWeight,
	    .PreviousSurface = m_frameResources.History.PreviousDirectLightReservoirSurface,
	    .CurrentSample = m_frameResources.History.CurrentDirectLightReservoirSample,
	    .CurrentWeight = m_frameResources.History.CurrentDirectLightReservoirWeight,
	    .CurrentSurface = m_frameResources.History.CurrentDirectLightReservoirSurface};
	if (!BindReservoirHistoryResources(
	        *m_frameGraph,
	        m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex(),
	        handles,
	        m_directLightReservoirHistoryResources,
	        m_directLightReservoirHistoryValid ? ResourceState::ShaderResource : ResourceState::Undefined))
	{
		m_directLightReservoirHistoryValid = false;
	}
}

void FramePipeline::ResetRestirLightingHistory() noexcept
{
	m_directLightReservoirHistoryValid = false;
	m_restirIndirectReservoirHistoryValid = false;
}

bool FramePipeline::HasDirectLightReservoirHistoryResources() const noexcept
{
	return HasReservoirHistoryResources(m_directLightReservoirHistoryResources);
}
