#include "PCH.h"
#include "FrameGraph/Execution/FrameGraphResourceCommands.h"

#include "Config/DepthConvention.h"
#include "FrameGraph/FrameGraph.h"
#include "Commands/RenderCommandContext.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Interop/RhiInteropService.h"

#include <array>
#include <cassert>

FrameGraphResourceCommands::FrameGraphResourceCommands(const FrameGraph& frameGraph) noexcept :
    m_frameGraph(frameGraph)
{
}

void FrameGraphResourceCommands::BeginRasterPass(RenderCommandContext& commandContext, const FrameGraphRasterPass& rasterPass) const noexcept
{
	std::array<FrameGraphTextureHandle, 8> colorHandles = {};
	for (std::uint32_t index = 0; index < rasterPass.ColorCount; ++index)
	{
		colorHandles[index] = rasterPass.Colors[index].Handle;
	}

	BindRenderTargets(
	    commandContext,
	    std::span<const FrameGraphTextureHandle>(colorHandles.data(), rasterPass.ColorCount),
	    rasterPass.HasDepthStencil ? rasterPass.DepthStencil.Handle : FrameGraphTextureHandle::Invalid());

	for (std::uint32_t index = 0; index < rasterPass.ColorCount; ++index)
	{
		if (rasterPass.Colors[index].Load == FrameGraphAttachmentLoadAction::Clear)
		{
			ClearRenderTarget(commandContext, rasterPass.Colors[index].Handle);
		}
	}
	if (rasterPass.HasDepthStencil && rasterPass.DepthStencil.Load == FrameGraphAttachmentLoadAction::Clear)
	{
		ClearDepthStencil(commandContext, rasterPass.DepthStencil.Handle);
	}
}

void FrameGraphResourceCommands::EndRasterPass(RenderCommandContext& commandContext) const noexcept
{
	commandContext.EndRasterPass();
}

void FrameGraphResourceCommands::BindRenderTarget(RenderCommandContext& commandContext, FrameGraphTextureHandle renderTargetHandle, FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	const RhiCpuDescriptorHandle renderTargetView = ResolveRenderTargetView(renderTargetHandle.GetResourceHandle());
	const RhiCpuDescriptorHandle depthStencilView = depthStencilHandle.IsValid() ? ResolveDepthStencilView(depthStencilHandle.GetResourceHandle()) : RhiCpuDescriptorHandle{};
	commandContext.SetRenderTarget(renderTargetView, depthStencilView ? &depthStencilView : nullptr);
}

void FrameGraphResourceCommands::BindRenderTargets(
    RenderCommandContext& commandContext,
    std::span<const FrameGraphTextureHandle> renderTargetHandles,
    FrameGraphTextureHandle depthStencilHandle) const noexcept
{
	assert(!renderTargetHandles.empty());
	assert(renderTargetHandles.size() <= 8u);

	std::array<RhiCpuDescriptorHandle, 8> renderTargetViews = {};
	for (std::size_t index = 0; index < renderTargetHandles.size(); ++index)
	{
		renderTargetViews[index] = ResolveRenderTargetView(renderTargetHandles[index].GetResourceHandle());
	}
	const RhiCpuDescriptorHandle depthStencilView = depthStencilHandle.IsValid() ? ResolveDepthStencilView(depthStencilHandle.GetResourceHandle()) : RhiCpuDescriptorHandle{};
	commandContext.SetRenderTargets(static_cast<std::uint32_t>(renderTargetHandles.size()), renderTargetViews.data(), depthStencilView ? &depthStencilView : nullptr);
}

void FrameGraphResourceCommands::CopyTexture(RenderCommandContext& commandContext, FrameGraphTextureHandle destinationHandle, FrameGraphTextureHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());
	CopyResource(commandContext, destinationHandle.GetResourceHandle(), sourceHandle.GetResourceHandle());
}

void FrameGraphResourceCommands::CopyBuffer(RenderCommandContext& commandContext, FrameGraphBufferHandle destinationHandle, FrameGraphBufferHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());
	CopyResource(commandContext, destinationHandle.GetResourceHandle(), sourceHandle.GetResourceHandle());
}

void FrameGraphResourceCommands::ClearRenderTarget(RenderCommandContext& commandContext, FrameGraphTextureHandle handle) const noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	const FrameGraphResourceMetadata& metadata = m_frameGraph.m_resourceRegistry.GetMetadata(resourceHandle);
	assert(metadata.kind == FrameGraphResourceKind::BackBuffer || metadata.kind == FrameGraphResourceKind::ColorRenderTarget);
	const std::array<float, 4> clearColor = metadata.textureDesc.clearColor;
	commandContext.ClearRenderTarget(ResolveRenderTargetView(resourceHandle), clearColor);
}

void FrameGraphResourceCommands::ClearDepthStencil(RenderCommandContext& commandContext, FrameGraphTextureHandle handle) const noexcept
{
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	const FrameGraphResourceMetadata& metadata = m_frameGraph.m_resourceRegistry.GetMetadata(resourceHandle);
	assert(metadata.kind == FrameGraphResourceKind::DepthStencil);
	commandContext.ClearDepthStencil(ResolveDepthStencilView(resourceHandle), DepthConvention::GetClearDepth());
}

RhiResourceHandle FrameGraphResourceCommands::ResolveResource(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph.ResolveResource(handle);
}

NativeTextureViewInfo FrameGraphResourceCommands::ResolveNativeTextureView(FrameGraphTextureHandle handle, ResourceState state, const RhiNativeInteropRequest& request) const noexcept
{
	assert(handle.IsValid());
	const FrameGraphResourceHandle resourceHandle = handle.GetResourceHandle();
	if (m_frameGraph.m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	const FrameGraphResourceMetadata& metadata = m_frameGraph.m_resourceRegistry.GetMetadata(resourceHandle);
	const FrameGraphResourceAccess& access = m_frameGraph.m_resourceResolver.GetResolvedAccess(resourceHandle);
	RhiResourceViewHandle view = {};
	switch (state)
	{
		case ResourceState::DepthRead:
		case ResourceState::DepthWrite:
			view = access.depthStencilView;
			break;

		case ResourceState::UnorderedAccess:
			view = access.unorderedAccessView;
			break;

		case ResourceState::RenderTarget:
			view = access.renderTargetView;
			break;

		case ResourceState::ShaderResource:
		case ResourceState::CopySource:
		case ResourceState::Common:
		default:
			view = access.shaderResourceView;
			break;
	}

	if (!view && metadata.kind == FrameGraphResourceKind::DepthStencil)
	{
		view = access.depthStencilView;
	}
	if (!view && metadata.kind == FrameGraphResourceKind::ColorRenderTarget)
	{
		view = access.shaderResourceView ? access.shaderResourceView : access.renderTargetView;
	}
	if (!view)
	{
		return {};
	}

	NativeTextureViewInfo nativeView = m_frameGraph.m_renderHardwareInterface->GetInteropService().GetNativeTextureViewInfo(view, m_frameGraph.ResolveResource(resourceHandle), state, request);
	if (nativeView.Width == 0u || nativeView.Height == 0u)
	{
		nativeView.Width = metadata.textureDesc.width;
		nativeView.Height = metadata.textureDesc.height;
	}
	return nativeView;
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept
{
	return m_frameGraph.ResolveShaderResourceView(handle);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveShaderResourceView(FrameGraphBufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return m_frameGraph.ResolveShaderResourceView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveUnorderedAccessView(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveUnorderedAccessView(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveUnorderedAccessView(FrameGraphBufferHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveUnorderedAccessView(handle.GetResourceHandle());
}

RhiResourceHandle FrameGraphResourceCommands::ResolveAccelerationStructure(FrameGraphAccelerationStructureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return m_frameGraph.ResolveResource(handle.GetResourceHandle());
}

void FrameGraphResourceCommands::BindGlobalDescriptorState(RenderCommandContext& commandContext) const noexcept
{
	GetRenderHardwareInterface().GetDescriptorService().BindGlobalDescriptorState(commandContext.GetRenderCommandList());
}

RenderHardwareInterface& FrameGraphResourceCommands::GetRenderHardwareInterface() const noexcept
{
	return *m_frameGraph.m_renderHardwareInterface;
}

RhiCpuDescriptorHandle FrameGraphResourceCommands::ResolveRenderTargetView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_frameGraph.m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_frameGraph.m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::BackBuffer || metadata.kind == FrameGraphResourceKind::ColorRenderTarget);

	if (metadata.kind == FrameGraphResourceKind::BackBuffer)
	{
		return m_frameGraph.m_renderHardwareInterface != nullptr ? m_frameGraph.m_renderHardwareInterface->GetPresentationService().GetBackBufferRenderTargetView() : RhiCpuDescriptorHandle{};
	}

	assert(access.renderTargetView);
	if (m_frameGraph.m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	return m_frameGraph.m_renderHardwareInterface->GetDescriptorService().GetResourceViewCpuHandle(access.renderTargetView);
}

RhiCpuDescriptorHandle FrameGraphResourceCommands::ResolveDepthStencilView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_frameGraph.m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_frameGraph.m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::DepthStencil);

	assert(access.depthStencilView);
	if (m_frameGraph.m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	return m_frameGraph.m_renderHardwareInterface->GetDescriptorService().GetResourceViewCpuHandle(access.depthStencilView);
}

RhiGpuDescriptorHandle FrameGraphResourceCommands::ResolveUnorderedAccessView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_frameGraph.m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_frameGraph.m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind != FrameGraphResourceKind::BackBuffer);
	assert(metadata.kind != FrameGraphResourceKind::DepthStencil);

	assert(access.unorderedAccessView);
	if (m_frameGraph.m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	return m_frameGraph.m_renderHardwareInterface->GetDescriptorService().GetResourceViewGpuHandle(access.unorderedAccessView);
}

void FrameGraphResourceCommands::CopyResource(RenderCommandContext& commandContext, FrameGraphResourceHandle destinationHandle, FrameGraphResourceHandle sourceHandle) const noexcept
{
	assert(destinationHandle.IsValid());
	assert(sourceHandle.IsValid());

	const FrameGraphResourceMetadata& destinationMetadata = m_frameGraph.m_resourceRegistry.GetMetadata(destinationHandle);
	const FrameGraphResourceMetadata& sourceMetadata = m_frameGraph.m_resourceRegistry.GetMetadata(sourceHandle);
	assert(destinationMetadata.resourceClass == sourceMetadata.resourceClass);

	assert(
	    destinationMetadata.kind == sourceMetadata.kind || (destinationMetadata.resourceClass == FrameGraphResourceClass::Texture && sourceMetadata.resourceClass == FrameGraphResourceClass::Texture));

	const RhiResourceHandle destinationResource = m_frameGraph.ResolveResource(destinationHandle);
	const RhiResourceHandle sourceResource = m_frameGraph.ResolveResource(sourceHandle);
	assert(destinationResource);
	assert(sourceResource);
	commandContext.CopyResource(destinationResource, sourceResource);
}
