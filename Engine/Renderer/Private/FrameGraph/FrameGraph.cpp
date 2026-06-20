#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.h"
#include "Renderer/Private/FrameGraph/Diagnostics/FrameGraphPlanDiagnostics.h"
#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"

static const auto g_frameGraphLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

FrameGraph::FrameGraph(RenderHardwareInterface* renderHardwareInterface, Window* window, RenderViewportExtent sceneExtent) :
    m_renderHardwareInterface(renderHardwareInterface),
    m_window(window),
    m_sceneExtent(sceneExtent),
    m_transientAllocator(
        renderHardwareInterface != nullptr ? std::make_unique<FrameGraphTransientAllocator>(*renderHardwareInterface) : nullptr)
{
	SPDLOG_LOGGER_INFO(g_frameGraphLogger, "FrameGraph created");
}

FrameGraph::~FrameGraph()
{
	if (m_transientAllocator != nullptr)
	{
		m_transientAllocator->Reset();
	}

	ReleaseExternalResourceViews();
	SPDLOG_LOGGER_INFO(g_frameGraphLogger, "FrameGraph destroyed");
}

FrameGraphPlan FrameGraph::Compile()
{
	SyncImportedResourceAccesses();
	BuildTransientMaterializationPlan(m_compiledPlan);
	FrameGraphCompiler compiler(m_compiledPlan, m_resourceRegistry, m_resourceStateTracker);
	compiler.Compile();
	FrameGraphPlanDiagnostics::LogIfEnabled(m_compiledPlan);
	return m_compiledPlan;
}

std::uint32_t FrameGraph::GetCompiledTransientResourceCount() const noexcept
{
	return static_cast<std::uint32_t>(std::count_if(
	    m_compiledPlan.resources.begin(),
	    m_compiledPlan.resources.end(),
	    [](const FrameGraphResourceNode& resource) noexcept
	    {
		    return resource.ownership == FrameGraphResourceOwnership::Transient;
	    }));
}

std::uint32_t FrameGraph::GetCompiledImportedResourceCount() const noexcept
{
	return static_cast<std::uint32_t>(std::count_if(
	    m_compiledPlan.resources.begin(),
	    m_compiledPlan.resources.end(),
	    [](const FrameGraphResourceNode& resource) noexcept
	    {
		    return resource.ownership == FrameGraphResourceOwnership::Imported;
	    }));
}

std::uint32_t FrameGraph::GetCompiledPersistentResourceCount() const noexcept
{
	return static_cast<std::uint32_t>(std::count_if(
	    m_compiledPlan.resources.begin(),
	    m_compiledPlan.resources.end(),
	    [](const FrameGraphResourceNode& resource) noexcept
	    {
		    return resource.ownership == FrameGraphResourceOwnership::ExternalPersistent;
	    }));
}

ResourceState FrameGraph::GetTrackedResourceState(FrameGraphResourceHandle handle) const noexcept
{
	if (!handle.IsValid() || !m_resourceRegistry.IsRegistered(handle))
	{
		return ResourceState::Common;
	}

	return m_resourceStateTracker.GetRuntimeState(handle).currentState;
}

void FrameGraph::UpdateTrackedResourceState(FrameGraphResourceHandle handle, ResourceState currentState) const noexcept
{
	if (!handle.IsValid() || !m_resourceRegistry.IsRegistered(handle))
	{
		return;
	}

	m_resourceStateTracker.UpdateCurrentState(handle, currentState);
}
