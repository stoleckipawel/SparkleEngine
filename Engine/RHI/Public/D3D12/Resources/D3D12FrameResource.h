// ============================================================================
// D3D12FrameResource.h
// ----------------------------------------------------------------------------
// Per-frame GPU resource management for multi-buffered rendering.
//
#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>
#include <cassert>
#include "D3D12LinearAllocator.h"
#include "RHIConfig.h"
#include "D3D12Rhi.h"

// ============================================================================
// FrameResource
// ============================================================================

struct D3D12FrameResource
{
	D3D12LinearAllocator CbAllocator;  // Per-frame CB ring buffer
	uint64_t FenceValue = 0;           // Fence value when this frame was submitted
	uint32_t FrameIndex = 0;           // Debug: which frame index this represents

	void Initialize(D3D12Rhi& rhi, uint64_t allocatorCapacity, uint32_t frameIdx)
	{
		FrameIndex = frameIdx;
		FenceValue = 0;

		wchar_t name[64];
		swprintf_s(name, L"FrameAllocator_%u", frameIdx);
		CbAllocator.Initialize(rhi, allocatorCapacity, name);
	}

	void Shutdown() { CbAllocator.Shutdown(); }

	// Reset allocator for new frame. Only call after fence confirms GPU completion.
	void Reset() { CbAllocator.Reset(); }
};

//------------------------------------------------------------------------------
// FrameResourceManager
//------------------------------------------------------------------------------
// Manages the ring of FrameResource instances, one per frame-in-flight.
// Handles synchronization between CPU and GPU to prevent data races.
//
// Synchronization Model:
//   1. BeginFrame(): Wait for the oldest frame's fence, then reset its allocator
//   2. AllocateXXX(): Allocate from current frame's linear allocator
//   3. EndFrame(): Signal fence with current frame's fence value
//   4. Advance frame index (wraps around)
//
// This guarantees that when we reuse a frame's allocator (after FramesInFlight
// frames), the GPU has definitely finished reading from it.
//
// Capacity Planning:
//   Default: 4MB per frame (supports ~16k draw calls with 256-byte CBs)
//   For very large scenes, increase to 8-16MB or implement dynamic growth.
//------------------------------------------------------------------------------

class D3D12FrameResourceManager final
{
  public:
	// Default capacity: 4MB per frame (16384 draws x 256 bytes)
	static constexpr uint64_t DefaultCapacityPerFrame = 4 * 1024 * 1024;

	// Construct and initialize all frame resources.
	// @param rhi Reference to the D3D12Rhi for GPU resource creation.
	// @param capacityPerFrame Allocator capacity per frame (default 4MB).
	explicit D3D12FrameResourceManager(D3D12Rhi& rhi, uint64_t capacityPerFrame = DefaultCapacityPerFrame) :
	    m_capacityPerFrame(capacityPerFrame)
	{
		for (uint32_t i = 0; i < RHISettings::FramesInFlight; ++i)
		{
			m_frameResources[i].Initialize(rhi, capacityPerFrame, i);
		}
	}

	~D3D12FrameResourceManager()
	{
		for (auto& frame : m_frameResources)
		{
			frame.Shutdown();
		}
	}

	D3D12FrameResourceManager(const D3D12FrameResourceManager&) = delete;
	D3D12FrameResourceManager& operator=(const D3D12FrameResourceManager&) = delete;
	D3D12FrameResourceManager(D3D12FrameResourceManager&&) = delete;
	D3D12FrameResourceManager& operator=(D3D12FrameResourceManager&&) = delete;

	//--------------------------------------------------------------------------
	// Frame Lifecycle
	//--------------------------------------------------------------------------

	// Begin a new frame. Waits for GPU if necessary, resets allocator.
	// @param fence The D3D12 fence for GPU synchronization.
	// @param fenceEvent Event handle for CPU wait.
	// @param frameIndex Current frame-in-flight index.
	void BeginFrame(ID3D12Fence* fence, HANDLE fenceEvent, uint32_t frameIndex)
	{
		m_currentFrameIndex = frameIndex;
		D3D12FrameResource& frame = m_frameResources[frameIndex];

		// Wait for GPU to finish with this frame's resources before reusing
		// This is the critical synchronization point that prevents races
		const uint64_t completedFence = fence->GetCompletedValue();
		if (completedFence < frame.FenceValue)
		{
			// GPU hasn't finished with this frame yet - must wait
			HRESULT hr = fence->SetEventOnCompletion(frame.FenceValue, fenceEvent);
			if (SUCCEEDED(hr))
			{
				WaitForSingleObject(fenceEvent, INFINITE);
			}
		}

		// Now safe to reset - GPU is done with this frame's data
		frame.Reset();
	}

	// Record fence value for current frame. Call after ExecuteCommandLists.
	// fenceValue The fence value that was signaled for this frame.
	void EndFrame(uint64_t fenceValue) { m_frameResources[m_currentFrameIndex].FenceValue = fenceValue; }

	//--------------------------------------------------------------------------
	// Allocation
	//--------------------------------------------------------------------------

	// Get the current frame's linear allocator.
	D3D12LinearAllocator& GetCurrentAllocator() noexcept { return m_frameResources[m_currentFrameIndex].CbAllocator; }

	// Allocate from current frame's allocator.
	D3D12LinearAllocation Allocate(uint64_t size, uint64_t alignment = 256)
	{
		return GetCurrentAllocator().Allocate(size, alignment);
	}

	// Allocate and copy data, return GPU address for CBV binding.
	template <typename T> D3D12_GPU_VIRTUAL_ADDRESS AllocateConstantBuffer(const T& data)
	{
		return GetCurrentAllocator().AllocateAndCopy(data);
	}

	//--------------------------------------------------------------------------
	// Diagnostics
	//--------------------------------------------------------------------------

	// Get current frame's allocator usage percentage.
	float GetCurrentUsagePercent() const noexcept
	{
		return m_frameResources[m_currentFrameIndex].CbAllocator.GetUsagePercent();
	}

	// Get high water mark across all frames (for capacity tuning).
	uint64_t GetMaxHighWaterMark() const noexcept
	{
		uint64_t maxHwm = 0;
		for (const auto& frame : m_frameResources)
		{
			maxHwm = (std::max) (maxHwm, frame.CbAllocator.GetHighWaterMark());
		}
		return maxHwm;
	}

	// Get capacity per frame.
	uint64_t GetCapacityPerFrame() const noexcept { return m_capacityPerFrame; }

  private:
	std::array<D3D12FrameResource, RHISettings::FramesInFlight> m_frameResources;
	uint64_t m_capacityPerFrame = DefaultCapacityPerFrame;
	uint32_t m_currentFrameIndex = 0;
};
