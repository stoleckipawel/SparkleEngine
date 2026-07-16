#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Config/DepthConvention.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"
#include "RHI/Public/Interop/RhiInteropService.h"

#include <cassert>

RhiCpuDescriptorHandle FrameGraph::ResolveRenderTargetView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::BackBuffer || metadata.kind == FrameGraphResourceKind::ColorRenderTarget);

	if (metadata.kind == FrameGraphResourceKind::BackBuffer)
	{
		return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetPresentationService().GetBackBufferRenderTargetView()
		                                          : RhiCpuDescriptorHandle{};
	}

	assert(access.renderTargetView);
	if (m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	return m_renderHardwareInterface->GetDescriptorService().GetResourceViewCpuHandle(access.renderTargetView);
}

RhiCpuDescriptorHandle FrameGraph::ResolveDepthStencilView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::DepthStencil);

	assert(access.depthStencilView);
	if (m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	return m_renderHardwareInterface->GetDescriptorService().GetResourceViewCpuHandle(access.depthStencilView);
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind != FrameGraphResourceKind::BackBuffer);

	assert(access.shaderResourceView);
	if (m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	return m_renderHardwareInterface->GetDescriptorService().GetResourceViewGpuHandle(access.shaderResourceView);
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind != FrameGraphResourceKind::BackBuffer);
	assert(metadata.kind != FrameGraphResourceKind::DepthStencil);

	assert(access.unorderedAccessView);
	if (m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	return m_renderHardwareInterface->GetDescriptorService().GetResourceViewGpuHandle(access.unorderedAccessView);
}

std::array<float, 4> FrameGraph::GetClearColor(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& resource = m_resourceRegistry.GetMetadata(handle);
	assert(resource.kind == FrameGraphResourceKind::BackBuffer || resource.kind == FrameGraphResourceKind::ColorRenderTarget);
	return resource.textureDesc.clearColor;
}

float FrameGraph::GetClearDepth(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& resource = m_resourceRegistry.GetMetadata(handle);
	assert(resource.kind == FrameGraphResourceKind::DepthStencil);
	return DepthConvention::GetClearDepth();
}

RhiResourceHandle FrameGraph::ResolveResource(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);

	if (access.resource)
	{
		return access.resource;
	}

	switch (metadata.kind)
	{
		case FrameGraphResourceKind::BackBuffer:
			return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetPresentationService().GetBackBufferResource()
			                                          : RhiResourceHandle{};
		case FrameGraphResourceKind::DepthStencil:
		case FrameGraphResourceKind::ColorRenderTarget:
		case FrameGraphResourceKind::Buffer:
			return access.resource;
		default:
			return {};
	}
}

NativeTextureViewInfo FrameGraph::ResolveNativeTextureView(
	FrameGraphResourceHandle handle,
	ResourceState state,
	const RhiNativeInteropRequest& request) const noexcept
{
	if (m_renderHardwareInterface == nullptr)
	{
		return {};
	}

	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
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

	NativeTextureViewInfo nativeView = m_renderHardwareInterface->GetDescriptorService().GetNativeTextureViewInfo(
	    view,
	    ResolveResource(handle),
	    state,
	    request);
	if (nativeView.Width == 0u || nativeView.Height == 0u)
	{
		nativeView.Width = metadata.textureDesc.width;
		nativeView.Height = metadata.textureDesc.height;
	}
	return nativeView;
}
