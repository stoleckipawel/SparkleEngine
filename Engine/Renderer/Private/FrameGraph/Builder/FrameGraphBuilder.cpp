#include "PCH.h"
#include "FrameGraph/Builder/FrameGraphBuilder.h"

FrameGraphBuilder::FrameGraphBuilder(
    FrameGraph& frameGraph,
    const PipelineStateManager& pipelineStateManager) noexcept :
	m_frameGraph(frameGraph),
	m_pipelineStateManager(pipelineStateManager)
{
}

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
