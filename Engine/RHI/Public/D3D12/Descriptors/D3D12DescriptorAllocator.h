// ============================================================================
// D3D12DescriptorAllocator.h
// ----------------------------------------------------------------------------
// Free-list based allocator for D3D12 descriptor heap slots.
//
#pragma once

#include <vector>
#include <mutex>
#include <optional>
#include "D3D12DescriptorHeap.h"

class D3D12DescriptorAllocator
{
  public:
	explicit D3D12DescriptorAllocator(D3D12DescriptorHeap* heap) : m_heap(heap) {}

	D3D12DescriptorHandle Allocate();

	// Used for descriptor tables that require sequential descriptors.
	D3D12DescriptorHandle AllocateContiguous(uint32_t count);

	void Free(const D3D12DescriptorHandle& handle) noexcept;

	void FreeContiguous(const D3D12DescriptorHandle& firstHandle, uint32_t count) noexcept;

  private:
	std::optional<UINT> TryAllocateContiguousFromFreeListLocked(uint32_t count);
	D3D12DescriptorHandle AllocateContiguousFromLinearRangeLocked(uint32_t count);

	D3D12DescriptorHeap* m_heap;
	std::vector<UINT> m_freeIndices;
	UINT m_currentOffset = 0;
	std::mutex m_mutex;
};
