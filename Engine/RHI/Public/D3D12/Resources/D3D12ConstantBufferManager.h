#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>
#include <array>
#include "RHIConfig.h"
#include "D3D12ConstantBufferData.h"
#include "D3D12ConstantBuffer.h"

class Timer;
class Window;
class D3D12Rhi;
class D3D12DescriptorHeapManager;
class D3D12FrameResourceManager;
class D3D12SwapChain;
class UI;

class D3D12ConstantBufferManager final
{
  public:
	D3D12ConstantBufferManager(
	    Timer& timer,
	    D3D12Rhi& rhi,
	    Window& window,
	    D3D12DescriptorHeapManager& descriptorHeapManager,
	    D3D12FrameResourceManager& frameResourceManager,
	    D3D12SwapChain& swapChain,
	    UI& ui);
	~D3D12ConstantBufferManager() noexcept;

	D3D12ConstantBufferManager(const D3D12ConstantBufferManager&) = delete;
	D3D12ConstantBufferManager& operator=(const D3D12ConstantBufferManager&) = delete;
	D3D12ConstantBufferManager(D3D12ConstantBufferManager&&) = delete;
	D3D12ConstantBufferManager& operator=(D3D12ConstantBufferManager&&) = delete;

	D3D12_GPU_VIRTUAL_ADDRESS GetPerFrameGpuAddress() const;

	D3D12_GPU_VIRTUAL_ADDRESS GetPerViewGpuAddress() const;

	void UpdatePerFrame();

	void UpdatePerView(const PerViewConstantBufferData& data);

	D3D12_GPU_VIRTUAL_ADDRESS UpdatePerObjectVS(const PerObjectVSConstantBufferData& data);

	D3D12_GPU_VIRTUAL_ADDRESS UpdatePerObjectPS(const PerObjectPSConstantBufferData& data);

  private:
	std::unique_ptr<D3D12ConstantBuffer<PerFrameConstantBufferData>> m_perFrameCB[RHISettings::FramesInFlight];

	std::unique_ptr<D3D12ConstantBuffer<PerViewConstantBufferData>> m_perViewCB[RHISettings::FramesInFlight];

	Timer* m_timer = nullptr;
	Window* m_window = nullptr;
	D3D12FrameResourceManager* m_frameResourceManager = nullptr;
	D3D12SwapChain* m_swapChain = nullptr;
	UI* m_ui = nullptr;
};
