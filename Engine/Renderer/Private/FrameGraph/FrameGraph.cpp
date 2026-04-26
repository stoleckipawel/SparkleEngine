#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.h"
#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"
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

	ReleaseExternalViewDescriptors();
	SPDLOG_LOGGER_INFO(g_frameGraphLogger, "FrameGraph destroyed");
}

FrameGraph::CompiledPlan FrameGraph::Compile()
{
	SyncImportedResourceAccesses();
	BuildTransientMaterializationPlan(m_compiledPlan);
	FrameGraphCompiler compiler(m_compiledPlan, m_resourceRegistry);
	compiler.Compile();
	return m_compiledPlan;
}

void FrameGraph::UpdateTrackedResourceState(ResourceHandle handle, ResourceState currentState) const noexcept
{
	if (!handle.IsValid() || !m_resourceRegistry.IsRegistered(handle))
	{
		return;
	}

	m_resourceRegistry.UpdateCurrentState(handle, currentState);
}
