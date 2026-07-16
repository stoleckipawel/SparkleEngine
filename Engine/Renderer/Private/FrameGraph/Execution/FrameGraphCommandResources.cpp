#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Commands/RenderCommandContext.h"

#include <array>
#include <cassert>

namespace
{
	struct FrameGraphFramebuffer final
	{
		std::array<RhiCpuDescriptorHandle, 8> renderTargetViews = {};
		std::uint32_t renderTargetCount = 0;
		RhiCpuDescriptorHandle depthStencilView = {};

		bool HasDepthStencil() const noexcept { return static_cast<bool>(depthStencilView); }
	};
}

void FrameGraph::BindRenderTarget(
    RenderCommandContext& cmd,
    FrameGraphTextureHandle renderTargetHandle,
    FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	FrameGraphFramebuffer framebuffer{};
	framebuffer.renderTargetViews[0] = ResolveRenderTargetView(renderTargetHandle.GetResourceHandle());
	framebuffer.renderTargetCount = 1u;
	if (depthStencilHandle.IsValid())
	{
		framebuffer.depthStencilView = ResolveDepthStencilView(depthStencilHandle.GetResourceHandle());
	}

	cmd.SetRenderTarget(
	    framebuffer.renderTargetViews[0],
	    framebuffer.HasDepthStencil() ? &framebuffer.depthStencilView : nullptr);
}

void FrameGraph::BindRenderTargets(
    RenderCommandContext& cmd,
    std::span<const FrameGraphTextureHandle> renderTargetHandles,
    FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	assert(!renderTargetHandles.empty());
	assert(renderTargetHandles.size() <= 8u);

	FrameGraphFramebuffer framebuffer{};
	framebuffer.renderTargetCount = static_cast<std::uint32_t>(renderTargetHandles.size());
	for (std::size_t index = 0; index < renderTargetHandles.size(); ++index)
	{
		framebuffer.renderTargetViews[index] = ResolveRenderTargetView(renderTargetHandles[index].GetResourceHandle());
	}

	if (depthStencilHandle.IsValid())
	{
		framebuffer.depthStencilView = ResolveDepthStencilView(depthStencilHandle.GetResourceHandle());
	}

	cmd.SetRenderTargets(
	    framebuffer.renderTargetCount,
	    framebuffer.renderTargetViews.data(),
	    framebuffer.HasDepthStencil() ? &framebuffer.depthStencilView : nullptr);
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveUnorderedAccessView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveUnorderedAccessView(handle.GetResourceHandle());
}

RhiGpuVirtualAddress FrameGraph::ResolveAccelerationStructureGpuAddress(FrameGraphAccelerationStructureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveAccelerationStructureGpuAddress(handle.GetResourceHandle());
}

void FrameGraph::CopyTexture(RenderCommandContext& cmd, FrameGraphTextureHandle destinationHandle, FrameGraphTextureHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());
	CopyResource(cmd, destinationHandle.GetResourceHandle(), sourceHandle.GetResourceHandle());
}

void FrameGraph::CopyBuffer(RenderCommandContext& cmd, FrameGraphBufferHandle destinationHandle, FrameGraphBufferHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());
	CopyResource(cmd, destinationHandle.GetResourceHandle(), sourceHandle.GetResourceHandle());
}

void FrameGraph::ClearRenderTarget(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	const std::array<float, 4> clearColor = GetClearColor(resourceHandle);
	cmd.ClearRenderTarget(ResolveRenderTargetView(resourceHandle), clearColor.data());
}

void FrameGraph::ClearDepthStencil(RenderCommandContext& cmd, FrameGraphTextureHandle handle) const noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	cmd.ClearDepthStencil(ResolveDepthStencilView(resourceHandle), GetClearDepth(resourceHandle));
}

RhiResourceHandle FrameGraph::ResolveResource(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveResource(handle.GetResourceHandle());
}

NativeTextureViewInfo FrameGraph::ResolveNativeTextureView(
	FrameGraphTextureHandle handle,
	ResourceState state,
	const RhiNativeInteropRequest& request) const noexcept
{
	assert(handle.IsValid());
	return ResolveNativeTextureView(handle.GetResourceHandle(), state, request);
}

void FrameGraph::CopyResource(RenderCommandContext& cmd, FrameGraphResourceHandle destinationHandle, FrameGraphResourceHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());

	const FrameGraphResourceMetadata& destinationMetadata = m_resourceRegistry.GetMetadata(destinationHandle);
	const FrameGraphResourceMetadata& sourceMetadata = m_resourceRegistry.GetMetadata(sourceHandle);
	assert(destinationMetadata.resourceClass == sourceMetadata.resourceClass);
	assert(
	    destinationMetadata.kind == sourceMetadata.kind || (destinationMetadata.resourceClass == FrameGraphResourceClass::Texture &&
	                                                        sourceMetadata.resourceClass == FrameGraphResourceClass::Texture));

	const RhiResourceHandle destinationResource = ResolveResource(destinationHandle);
	const RhiResourceHandle sourceResource = ResolveResource(sourceHandle);
	assert(destinationResource);
	assert(sourceResource);
	cmd.CopyResource(destinationResource, sourceResource);
}
