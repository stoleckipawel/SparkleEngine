#include "PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

#include "Frame/Core/Frame.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "Window/Window.h"

#include <string_view>

namespace
{
	void ExportTextureIfValid(FrameGraphBuilder& builder, FrameGraphTextureHandle handle, std::string_view name) noexcept
	{
		if (handle.IsValid())
		{
			builder.ExportTexture(handle, name);
		}
	}

	void ExportFrameProductRoots(FrameGraphBuilder& builder, const FrameAssemblyResourceLayout& resources) noexcept
	{
		ExportTextureIfValid(builder, resources.ViewportProducts.SceneColor, "Viewport.SceneColor");
		ExportTextureIfValid(builder, resources.ViewportProducts.FinalSceneColor, "Viewport.FinalSceneColor");
		ExportTextureIfValid(builder, resources.ViewportProducts.Exposure, "Viewport.Exposure");
		ExportTextureIfValid(builder, resources.ViewportProducts.SceneDepth, "Viewport.SceneDepth");
		ExportTextureIfValid(builder, resources.ViewportProducts.Normals, "Viewport.Normals");
		ExportTextureIfValid(builder, resources.ViewportProducts.MotionVectors, "Viewport.MotionVectors");
	}
}  // namespace

FrameGraphBuilder::FrameGraphBuilder(FrameGraph& frameGraph) noexcept : m_frameGraph(frameGraph) {}

FrameGraphTextureHandle FrameGraphBuilder::ImportBackBuffer(const FrameGraphTextureDesc& desc, ResourceState initialState) noexcept
{
	return m_frameGraph.ImportBackBuffer(desc, initialState);
}

FrameGraphTextureHandle FrameGraphBuilder::ReservePersistentTexture(
    const FrameGraphTextureDesc& desc,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ReservePersistentTexture(desc, initialState);
}

FrameGraphTextureHandle FrameGraphBuilder::CreateTexture(const FrameGraphTextureDesc& desc) noexcept
{
	return m_frameGraph.CreateTexture(desc);
}

FrameGraphTextureHistory FrameGraphBuilder::CreateTextureHistory(const FrameGraphTextureDesc& desc) noexcept
{
	return m_frameGraph.CreateTextureHistory(desc);
}

FrameGraphBufferHandle FrameGraphBuilder::ReservePersistentBuffer(
    const FrameGraphBufferDesc& desc,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ReservePersistentBuffer(desc, initialState);
}

FrameGraphBufferHandle FrameGraphBuilder::CreateBuffer(const FrameGraphBufferDesc& desc) noexcept
{
	return m_frameGraph.CreateBuffer(desc);
}

FrameGraphAccelerationStructureHandle FrameGraphBuilder::ReservePersistentAccelerationStructure(
    const FrameGraphAccelerationStructureDesc& desc,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ReservePersistentAccelerationStructure(desc, initialState);
}

void FrameGraphBuilder::ExportTexture(FrameGraphTextureHandle handle, std::string_view name) noexcept
{
	m_frameGraph.ExportTexture(handle, name);
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
	    std::make_unique<FrameGraph>(&m_dependencies.renderHardwareInterface, &m_dependencies.window);

	FrameGraphBuilder builder(*frameGraph);
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
