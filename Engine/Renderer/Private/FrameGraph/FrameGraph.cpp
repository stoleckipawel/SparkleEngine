#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.h"
#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"

static const auto g_frameGraphLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

FrameGraph::FrameGraph(RenderHardwareInterface* renderHardwareInterface, Window* window) :
    m_renderHardwareInterface(renderHardwareInterface),
    m_window(window),
    m_transientAllocator(
        renderHardwareInterface != nullptr ? std::make_unique<FrameGraphTransientAllocator>(*renderHardwareInterface) : nullptr)
{
}

FrameGraph::~FrameGraph()
{
	if (m_transientAllocator != nullptr)
	{
		m_transientAllocator->Reset();
	}

	ReleaseExternalResourceViews();
}

FrameGraphPlan FrameGraph::Compile()
{
	SyncImportedResourceAccesses();
	BuildTransientMaterializationPlan(m_compiledPlan);
	FrameGraphCompiler compiler(m_compiledPlan, m_resourceRegistry, m_resourceStateTracker);
	compiler.Compile();
	return m_compiledPlan;
}

void FrameGraph::ExportTexture(FrameGraphTextureHandle handle, std::string_view name) noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!resourceHandle.IsValid() || !m_resourceRegistry.IsRegistered(resourceHandle))
	{
		return;
	}

	m_productRoots.push_back(
	    FrameGraphProductRoot{
	        .handle = resourceHandle,
	        .name = std::string(name)});
}

ResourceState FrameGraph::GetTrackedResourceState(FrameGraphResourceHandle handle) const noexcept
{
	if (!handle.IsValid() || !m_resourceRegistry.IsRegistered(handle))
	{
		return ResourceState::Common;
	}

	return m_resourceStateTracker.GetRuntimeState(handle);
}

void FrameGraph::UpdateTrackedResourceState(FrameGraphResourceHandle handle, ResourceState currentState) const noexcept
{
	if (!handle.IsValid() || !m_resourceRegistry.IsRegistered(handle))
	{
		return;
	}

	m_resourceStateTracker.UpdateCurrentState(handle, currentState);
}
