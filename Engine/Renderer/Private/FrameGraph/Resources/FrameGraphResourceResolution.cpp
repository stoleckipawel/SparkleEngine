#include "PCH.h"
#include "FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "Config/DepthConvention.h"

#include <cassert>

RhiCpuDescriptorHandle FrameGraph::ResolveRenderTargetView(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::BackBuffer || metadata.kind == FrameGraphResourceKind::ColorRenderTarget);

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientRenderTargetView(handle);
	}

	if (metadata.kind == FrameGraphResourceKind::BackBuffer)
	{
		return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetBackBufferRenderTargetView() : RhiCpuDescriptorHandle{};
	}

	assert(access.renderTargetView);
	return access.renderTargetView;
}

RhiCpuDescriptorHandle FrameGraph::ResolveDepthStencilView(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::DepthStencil);

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientDepthStencilView(handle);
	}

	assert(access.depthStencilView);
	return access.depthStencilView;
}

RhiGpuDescriptorHandle FrameGraph::ResolveShaderResourceView(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
	assert(metadata.kind != FrameGraphResourceKind::BackBuffer);
	assert(metadata.kind != FrameGraphResourceKind::DepthStencil && "Depth SRV resolution is not implemented yet.");

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientShaderResourceView(handle);
	}

	assert(access.shaderResourceViewGpu);
	return access.shaderResourceViewGpu;
}

RhiGpuDescriptorHandle FrameGraph::ResolveUnorderedAccessView(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
	assert(metadata.kind != FrameGraphResourceKind::BackBuffer);
	assert(metadata.kind != FrameGraphResourceKind::DepthStencil);

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientUnorderedAccessView(handle);
	}

	assert(access.unorderedAccessViewGpu);
	return access.unorderedAccessViewGpu;
}

RhiCpuDescriptorHandle FrameGraph::ResolveTransientRenderTargetView(ResourceHandle handle) const noexcept
{
	assert(m_transientAllocator != nullptr);
	const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindColorAllocation(handle);
	assert(allocation != nullptr);
	assert(allocation->renderTargetView.IsValid());
	return allocation->renderTargetView.CpuHandle;
}

RhiCpuDescriptorHandle FrameGraph::ResolveTransientDepthStencilView(ResourceHandle handle) const noexcept
{
	assert(m_transientAllocator != nullptr);
	const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindDepthAllocation(handle);
	assert(allocation != nullptr);
	assert(allocation->depthStencilView.IsValid());
	return allocation->depthStencilView.CpuHandle;
}

RhiGpuDescriptorHandle FrameGraph::ResolveTransientShaderResourceView(ResourceHandle handle) const noexcept
{
	assert(m_transientAllocator != nullptr);

	if (const FrameGraphTransientAllocator::AllocationRecord* colorAllocation = m_transientAllocator->FindColorAllocation(handle))
	{
		assert(colorAllocation->shaderResourceView.IsValid());
		return colorAllocation->shaderResourceView.GpuHandle;
	}

	const FrameGraphTransientAllocator::AllocationRecord* bufferAllocation = m_transientAllocator->FindBufferAllocation(handle);
	assert(bufferAllocation != nullptr);
	assert(bufferAllocation->shaderResourceView.IsValid());
	return bufferAllocation->shaderResourceView.GpuHandle;
}

RhiGpuDescriptorHandle FrameGraph::ResolveTransientUnorderedAccessView(ResourceHandle handle) const noexcept
{
	assert(m_transientAllocator != nullptr);

	if (const FrameGraphTransientAllocator::AllocationRecord* colorAllocation = m_transientAllocator->FindColorAllocation(handle))
	{
		assert(colorAllocation->unorderedAccessView.IsValid());
		return colorAllocation->unorderedAccessView.GpuHandle;
	}

	const FrameGraphTransientAllocator::AllocationRecord* bufferAllocation = m_transientAllocator->FindBufferAllocation(handle);
	assert(bufferAllocation != nullptr);
	assert(bufferAllocation->unorderedAccessView.IsValid());
	return bufferAllocation->unorderedAccessView.GpuHandle;
}

std::array<float, 4> FrameGraph::GetClearColor(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& resource = m_resourceRegistry.GetMetadata(handle);
	assert(resource.kind == FrameGraphResourceKind::BackBuffer || resource.kind == FrameGraphResourceKind::ColorRenderTarget);
	return resource.textureDesc.clearColor;
}

float FrameGraph::GetClearDepth(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& resource = m_resourceRegistry.GetMetadata(handle);
	assert(resource.kind == FrameGraphResourceKind::DepthStencil);
	return DepthConvention::GetClearDepth();
}

NativeResourceHandle FrameGraph::ResolveResource(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientResource(handle, metadata.kind);
	}

	if (access.externalResource)
	{
		return access.externalResource;
	}

	switch (metadata.kind)
	{
		case FrameGraphResourceKind::BackBuffer:
			return m_renderHardwareInterface != nullptr ? m_renderHardwareInterface->GetBackBufferResource() : NativeResourceHandle{};
		case FrameGraphResourceKind::DepthStencil:
		case FrameGraphResourceKind::ColorRenderTarget:
		case FrameGraphResourceKind::Buffer:
			return access.externalResource;
		default:
			return {};
	}
}

NativeResourceHandle FrameGraph::ResolveTransientResource(ResourceHandle handle, FrameGraphResourceKind kind) const noexcept
{
	assert(m_transientAllocator != nullptr);

	switch (kind)
	{
		case FrameGraphResourceKind::DepthStencil:
		{
			const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindDepthAllocation(handle);
			return allocation != nullptr ? allocation->depthStencilResource : NativeResourceHandle{};
		}
		case FrameGraphResourceKind::ColorRenderTarget:
		{
			const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindColorAllocation(handle);
			return allocation != nullptr ? allocation->renderTargetResource : NativeResourceHandle{};
		}
		case FrameGraphResourceKind::Buffer:
		{
			const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindBufferAllocation(handle);
			return allocation != nullptr ? allocation->buffer : NativeResourceHandle{};
		}
		default:
			return {};
	}
}
