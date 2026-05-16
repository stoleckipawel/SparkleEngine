#include "../../PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "Frame/Frame.h"

#include "Window/Window.h"

FrameGraphBuilder::FrameGraphBuilder(FrameGraph& frameGraph) noexcept : m_frameGraph(frameGraph) {}

FrameGraphTextureHandle FrameGraphBuilder::ImportTexture(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept
{
	return m_frameGraph.ImportTexture(desc, initialState);
}

FrameGraphTextureHandle FrameGraphBuilder::ImportTexture(
    const FrameGraphTextureDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ImportTexture(desc, resource, initialState);
}

FrameGraphTextureHandle FrameGraphBuilder::CreateTexture(const FrameGraphTextureDesc& desc) noexcept
{
	return m_frameGraph.CreateTexture(desc);
}

FrameGraphBufferHandle FrameGraphBuilder::ImportBuffer(
    const FrameGraphBufferDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ImportBuffer(desc, resource, initialState);
}

FrameGraphBufferHandle FrameGraphBuilder::CreateBuffer(const FrameGraphBufferDesc& desc) noexcept
{
	return m_frameGraph.CreateBuffer(desc);
}

ShaderRenderTarget FrameGraphBuilder::CreateRenderTarget(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph.CreateRenderTarget(handle);
}

ShaderDepthTarget FrameGraphBuilder::CreateDepthTarget(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph.CreateDepthTarget(handle);
}

FrameGraphFactory::FrameGraphFactory(const FrameGraphDependencies& dependencies) noexcept : m_dependencies(dependencies) {}

FrameGraphBuildResult FrameGraphFactory::Build() const
{
	auto frameGraph =
	    std::make_unique<FrameGraph>(&m_dependencies.renderHardwareInterface, &m_dependencies.window, m_dependencies.sceneExtent);

	FrameGraphBuilder builder(*frameGraph);
	const FrameBuildResult frameLoop = BuildFrame(builder, m_dependencies.sceneExtent, m_dependencies.presentSceneToBackBuffer);

	FrameGraphBuildResult result{};
	result.SceneColor = frameLoop.Targets.SceneColor;
	result.SceneDepth = frameLoop.Targets.MainDepth;
	result.Graph = std::move(frameGraph);
	return result;
}
