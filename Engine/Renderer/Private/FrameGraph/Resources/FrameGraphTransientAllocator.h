#pragma once

#include "Renderer/Public/FrameGraph/FrameGraph.h"

#include "D3D12DescriptorHandle.h"

#include <d3d12.h>
#include <vector>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12DescriptorHeapManager;
class D3D12Rhi;

class FrameGraphTransientAllocator final
{
  public:
	struct AllocationRecord
	{
		ResourceHandle handle = ResourceHandle::Invalid();
		FrameGraphResourceKind kind = FrameGraphResourceKind::ColorRenderTarget;
		std::uint32_t allocationIndex = FrameGraph::INVALID_RESOURCE_INDEX;
		std::uint32_t physicalBlockIndex = FrameGraph::INVALID_RESOURCE_INDEX;
		UINT64 sizeInBytes = 0;
		UINT64 alignment = 0;
		UINT64 heapOffset = 0;
		ComPtr<ID3D12Heap> heap;
		D3D12DescriptorHandle renderTargetView;
		D3D12DescriptorHandle depthStencilView;
		D3D12DescriptorHandle shaderResourceView;
		bool hasShaderResourceView = false;
		ComPtr<ID3D12Resource> depthStencilResource;
		ComPtr<ID3D12Resource> renderTargetResource;
		ComPtr<ID3D12Resource> buffer;
	};

	struct PhysicalBlockRecord
	{
		std::uint32_t physicalBlockIndex = FrameGraph::INVALID_RESOURCE_INDEX;
		FrameGraph::CompiledTransientResourcePlan::AllocationPool pool = FrameGraph::CompiledTransientResourcePlan::AllocationPool::Color;
		UINT64 sizeInBytes = 0;
		UINT64 alignment = 0;
		UINT64 heapOffset = 0;
		ComPtr<ID3D12Heap> heap;
	};

	FrameGraphTransientAllocator(D3D12Rhi& rhi, D3D12DescriptorHeapManager& descriptorHeapManager) noexcept;
	~FrameGraphTransientAllocator() = default;

	FrameGraphTransientAllocator(const FrameGraphTransientAllocator&) = delete;
	FrameGraphTransientAllocator& operator=(const FrameGraphTransientAllocator&) = delete;
	FrameGraphTransientAllocator(FrameGraphTransientAllocator&&) = delete;
	FrameGraphTransientAllocator& operator=(FrameGraphTransientAllocator&&) = delete;

	void Reset() noexcept;
	AllocationRecord& Materialize(const FrameGraph::CompiledTransientResourcePlan& transientPlan);
	const AllocationRecord* FindAllocation(ResourceHandle handle) const noexcept;
	const AllocationRecord* FindDepthAllocation(ResourceHandle handle) const noexcept;
	const AllocationRecord* FindColorAllocation(ResourceHandle handle) const noexcept;
	const AllocationRecord* FindBufferAllocation(ResourceHandle handle) const noexcept;

  private:
	using AllocationList = std::vector<AllocationRecord>;
	using BlockList = std::vector<PhysicalBlockRecord>;

	void ReleaseAllocationDescriptors(AllocationList& allocations) noexcept;
	AllocationRecord CreateAllocationRecord(const FrameGraph::CompiledTransientResourcePlan& transientPlan);
	PhysicalBlockRecord& GetOrCreatePhysicalBlock(const FrameGraph::CompiledTransientResourcePlan& transientPlan);
	AllocationList& GetAllocationList(FrameGraph::CompiledTransientResourcePlan::AllocationPool pool) noexcept;
	const AllocationList& GetAllocationList(FrameGraph::CompiledTransientResourcePlan::AllocationPool pool) const noexcept;
	BlockList& GetBlockList(FrameGraph::CompiledTransientResourcePlan::AllocationPool pool) noexcept;
	const BlockList& GetBlockList(FrameGraph::CompiledTransientResourcePlan::AllocationPool pool) const noexcept;
	const AllocationRecord* FindAllocationInList(const AllocationList& allocations, ResourceHandle handle) const noexcept;
	PhysicalBlockRecord* FindPhysicalBlock(BlockList& blocks, std::uint32_t physicalBlockIndex) noexcept;

	D3D12Rhi* m_rhi = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
	AllocationList m_colorAllocations;
	AllocationList m_depthAllocations;
	AllocationList m_bufferAllocations;
	BlockList m_colorBlocks;
	BlockList m_depthBlocks;
	BlockList m_bufferBlocks;
};