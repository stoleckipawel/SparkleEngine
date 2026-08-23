#include "PCH.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"

#include "FrameGraph/FrameGraph.h"
#include "Commands/RenderCommandContext.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Interop/RhiInteropService.h"

FrameGraphResourceCommands::FrameGraphResourceCommands(const FrameGraph& frameGraph) noexcept :
    m_frameGraph(frameGraph)
{
}

void FrameGraphResourceCommands::BindRenderTarget(
    RenderCommandContext& commandContext,
    FrameGraphTextureHandle renderTargetHandle,
    FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	m_frameGraph.BindRenderTarget(commandContext, renderTargetHandle, depthStencilHandle);
}

void FrameGraphResourceCommands::BindRenderTargets(
    RenderCommandContext& commandContext,
    std::span<const FrameGraphTextureHandle> renderTargetHandles,
    FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	m_frameGraph.BindRenderTargets(commandContext, renderTargetHandles, depthStencilHandle);
}

void FrameGraphResourceCommands::CopyTexture(
    RenderCommandContext& commandContext,
    FrameGraphTextureHandle destinationHandle,
    FrameGraphTextureHandle sourceHandle) const noexcept
{
	m_frameGraph.CopyTexture(commandContext, destinationHandle, sourceHandle);
}

void FrameGraphResourceCommands::CopyBuffer(
    RenderCommandContext& commandContext,
    FrameGraphBufferHandle destinationHandle,
    FrameGraphBufferHandle sourceHandle) const noexcept
{
	m_frameGraph.CopyBuffer(commandContext, destinationHandle, sourceHandle);
}

void FrameGraphResourceCommands::ClearRenderTarget(RenderCommandContext& commandContext, FrameGraphTextureHandle handle) const noexcept
{
	m_frameGraph.ClearRenderTarget(commandContext, handle);
}

void FrameGraphResourceCommands::ClearDepthStencil(RenderCommandContext& commandContext, FrameGraphTextureHandle handle) const noexcept
{
	m_frameGraph.ClearDepthStencil(commandContext, handle);
}

RhiResourceHandle FrameGraphResourceCommands::ResolveResource(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph.ResolveResource(handle);
}

NativeTextureViewInfo FrameGraphResourceCommands::ResolveNativeTextureView(
    FrameGraphTextureHandle handle,
    ResourceState state,
    const RhiNativeInteropRequest& request) const noexcept
{
	return m_frameGraph.ResolveNativeTextureView(handle, state, request);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph.ResolveShaderResourceView(handle);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept
{
	return m_frameGraph.ResolveShaderResourceView(handle);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph.ResolveUnorderedAccessView(handle);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept
{
	return m_frameGraph.ResolveUnorderedAccessView(handle);
}

RhiResourceHandle FrameGraphResourceCommands::ResolveAccelerationStructure(
    FrameGraphAccelerationStructureHandle handle) const noexcept
{
	return m_frameGraph.ResolveAccelerationStructure(handle);
}

void FrameGraphResourceCommands::BindGlobalDescriptorState(RenderCommandContext& commandContext) const noexcept
{
	GetRenderHardwareInterface().GetDescriptorService().BindGlobalDescriptorState(commandContext.GetRenderCommandList());
}

RenderHardwareInterface& FrameGraphResourceCommands::GetRenderHardwareInterface() const noexcept
{
	return *m_frameGraph.m_renderHardwareInterface;
}
