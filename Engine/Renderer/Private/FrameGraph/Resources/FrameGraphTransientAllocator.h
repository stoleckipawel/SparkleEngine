#pragma once

#include "FrameGraph/Compiler/FrameGraphPlan.h"
#include "RHI/Public/Resources/RhiResourceHandles.h"
#include "RHI/Public/Resources/RhiResourceView.h"

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
		RhiResourceViewHandle renderTargetView = {};
		RhiResourceViewHandle depthStencilView = {};
		RhiResourceViewHandle shaderResourceView = {};
		RhiResourceViewHandle unorderedAccessView = {};
		RhiOwnedResourceHandle ownedResource = {};
		RhiResourceHandle resource = {};
	};

	explicit FrameGraphTransientAllocator(RenderHardwareInterface& renderHardwareInterface) noexcept;
	~FrameGraphTransientAllocator() = default;

	FrameGraphTransientAllocator(const FrameGraphTransientAllocator&) = delete;
	FrameGraphTransientAllocator& operator=(const FrameGraphTransientAllocator&) = delete;
	FrameGraphTransientAllocator(FrameGraphTransientAllocator&&) = delete;
	FrameGraphTransientAllocator& operator=(FrameGraphTransientAllocator&&) = delete;

	void Reset() noexcept;
	void Prepare(const FrameGraphTransientPlan& plan) noexcept;
	AllocationRecord& Materialize(const FrameGraphTransientResourcePlan& transientPlan);
	const AllocationRecord* FindAllocation(FrameGraphResourceHandle handle) const noexcept;

  private:
	struct PlanEntry
	{
		FrameGraphResourceHandle handle = FrameGraphResourceHandle::Invalid();
		std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
		RhiTextureResourceDesc textureDesc = {};
		RhiBufferResourceDesc bufferDesc = {};
		bool requiresShaderResourceView = false;
		bool requiresUnorderedAccessView = false;
		bool operator==(const PlanEntry&) const = default;
	};

	struct MemoryBlockRecord
	{
		std::uint32_t physicalBlockIndex = INVALID_FRAME_GRAPH_RESOURCE_INDEX;
		RhiOwnedMemoryBlockHandle memoryBlock = {};
	};

	void ReleaseAllocations() noexcept;
	AllocationRecord CreateAllocationRecord(const FrameGraphTransientResourcePlan& transientPlan);
	RhiOwnedMemoryBlockHandle GetOrCreateMemoryBlock(const FrameGraphTransientResourcePlan& transientPlan);

	RenderHardwareInterface* m_renderHardwareInterface = nullptr;
	std::vector<PlanEntry> m_planEntries;
	std::vector<MemoryBlockRecord> m_memoryBlocks;
	std::vector<AllocationRecord> m_allocations;
};
