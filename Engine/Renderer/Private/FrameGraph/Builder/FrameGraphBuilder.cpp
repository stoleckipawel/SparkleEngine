#include "../../PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "FrameGraph/Features/ComputeShowcasePasses.h"
#include "FrameGraph/Features/ForwardPasses.h"
#include "FrameGraph/Features/PresentationPasses.h"
#include "FrameGraph/Features/ShadowPasses.h"

#include "FrameGraph/FrameGraph.h"
#include "Window/Window.h"

FrameGraphBuilder::FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept : m_dependencies(dependencies) {}

FrameGraphBuildResult FrameGraphBuilder::Build() const
{
	auto frameGraph = std::make_unique<FrameGraph>(&m_dependencies.renderHardwareInterface, &m_dependencies.window, m_dependencies.sceneExtent);

	const FrameGraphSceneTargets sceneTargets =
	    FrameGraphFeatures::CreateSceneTargets(*frameGraph, m_dependencies.window, m_dependencies.sceneExtent);
	const FrameGraphShadowOutputs shadowOutputs = FrameGraphFeatures::AddShadowPasses(*frameGraph);
	FrameGraphFeatures::AddForwardOpaquePass(*frameGraph, sceneTargets, shadowOutputs);
	if (m_dependencies.presentSceneToBackBuffer)
	{
		FrameGraphFeatures::AddCopyToBackBufferPass(*frameGraph, sceneTargets);
	}

	FrameGraphBuildResult result{};
	result.SceneColor = sceneTargets.SceneColor;
	result.SceneDepth = sceneTargets.MainDepth;
	result.Graph = std::move(frameGraph);
	return result;
}