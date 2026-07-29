#include "PCH.h"

#include "FrameGraph/Builder/FrameGraphFactory.h"

#include "Frame/Core/Frame.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"

#include <string_view>
#include <utility>

FrameGraphBuildResult::FrameGraphBuildResult() noexcept = default;

FrameGraphBuildResult::~FrameGraphBuildResult() noexcept = default;

FrameGraphBuildResult::FrameGraphBuildResult(FrameGraphBuildResult&&) noexcept = default;

FrameGraphBuildResult& FrameGraphBuildResult::operator=(FrameGraphBuildResult&&) noexcept = default;

FrameGraphFactory::FrameGraphFactory(const FrameGraphDependencies& dependencies) noexcept :
	m_dependencies(dependencies)
{
}

void FrameGraphFactory::ExportTextureIfValid(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle handle,
    std::string_view name) noexcept
{
	if (handle.IsValid())
	{
		builder.ExportTexture(handle, name);
	}
}

void FrameGraphFactory::ExportFrameProductRoots(
    FrameGraphBuilder& builder,
    const FrameAssemblyResourceLayout& resources) noexcept
{
	ExportTextureIfValid(builder, resources.ViewportProducts.SceneColor, "Viewport.SceneColor");
	ExportTextureIfValid(builder, resources.ViewportProducts.FinalSceneColor, "Viewport.FinalSceneColor");
	ExportTextureIfValid(builder, resources.ViewportProducts.Exposure, "Viewport.Exposure");
	ExportTextureIfValid(builder, resources.ViewportProducts.SceneDepth, "Viewport.SceneDepth");
	ExportTextureIfValid(builder, resources.ViewportProducts.Normals, "Viewport.Normals");
	ExportTextureIfValid(builder, resources.ViewportProducts.MotionVectors, "Viewport.MotionVectors");
}

FrameGraphBuildResult FrameGraphFactory::Build() const
{
	auto frameGraph =
	    std::make_unique<FrameGraph>(&m_dependencies.renderHardwareInterface, &m_dependencies.window);

	FrameGraphBuilder builder(*frameGraph, m_dependencies.renderPassRuntimeCache);
	const FrameBuildResult frameLoop = BuildFrame(
	    builder,
	    m_dependencies.renderExtent,
	    m_dependencies.outputExtent,
	    m_dependencies.renderHardwareInterface.GetPresentationService().GetPresentColorFormat(),
	    m_dependencies.presentSceneToBackBuffer);

	ExportFrameProductRoots(builder, frameLoop.Resources);

	FrameGraphBuildResult result{};
	result.Resources = frameLoop.Resources;
	result.Graph = std::move(frameGraph);
	return result;
}
