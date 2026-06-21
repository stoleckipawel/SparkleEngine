#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <memory>

#include "Frame/RhiFrameConstants.h"
#include "D3D12/RayTracing/D3D12NvapiRayTracingProvider.h"
#include "Device/RenderHardwareInterface.h"

using Microsoft::WRL::ComPtr;

struct RhiDiagnosticMessage;

#if ENGINE_GPU_VALIDATION
class D3D12DebugLayer;
#endif

class D3D12GpuMemoryAllocator;

class D3D12Rhi final
{
  public:
	explicit D3D12Rhi() noexcept;

	~D3D12Rhi() noexcept;

	D3D12Rhi(const D3D12Rhi&) = delete;
	D3D12Rhi& operator=(const D3D12Rhi&) = delete;
	D3D12Rhi(D3D12Rhi&&) = delete;
	D3D12Rhi& operator=(D3D12Rhi&&) = delete;

	void ResetCommandAllocator(uint32_t frameInFlightIndex) noexcept;

	void ResetCommandList(uint32_t frameInFlightIndex) noexcept;

	void CloseCommandList(uint32_t frameInFlightIndex) noexcept;

	void ExecuteCommandList(uint32_t frameInFlightIndex) noexcept;

	void SetCurrentFrameIndex(uint32_t frameInFlightIndex) noexcept;
	uint32_t GetCurrentFrameIndex() const noexcept;

	void Signal(uint32_t frameInFlightIndex) noexcept;

	void WaitForGPU(uint32_t frameInFlightIndex) noexcept;

	void Flush() noexcept;
	bool IsValidationEnabled() const noexcept;
	bool SupportsDebugMessages() const noexcept;
	bool TryPopDebugMessage(RhiDiagnosticMessage& outMessage) noexcept;
	void ClearDebugMessages() noexcept;
	bool SupportsLiveObjectReports() const noexcept;
	bool SupportsCrashDiagnostics() const noexcept;
	void ReportLiveObjects() noexcept;
	void CollectCrashDiagnostics() noexcept;

	void CheckShaderModel6Support() const noexcept;
	RhiRayTracingCapabilities GetRayTracingCapabilities() const noexcept;

	const ComPtr<IDXGIFactory7>& GetDxgiFactory() const noexcept;
	const ComPtr<IDXGIAdapter1>& GetAdapter() const noexcept;
	const ComPtr<ID3D12Device10>& GetDevice() const noexcept;
	const ComPtr<ID3D12CommandQueue>& GetCommandQueue() const noexcept;
	const ComPtr<ID3D12CommandAllocator>& GetCommandAllocator(uint32_t frameInFlightIndex) const noexcept;
	const ComPtr<ID3D12GraphicsCommandList7>& GetCommandList(uint32_t frameInFlightIndex) const noexcept;
	const ComPtr<ID3D12Fence1>& GetFence() const noexcept;
	HANDLE GetFenceEvent() const noexcept;
	uint64_t GetNextFenceValue() const noexcept;
	D3D_FEATURE_LEVEL GetDeviceFeatureLevel() const noexcept;
	D3D12NvapiRayTracingProvider& GetNvapiRayTracingProvider() noexcept;
	const D3D12NvapiRayTracingProvider& GetNvapiRayTracingProvider() const noexcept;
	D3D12GpuMemoryAllocator& GetMemoryAllocator() noexcept;
	const D3D12GpuMemoryAllocator& GetMemoryAllocator() const noexcept;

  private:
	void SelectAdapter() noexcept;
	void CreateFactory();
	void CreateDevice();
	void CreateMemoryAllocator();
	void CheckRayTracingSupport() noexcept;
	void RefreshPartitionedTlasCommandListCapability() noexcept;
	void CreateCommandQueue();
	void CreateCommandAllocators();
	void CreateCommandLists();
	void CreateFenceAndEvent();

#if ENGINE_GPU_VALIDATION
	std::unique_ptr<D3D12DebugLayer> m_debugLayer;
#endif

	ComPtr<IDXGIFactory7> m_dxgiFactory = nullptr;
	ComPtr<IDXGIAdapter1> m_adapter = nullptr;
	ComPtr<ID3D12Device10> m_device = nullptr;
	D3D12NvapiRayTracingProvider m_nvapiRayTracingProvider;
	std::unique_ptr<D3D12GpuMemoryAllocator> m_memoryAllocator;
	ComPtr<ID3D12CommandQueue> m_cmdQueue = nullptr;
	ComPtr<ID3D12CommandAllocator> m_cmdAllocator[RhiFrameConstants::FramesInFlight] = {};
	ComPtr<ID3D12GraphicsCommandList7> m_cmdList[RhiFrameConstants::FramesInFlight] = {};
	uint32_t m_currentFrameIndex = 0;

	uint64_t m_fenceValues[RhiFrameConstants::FramesInFlight] = {0};
	uint64_t m_nextFenceValue = 1;
	ComPtr<ID3D12Fence1> m_fence = nullptr;
	HANDLE m_fenceEvent = nullptr;
	D3D_FEATURE_LEVEL m_desiredD3DFeatureLevel = D3D_FEATURE_LEVEL_12_1;
	RhiRayTracingCapabilities m_rayTracingCapabilities = {};
};
