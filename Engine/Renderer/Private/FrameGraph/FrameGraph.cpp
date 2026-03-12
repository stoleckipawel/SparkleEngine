#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Renderer/Public/RenderContext.h"
#include "Renderer/Public/SceneData/SceneView.h"

#include "Core/Public/Diagnostics/Log.h"

FrameGraph::FrameGraph(D3D12SwapChain* swapChain, D3D12DepthStencil* depthStencil) : m_swapChain(swapChain), m_depthStencil(depthStencil)
{
	LOG_INFO("FrameGraph created");
}

FrameGraph::~FrameGraph()
{
	LOG_INFO("FrameGraph destroyed");
}

void FrameGraph::Setup(const SceneView& sceneView)
{
	for (auto& pass : m_passes)
	{
		pass->Setup(m_builder, sceneView);
	}
}

void FrameGraph::Compile()
{
}

void FrameGraph::Execute(RenderContext& context)
{
	for (auto& pass : m_passes)
	{
		pass->Execute(context);
	}
}
