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
	D3D12SwapChain(D3D12Rhi& rhi, Window& window, D3D12DescriptorHeapManager& descriptorHeapManager);

	~D3D12SwapChain() noexcept;

	D3D12SwapChain(const D3D12SwapChain&) = delete;
	D3D12SwapChain& operator=(const D3D12SwapChain&) = delete;
	D3D12SwapChain(D3D12SwapChain&&) = delete;
	D3D12SwapChain& operator=(D3D12SwapChain&&) = delete;

	void Present();

	void Resize();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const { return m_rtvHandles[index].GetCPU(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return GetCPUHandle(m_frameInFlightIndex); }

	ID3D12Resource* GetCurrentResource() const noexcept { return m_buffers[m_frameInFlightIndex].Get(); }

	UINT GetFrameInFlightIndex() const { return m_frameInFlightIndex; }

	void UpdateFrameInFlightIndex() { m_frameInFlightIndex = m_swapChain->GetCurrentBackBufferIndex(); }

	D3D12_VIEWPORT GetDefaultViewport() const;

	D3D12_RECT GetDefaultScissorRect() const;

	DXGI_FORMAT GetBackBufferFormat() const { return RHISettings::BackBufferFormat; }

	UINT GetAllowTearingFlag() const;

	UINT GetFrameLatencyWaitableFlag() const
	{
		return (RHISettings::FramesInFlight > 1) ? DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT : 0u;
	}

	UINT ComputeSwapChainFlags() const;

  private:
	void CreateRenderTargetViews();
	void AllocateHandles();
	void Create();
	void ReleaseBuffers();

	D3D12Rhi& m_rhi;
	UINT m_frameInFlightIndex = 0;
	ComPtr<IDXGISwapChain3> m_swapChain = nullptr;
	ComPtr<ID3D12Resource2> m_buffers[RHISettings::FramesInFlight];
	D3D12DescriptorHandle m_rtvHandles[RHISettings::FramesInFlight];
	HANDLE m_waitableObject = nullptr;
	Window* m_window = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
};
