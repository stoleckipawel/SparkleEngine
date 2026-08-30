#pragma once

#include "D3D12DescriptorHeap.h"

#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

namespace spdlog
{
	class logger;
}

class D3D12DescriptorAllocator final
{
public:
	explicit D3D12DescriptorAllocator(D3D12DescriptorHeap* heap) noexcept;

	D3D12DescriptorHandle Allocate();

	D3D12DescriptorHandle AllocateContiguous(uint32_t count);

	void Free(const D3D12DescriptorHandle& handle) noexcept;

	void FreeContiguous(const D3D12DescriptorHandle& firstHandle, uint32_t count) noexcept;

private:
	std::optional<UINT> TryAllocateContiguousFromFreeListLocked(uint32_t count);
	D3D12DescriptorHandle AllocateContiguousFromLinearRangeLocked(uint32_t count);
	static const std::shared_ptr<spdlog::logger>& Logger() noexcept;

	D3D12DescriptorHeap* m_heap;
	std::vector<UINT> m_freeIndices;
	UINT m_currentOffset = 0;
	mutable std::mutex m_mutex;
};
