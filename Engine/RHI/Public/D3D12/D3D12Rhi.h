// =============================================================================
// D3D12Rhi.h - Direct3D 12 Rendering Hardware Interface
// =============================================================================
//
// Core RHI layer managing D3D12 device, command queues, allocators, and GPU
// synchronization. Provides a lightweight abstraction over Direct3D 12 with
// minimal COM reference counting overhead.
//
#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <memory>
#include "RHIConfig.h"

using Microsoft::WRL::ComPtr;

#ifdef ENGINE_GPU_VALIDATION
class D3D12DebugLayer;
#endif

// =============================================================================
// D3D12Rhi
// =============================================================================

class D3D12Rhi final
{
  public:
	// Constructs and initializes device, command queue, allocators, and fences.
	explicit D3D12Rhi(bool requireDXRSupport = false) noexcept;

	// Releases all D3D12 resources.
	~D3D12Rhi() noexcept;

	D3D12Rhi(const D3D12Rhi&) = delete;
	D3D12Rhi& operator=(const D3D12Rhi&) = delete;
	D3D12Rhi(D3D12Rhi&&) = delete;
	D3D12Rhi& operator=(D3D12Rhi&&) = delete;

	// =========================================================================
	// Command Recording
	// =========================================================================

	// Resets the command allocator for the specified frame. Call at frame start.
	void ResetCommandAllocator(uint32_t frameInFlightIndex) noexcept;

	// Resets and reopens the command list for recording.
	void ResetCommandList(uint32_t frameInFlightIndex) noexcept;

	// Closes the command list. Must be called before ExecuteCommandList().
	void CloseCommandList(uint32_t frameInFlightIndex) noexcept;

	// Submits the closed command list to the GPU queue.
	void ExecuteCommandList(uint32_t frameInFlightIndex) noexcept;

	// Records a resource barrier for state transition.
	void SetBarrier(
	    uint32_t frameInFlightIndex,
	    ID3D12Resource* resource,
	    D3D12_RESOURCE_STATES stateBefore,
	    D3D12_RESOURCE_STATES stateAfter) noexcept;

	// Sets the current frame index for convenience methods.
	void SetCurrentFrameIndex(uint32_t frameInFlightIndex) noexcept { m_currentFrameIndex = frameInFlightIndex; }
	uint32_t GetCurrentFrameIndex() const noexcept { return m_currentFrameIndex; }

	// Convenience overloads using current frame index
	void CloseCommandList() noexcept { CloseCommandList(m_currentFrameIndex); }
	void ExecuteCommandList() noexcept { ExecuteCommandList(m_currentFrameIndex); }
	void SetBarrier(ID3D12Resource* resource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter) noexcept
	{
		SetBarrier(m_currentFrameIndex, resource, stateBefore, stateAfter);
	}
	const ComPtr<ID3D12GraphicsCommandList7>& GetCommandList() const noexcept { return m_cmdList[m_currentFrameIndex]; }

	// =========================================================================
	// Synchronization
	// =========================================================================

	// Signals the fence with the next value. Call at end of frame.
	void Signal(uint32_t frameInFlightIndex) noexcept;

	// Blocks CPU until GPU completes work for specified frame.
	void WaitForGPU(uint32_t frameInFlightIndex) noexcept;

	// Signal and wait (convenience for shutdown/resize).
	void Flush() noexcept;

	// =========================================================================
	// Device Capabilities
	// =========================================================================

	// Validates Shader Model 6.0 support. Fatals if unsupported.
	void CheckShaderModel6Support() const noexcept;

	// =========================================================================
	// D3D12-Specific Accessors
	// =========================================================================

	const ComPtr<IDXGIFactory7>& GetDxgiFactory() const noexcept { return m_dxgiFactory; }
	const ComPtr<IDXGIAdapter1>& GetAdapter() const noexcept { return m_adapter; }
	const ComPtr<ID3D12Device10>& GetDevice() const noexcept { return m_device; }
	const ComPtr<ID3D12CommandQueue>& GetCommandQueue() const noexcept { return m_cmdQueue; }
	const ComPtr<ID3D12CommandAllocator>& GetCommandAllocator(uint32_t frameInFlightIndex) const noexcept
	{
		return m_cmdAllocator[frameInFlightIndex];
	}
	const ComPtr<ID3D12GraphicsCommandList7>& GetCommandList(uint32_t frameInFlightIndex) const noexcept
	{
		return m_cmdList[frameInFlightIndex];
	}
	const ComPtr<ID3D12Fence1>& GetFence() const noexcept { return m_fence; }

	// =========================================================================
	// D3D12-Specific Fence Management
	// =========================================================================

	uint64_t GetFenceValueForFrame(uint32_t frameInFlightIndex) const noexcept { return m_fenceValues[frameInFlightIndex]; }
	void SetFenceValueForFrame(uint32_t frameInFlightIndex, uint64_t value) noexcept { m_fenceValues[frameInFlightIndex] = value; }
	HANDLE GetFenceEvent() const noexcept { return m_fenceEvent; }
	uint64_t GetNextFenceValue() const noexcept { return m_nextFenceValue; }
	void SetNextFenceValue(uint64_t value) noexcept { m_nextFenceValue = value; }

  private:
	// -------------------------------------------------------------------------
	// Initialization Helpers
	// -------------------------------------------------------------------------

	void SelectAdapter() noexcept;
	void CreateFactory();
	void CreateDevice(bool requireDXRSupport);
	void CreateCommandQueue();
	void CreateCommandAllocators();
	void CreateCommandLists();
	void CreateFenceAndEvent();

#ifdef ENGINE_GPU_VALIDATION
	std::unique_ptr<D3D12DebugLayer> m_debugLayer;
#endif

	// -------------------------------------------------------------------------
	// D3D12 Resources
	// -------------------------------------------------------------------------

	ComPtr<IDXGIFactory7> m_dxgiFactory = nullptr;
	ComPtr<IDXGIAdapter1> m_adapter = nullptr;
	ComPtr<ID3D12Device10> m_device = nullptr;
	ComPtr<ID3D12CommandQueue> m_cmdQueue = nullptr;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator[RHISettings::FramesInFlight] = {};
	ComPtr<ID3D12GraphicsCommandList7> m_cmdList[RHISettings::FramesInFlight] = {};
	uint32_t m_currentFrameIndex = 0;  // Tracks current frame for methods that don't take frame index

	// -------------------------------------------------------------------------
	// Synchronization State
	// -------------------------------------------------------------------------

	uint64_t m_fenceValues[RHISettings::FramesInFlight] = {0};  // per-frame fence values
	uint64_t m_nextFenceValue = 1;                              // monotonically increasing
	ComPtr<ID3D12Fence1> m_fence = nullptr;
	HANDLE m_fenceEvent = nullptr;
	D3D_FEATURE_LEVEL m_desiredD3DFeatureLevel = D3D_FEATURE_LEVEL_12_1;
};
