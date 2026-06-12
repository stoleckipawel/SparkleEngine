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

FrameGraphTextureHandle FrameGraphBuilder::ImportPersistentTexture(
    const FrameGraphTextureDesc& desc,
    NativeResourceHandle resource,
    ResourceState initialState) noexcept
{
	return m_frameGraph.ImportPersistentTexture(desc, resource, initialState);
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
	result.SceneColor = frameLoop.Scene.SceneColor;
	result.FinalSceneColor = frameLoop.Scene.FinalSceneColor;
	result.SceneDepth = frameLoop.Scene.MainDepth;
	result.MotionVectors = frameLoop.GBuffer.MotionVector;
	result.SceneTlas = frameLoop.SceneTlas;
	result.Graph = std::move(frameGraph);
	return result;
}
