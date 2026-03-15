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

class D3D12Rhi final
{
  public:
	explicit D3D12Rhi(bool requireDXRSupport = false) noexcept;

	~D3D12Rhi() noexcept;

	D3D12Rhi(const D3D12Rhi&) = delete;
	D3D12Rhi& operator=(const D3D12Rhi&) = delete;
	D3D12Rhi(D3D12Rhi&&) = delete;
	D3D12Rhi& operator=(D3D12Rhi&&) = delete;

	void ResetCommandAllocator(uint32_t frameInFlightIndex) noexcept;

	void ResetCommandList(uint32_t frameInFlightIndex) noexcept;

	void CloseCommandList(uint32_t frameInFlightIndex) noexcept;

	void ExecuteCommandList(uint32_t frameInFlightIndex) noexcept;

	void SetCurrentFrameIndex(uint32_t frameInFlightIndex) noexcept { m_currentFrameIndex = frameInFlightIndex; }
	uint32_t GetCurrentFrameIndex() const noexcept { return m_currentFrameIndex; }

	void Signal(uint32_t frameInFlightIndex) noexcept;

	void WaitForGPU(uint32_t frameInFlightIndex) noexcept;

	void Flush() noexcept;

	void CheckShaderModel6Support() const noexcept;

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
	HANDLE GetFenceEvent() const noexcept { return m_fenceEvent; }
	uint64_t GetNextFenceValue() const noexcept { return m_nextFenceValue; }

  private:
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

	ComPtr<IDXGIFactory7> m_dxgiFactory = nullptr;
	ComPtr<IDXGIAdapter1> m_adapter = nullptr;
	ComPtr<ID3D12Device10> m_device = nullptr;
	ComPtr<ID3D12CommandQueue> m_cmdQueue = nullptr;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator[RHISettings::FramesInFlight] = {};
	ComPtr<ID3D12GraphicsCommandList7> m_cmdList[RHISettings::FramesInFlight] = {};
	uint32_t m_currentFrameIndex = 0;

	uint64_t m_fenceValues[RHISettings::FramesInFlight] = {0};
	uint64_t m_nextFenceValue = 1;
	ComPtr<ID3D12Fence1> m_fence = nullptr;
	HANDLE m_fenceEvent = nullptr;
	D3D_FEATURE_LEVEL m_desiredD3DFeatureLevel = D3D_FEATURE_LEVEL_12_1;
};
