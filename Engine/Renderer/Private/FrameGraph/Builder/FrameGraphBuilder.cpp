#include "../../PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "Frame/Frame.h"

#include "FrameGraph/FrameGraph.h"
#include "Window/Window.h"

FrameGraphBuilder::FrameGraphBuilder(const FrameGraphDependencies& dependencies) noexcept : m_dependencies(dependencies) {}

FrameGraphBuildResult FrameGraphBuilder::Build() const
{
	auto frameGraph =
	    std::make_unique<FrameGraph>(&m_dependencies.renderHardwareInterface, &m_dependencies.window, m_dependencies.sceneExtent);

	const FrameBuildResult frameLoop = BuildFrame(*frameGraph, m_dependencies.sceneExtent, m_dependencies.presentSceneToBackBuffer);

	FrameGraphBuildResult result{};
	result.SceneColor = frameLoop.Targets.SceneColor;
	result.SceneDepth = frameLoop.Targets.MainDepth;
	result.Graph = std::move(frameGraph);
	return result;
}