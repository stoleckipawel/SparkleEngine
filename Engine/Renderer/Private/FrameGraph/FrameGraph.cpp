#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.h"
#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "RHI/Public/Interop/RenderHardwareInterface.h"
#include "Window/Window.h"

#include "Core/Public/Diagnostics/Log.h"

FrameGraph::FrameGraph(RenderHardwareInterface* renderHardwareInterface, Window* window, RenderViewportExtent sceneExtent) :
	m_renderHardwareInterface(renderHardwareInterface),
	m_window(window),
	m_sceneExtent(sceneExtent),
	m_transientAllocator(renderHardwareInterface != nullptr ? std::make_unique<FrameGraphTransientAllocator>(*renderHardwareInterface)
															: nullptr)
{
	LOG_INFO("FrameGraph created");
}

FrameGraph::~FrameGraph()
{
	if (m_transientAllocator != nullptr)
	{
		m_transientAllocator->Reset();
	}

	ReleaseExternalViewDescriptors();
	LOG_INFO("FrameGraph destroyed");
}

FrameGraph::CompiledPlan FrameGraph::Compile()
{
	SyncImportedResourceAccesses();
	BuildTransientMaterializationPlan(m_compiledPlan);
	FrameGraphCompiler compiler(m_compiledPlan, m_resourceRegistry);
	compiler.Compile();
	return m_compiledPlan;
}
