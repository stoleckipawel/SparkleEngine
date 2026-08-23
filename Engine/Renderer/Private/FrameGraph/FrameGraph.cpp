#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.h"
#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"

static const auto g_frameGraphLogger = Logging::GetOrCreateLogger("Renderer.FrameGraph");

FrameGraph::AllocatedParameterInstanceBase::~AllocatedParameterInstanceBase() noexcept = default;

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
	ReleaseTextureHistories();
}

ShaderAccelerationStructure FrameGraph::CreateAccelerationStructureBinding(FrameGraphAccelerationStructureHandle handle) const noexcept
{
	ShaderAccelerationStructure field;
	field = handle;
	return field;
}

const FrameGraphPlan& FrameGraph::Compile()
{
	PrepareTextureHistories(m_compiledPlan);
	SyncImportedResourceAccesses();
	BuildTransientMaterializationPlan(m_compiledPlan);
	const RhiQueueCapabilities queueCapabilities =
	    m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetCapabilities().Queues : RhiQueueCapabilities{};
	FrameGraphCompiler compiler(m_compiledPlan, m_resourceRegistry, m_resourceStateTracker, queueCapabilities);
	compiler.Compile();
	m_submissionBatchTokens.resize(m_compiledPlan.submissionBatches.size());
	return m_compiledPlan;
}

void FrameGraph::ExportTexture(FrameGraphTextureHandle handle, std::string_view name) noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!resourceHandle.IsValid() || !m_resourceRegistry.IsRegistered(resourceHandle))
	{
		return;
	}

	m_productRoots.push_back(FrameGraphProductRoot{.handle = resourceHandle, .name = std::string(name)});
}

PixelFormat FrameGraph::GetTextureFormat(FrameGraphTextureHandle handle) const noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (!resourceHandle.IsValid() || !m_resourceRegistry.IsRegistered(resourceHandle))
	{
		return PixelFormat::Unknown;
	}
	return m_resourceRegistry.GetMetadata(resourceHandle).textureDesc.format;
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
