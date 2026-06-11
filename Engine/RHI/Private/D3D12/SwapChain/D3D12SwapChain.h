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

#include "Config/RenderConfig.h"
#include "D3D12/Descriptors/D3D12DescriptorHandle.h"
#include "Device/RenderHardwareInterface.h"

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

	bool UpgradeNativeInterface(RhiNativeInterfaceUpgradeCallback callback, void* userData) noexcept;

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(UINT index) const { return m_rtvHandles[index].GetCPU(); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle() const { return m_rtvHandles[m_frameInFlightIndex].GetCPU(); }

	ID3D12Resource* GetCurrentResource() const noexcept { return m_buffers[m_frameInFlightIndex].Get(); }

	UINT GetFrameInFlightIndex() const { return m_frameInFlightIndex; }

	void UpdateFrameInFlightIndex() { m_frameInFlightIndex = m_swapChain->GetCurrentBackBufferIndex(); }

	RhiViewport GetDefaultViewport() const;

	RhiRect GetDefaultScissorRect() const;

	PixelFormat GetBackBufferFormat() const { return RenderConfig::BackBufferFormat; }

	UINT GetAllowTearingFlag() const;

	UINT GetFrameLatencyWaitableFlag() const
	{
		return (RenderConfig::FramesInFlight > 1) ? DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT : 0u;
	}

	UINT ComputeSwapChainFlags() const;

  private:
	UINT GetWindowWidth() const noexcept;
	UINT GetWindowHeight() const noexcept;
	bool HasValidWindowSize() const noexcept;
	void ResizeBuffersToWindow();
	void CreateRenderTargetViews();
	void AllocateHandles();
	void Create();
	void ReleaseBuffers();
	void ReleaseRenderTargetHandles() noexcept;

	D3D12Rhi& m_rhi;
	UINT m_frameInFlightIndex = 0;
	ComPtr<IDXGISwapChain3> m_swapChain = nullptr;
	ComPtr<ID3D12Resource2> m_buffers[RenderConfig::FramesInFlight];
	D3D12DescriptorHandle m_rtvHandles[RenderConfig::FramesInFlight];
	HANDLE m_waitableObject = nullptr;
	Window* m_window = nullptr;
	D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;
};
