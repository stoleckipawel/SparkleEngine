// ============================================================================
// D3D12SwapChain.h
// ----------------------------------------------------------------------------
// Manages the DXGI swap chain and associated render targets for presentation.
//
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

	D3D12SwapChain(D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager);

	~D3D12SwapChain() noexcept;

	D3D12SwapChain(const D3D12SwapChain&) = delete;
	D3D12SwapChain& operator=(const D3D12SwapChain&) = delete;
	D3D12SwapChain(D3D12SwapChain&&) = delete;
	D3D12SwapChain& operator=(D3D12SwapChain&&) = delete;

	// ========================================================================
	// Frame Operations
	// ========================================================================

	void Present();

	void Clear();

	void SetRenderTargetState();

	void SetPresentState();

	void Resize();

	// ========================================================================
	// Accessors
	// ========================================================================

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const { return m_rtvHandles[index].GetCPU(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return GetCPUHandle(m_frameInFlightIndex); }

	UINT GetFrameInFlightIndex() const { return m_frameInFlightIndex; }

	void UpdateFrameInFlightIndex() { m_frameInFlightIndex = m_swapChain->GetCurrentBackBufferIndex(); }

	HANDLE GetWaitableObject() const { return m_waitableObject; }

	D3D12_VIEWPORT GetDefaultViewport() const;

	D3D12_RECT GetDefaultScissorRect() const;

	DXGI_FORMAT GetBackBufferFormat() const { return RHISettings::BackBufferFormat; }

	// ========================================================================
	// Feature Queries
	// ========================================================================

	UINT GetAllowTearingFlag() const;

	UINT GetFrameLatencyWaitableFlag() const
	{
		return (RHISettings::FramesInFlight > 1) ? DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT : 0u;
	}

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
