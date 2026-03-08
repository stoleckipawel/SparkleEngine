#include "PCH.h"
#include "D3D12DescriptorAllocator.h"
#include "Log.h"

#include <algorithm>

std::optional<UINT> D3D12DescriptorAllocator::TryAllocateContiguousFromFreeListLocked(uint32_t count)
{
	if (m_freeIndices.size() < count)
	{
		return std::nullopt;
	}

	std::sort(m_freeIndices.begin(), m_freeIndices.end());

	for (std::size_t start = 0; start + count <= m_freeIndices.size(); ++start)
	{
		bool isContiguousRun = true;
		for (std::size_t offset = 1; offset < count; ++offset)
		{
			if (m_freeIndices[start + offset] != m_freeIndices[start] + offset)
			{
				isContiguousRun = false;
				break;
			}
		}

		if (isContiguousRun)
		{
			const UINT startIndex = m_freeIndices[start];
			m_freeIndices.erase(m_freeIndices.begin() + static_cast<std::ptrdiff_t>(start),
			                   m_freeIndices.begin() + static_cast<std::ptrdiff_t>(start + count));
			return startIndex;
		}
	}

	return std::nullopt;
}

D3D12DescriptorHandle D3D12DescriptorAllocator::AllocateContiguousFromLinearRangeLocked(uint32_t count)
{
	if (m_currentOffset + count > m_heap->GetNumDescriptors())
	{
		LOG_FATAL("Descriptor heap cannot allocate contiguous block (insufficient space).");
		return D3D12DescriptorHandle{};
	}

	const UINT startIndex = m_currentOffset;
	m_currentOffset += count;
	return m_heap->GetHandleAt(startIndex);
}

D3D12DescriptorHandle D3D12DescriptorAllocator::Allocate()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	uint32_t index = ~0u;

	if (!m_freeIndices.empty())
	{
		index = m_freeIndices.back();
		m_freeIndices.pop_back();
	}
	else if (m_currentOffset < m_heap->GetNumDescriptors())
	{
		index = m_currentOffset++;
	}
	else
	{
		LOG_FATAL("Descriptor heap is full.");
	}

	if (index == ~0u)
	{
		LOG_FATAL("Invalid descriptor index.");
	}

	return m_heap->GetHandleAt(index);
}

D3D12DescriptorHandle D3D12DescriptorAllocator::AllocateContiguous(uint32_t count)
{
	if (count == 0)
	{
		return D3D12DescriptorHandle{};
	}

	std::lock_guard<std::mutex> lock(m_mutex);

	if (const auto reusedStartIndex = TryAllocateContiguousFromFreeListLocked(count))
	{
		return m_heap->GetHandleAt(*reusedStartIndex);
	}

	return AllocateContiguousFromLinearRangeLocked(count);
}

void D3D12DescriptorAllocator::Free(const D3D12DescriptorHandle& handle) noexcept
{
	if (handle.IsValid())
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_freeIndices.push_back(handle.GetIndex());
	}
}

void D3D12DescriptorAllocator::FreeContiguous(const D3D12DescriptorHandle& firstHandle, uint32_t count) noexcept
{
	if (firstHandle.IsValid() && count > 0)
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		uint32_t startIndex = firstHandle.GetIndex();
		for (uint32_t i = 0; i < count; ++i)
		{
			m_freeIndices.push_back(startIndex + i);
		}
	}
}