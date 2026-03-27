#include "../../PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "FrameGraph/Features/ComputeShowcasePasses.h"
#include "FrameGraph/Features/ForwardPasses.h"
#include "FrameGraph/Features/PresentationPasses.h"
#include "FrameGraph/Features/ShadowPasses.h"

#include "Renderer/Public/FrameGraph/FrameGraph.h"
#include "Window.h"

FrameGraphBuilder::FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept : m_dependencies(dependencies) {}

std::unique_ptr<FrameGraph> FrameGraphBuilder::Build() const
{
	auto frameGraph = std::make_unique<FrameGraph>(
	    &m_dependencies.rhi,
	    &m_dependencies.window,
	    &m_dependencies.descriptorHeapManager,
	    &m_dependencies.swapChain);

	const FrameGraphSceneTargets sceneTargets = FrameGraphFeatures::CreateSceneTargets(*frameGraph, m_dependencies.window);
	const FrameGraphPresentationInputs presentationInputs{.BackBuffer = sceneTargets.BackBuffer};
	const FrameGraphShadowOutputs shadowOutputs = FrameGraphFeatures::AddShadowPasses(*frameGraph);
	FrameGraphFeatures::AddForwardOpaquePass(*frameGraph, sceneTargets, shadowOutputs);
	FrameGraphFeatures::AddUiCompositionPass(*frameGraph, m_dependencies.ui, presentationInputs);

	return frameGraph;
}