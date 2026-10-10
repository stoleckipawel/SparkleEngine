#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <cassert>

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
			return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetPresentationService().GetBackBufferResource() : RhiResourceHandle{};
		case FrameGraphResourceKind::DepthStencil:
		case FrameGraphResourceKind::ColorRenderTarget:
		case FrameGraphResourceKind::Buffer:
			return access.resource;

		default:
			return {};
	}
}

RhiResourceHandle FrameGraph::ResolveResource(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveResource(handle.GetResourceHandle());
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(FrameGraphTextureHandle handle) const noexcept
{
	assert(handle.IsValid());
	return ResolveShaderResourceView(handle.GetResourceHandle());
}
