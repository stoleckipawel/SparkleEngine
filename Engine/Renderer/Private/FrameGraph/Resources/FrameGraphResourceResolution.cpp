#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "Renderer/Public/DepthConvention.h"

#include "D3D12SwapChain.h"

#include <cassert>

D3D12_CPU_DESCRIPTOR_HANDLE FrameGraph::ResolveRenderTargetView(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::BackBuffer || metadata.kind == FrameGraphResourceKind::ColorRenderTarget);

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientRenderTargetView(handle);
	}

	if (access.swapChain != nullptr)
	{
		return access.swapChain->GetCPUHandle();
	}

	assert(access.renderTargetView.IsValid());
	return access.renderTargetView.GetCPU();
}

D3D12_CPU_DESCRIPTOR_HANDLE FrameGraph::ResolveDepthStencilView(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
	assert(metadata.kind == FrameGraphResourceKind::DepthStencil);

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientDepthStencilView(handle);
	}

	assert(access.depthStencilView.IsValid());
	return access.depthStencilView.GetCPU();
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameGraph::ResolveShaderResourceView(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);
	assert(metadata.kind != FrameGraphResourceKind::BackBuffer);
	assert(metadata.kind != FrameGraphResourceKind::DepthStencil && "Depth SRV resolution is not implemented yet.");

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientShaderResourceView(handle);
	}

	assert(access.shaderResourceView.IsValid());
	return access.shaderResourceView.GetGPU();
}

D3D12_CPU_DESCRIPTOR_HANDLE FrameGraph::ResolveTransientRenderTargetView(ResourceHandle handle) const noexcept
{
	assert(m_transientAllocator != nullptr);
	const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindColorAllocation(handle);
	assert(allocation != nullptr);
	assert(allocation->renderTargetView.IsValid());
	return allocation->renderTargetView.GetCPU();
}

D3D12_CPU_DESCRIPTOR_HANDLE FrameGraph::ResolveTransientDepthStencilView(ResourceHandle handle) const noexcept
{
	assert(m_transientAllocator != nullptr);
	const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindDepthAllocation(handle);
	assert(allocation != nullptr);
	assert(allocation->depthStencilView.IsValid());
	return allocation->depthStencilView.GetCPU();
}

D3D12_GPU_DESCRIPTOR_HANDLE FrameGraph::ResolveTransientShaderResourceView(ResourceHandle handle) const noexcept
{
	assert(m_transientAllocator != nullptr);

	if (const FrameGraphTransientAllocator::AllocationRecord* colorAllocation = m_transientAllocator->FindColorAllocation(handle))
	{
		assert(colorAllocation->shaderResourceView.IsValid());
		return colorAllocation->shaderResourceView.GetGPU();
	}

	const FrameGraphTransientAllocator::AllocationRecord* bufferAllocation = m_transientAllocator->FindBufferAllocation(handle);
	assert(bufferAllocation != nullptr);
	assert(bufferAllocation->shaderResourceView.IsValid());
	return bufferAllocation->shaderResourceView.GetGPU();
}

std::array<float, 4> FrameGraph::GetClearColor(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& resource = m_resourceRegistry.GetMetadata(handle);
	assert(resource.kind == FrameGraphResourceKind::BackBuffer || resource.kind == FrameGraphResourceKind::ColorRenderTarget);
	return {0.0f, 0.0f, 0.0f, 1.0f};
}

float FrameGraph::GetClearDepth(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& resource = m_resourceRegistry.GetMetadata(handle);
	assert(resource.kind == FrameGraphResourceKind::DepthStencil);
	return DepthConvention::GetClearDepth();
}

ID3D12Resource* FrameGraph::ResolveResource(ResourceHandle handle) const noexcept
{
	const FrameGraphResourceMetadata& metadata = m_resourceRegistry.GetMetadata(handle);
	const FrameGraphResourceAccess& access = m_resourceRegistry.GetResolvedAccess(handle);

	if (metadata.ownership == FrameGraphResourceOwnership::Transient)
	{
		return ResolveTransientResource(handle, metadata.kind);
	}

	if (access.externalResource != nullptr)
	{
		return access.externalResource;
	}

	switch (metadata.kind)
	{
		case FrameGraphResourceKind::BackBuffer:
			return access.swapChain != nullptr ? access.swapChain->GetCurrentResource() : nullptr;
		case FrameGraphResourceKind::DepthStencil:
		case FrameGraphResourceKind::ColorRenderTarget:
		case FrameGraphResourceKind::Buffer:
			return access.externalResource;
		default:
			return nullptr;
	}
}

ID3D12Resource* FrameGraph::ResolveTransientResource(ResourceHandle handle, FrameGraphResourceKind kind) const noexcept
{
	assert(m_transientAllocator != nullptr);

	switch (kind)
	{
		case FrameGraphResourceKind::DepthStencil:
		{
			const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindDepthAllocation(handle);
			return allocation != nullptr ? allocation->depthStencilResource.Get() : nullptr;
		}
		case FrameGraphResourceKind::ColorRenderTarget:
		{
			const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindColorAllocation(handle);
			return allocation != nullptr ? allocation->renderTargetResource.Get() : nullptr;
		}
		case FrameGraphResourceKind::Buffer:
		{
			const FrameGraphTransientAllocator::AllocationRecord* allocation = m_transientAllocator->FindBufferAllocation(handle);
			return allocation != nullptr ? allocation->buffer.Get() : nullptr;
		}
		default:
			return nullptr;
	}
}
