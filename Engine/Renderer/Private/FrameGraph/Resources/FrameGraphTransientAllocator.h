#pragma once

#include "FrameGraph/FrameGraph.h"

#include <memory>
#include <vector>

class RenderHardwareInterface;

class FrameGraphTransientAllocator final
{
  public:
	struct AllocationRecord
	{
		ResourceHandle handle = ResourceHandle::Invalid();
		FrameGraphResourceKind kind = FrameGraphResourceKind::ColorRenderTarget;
		std::uint32_t allocationIndex = FrameGraph::INVALID_RESOURCE_INDEX;
		std::uint32_t physicalBlockIndex = FrameGraph::INVALID_RESOURCE_INDEX;
		std::uint64_t sizeInBytes = 0;
		std::uint64_t alignment = 0;
		std::uint64_t heapOffset = 0;
		RhiDescriptorAllocation renderTargetView = {};
		RhiDescriptorAllocation depthStencilView = {};
		RhiDescriptorAllocation shaderResourceView = {};
		RhiDescriptorAllocation unorderedAccessView = {};
		bool hasShaderResourceView = false;
		bool hasUnorderedAccessView = false;
		RhiOwnedResourceHandle ownedDepthStencilResource = {};
		RhiOwnedResourceHandle ownedRenderTargetResource = {};
		RhiOwnedResourceHandle ownedBuffer = {};
		NativeResourceHandle depthStencilResource;
		NativeResourceHandle renderTargetResource;
		NativeResourceHandle buffer;
	};

	struct PhysicalBlockRecord
	{
		std::uint32_t physicalBlockIndex = FrameGraph::INVALID_RESOURCE_INDEX;
		FrameGraph::CompiledTransientResourcePlan::AllocationPool pool = FrameGraph::CompiledTransientResourcePlan::AllocationPool::Color;
		std::uint64_t sizeInBytes = 0;
		std::uint64_t alignment = 0;
		std::uint64_t heapOffset = 0;
		RhiOwnedHeapHandle ownedHeap = {};
	};

	explicit FrameGraphTransientAllocator(RenderHardwareInterface& renderHardwareInterface) noexcept;
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

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	AllocationList m_colorAllocations;
	AllocationList m_depthAllocations;
	AllocationList m_bufferAllocations;
	BlockList m_colorBlocks;
	BlockList m_depthBlocks;
	BlockList m_bufferBlocks;
};