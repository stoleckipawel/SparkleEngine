// ============================================================================
// D3D12DescriptorAllocator.h
// ----------------------------------------------------------------------------
// Free-list based allocator for D3D12 descriptor heap slots.
//
// USAGE:
//   D3D12DescriptorAllocator allocator(&heap);
//   auto handle = allocator.Allocate();
//   // ... use descriptor ...
//   allocator.Free(handle);
//
// DESIGN:
//   - Fast Allocate/Free of individual descriptor slots by index
//   - Supports contiguous block allocation for descriptor tables
//   - Thread-safe via internal mutex
//
// NOTES:
//   - Does not own the heap; heap must outlive allocator
//   - Freed slots are reused on subsequent allocations
// ============================================================================

#pragma once

#include <vector>
#include <mutex>
#include <optional>
#include "D3D12DescriptorHeap.h"

class D3D12DescriptorAllocator
{
  public:
	// Constructs allocator for an existing heap (does not take ownership).
	explicit D3D12DescriptorAllocator(D3D12DescriptorHeap* heap) : m_heap(heap) {}

	// Allocates a single descriptor slot.
	D3D12DescriptorHandle Allocate();

	// Allocates a contiguous block of descriptor slots.
	// Used for descriptor tables that require sequential descriptors.
	// Returns handle to first descriptor; subsequent slots are offset by descriptor size.
	D3D12DescriptorHandle AllocateContiguous(uint32_t count);

	// Returns a previously allocated descriptor slot to the free-list.
	void Free(const D3D12DescriptorHandle& handle) noexcept;

	// Returns a contiguous block to the free-list.
	void FreeContiguous(const D3D12DescriptorHandle& firstHandle, uint32_t count) noexcept;

  private:
	[[nodiscard]] std::optional<UINT> TryAllocateContiguousFromFreeListLocked(uint32_t count);
	[[nodiscard]] D3D12DescriptorHandle AllocateContiguousFromLinearRangeLocked(uint32_t count);

	D3D12DescriptorHeap* m_heap;
	std::vector<UINT> m_freeIndices;
	UINT m_currentOffset = 0;
	std::mutex m_mutex;
};
