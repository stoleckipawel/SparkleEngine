#include "PCH.h"
#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "Renderer/Public/DepthConvention.h"

#include "D3D12Rhi.h"

#include <cassert>
#include <string>

namespace
{
	std::string BuildTransientDisplayLabel(ResourceHandle handle, const FrameGraphResourceMetadata& metadata)
	{
		const std::string name = metadata.debugName.empty() ? std::string{"Transient"} : metadata.debugName;
		return std::string{"#"} + std::to_string(handle.index) + ":" + name;
	}

	std::string BuildTransientEventScopeLabel(ResourceHandle handle, const FrameGraphResourceMetadata& metadata)
	{
		std::string label{"FG/Transient/"};
		label += metadata.resourceClass == FrameGraphResourceClass::Buffer ? "Buffer/" : "Texture/";
		label += std::to_string(handle.index);
		label += "/";
		label += metadata.debugName.empty() ? "Transient" : metadata.debugName;
		return label;
	}

	D3D12_RESOURCE_DESC BuildTransientBufferDesc(const FrameGraphBufferDesc& desc) noexcept
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = desc.sizeInBytes;
		resourceDesc.Height = 1;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		return resourceDesc;
	}

	D3D12_RESOURCE_FLAGS ResolveTransientResourceFlags(FrameGraphResourceKind kind) noexcept
	{
		switch (kind)
		{
			case FrameGraphResourceKind::DepthStencil:
				return D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			case FrameGraphResourceKind::ColorRenderTarget:
				return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			default:
				return D3D12_RESOURCE_FLAG_NONE;
		}
	}

	D3D12_RESOURCE_DESC BuildTransientResourceDesc(const FrameGraphTextureDesc& desc, FrameGraphResourceKind kind) noexcept
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = static_cast<UINT64>(desc.width);
		resourceDesc.Height = static_cast<UINT>(desc.height);
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = desc.format;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = ResolveTransientResourceFlags(kind);
		return resourceDesc;
	}

	D3D12_CLEAR_VALUE BuildTransientOptimizedClearValue(const FrameGraphTextureDesc& desc, FrameGraphResourceKind kind) noexcept
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = desc.format;

		if (kind == FrameGraphResourceKind::DepthStencil)
		{
			clearValue.DepthStencil.Depth = DepthConvention::GetClearDepth();
			clearValue.DepthStencil.Stencil = 0;
			return clearValue;
		}

		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;
		return clearValue;
	}

	FrameGraph::CompiledTransientResourcePlan::AllocationPool ResolveTransientAllocationPool(FrameGraphResourceKind kind) noexcept
	{
		if (kind == FrameGraphResourceKind::DepthStencil)
		{
			return FrameGraph::CompiledTransientResourcePlan::AllocationPool::Depth;
		}

		if (kind == FrameGraphResourceKind::Buffer)
		{
			return FrameGraph::CompiledTransientResourcePlan::AllocationPool::Buffer;
		}

		return FrameGraph::CompiledTransientResourcePlan::AllocationPool::Color;
	}
}

void FrameGraph::BuildTransientMaterializationPlan(CompiledPlan& plan) const noexcept
{
	assert(m_rhi != nullptr);

	if (m_transientAllocator != nullptr)
	{
		m_transientAllocator->Reset();
	}

	plan.transientResources.clear();
	plan.transientResources.reserve(m_virtualTransientResources.size());

	for (const VirtualTransientResource& transientResource : m_virtualTransientResources)
	{
		const FrameGraphResourceMetadata& resourceMetadata = m_resourceRegistry.GetMetadata(transientResource.handle);
		if (resourceMetadata.ownership != FrameGraphResourceOwnership::Transient)
		{
			continue;
		}

		const bool isBuffer = resourceMetadata.resourceClass == FrameGraphResourceClass::Buffer;
		const D3D12_RESOURCE_DESC resourceDesc = isBuffer ? BuildTransientBufferDesc(transientResource.bufferDesc)
		                                               : BuildTransientResourceDesc(transientResource.textureDesc, resourceMetadata.kind);
		const D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = m_rhi->GetDevice()->GetResourceAllocationInfo(0, 1, &resourceDesc);
		const std::uint32_t allocationIndex = static_cast<std::uint32_t>(plan.transientResources.size());
		plan.transientResources.push_back(
		    CompiledTransientResourcePlan{
		        .handle = transientResource.handle,
		        .resourceClass = resourceMetadata.resourceClass,
		        .textureDesc = transientResource.textureDesc,
		        .bufferDesc = transientResource.bufferDesc,
		        .kind = resourceMetadata.kind,
		        .physicalAllocation =
		            CompiledTransientResourcePlan::PhysicalAllocationPlan{
		                .allocationIndex = allocationIndex,
		                .physicalBlockIndex = INVALID_RESOURCE_INDEX,
		                .pool = ResolveTransientAllocationPool(resourceMetadata.kind),
		                .sizeInBytes = allocationInfo.SizeInBytes,
		                .alignment = allocationInfo.Alignment,
		                .heapOffset = 0,
		                .heapFlags = isBuffer ? D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS : D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES,
		                .resourceDesc = resourceDesc,
		                .optimizedClearValue = isBuffer ? D3D12_CLEAR_VALUE{}
		                                               : BuildTransientOptimizedClearValue(transientResource.textureDesc, resourceMetadata.kind),
		                .hasOptimizedClearValue = !isBuffer,
		                .initialState = resourceMetadata.initialState},
		        .displayLabel = BuildTransientDisplayLabel(transientResource.handle, resourceMetadata),
		        .eventScopeLabel = BuildTransientEventScopeLabel(transientResource.handle, resourceMetadata)});
	}
}

void FrameGraph::EnsureTransientResourcesMaterialized(const CompiledPlan& plan) const noexcept
{
	assert(m_transientAllocator != nullptr);

	for (const CompiledTransientResourcePlan& transientPlan : plan.transientResources)
	{
		(void)m_transientAllocator->Materialize(transientPlan);
	}
}
