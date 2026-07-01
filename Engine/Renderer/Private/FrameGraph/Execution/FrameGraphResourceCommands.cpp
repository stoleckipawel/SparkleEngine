#include "PCH.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"

#include "FrameGraph/FrameGraph.h"

FrameGraphResourceCommands::FrameGraphResourceCommands(const FrameGraph& frameGraph) noexcept : m_frameGraph(&frameGraph)
{
}

void FrameGraphResourceCommands::BindRenderTarget(
    RenderCommandContext& cmd,
    FrameGraphTextureHandle renderTargetHandle,
    FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	m_frameGraph->BindRenderTarget(cmd, renderTargetHandle, depthStencilHandle);
}

void FrameGraphResourceCommands::BindRenderTargets(
    RenderCommandContext& cmd,
    std::span<const FrameGraphTextureHandle> renderTargetHandles,
    FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	m_frameGraph->BindRenderTargets(cmd, renderTargetHandles, depthStencilHandle);
}

void FrameGraphResourceCommands::CopyTexture(
    RenderCommandContext& cmd,
    FrameGraphTextureHandle destinationHandle,
    FrameGraphTextureHandle sourceHandle) const noexcept
{
	m_frameGraph->CopyTexture(cmd, destinationHandle, sourceHandle);
}

void FrameGraphResourceCommands::CopyBuffer(
    RenderCommandContext& cmd,
    FrameGraphBufferHandle destinationHandle,
    FrameGraphBufferHandle sourceHandle) const noexcept
{
	m_frameGraph->CopyBuffer(cmd, destinationHandle, sourceHandle);
}

void FrameGraphResourceCommands::ClearRenderTarget(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept
{
	m_frameGraph->ClearRenderTarget(cmd, handle);
}

void FrameGraphResourceCommands::ClearDepthStencil(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept
{
	m_frameGraph->ClearDepthStencil(cmd, handle);
}

NativeResourceHandle FrameGraphResourceCommands::ResolveResource(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph->ResolveResource(handle);
}

NativeTextureViewInfo FrameGraphResourceCommands::ResolveNativeTextureView(FrameGraphTextureHandle handle, ResourceState state) const noexcept
{
	return m_frameGraph->ResolveNativeTextureView(handle, state);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph->ResolveShaderResourceView(handle);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept
{
	return m_frameGraph->ResolveShaderResourceView(handle);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph->ResolveUnorderedAccessView(handle);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept
{
	return m_frameGraph->ResolveUnorderedAccessView(handle);
}

RhiGpuVirtualAddress FrameGraphResourceCommands::ResolveAccelerationStructureGpuAddress(
    FrameGraphAccelerationStructureHandle handle) const noexcept
{
	return m_frameGraph->ResolveAccelerationStructureGpuAddress(handle);
}
