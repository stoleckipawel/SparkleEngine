#include "PCH.h"

#include "Frame/Graph/RenderFrameGraphFactory.h"

#include "Frame/Graph/BuildRenderFrameGraph.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"
#include "FrameGraph/FrameGraph.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"

#include <string_view>
#include <utility>

RenderFrameGraphBuildResult::RenderFrameGraphBuildResult() noexcept = default;

RenderFrameGraphBuildResult::~RenderFrameGraphBuildResult() noexcept = default;

RenderFrameGraphBuildResult::RenderFrameGraphBuildResult(RenderFrameGraphBuildResult&&) noexcept = default;

RenderFrameGraphBuildResult& RenderFrameGraphBuildResult::operator=(RenderFrameGraphBuildResult&&) noexcept = default;

RenderFrameGraphFactory::RenderFrameGraphFactory(const RenderFrameGraphDependencies& dependencies) noexcept :
    m_dependencies(dependencies)
{
}

void RenderFrameGraphFactory::ExportTextureIfValid(
    FrameGraphBuilder& builder,
    FrameGraphTextureHandle handle,
    std::string_view name) noexcept
{
	if (handle.IsValid())
	{
		builder.ExportTexture(handle, name);
	}
}

void RenderFrameGraphFactory::ExportFrameProductRoots(
    FrameGraphBuilder& builder,
    const RenderFrameGraphSettings& settings,
    const RenderFrameGraphResources& resources) noexcept
{
	ExportTextureIfValid(builder, resources.ViewportProducts.FinalSceneColor, "Viewport.FinalSceneColor");
	if (HasAnyRenderOutputFlags(settings.RequestedOutputs, RenderOutputFlags::SceneDepth))
	{
		ExportTextureIfValid(builder, resources.ViewportProducts.SceneDepth, "Viewport.SceneDepth");
	}
	if (HasAnyRenderOutputFlags(settings.RequestedOutputs, RenderOutputFlags::Normals))
	{
		ExportTextureIfValid(builder, resources.ViewportProducts.Normals, "Viewport.Normals");
	}
}

RenderFrameGraphBuildResult RenderFrameGraphFactory::Build() const
{
	auto frameGraph = std::make_unique<FrameGraph>(&m_dependencies.renderHardwareInterface, &m_dependencies.window);

	FrameGraphBuilder builder(*frameGraph, m_dependencies.renderPassRuntimeCache);
	RenderFrameGraphResources resources = BuildRenderFrameGraph(
	    builder,
	    m_dependencies.settings,
	    m_dependencies.gpuMeshCache,
	    m_dependencies.rayTracingScene,
	    m_dependencies.upscalerProvider,
	    m_dependencies.rayReconstructionProvider);

	ExportFrameProductRoots(builder, m_dependencies.settings, resources);

	RenderFrameGraphBuildResult result{};
	result.Resources = std::move(resources);
	result.Graph = std::move(frameGraph);
	return result;
}
