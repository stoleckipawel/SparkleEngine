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

FrameGraphTextureHandle FrameGraphBuilder::ImportPersistentTexture(
    const FrameGraphTextureDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ImportPersistentTexture(desc, resource, initialState);
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

FrameGraphBufferHandle FrameGraphBuilder::ImportBuffer(
    const FrameGraphBufferDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ImportBuffer(desc, resource, initialState);
}

FrameGraphBufferHandle FrameGraphBuilder::ImportPersistentBuffer(
    const FrameGraphBufferDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ImportPersistentBuffer(desc, resource, initialState);
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

FrameGraphAccelerationStructureHandle FrameGraphBuilder::ImportAccelerationStructure(
    const FrameGraphAccelerationStructureDesc& desc,
    NativeResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ImportAccelerationStructure(desc, resource, gpuAddress, initialState);
}

FrameGraphAccelerationStructureHandle FrameGraphBuilder::ImportPersistentAccelerationStructure(
    const FrameGraphAccelerationStructureDesc& desc,
    NativeResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ImportPersistentAccelerationStructure(desc, resource, gpuAddress, initialState);
}

FrameGraphAccelerationStructureHandle FrameGraphBuilder::ReservePersistentAccelerationStructure(
    const FrameGraphAccelerationStructureDesc& desc,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ReservePersistentAccelerationStructure(desc, initialState);
}

void FrameGraphBuilder::BindPersistentAccelerationStructure(
    FrameGraphAccelerationStructureHandle handle,
    NativeResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState currentState) noexcept
{
	m_frameGraph.BindPersistentAccelerationStructure(handle, resource, gpuAddress, currentState);
}

void FrameGraphBuilder::BindPersistentAccelerationStructure(
    FrameGraphAccelerationStructureHandle handle,
    RhiOwnedResourceHandle resource,
    RhiGpuVirtualAddress gpuAddress,
    ResourceState currentState) noexcept
{
	m_frameGraph.BindPersistentAccelerationStructure(handle, resource, gpuAddress, currentState);
}

void FrameGraphBuilder::ClearPersistentAccelerationStructureBinding(FrameGraphAccelerationStructureHandle handle) noexcept
{
	m_frameGraph.ClearPersistentAccelerationStructureBinding(handle);
}

void FrameGraphBuilder::BindPersistentTexture(
    FrameGraphTextureHandle handle,
    NativeResourceHandle resource,
    ResourceState currentState) noexcept
{
	m_frameGraph.BindPersistentTexture(handle, resource, currentState);
}

void FrameGraphBuilder::BindPersistentTexture(
    FrameGraphTextureHandle handle,
    RhiOwnedResourceHandle resource,
    ResourceState currentState) noexcept
{
	m_frameGraph.BindPersistentTexture(handle, resource, currentState);
}

void FrameGraphBuilder::ClearPersistentTextureBinding(FrameGraphTextureHandle handle) noexcept
{
	m_frameGraph.ClearPersistentTextureBinding(handle);
}

void FrameGraphBuilder::BindPersistentBuffer(
    FrameGraphBufferHandle handle,
    NativeResourceHandle resource,
    ResourceState currentState) noexcept
{
	m_frameGraph.BindPersistentBuffer(handle, resource, currentState);
}

void FrameGraphBuilder::BindPersistentBuffer(
    FrameGraphBufferHandle handle,
    RhiOwnedResourceHandle resource,
    ResourceState currentState) noexcept
{
	m_frameGraph.BindPersistentBuffer(handle, resource, currentState);
}

void FrameGraphBuilder::ClearPersistentBufferBinding(FrameGraphBufferHandle handle) noexcept
{
	m_frameGraph.ClearPersistentBufferBinding(handle);
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
