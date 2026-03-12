// ============================================================================
// D3D12SwapChain.h
// ----------------------------------------------------------------------------
// Manages the DXGI swap chain and associated render targets for presentation.
//
// USAGE:
//   D3D12SwapChain swapChain(window, descriptorHeapManager);
//   // Each frame:
//   swapChain.SetRenderTargetState();
//   swapChain.Clear();
//   // ... render ...
//   swapChain.SetPresentState();
//   swapChain.Present();
//
// DESIGN:
//   - Owns IDXGISwapChain3 and per-frame back buffer resources
//   - Supports variable-rate tearing and frame latency waitable objects
//   - Handles resize by releasing and recreating buffers
//
// NOTES:
//   - Owned by Renderer, passed by reference where needed
//   - Frame index updated via UpdateFrameInFlightIndex() after Present()
//   - Buffer count configured by RHISettings::FramesInFlight
// ============================================================================

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
	#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
	#define NOMINMAX
#endif
#include <Windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include "D3D12DescriptorHandle.h"
#include "RHIConfig.h"

using Microsoft::WRL::ComPtr;

class Window;
class D3D12DescriptorHeapManager;
class D3D12Rhi;

class D3D12SwapChain final
{
  public:
	// ========================================================================
	// Lifecycle
	// ========================================================================

	/// Constructs the swap chain and render target views.
	D3D12SwapChain(D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager);

	/// Releases all swap chain resources.
	~D3D12SwapChain() noexcept;

	D3D12SwapChain(const D3D12SwapChain&) = delete;
	D3D12SwapChain& operator=(const D3D12SwapChain&) = delete;
	D3D12SwapChain(D3D12SwapChain&&) = delete;
	D3D12SwapChain& operator=(D3D12SwapChain&&) = delete;

	// ========================================================================
	// Frame Operations
	// ========================================================================

	/// Presents the current back buffer to the screen.
	void Present();

	/// Clears the current render target view with the clear color.
	void Clear();

	/// Transitions current buffer to render target state.
	void SetRenderTargetState();

	/// Transitions current buffer to present state.
	void SetPresentState();

	/// Resizes swap chain buffers (called on window resize).
	void Resize();

	// ========================================================================
	// Accessors
	// ========================================================================

	/// Returns the CPU descriptor handle for a specific buffer index.
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const { return m_rtvHandles[index].GetCPU(); }

	/// Returns the CPU descriptor handle for the current back buffer.
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return GetCPUHandle(m_frameInFlightIndex); }

	/// Returns the current frame-in-flight index (0 to FramesInFlight-1).
	UINT GetFrameInFlightIndex() const { return m_frameInFlightIndex; }

	/// Updates frame index from swap chain (call after Present).
	void UpdateFrameInFlightIndex() { m_frameInFlightIndex = m_swapChain->GetCurrentBackBufferIndex(); }

	/// Returns the waitable object handle for frame pacing.
	HANDLE GetWaitableObject() const { return m_waitableObject; }

	/// Returns the default viewport matching window dimensions.
	D3D12_VIEWPORT GetDefaultViewport() const;

	/// Returns the default scissor rect matching window dimensions.
	D3D12_RECT GetDefaultScissorRect() const;

	/// Returns the back buffer format from engine settings.
	DXGI_FORMAT GetBackBufferFormat() const { return RHISettings::BackBufferFormat; }

	// ========================================================================
	// Feature Queries
	// ========================================================================

	/// Returns DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING if supported, else 0.
	UINT GetAllowTearingFlag() const;

	/// Returns waitable object flag if multiple frames in flight.
	UINT GetFrameLatencyWaitableFlag() const
	{
		return (RHISettings::FramesInFlight > 1) ? DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT : 0u;
	}

	/// Returns combined swap chain flags for creation.
	UINT ComputeSwapChainFlags() const;

  private:
	// ------------------------------------------------------------------------
	// Initialization Helpers
	// ------------------------------------------------------------------------

	void CreateRenderTargetViews();
	void AllocateHandles();
	void Create();
	void ReleaseBuffers();

	// ------------------------------------------------------------------------
	// State
	// ------------------------------------------------------------------------

	D3D12Rhi& m_rhi;                                                  ///< RHI reference
	UINT m_frameInFlightIndex = 0;                                    ///< Current back buffer index
	ComPtr<IDXGISwapChain3> m_swapChain = nullptr;                    ///< DXGI swap chain
	ComPtr<ID3D12Resource2> m_buffers[RHISettings::FramesInFlight];   ///< Back buffer resources
	D3D12DescriptorHandle m_rtvHandles[RHISettings::FramesInFlight];  ///< RTV descriptor handles
	HANDLE m_waitableObject = nullptr;                                ///< Frame latency waitable object
	Window* m_window = nullptr;                                       ///< Window reference for dimensions
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;    ///< Descriptor heap manager reference
};
