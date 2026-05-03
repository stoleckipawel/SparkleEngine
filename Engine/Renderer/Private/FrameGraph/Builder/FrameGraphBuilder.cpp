#include "../../PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "FrameGraph/Features/ComputeShowcasePasses.h"
#include "FrameGraph/Features/GBufferPasses.h"
#include "FrameGraph/Features/PresentationPasses.h"

#include "FrameGraph/FrameGraph.h"
#include "Passes/DeferredLightingPass.h"
#include "Window/Window.h"

FrameGraphBuilder::FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept : m_dependencies(dependencies) {}

FrameGraphBuildResult FrameGraphBuilder::Build() const
{
	auto frameGraph =
	    std::make_unique<FrameGraph>(&m_dependencies.renderHardwareInterface, &m_dependencies.window, m_dependencies.sceneExtent);

	const FrameGraphSceneTargets sceneTargets =
	    FrameGraphFeatures::CreateSceneTargets(*frameGraph, m_dependencies.window, m_dependencies.sceneExtent);
	const FrameGraphGBufferTargets gBufferTargets =
	    FrameGraphFeatures::AddGBufferPass(*frameGraph, m_dependencies.window, m_dependencies.sceneExtent, sceneTargets);

	auto& deferredLightingParameters = frameGraph->AllocPassParameters<DeferredLightingPass>();
	deferredLightingParameters->SceneColor = frameGraph->CreateUAV(sceneTargets.SceneColor);
	deferredLightingParameters->GBufferBaseColor = frameGraph->CreateSRV(gBufferTargets.BaseColor);
	deferredLightingParameters->GBufferNormal = frameGraph->CreateSRV(gBufferTargets.Normal);
	deferredLightingParameters->GBufferMaterial = frameGraph->CreateSRV(gBufferTargets.Material);
	deferredLightingParameters->GBufferEmissive = frameGraph->CreateSRV(gBufferTargets.Emissive);
	frameGraph->AddComputePass<DeferredLightingPass>(DeferredLightingPass::PassName, deferredLightingParameters);

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