#include "PCH.h"
#include "FramePipeline/FramePipeline.h"

#include "Debug/RendererCVars.h"
#include "FrameGraph/FrameGraph.h"
#include "Host/RendererSystemRoot.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

namespace
{
	constexpr ReservoirHistoryDebugNames RestirIndirectReservoirNames{
	    .Sample = L"RestirIndirectReservoirSampleHistory",
	    .Weight = L"RestirIndirectReservoirWeightHistory",
	    .Surface = L"RestirIndirectReservoirSurfaceHistory"};
}

void FramePipeline::CreateRestirIndirectReservoirHistoryResources() noexcept
{
	RhiResourceService& resourceService = m_systems->GetRenderHardwareInterface().GetResourceService();
	if (GetLightingMode() != LightingMode::RestirPathTraced)
	{
		ReleaseReservoirHistoryResources(resourceService, m_restirIndirectReservoirHistoryResources);
		m_restirIndirectReservoirHistoryValid = false;
		return;
	}
	if (!m_frameGraphRenderExtent.IsValid())
	{
		return;
	}

	EnsureReservoirHistoryResources(
	    resourceService,
	    m_frameGraphRenderExtent,
	    RestirIndirectReservoirNames,
	    m_restirIndirectReservoirHistoryResources);
	m_restirIndirectReservoirHistoryValid = false;
}

void FramePipeline::ReleaseRestirIndirectReservoirHistoryResources() noexcept
{
	if (m_systems != nullptr)
	{
		ReleaseReservoirHistoryResources(
		    m_systems->GetRenderHardwareInterface().GetResourceService(),
		    m_restirIndirectReservoirHistoryResources);
	}
	m_restirIndirectReservoirHistoryValid = false;
}

void FramePipeline::BindRestirIndirectReservoirHistoryFrameGraphResources() noexcept
{
	if (m_frameGraph == nullptr || !m_frameResources.History.HasRestirIndirectReservoirHistory())
	{
		return;
	}

	const ReservoirHistoryFrameGraphHandles handles{
	    .PreviousSample = m_frameResources.History.PreviousRestirIndirectReservoirSample,
	    .PreviousWeight = m_frameResources.History.PreviousRestirIndirectReservoirWeight,
	    .PreviousSurface = m_frameResources.History.PreviousRestirIndirectReservoirSurface,
	    .CurrentSample = m_frameResources.History.CurrentRestirIndirectReservoirSample,
	    .CurrentWeight = m_frameResources.History.CurrentRestirIndirectReservoirWeight,
	    .CurrentSurface = m_frameResources.History.CurrentRestirIndirectReservoirSurface};
	if (!BindReservoirHistoryResources(
	        *m_frameGraph,
	        m_systems->GetRenderHardwareInterface().GetCurrentFrameIndex(),
	        handles,
	        m_restirIndirectReservoirHistoryResources,
	        m_restirIndirectReservoirHistoryValid ? ResourceState::ShaderResource : ResourceState::Undefined))
	{
		m_restirIndirectReservoirHistoryValid = false;
	}
}

bool FramePipeline::HasRestirIndirectReservoirHistoryResources() const noexcept
{
	return HasReservoirHistoryResources(m_restirIndirectReservoirHistoryResources);
}
