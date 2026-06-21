#pragma once

#include <cstdint>
#include <vector>
#include <mutex>
#include <optional>
#include "D3D12DescriptorHeap.h"

struct D3D12DescriptorAllocatorStats
{
	std::uint32_t Capacity = 0;
	std::uint32_t Allocated = 0;
	std::uint32_t Free = 0;
	std::uint32_t HighWatermark = 0;
};

class D3D12DescriptorAllocator
{
  public:
	explicit D3D12DescriptorAllocator(D3D12DescriptorHeap* heap) : m_heap(heap) {}

	D3D12DescriptorHandle Allocate();

	D3D12DescriptorHandle AllocateContiguous(uint32_t count);

	void Free(const D3D12DescriptorHandle& handle) noexcept;

	void FreeContiguous(const D3D12DescriptorHandle& firstHandle, uint32_t count) noexcept;

	D3D12DescriptorAllocatorStats CaptureStats() const noexcept;

  private:
	std::optional<UINT> TryAllocateContiguousFromFreeListLocked(uint32_t count);
	D3D12DescriptorHandle AllocateContiguousFromLinearRangeLocked(uint32_t count);

	D3D12DescriptorHeap* m_heap;
	std::vector<UINT> m_freeIndices;
	UINT m_currentOffset = 0;
	mutable std::mutex m_mutex;
};
