#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "RHI/Public/Device/RenderHardwareInterface.h"

#include <memory>
#include <vector>

class RenderHardwareInterface;

class FrameGraphTransientAllocator final
{
  public:
	struct AllocationRecord
	{
		FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
		FrameGraphResourceKind kind = FrameGraphResourceKind::ColorRenderTarget;
		std::uint32_t allocationIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
		std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
		std::uint64_t sizeInBytes = 0;
		std::uint64_t alignment = 0;
		std::uint64_t memoryBlockOffset = 0;
		RhiResourceViewHandle renderTargetView = {};
		RhiResourceViewHandle depthStencilView = {};
		RhiResourceViewHandle shaderResourceView = {};
		RhiResourceViewHandle unorderedAccessView = {};
		RhiOwnedResourceHandle ownedDepthStencilResource = {};
		RhiOwnedResourceHandle ownedRenderTargetResource = {};
		RhiOwnedResourceHandle ownedBuffer = {};
		NativeResourceHandle depthStencilResource;
		NativeResourceHandle renderTargetResource;
		NativeResourceHandle buffer;
	};

	struct PhysicalBlockRecord
	{
		std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
		FrameGraphTransientResourcePlan::AllocationPool pool = FrameGraphTransientResourcePlan::AllocationPool::Color;
		std::uint64_t sizeInBytes = 0;
		std::uint64_t alignment = 0;
		std::uint64_t memoryBlockOffset = 0;
		RhiOwnedMemoryBlockHandle ownedMemoryBlock = {};
	};

	explicit FrameGraphTransientAllocator(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~FrameGraphTransientAllocator() = default;

	FrameGraphTransientAllocator(const FrameGraphTransientAllocator&) = delete;
	FrameGraphTransientAllocator& operator=(const FrameGraphTransientAllocator&) = delete;
	FrameGraphTransientAllocator(FrameGraphTransientAllocator&&) = delete;
	FrameGraphTransientAllocator& operator=(FrameGraphTransientAllocator&&) = delete;

	void Reset() noexcept;
	AllocationRecord& Materialize(const FrameGraphTransientResourcePlan& transientPlan);
	const AllocationRecord* FindAllocation(FrameGraphResourceHandle handle) const noexcept;
	const AllocationRecord* FindDepthAllocation(FrameGraphResourceHandle handle) const noexcept;
	const AllocationRecord* FindColorAllocation(FrameGraphResourceHandle handle) const noexcept;
	const AllocationRecord* FindBufferAllocation(FrameGraphResourceHandle handle) const noexcept;

  private:
	using AllocationList = std::vector<AllocationRecord>;
	using BlockList = std::vector<PhysicalBlockRecord>;

	void ReleaseAllocationDescriptors(AllocationList& allocations) noexcept;
	AllocationRecord CreateAllocationRecord(const FrameGraphTransientResourcePlan& transientPlan);
	PhysicalBlockRecord& GetOrCreatePhysicalBlock(const FrameGraphTransientResourcePlan& transientPlan);
	AllocationList& GetAllocationList(FrameGraphTransientResourcePlan::AllocationPool pool) noexcept;
	const AllocationList& GetAllocationList(FrameGraphTransientResourcePlan::AllocationPool pool) const noexcept;
	BlockList& GetBlockList(FrameGraphTransientResourcePlan::AllocationPool pool) noexcept;
	const BlockList& GetBlockList(FrameGraphTransientResourcePlan::AllocationPool pool) const noexcept;
	const AllocationRecord* FindAllocationInList(const AllocationList& allocations, FrameGraphResourceHandle handle) const noexcept;
	PhysicalBlockRecord* FindPhysicalBlock(BlockList& blocks, std::uint32_t physicalBlockIndex) noexcept;

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	AllocationList m_colorAllocations;
	AllocationList m_depthAllocations;
	AllocationList m_bufferAllocations;
	BlockList m_colorBlocks;
	BlockList m_depthBlocks;
	BlockList m_bufferBlocks;
};
