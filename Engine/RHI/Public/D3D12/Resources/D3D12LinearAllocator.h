// ============================================================================
// D3D12LinearAllocator.h
// ----------------------------------------------------------------------------
// High-performance per-frame linear (bump) allocator for GPU upload memory.
//
#pragma once

#include "DebugUtils.h"

#include <atomic>
#include <cassert>
#include <cstdint>
#include <d3d12.h>
#include <stdexcept>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Rhi;

// ============================================================================
// D3D12LinearAllocation Result
// ============================================================================

struct D3D12LinearAllocation
{
	void* CpuPtr = nullptr;                    // Write destination
	D3D12_GPU_VIRTUAL_ADDRESS GpuAddress = 0;  // Bind address for CBV
	uint64_t Size = 0;                         // Allocated size (aligned)
	uint64_t Offset = 0;                       // Offset from buffer start
};

class D3D12LinearAllocator
{
  public:
	D3D12LinearAllocator() = default;
	~D3D12LinearAllocator() noexcept { Shutdown(); }

	// Non-copyable, non-movable (owns GPU resource)
	D3D12LinearAllocator(const D3D12LinearAllocator&) = delete;
	D3D12LinearAllocator& operator=(const D3D12LinearAllocator&) = delete;
	D3D12LinearAllocator(D3D12LinearAllocator&&) = delete;
	D3D12LinearAllocator& operator=(D3D12LinearAllocator&&) = delete;

	void Initialize(D3D12Rhi& rhi, uint64_t capacity, const wchar_t* debugName = L"D3D12LinearAllocator");

	void Shutdown();

	void Reset() noexcept;

	D3D12LinearAllocation Allocate(uint64_t size, uint64_t alignment = 256);

	template <typename T> D3D12_GPU_VIRTUAL_ADDRESS AllocateAndCopy(const T& data)
	{
		static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");

		D3D12LinearAllocation alloc = Allocate(sizeof(T), 256);
		std::memcpy(alloc.CpuPtr, &data, sizeof(T));
		return alloc.GpuAddress;
	}

	uint64_t GetCurrentOffset() const noexcept { return m_Offset.load(std::memory_order_relaxed); }

	uint64_t GetCapacity() const noexcept { return m_Capacity; }

	uint64_t GetHighWaterMark() const noexcept { return m_HighWaterMark.load(std::memory_order_relaxed); }

	float GetUsagePercent() const noexcept
	{
		if (m_Capacity == 0)
			return 0.0f;
		return static_cast<float>(GetCurrentOffset()) / static_cast<float>(m_Capacity) * 100.0f;
	}

	bool IsInitialized() const noexcept { return m_bInitialized; }

  private:
	static constexpr uint64_t AlignUp(uint64_t value, uint64_t alignment) noexcept
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

  private:
	D3D12Rhi* m_rhi = nullptr;
	ComPtr<ID3D12Resource> m_Resource;
	uint8_t* m_CpuBase = nullptr;
	D3D12_GPU_VIRTUAL_ADDRESS m_GpuBase = 0;
	uint64_t m_Capacity = 0;
	std::atomic<uint64_t> m_Offset{0};
	std::atomic<uint64_t> m_HighWaterMark{0};
	bool m_bInitialized = false;
};
