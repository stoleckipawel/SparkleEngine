#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Compiler/FrameGraphCompiler.h"
#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "D3D12DescriptorHeapManager.h"
#include "D3D12SwapChain.h"
#include "Window.h"

#include "Core/Public/Diagnostics/Log.h"

FrameGraph::FrameGraph(D3D12Rhi* rhi, Window* window, D3D12DescriptorHeapManager* descriptorHeapManager, D3D12SwapChain* swapChain) :
	m_rhi(rhi),
	m_window(window),
	m_descriptorHeapManager(descriptorHeapManager),
	m_swapChain(swapChain),
	m_transientAllocator(
	    rhi != nullptr && descriptorHeapManager != nullptr ? std::make_unique<FrameGraphTransientAllocator>(*rhi, *descriptorHeapManager) : nullptr)
{
	LOG_INFO("FrameGraph created");
}

FrameGraph::~FrameGraph()
{
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
