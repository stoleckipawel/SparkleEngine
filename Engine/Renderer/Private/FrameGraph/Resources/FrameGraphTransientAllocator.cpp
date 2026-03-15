#include "PCH.h"
#include "Renderer/Private/FrameGraph/Resources/FrameGraphTransientAllocator.h"

#include "Renderer/Public/DepthConvention.h"
#include "Renderer/Public/FrameGraph/ResourceState.h"

#include "D3D12DescriptorHeapManager.h"
#include "D3D12Rhi.h"

#include <algorithm>
#include <cassert>
#include <string>

namespace
{
	bool RequiresShaderResourceView(const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		return transientPlan.kind != FrameGraphResourceKind::DepthStencil &&
		       std::find(transientPlan.requiredStates.begin(), transientPlan.requiredStates.end(), ResourceState::ShaderResource) !=
		           transientPlan.requiredStates.end();
	}

	std::wstring BuildWideDebugName(const std::string& name, const wchar_t* fallbackName)
	{
		if (name.empty())
		{
			return fallbackName;
		}

		return std::wstring(name.begin(), name.end());
	}

	std::wstring BuildHeapDebugName(const FrameGraph::CompiledTransientResourcePlan& transientPlan)
	{
		const std::string& debugName = transientPlan.resourceClass == FrameGraphResourceClass::Buffer ? transientPlan.bufferDesc.name
		                                                                                              : transientPlan.textureDesc.name;
		std::wstring heapName = BuildWideDebugName(debugName, L"FG_Transient");
		heapName += L"_Block";
		heapName += std::to_wstring(transientPlan.physicalAllocation.physicalBlockIndex);
		heapName += L"_Heap";
		return heapName;
	}

	D3D12_HEAP_DESC BuildHeapDesc(const FrameGraph::CompiledTransientResourcePlan::PhysicalAllocationPlan& physicalPlan) noexcept
	{
		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.SizeInBytes = physicalPlan.sizeInBytes;
		heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
		heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.Properties.CreationNodeMask = 0;
		heapDesc.Properties.VisibleNodeMask = 0;
		heapDesc.Alignment = physicalPlan.alignment;
		heapDesc.Flags = physicalPlan.heapFlags;
		return heapDesc;
	}

	D3D12_RESOURCE_DESC BuildDepthStencilResourceDesc(const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = transientPlan.textureDesc.width;
		resourceDesc.Height = transientPlan.textureDesc.height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = transientPlan.textureDesc.format;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		return resourceDesc;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC BuildDepthStencilViewDesc(const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = {};
		viewDesc.Format = transientPlan.textureDesc.format;
		viewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		viewDesc.Flags = D3D12_DSV_FLAG_NONE;
		return viewDesc;
	}

	D3D12_CLEAR_VALUE BuildDepthStencilClearValue(const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = transientPlan.textureDesc.format;
		clearValue.DepthStencil.Depth = DepthConvention::GetClearDepth();
		clearValue.DepthStencil.Stencil = 0;
		return clearValue;
	}

	D3D12_RESOURCE_DESC BuildRenderTargetResourceDesc(const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		D3D12_RESOURCE_DESC resourceDesc = {};
		resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
		resourceDesc.Width = transientPlan.textureDesc.width;
		resourceDesc.Height = transientPlan.textureDesc.height;
		resourceDesc.DepthOrArraySize = 1;
		resourceDesc.MipLevels = 1;
		resourceDesc.Format = transientPlan.textureDesc.format;
		resourceDesc.SampleDesc.Count = 1;
		resourceDesc.SampleDesc.Quality = 0;
		resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		return resourceDesc;
	}

	D3D12_RENDER_TARGET_VIEW_DESC BuildRenderTargetViewDesc(const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		D3D12_RENDER_TARGET_VIEW_DESC viewDesc = {};
		viewDesc.Format = transientPlan.textureDesc.format;
		viewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		viewDesc.Texture2D.MipSlice = 0;
		viewDesc.Texture2D.PlaneSlice = 0;
		return viewDesc;
	}

	D3D12_CLEAR_VALUE BuildRenderTargetClearValue(const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = transientPlan.textureDesc.format;
		clearValue.Color[0] = 0.0f;
		clearValue.Color[1] = 0.0f;
		clearValue.Color[2] = 0.0f;
		clearValue.Color[3] = 1.0f;
		return clearValue;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC BuildShaderResourceViewDesc(const FrameGraph::CompiledTransientResourcePlan& transientPlan) noexcept
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

		if (transientPlan.resourceClass == FrameGraphResourceClass::Buffer)
		{
			if (transientPlan.bufferDesc.strideInBytes > 0)
			{
				srvDesc.Format = DXGI_FORMAT_UNKNOWN;
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
				srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
				srvDesc.Buffer.StructureByteStride = transientPlan.bufferDesc.strideInBytes;
				srvDesc.Buffer.NumElements =
				    static_cast<UINT>(transientPlan.bufferDesc.sizeInBytes / transientPlan.bufferDesc.strideInBytes);
				return srvDesc;
			}

			assert(transientPlan.bufferDesc.sizeInBytes % sizeof(std::uint32_t) == 0);
			srvDesc.Format = DXGI_FORMAT_R32_TYPELESS;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
			srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
			srvDesc.Buffer.StructureByteStride = 0;
			srvDesc.Buffer.NumElements = static_cast<UINT>(transientPlan.bufferDesc.sizeInBytes / sizeof(std::uint32_t));
			return srvDesc;
		}

		srvDesc.Format = transientPlan.textureDesc.format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.PlaneSlice = 0;
		srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		return srvDesc;
	}
}

FrameGraphTransientAllocator::FrameGraphTransientAllocator(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept :
	m_rhi(&rhi),
	m_descriptorHeapManager(&descriptorHeapManager)
{
}

void FrameGraphTransientAllocator::Reset() noexcept
{
	ReleaseAllocationDescriptors(m_colorAllocations);
	ReleaseAllocationDescriptors(m_depthAllocations);
	ReleaseAllocationDescriptors(m_bufferAllocations);
	m_colorAllocations.clear();
	m_depthAllocations.clear();
	m_bufferAllocations.clear();
	m_colorBlocks.clear();
	m_depthBlocks.clear();
	m_bufferBlocks.clear();
}

void FrameGraphTransientAllocator::ReleaseAllocationDescriptors(AllocationList& allocations) noexcept
{
	if (m_descriptorHeapManager == nullptr)
	{
		return;
	}

	for (AllocationRecord& allocation : allocations)
	{
		if (allocation.renderTargetView.IsValid())
		{
			m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, allocation.renderTargetView);
			allocation.renderTargetView = {};
		}

		if (allocation.depthStencilView.IsValid())
		{
			m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, allocation.depthStencilView);
			allocation.depthStencilView = {};
		}

		if (allocation.shaderResourceView.IsValid())
		{
			m_descriptorHeapManager->FreeHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, allocation.shaderResourceView);
			allocation.shaderResourceView = {};
			allocation.hasShaderResourceView = false;
		}
	}
}

FrameGraphTransientAllocator::AllocationRecord& FrameGraphTransientAllocator::Materialize(
	const FrameGraph::CompiledTransientResourcePlan& transientPlan)
{
	AllocationList& allocations = GetAllocationList(transientPlan.physicalAllocation.pool);
	if (AllocationRecord* existingAllocation = const_cast<AllocationRecord*>(FindAllocationInList(allocations, transientPlan.handle)))
	{
		return *existingAllocation;
	}

	allocations.push_back(CreateAllocationRecord(transientPlan));
	return allocations.back();
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindAllocation(ResourceHandle handle) const noexcept
{
	if (const AllocationRecord* depthAllocation = FindDepthAllocation(handle))
	{
		return depthAllocation;
	}

	if (const AllocationRecord* bufferAllocation = FindBufferAllocation(handle))
	{
		return bufferAllocation;
	}

	return FindColorAllocation(handle);
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindDepthAllocation(ResourceHandle handle) const noexcept
{
	return FindAllocationInList(m_depthAllocations, handle);
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindColorAllocation(ResourceHandle handle) const noexcept
{
	return FindAllocationInList(m_colorAllocations, handle);
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindBufferAllocation(ResourceHandle handle) const noexcept
{
	return FindAllocationInList(m_bufferAllocations, handle);
}

FrameGraphTransientAllocator::BlockList& FrameGraphTransientAllocator::GetBlockList(
	FrameGraph::CompiledTransientResourcePlan::AllocationPool pool) noexcept
{
	switch (pool)
	{
		case FrameGraph::CompiledTransientResourcePlan::AllocationPool::Depth:
			return m_depthBlocks;
		case FrameGraph::CompiledTransientResourcePlan::AllocationPool::Buffer:
			return m_bufferBlocks;
		default:
			return m_colorBlocks;
	}
}

const FrameGraphTransientAllocator::BlockList& FrameGraphTransientAllocator::GetBlockList(
	FrameGraph::CompiledTransientResourcePlan::AllocationPool pool) const noexcept
{
	switch (pool)
	{
		case FrameGraph::CompiledTransientResourcePlan::AllocationPool::Depth:
			return m_depthBlocks;
		case FrameGraph::CompiledTransientResourcePlan::AllocationPool::Buffer:
			return m_bufferBlocks;
		default:
			return m_colorBlocks;
	}
}

FrameGraphTransientAllocator::AllocationList& FrameGraphTransientAllocator::GetAllocationList(
	FrameGraph::CompiledTransientResourcePlan::AllocationPool pool) noexcept
{
	switch (pool)
	{
		case FrameGraph::CompiledTransientResourcePlan::AllocationPool::Depth:
			return m_depthAllocations;
		case FrameGraph::CompiledTransientResourcePlan::AllocationPool::Buffer:
			return m_bufferAllocations;
		default:
			return m_colorAllocations;
	}
}

const FrameGraphTransientAllocator::AllocationList& FrameGraphTransientAllocator::GetAllocationList(
	FrameGraph::CompiledTransientResourcePlan::AllocationPool pool) const noexcept
{
	switch (pool)
	{
		case FrameGraph::CompiledTransientResourcePlan::AllocationPool::Depth:
			return m_depthAllocations;
		case FrameGraph::CompiledTransientResourcePlan::AllocationPool::Buffer:
			return m_bufferAllocations;
		default:
			return m_colorAllocations;
	}
}

const FrameGraphTransientAllocator::AllocationRecord* FrameGraphTransientAllocator::FindAllocationInList(
	const AllocationList& allocations,
	ResourceHandle handle) const noexcept
{
	const auto it = std::find_if(
	    allocations.begin(),
	    allocations.end(),
	    [handle](const AllocationRecord& allocation)
	    {
		    return allocation.handle == handle;
	    });

	return it != allocations.end() ? &(*it) : nullptr;
}

FrameGraphTransientAllocator::PhysicalBlockRecord* FrameGraphTransientAllocator::FindPhysicalBlock(
	BlockList& blocks,
	std::uint32_t physicalBlockIndex) noexcept
{
	const auto it = std::find_if(
	    blocks.begin(),
	    blocks.end(),
	    [physicalBlockIndex](const PhysicalBlockRecord& block)
	    {
		    return block.physicalBlockIndex == physicalBlockIndex;
	    });

	return it != blocks.end() ? &(*it) : nullptr;
}

FrameGraphTransientAllocator::PhysicalBlockRecord& FrameGraphTransientAllocator::GetOrCreatePhysicalBlock(
	const FrameGraph::CompiledTransientResourcePlan& transientPlan)
{
	assert(transientPlan.physicalAllocation.physicalBlockIndex != FrameGraph::INVALID_RESOURCE_INDEX);
	BlockList& blocks = GetBlockList(transientPlan.physicalAllocation.pool);
	if (PhysicalBlockRecord* existingBlock = FindPhysicalBlock(blocks, transientPlan.physicalAllocation.physicalBlockIndex))
	{
		return *existingBlock;
	}

	PhysicalBlockRecord block;
	block.physicalBlockIndex = transientPlan.physicalAllocation.physicalBlockIndex;
	block.pool = transientPlan.physicalAllocation.pool;
	block.sizeInBytes = transientPlan.physicalAllocation.sizeInBytes;
	block.alignment = transientPlan.physicalAllocation.alignment;
	block.heapOffset = transientPlan.physicalAllocation.heapOffset;

	const D3D12_HEAP_DESC heapDesc = BuildHeapDesc(transientPlan.physicalAllocation);
	CHECK(m_rhi->GetDevice()->CreateHeap(&heapDesc, IID_PPV_ARGS(block.heap.ReleaseAndGetAddressOf())));
	block.heap->SetName(BuildHeapDebugName(transientPlan).c_str());

	blocks.push_back(std::move(block));
	return blocks.back();
}

FrameGraphTransientAllocator::AllocationRecord FrameGraphTransientAllocator::CreateAllocationRecord(
	const FrameGraph::CompiledTransientResourcePlan& transientPlan)
{
	assert(m_rhi != nullptr);
	assert(m_descriptorHeapManager != nullptr);
	assert(transientPlan.handle.IsValid());
	assert(transientPlan.physicalAllocation.allocationIndex != FrameGraph::INVALID_RESOURCE_INDEX);
	assert(transientPlan.physicalAllocation.physicalBlockIndex != FrameGraph::INVALID_RESOURCE_INDEX);
	assert(transientPlan.physicalAllocation.sizeInBytes > 0);
	assert(transientPlan.physicalAllocation.alignment > 0);

	AllocationRecord allocation;
	allocation.handle = transientPlan.handle;
	allocation.kind = transientPlan.kind;
	allocation.allocationIndex = transientPlan.physicalAllocation.allocationIndex;
	allocation.physicalBlockIndex = transientPlan.physicalAllocation.physicalBlockIndex;
	allocation.sizeInBytes = transientPlan.physicalAllocation.sizeInBytes;
	allocation.alignment = transientPlan.physicalAllocation.alignment;
	allocation.heapOffset = transientPlan.physicalAllocation.heapOffset;

	PhysicalBlockRecord& block = GetOrCreatePhysicalBlock(transientPlan);
	allocation.heap = block.heap;

	switch (transientPlan.kind)
	{
		case FrameGraphResourceKind::DepthStencil:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.textureDesc.name, L"FG_DepthTransient");
			const D3D12_RESOURCE_DESC resourceDesc = BuildDepthStencilResourceDesc(transientPlan);
			const D3D12_CLEAR_VALUE clearValue = BuildDepthStencilClearValue(transientPlan);
			CHECK(m_rhi->GetDevice()->CreatePlacedResource(
			    block.heap.Get(),
			    allocation.heapOffset,
			    &resourceDesc,
			    D3D12_RESOURCE_STATE_DEPTH_READ,
			    &clearValue,
			    IID_PPV_ARGS(allocation.depthStencilResource.ReleaseAndGetAddressOf())));
			allocation.depthStencilResource->SetName(debugName.c_str());
			allocation.depthStencilView = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			const D3D12_DEPTH_STENCIL_VIEW_DESC viewDesc = BuildDepthStencilViewDesc(transientPlan);
			m_rhi->GetDevice()->CreateDepthStencilView(allocation.depthStencilResource.Get(), &viewDesc, allocation.depthStencilView.GetCPU());
			break;
		}

		case FrameGraphResourceKind::ColorRenderTarget:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.textureDesc.name, L"FG_ColorTransient");
			const D3D12_RESOURCE_DESC resourceDesc = BuildRenderTargetResourceDesc(transientPlan);
			const D3D12_CLEAR_VALUE clearValue = BuildRenderTargetClearValue(transientPlan);
			CHECK(m_rhi->GetDevice()->CreatePlacedResource(
			    block.heap.Get(),
			    allocation.heapOffset,
			    &resourceDesc,
			    MapToD3D12ResourceState(transientPlan.physicalAllocation.initialState),
			    &clearValue,
			    IID_PPV_ARGS(allocation.renderTargetResource.ReleaseAndGetAddressOf())));
			allocation.renderTargetResource->SetName(debugName.c_str());
			allocation.renderTargetView = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			const D3D12_RENDER_TARGET_VIEW_DESC viewDesc = BuildRenderTargetViewDesc(transientPlan);
			m_rhi->GetDevice()->CreateRenderTargetView(allocation.renderTargetResource.Get(), &viewDesc, allocation.renderTargetView.GetCPU());

			if (RequiresShaderResourceView(transientPlan))
			{
				allocation.shaderResourceView = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = BuildShaderResourceViewDesc(transientPlan);
				m_rhi->GetDevice()->CreateShaderResourceView(allocation.renderTargetResource.Get(), &srvDesc, allocation.shaderResourceView.GetCPU());
				allocation.hasShaderResourceView = true;
			}
			break;
		}

		case FrameGraphResourceKind::Buffer:
		{
			const std::wstring debugName = BuildWideDebugName(transientPlan.bufferDesc.name, L"FG_BufferTransient");
			CHECK(m_rhi->GetDevice()->CreatePlacedResource(
			    block.heap.Get(),
			    allocation.heapOffset,
			    &transientPlan.physicalAllocation.resourceDesc,
			    MapToD3D12ResourceState(transientPlan.physicalAllocation.initialState),
			    nullptr,
			    IID_PPV_ARGS(allocation.buffer.ReleaseAndGetAddressOf())));
			allocation.buffer->SetName(debugName.c_str());

			if (RequiresShaderResourceView(transientPlan))
			{
				allocation.shaderResourceView = m_descriptorHeapManager->AllocateHandle(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				const D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = BuildShaderResourceViewDesc(transientPlan);
				m_rhi->GetDevice()->CreateShaderResourceView(allocation.buffer.Get(), &srvDesc, allocation.shaderResourceView.GetCPU());
				allocation.hasShaderResourceView = true;
			}
			break;
		}

		default:
			LOG_FATAL("FrameGraphTransientAllocator: unsupported transient resource kind for heap-backed allocation");
			break;
	}

	return allocation;
}