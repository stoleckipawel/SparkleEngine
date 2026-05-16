#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Config/DepthConvention.h"

#include <cassert>

RhiCpuDescriptorHandle FrameGraph::ResolveRenderTargetView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::BackBuffer || metadata.kind == FrameGraphResourceKind::ColorRenderTarget);

	if (metadata.kind == FrameGraphResourceKind::BackBuffer)
	{
		return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetBackBufferRenderTargetView() : RhiCpuDescriptorHandle{};
	}

	assert(access.renderTargetView);
	return access.renderTargetView;
}

RhiCpuDescriptorHandle FrameGraph::ResolveDepthStencilView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::DepthStencil);

	assert(access.depthStencilView);
	return access.depthStencilView;
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind != FrameGraphResourceKind::BackBuffer);
	assert(metadata.kind != FrameGraphResourceKind::DepthStencil && "Depth SRV resolution is not implemented yet.");

	assert(access.shaderResourceViewGpu);
	return access.shaderResourceViewGpu;
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(FrameGraphResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceResolver.GetResolvedAccess(handle);
	assert(metadata.kind != FrameGraphResourceKind::BackBuffer);
	assert(metadata.kind != FrameGraphResourceKind::DepthStencil);

	assert(access.unorderedAccessViewGpu);
	return access.unorderedAccessViewGpu;
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

NativeResourceHandle FrameGraph::ResolveResource(FrameGraphResourceHandle handle) const noexcept
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
			return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetBackBufferResource() : NativeResourceHandle{};
		case FrameGraphResourceKind::DepthStencil:
		case FrameGraphResourceKind::ColorRenderTarget:
		case FrameGraphResourceKind::Buffer:
			return access.resource;
		default:
			return {};
	}
}
